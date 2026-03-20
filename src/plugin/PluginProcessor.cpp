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

namespace
{
juce::String normaliseSourceDisplayName(const juce::AudioProcessor::TrackProperties& properties)
{
    if (properties.name.has_value())
    {
        const auto trimmed = properties.name->trim();
        if (trimmed.isNotEmpty())
            return trimmed;
    }

    return "Oscil Track";
}
} // namespace

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
    auto config = state_.getCaptureQualityConfig();
    setCaptureQualityConfig(config);
    memoryBudgetManager_.setGlobalConfig(config,
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
    currentSampleRate_.store(sampleRate, std::memory_order_release);
    currentBlockSize_.store(samplesPerBlock, std::memory_order_release);

    // Pre-calculate ticks to seconds conversion for real-time safe CPU timing
    ticksToSecondsScale_ = 1.0 / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());

    // TimingEngine setup is lock-free, safe to call from audio thread
    timingEngine_.setSampleRate(sampleRate);
    
    if (analysisEngine_)
    {
        analysisEngine_->prepare(sampleRate, samplesPerBlock);
        analysisEngine_->reset();
    }

    // Memory allocation and mutex-protected operations must happen on message thread
    // This is critical because prepareToPlay can be called from audio thread in some hosts
    // (e.g., Pro Tools, Reaper in certain configurations)
    // The DecimatingCaptureBuffer is already initialized with defaults in constructor,
    // so processBlock can safely write to it even before registration completes.
    //
    // Use WeakReference to handle case where processor is destroyed before callback runs.
    auto doRegistration = [weakThis = juce::WeakReference<OscilPluginProcessor>(this),
                           sampleRate,
                           captureConfig = getCaptureQualityConfig(), // Thread-safe read
                           trackId = trackIdentifier_,
                           channelCount = getTotalNumInputChannels()]() {
        auto* processor = weakThis.get();
        if (!processor) return;

        // Update MemoryBudgetManager with actual sample rate from host
        processor->memoryBudgetManager_.setGlobalConfig(captureConfig, static_cast<int>(sampleRate));

        // Configure capture buffer (allocates memory)
        processor->captureBuffer_->configure(captureConfig, static_cast<int>(sampleRate));

        // Register buffer with MemoryBudgetManager for auto-quality adjustment
        processor->memoryBudgetManager_.registerBuffer(trackId, processor->captureBuffer_);

        // Register with instance registry (uses mutex, allocates)
        // Only register if not already registered (handles re-entrancy from host)
        if (!processor->sourceId_.isValid())
        {
            processor->sourceId_ = processor->instanceRegistry_.registerInstance(
                trackId,
                processor->captureBuffer_,
                processor->sourceDisplayName_,
                channelCount,
                sampleRate,
                processor->analysisEngine_);
        }
        else
        {
            // Update existing registration with new sample rate
            processor->instanceRegistry_.updateSource(processor->sourceId_,
                                                      processor->sourceDisplayName_,
                                                      channelCount,
                                                      sampleRate);
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

void OscilPluginProcessor::updateTrackProperties(const TrackProperties& properties)
{
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    const juce::String normalizedName = normaliseSourceDisplayName(properties);
    if (sourceDisplayName_ == normalizedName)
        return;

    sourceDisplayName_ = normalizedName;

    if (!sourceId_.isValid())
        return;

    instanceRegistry_.updateSource(sourceId_,
                                   sourceDisplayName_,
                                   getTotalNumInputChannels(),
                                   currentSampleRate_.load(std::memory_order_relaxed));
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
    metadata.sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
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
        analysisEngine_->process(buffer, currentSampleRate_.load(std::memory_order_relaxed));

    // Process triggers (MIDI and Audio)
    // Note: TimingEngine uses internal atomic flags to store trigger state
    // processMidi checks for Note On events if in MIDI mode
    // processBlock checks for audio threshold if in Audio mode
    (void) timingEngine_.processMidi(midiMessages);
    (void) timingEngine_.processBlock(buffer);

    // Audio passes through unchanged - this is a visualization plugin

    // Calculate CPU usage using pre-calculated conversion factor (real-time safe)
    // Guard against division by zero: some hosts may call processBlock before prepareToPlay
    double sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
    if (sampleRate <= 0.0)
        return;

    auto endTime = juce::Time::getHighResolutionTicks();
    double elapsedSeconds = static_cast<double>(endTime - startTime) * ticksToSecondsScale_;
    double availableSeconds = static_cast<double>(numSamples) / sampleRate;
    float usage = static_cast<float>(elapsedSeconds / availableSeconds * 100.0);

    // Smooth CPU usage with exponential moving average
    float currentUsage = cpuUsage_.load(std::memory_order_relaxed);
    cpuUsage_.store(currentUsage * 0.9f + usage * 0.1f, std::memory_order_relaxed);
}

bool OscilPluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* OscilPluginProcessor::createEditor()
{
    // Use unique_ptr for exception safety - if constructor throws, memory is cleaned up
    auto editor = std::make_unique<OscilPluginEditor>(*this);
    // JUCE API requires raw pointer (framework takes ownership and deletes the editor)
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return editor.release();
}

void OscilPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // CRITICAL: Some DAWs (Pro Tools, Reaper) may call this from audio thread
    // during save operations. We must not allocate memory in that case.
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        // Audio thread path: return cached UTF-8 bytes (no allocation, no conversion)
        const juce::SpinLock::ScopedTryLockType sl(cachedStateLock_);
        if (sl.isLocked() && !cachedStateUtf8_.empty())
        {
            destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
        }
        // If lock failed or cache empty, return empty - host will retry
        return;
    }

    // Message thread path: perform full serialization (allocates memory)
    updateCachedState();

    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    destData.replaceAll(cachedStateUtf8_.data(), cachedStateUtf8_.size());
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

    // Convert to UTF-8 bytes now (on message thread) to avoid any
    // potential allocation when audio thread reads the cached state.
    const char* utf8Ptr = xmlString.toRawUTF8();
    size_t utf8Size = xmlString.getNumBytesAsUTF8();

    // Update cache with lock
    const juce::SpinLock::ScopedLockType sl(cachedStateLock_);
    cachedStateUtf8_.assign(utf8Ptr, utf8Ptr + utf8Size);
}

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // CRITICAL: Some DAW hosts (Pro Tools, Reaper) may call setStateInformation from
    // the audio thread during session load or preset recall. We must defer ALL state
    // modifications to the message thread because:
    // 1. fromXmlString() allocates memory (not real-time safe)
    // 2. state_ may be accessed by UI thread concurrently (data race)
    // 3. ValueTree operations are not thread-safe
    //
    // Copy the data to a heap-allocated string that can be captured by the lambda.
    // Use WeakReference to handle case where processor is destroyed before callback runs.

    auto xmlString = std::make_shared<juce::String>(
        juce::String::createStringFromData(data, sizeInBytes));
    auto sampleRate = static_cast<int>(currentSampleRate_);

    auto doRestoreState = [weakThis = juce::WeakReference<OscilPluginProcessor>(this),
                           xmlString, sampleRate]() {
        auto* processor = weakThis.get();
        if (!processor) return;

        // Parse and apply state (now safely on message thread)
        if (!processor->state_.fromXmlString(*xmlString))
        {
            DBG("PluginProcessor::setStateInformation - Failed to parse state XML");
            return;
        }

        // Apply restored timing state (TimingEngine uses atomics internally)
        auto timingTree = processor->state_.getState().getChildWithName(StateIds::Timing);
        if (timingTree.isValid())
        {
            processor->timingEngine_.fromValueTree(timingTree);
        }
        else
        {
            // Legacy/malformed payloads may omit Timing node.
            // Re-apply current persisted config to clear runtime-only timing latches.
            processor->timingEngine_.fromValueTree(processor->timingEngine_.toValueTree());
        }

        // Sync restored capture quality config to MemoryBudgetManager
        auto captureConfig = processor->state_.getCaptureQualityConfig();
        processor->setCaptureQualityConfig(captureConfig);
        processor->memoryBudgetManager_.setGlobalConfig(captureConfig, sampleRate);

        // Update capture buffer config (allocates memory)
        processor->captureBuffer_->configure(captureConfig, sampleRate);

        // Update cached state for real-time safe getStateInformation()
        processor->updateCachedState();
    };

    // If already on message thread, execute directly; otherwise defer
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        doRestoreState();
    }
    else
    {
        juce::MessageManager::callAsync(std::move(doRestoreState));
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

int OscilPluginProcessor::getCaptureRate() const
{
    if (captureBuffer_)
        return captureBuffer_->getCaptureRate();
    // Fall back to standard capture rate if buffer not yet configured
    return CaptureRate::STANDARD;
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
        // ValueTree listeners should be called on message thread, but we verify defensively.
        // The state read MUST happen on message thread to avoid race conditions.
        // Use WeakReference to handle case where processor is destroyed before callback runs.
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscilPluginProcessor>(this)]() {
            if (auto* processor = weakThis.get())
            {
                // Read state only on message thread (thread-safe)
                auto captureConfig = processor->state_.getCaptureQualityConfig();
                processor->setCaptureQualityConfig(captureConfig);

                auto sampleRate = static_cast<int>(processor->currentSampleRate_.load(std::memory_order_relaxed));
                processor->captureBuffer_->configure(captureConfig, sampleRate);
            }
        });
    }
}

CaptureQualityConfig OscilPluginProcessor::getCaptureQualityConfig() const
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    return cachedCaptureConfig_;
}

void OscilPluginProcessor::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    cachedCaptureConfig_ = config;
}

} // namespace oscil

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return oscil::PluginFactory::getInstance().createPluginProcessor().release();
}
