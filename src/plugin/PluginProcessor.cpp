/*
    Oscil - Plugin Processor Implementation
    Main audio plugin processor
*/

#include "plugin/PluginProcessor.h"

#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

namespace oscil
{

OscilPluginProcessor::OscilPluginProcessor(IInstanceRegistry& instanceRegistry, IThemeService& themeService, ShaderRegistry& shaderRegistry, MemoryBudgetManager& memoryBudgetManager)
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , instanceRegistry_(instanceRegistry)
    , themeService_(themeService)
    , shaderRegistry_(shaderRegistry)
    , memoryBudgetManager_(memoryBudgetManager)
{
    // Create capture buffer
    captureBuffer_ = std::make_shared<DecimatingCaptureBuffer>();
    analysisEngine_ = std::make_shared<AnalysisEngine>();

    // Generate unique track identifier
    trackIdentifier_ = juce::Uuid().toString();

    // Initialize MemoryBudgetManager with config from state
    // Use default sample rate until prepareToPlay is called
    memoryBudgetManager_.setGlobalConfig(state_.getCaptureQualityConfig(),
                                         static_cast<int>(currentSampleRate_));

    // Listen for state changes (quality settings)
    state_.getState().addListener(this);

    // Initialize cached state for real-time safe getStateInformation()
    updateCachedState();
}

OscilPluginProcessor::~OscilPluginProcessor()
{
    state_.getState().removeListener(this);

    // Unregister from MemoryBudgetManager
    memoryBudgetManager_.unregisterBuffer(trackIdentifier_);

    // Unregister from instance registry
    if (sourceId_.isValid())
    {
        instanceRegistry_.unregisterInstance(sourceId_);
    }
}

const juce::String OscilPluginProcessor::getName() const
{
    return "Oscil";
}

bool OscilPluginProcessor::acceptsMidi() const { return true; }
bool OscilPluginProcessor::producesMidi() const { return false; }
bool OscilPluginProcessor::isMidiEffect() const { return false; }

double OscilPluginProcessor::getTailLengthSeconds() const { return 0.0; }

int OscilPluginProcessor::getNumPrograms() { return 1; }
int OscilPluginProcessor::getCurrentProgram() { return 0; }
void OscilPluginProcessor::setCurrentProgram(int) {}
const juce::String OscilPluginProcessor::getProgramName(int) { return {}; }
void OscilPluginProcessor::changeProgramName(int, const juce::String&) {}

void OscilPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = samplesPerBlock;

    // Pre-calculate ticks to seconds conversion for real-time safe CPU timing
    ticksToSecondsScale_ = 1.0 / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());

    // TimingEngine setup is lock-free, safe to call from audio thread
    timingEngine_.setSampleRate(sampleRate);
    
    if (analysisEngine_)
        analysisEngine_->reset();

    // Memory allocation and mutex-protected operations must happen on message thread
    // This is critical because prepareToPlay can be called from audio thread in some hosts
    // (e.g., Pro Tools, Reaper in certain configurations)
    // The DecimatingCaptureBuffer is already initialized with defaults in constructor,
    // so processBlock can safely write to it even before registration completes.
    auto doRegistration = [this,
                           sampleRate,
                           captureConfig = state_.getCaptureQualityConfig(),
                           trackId = trackIdentifier_,
                           channelCount = getTotalNumInputChannels()]() {
        // Update MemoryBudgetManager with actual sample rate from host
        memoryBudgetManager_.setGlobalConfig(captureConfig, static_cast<int>(sampleRate));

        // Configure capture buffer (allocates memory)
        captureBuffer_->configure(captureConfig, static_cast<int>(sampleRate));

        // Register buffer with MemoryBudgetManager for auto-quality adjustment
        memoryBudgetManager_.registerBuffer(trackId, captureBuffer_);

        // Register with instance registry (uses mutex, allocates)
        // Only register if not already registered (handles re-entrancy from host)
        if (!sourceId_.isValid())
        {
            sourceId_ = instanceRegistry_.registerInstance(
                trackId,
                captureBuffer_,
                "Oscil Track",
                channelCount,
                sampleRate,
                analysisEngine_);
        }
        else
        {
            // Update existing registration with new sample rate
            instanceRegistry_.updateSource(sourceId_, "Oscil Track", channelCount, sampleRate);
        }
    };

    // If already on message thread, execute directly; otherwise defer
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        doRegistration();
    }
    else
    {
        juce::MessageManager::callAsync(std::move(doRegistration));
    }
}

void OscilPluginProcessor::releaseResources()
{
    // Nothing to release
}

bool OscilPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // CRITICAL: Reject disabled buses - this prevents hosts from passing 0-channel buffers
    // Without this check, some hosts (FL Studio, Ableton) may disable audio routing entirely
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled() || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    // Support mono and stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output for effect plugins
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void OscilPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // Safety check
    if (numSamples == 0 || numChannels == 0)
        return;

    // Measure start time for CPU usage calculation
    auto startTime = juce::Time::getHighResolutionTicks();

    // Get host timing information
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto positionInfo = hostPlayHead->getPosition())
        {
            timingEngine_.updateHostInfo(*positionInfo);
        }
    }

    // Prepare metadata for visualization capture
    CaptureFrameMetadata metadata;
    metadata.sampleRate = currentSampleRate_;
    metadata.numChannels = numChannels;
    metadata.numSamples = numSamples;
    metadata.isPlaying = timingEngine_.getHostInfo().isPlaying;
    metadata.bpm = timingEngine_.getHostInfo().bpm;
    metadata.timestamp = timingEngine_.getHostInfo().timeInSamples;

    // Capture audio for visualization BEFORE any modifications (read-only operation)
    // Use tryLock=true to prevent audio thread blocking if test server is injecting data
    // DecimatingCaptureBuffer handles downsampling internally
    captureBuffer_->write(buffer, metadata);

    // Run analysis (Real-time stats)
    if (analysisEngine_)
        analysisEngine_->process(buffer, currentSampleRate_);

    // Process triggers (MIDI and Audio)
    // Note: TimingEngine uses internal atomic flags to store trigger state
    // processMidi checks for Note On events if in MIDI mode
    // processBlock checks for audio threshold if in Audio mode
    (void) timingEngine_.processMidi(midiMessages);
    (void) timingEngine_.processBlock(buffer);

    // Audio passes through unchanged - this is a visualization plugin

    // Calculate CPU usage using pre-calculated conversion factor (real-time safe)
    auto endTime = juce::Time::getHighResolutionTicks();
    double elapsedSeconds = static_cast<double>(endTime - startTime) * ticksToSecondsScale_;
    double availableSeconds = static_cast<double>(numSamples) / currentSampleRate_;
    float usage = static_cast<float>(elapsedSeconds / availableSeconds * 100.0);

    // Smooth CPU usage with exponential moving average
    float currentUsage = cpuUsage_.load(std::memory_order_relaxed);
    cpuUsage_.store(currentUsage * 0.9f + usage * 0.1f, std::memory_order_relaxed);
}

bool OscilPluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* OscilPluginProcessor::createEditor()
{
    return new OscilPluginEditor(*this);
}

void OscilPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL: Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We must not allocate memory in that case.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Audio thread path: return cached state (no allocation)
        const juce::SpinLock::ScopedTryLockType sl(cachedStateLock_);
        if (sl.isLocked() && cachedStateXml_.isNotEmpty())
        {
            destData.replaceAll(cachedStateXml_.toRawUTF8(), cachedStateXml_.getNumBytesAsUTF8());
        }
        // If lock failed or cache empty, return empty - host will retry
        return;
    }

    // Message thread path: perform full serialization (allocates memory)
    updateCachedState();

    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    destData.replaceAll(cachedStateXml_.toRawUTF8(), cachedStateXml_.getNumBytesAsUTF8());
}

void OscilPluginProcessor::updateCachedState()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Sync timing engine state to OscilState before saving
    auto timingState = timingEngine_.toValueTree();
    auto& stateTree = state_.getState();

    // Remove existing Timing node if present
    auto existingTiming = stateTree.getChildWithName(StateIds::Timing);
    if (existingTiming.isValid())
        stateTree.removeChild(existingTiming, nullptr);

    // Add current timing state
    stateTree.appendChild(timingState, nullptr);

    auto xmlString = state_.toXmlString();

    // Update cache with lock
    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    cachedStateXml_ = std::move(xmlString);
}

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xmlString = juce::String::createStringFromData(data, sizeInBytes);
    if (!state_.fromXmlString(xmlString))
    {
        DBG("PluginProcessor::setStateInformation - Failed to parse state XML");
        return;
    }

    // Apply restored state (TimingEngine uses atomics, safe from any thread)
    auto timingTree = state_.getState().getChildWithName(StateIds::Timing);
    if (timingTree.isValid())
        timingEngine_.fromValueTree(timingTree);

    // CRITICAL: Defer memory-allocating operations to message thread
    // Some DAW hosts (Pro Tools, Reaper) may call setStateInformation from audio thread
    // during session load or preset recall. Direct allocation here would cause dropouts.
    auto captureConfig = state_.getCaptureQualityConfig();
    auto sampleRate = static_cast<int>(currentSampleRate_);

    auto doConfigure = [this, captureConfig, sampleRate]() {
        // Sync restored capture quality config to MemoryBudgetManager
        memoryBudgetManager_.setGlobalConfig(captureConfig, sampleRate);

        // Update capture buffer config (allocates memory)
        captureBuffer_->configure(captureConfig, sampleRate);

        // Update cached state for real-time safe getStateInformation()
        updateCachedState();
    };

    // If already on message thread, execute directly; otherwise defer
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        doConfigure();
    }
    else
    {
        juce::MessageManager::callAsync(std::move(doConfigure));
    }
}

std::shared_ptr<SharedCaptureBuffer> OscilPluginProcessor::getCaptureBuffer() const
{
    return captureBuffer_->getInternalBuffer();
}

SourceId OscilPluginProcessor::getSourceId() const
{
    return sourceId_;
}

OscilState& OscilPluginProcessor::getState()
{
    return state_;
}

TimingEngine& OscilPluginProcessor::getTimingEngine()
{
    return timingEngine_;
}

IInstanceRegistry& OscilPluginProcessor::getInstanceRegistry()
{
    return instanceRegistry_;
}

IThemeService& OscilPluginProcessor::getThemeService()
{
    return themeService_;
}

ShaderRegistry& OscilPluginProcessor::getShaderRegistry()
{
    return shaderRegistry_;
}

std::shared_ptr<IAudioBuffer> OscilPluginProcessor::getBuffer(const SourceId& sourceId)
{
    // If the requested source is this processor's own source, return local buffer
    if (sourceId == sourceId_)
    {
        return captureBuffer_;
    }

    // Otherwise, look up in registry
    return instanceRegistry_.getCaptureBuffer(sourceId);
}

void OscilPluginProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    // Check if CaptureQuality settings changed
    if (tree.hasType(StateIds::CaptureQuality) ||
        (tree.getParent().isValid() && tree.getParent().hasType(StateIds::CaptureQuality)))
    {
        // CRITICAL: Always defer to message thread to guarantee real-time safety
        // While ValueTree listeners are *usually* called on message thread,
        // rapid parameter changes could theoretically trigger from other threads.
        auto captureConfig = state_.getCaptureQualityConfig();
        auto sampleRate = static_cast<int>(currentSampleRate_);

        juce::MessageManager::callAsync([this, captureConfig, sampleRate]() {
            captureBuffer_->configure(captureConfig, sampleRate);
        });
    }
}

} // namespace oscil

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return oscil::PluginFactory::getInstance().createPluginProcessor().release();
}
