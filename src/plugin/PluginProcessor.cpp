/*
    MultiScoper - Plugin Processor Implementation
    Main audio plugin processor
*/

#include "plugin/PluginProcessor.h"

#include "core/InstanceRegistry.h"
#include "core/MultiScoperLog.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/theme/ThemeManager.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"
#include "rendering/PresetManager.h"
#include "rendering/ShaderRegistry.h"

namespace multiscoper
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

    return "MultiScoper Track";
}
} // namespace

MultiScoperPluginProcessor::MultiScoperPluginProcessor(const PluginProcessorConfig& config)
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , instanceRegistry_(config.instanceRegistry)
    , themeService_(config.themeService)
    , shaderRegistry_(config.shaderRegistry)
    , presetManager_(config.presetManager)
    , memoryBudgetManager_(config.memoryBudgetManager)
{
    // Create capture buffer
    captureBuffer_ = std::make_shared<DecimatingCaptureBuffer>();
    analysisEngine_ = std::make_shared<AnalysisEngine>();

    // Generate a provisional unique track identifier. If the host later calls
    // setStateInformation with a persisted trackIdentifier, we replace this
    // value and re-register under the persisted identity — see
    // PluginProcessorState.cpp::setStateInformation and reRegisterUnderPersistedTrackIdentifier.
    trackIdentifier_ = juce::Uuid().toString();
    state_.setTrackIdentifier(trackIdentifier_);

    // Initialize MemoryBudgetManager with config from state
    // Use default sample rate until prepareToPlay is called
    auto qualityConfig = state_.getCaptureQualityConfig();
    setCaptureQualityConfig(qualityConfig);
    memoryBudgetManager_.setGlobalConfig(qualityConfig, static_cast<int>(currentSampleRate_));

    // Listen for state changes (quality settings)
    state_.getState().addListener(this);

    // Initialize cached state for real-time safe getStateInformation()
    updateCachedState();

    MULTISCOPER_LOG(PLUGIN, "PluginProcessor created: trackId=" << trackIdentifier_);
}

MultiScoperPluginProcessor::~MultiScoperPluginProcessor()
{
    MULTISCOPER_LOG(PLUGIN, "PluginProcessor destroying: trackId=" << trackIdentifier_ << " sourceId=" << sourceId_.id);
    state_.getState().removeListener(this);

    // Unregister from MemoryBudgetManager
    memoryBudgetManager_.unregisterBuffer(trackIdentifier_);

    // Unregister from instance registry
    if (sourceId_.isValid())
    {
        instanceRegistry_.unregisterInstance(sourceId_);
    }
}

const juce::String MultiScoperPluginProcessor::getName() const { return "MultiScoper"; }

bool MultiScoperPluginProcessor::acceptsMidi() const { return true; }
bool MultiScoperPluginProcessor::producesMidi() const { return false; }
bool MultiScoperPluginProcessor::isMidiEffect() const { return false; }

double MultiScoperPluginProcessor::getTailLengthSeconds() const { return 0.0; }

int MultiScoperPluginProcessor::getNumPrograms() { return 1; }
int MultiScoperPluginProcessor::getCurrentProgram() { return 0; }
void MultiScoperPluginProcessor::setCurrentProgram(int /*index*/) {}
const juce::String MultiScoperPluginProcessor::getProgramName(int /*index*/) { return {}; }
void MultiScoperPluginProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/) {}

void MultiScoperPluginProcessor::reRegisterUnderPersistedTrackIdentifier(const juce::String& newTrackIdentifier)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (newTrackIdentifier.isEmpty() || newTrackIdentifier == trackIdentifier_)
        return;

    MULTISCOPER_LOG(PLUGIN, "reRegisterUnderPersistedTrackIdentifier: trackId="
                                << trackIdentifier_ << " -> " << newTrackIdentifier << " (sourceId was " << sourceId_.id
                                << ")");

    const bool wasRegistered = sourceId_.isValid();
    if (wasRegistered)
    {
        instanceRegistry_.unregisterInstance(sourceId_);
        memoryBudgetManager_.unregisterBuffer(trackIdentifier_);
        sourceId_ = SourceId::invalid();
    }

    trackIdentifier_ = newTrackIdentifier;
    state_.setTrackIdentifier(trackIdentifier_);

    // If registration already happened, re-run deferRegistration to bind under
    // the persisted identity. If prepareToPlay has not run yet, the eventual
    // first call will naturally pick up the new trackIdentifier_.
    if (wasRegistered)
        deferRegistration(currentSampleRate_.load(std::memory_order_relaxed));
}

void MultiScoperPluginProcessor::deferRegistration(double sampleRate)
{
    // Memory allocation and mutex operations must happen on message thread.
    // prepareToPlay can be called from audio thread in some hosts (Pro Tools, Reaper).
    auto doRegistration = [weakThis = juce::WeakReference<MultiScoperPluginProcessor>(this), sampleRate,
                           captureConfig = getCaptureQualityConfig(), trackId = trackIdentifier_,
                           channelCount = getTotalNumInputChannels()]() {
        auto* p = weakThis.get();
        if (!p)
            return;

        p->memoryBudgetManager_.setGlobalConfig(captureConfig, static_cast<int>(sampleRate));
        p->captureBuffer_->configure(captureConfig, static_cast<int>(sampleRate));
        p->memoryBudgetManager_.registerBuffer(trackId, p->captureBuffer_);

        if (!p->sourceId_.isValid())
        {
            p->sourceId_ = p->instanceRegistry_.registerInstance(trackId, p->captureBuffer_, p->sourceDisplayName_,
                                                                 channelCount, sampleRate, p->analysisEngine_);
        }
        else
        {
            p->instanceRegistry_.updateSource(p->sourceId_, p->sourceDisplayName_, channelCount, sampleRate);
        }
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        doRegistration();
    else
        juce::MessageManager::callAsync(std::move(doRegistration));
}

void MultiScoperPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_.store(sampleRate, std::memory_order_release);
    currentBlockSize_.store(samplesPerBlock, std::memory_order_release);
    ticksToSecondsScale_ = 1.0 / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());

    timingEngine_.setSampleRate(sampleRate);

    if (analysisEngine_)
    {
        analysisEngine_->prepare(sampleRate, samplesPerBlock);
        (*analysisEngine_).reset();
    }

    MULTISCOPER_LOG(PLUGIN, "prepareToPlay: sampleRate=" << sampleRate << " blockSize=" << samplesPerBlock
                                                         << " inputChannels=" << getTotalNumInputChannels());
    deferRegistration(sampleRate);
}

void MultiScoperPluginProcessor::updateTrackProperties(const TrackProperties& properties)
{
    jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
            juce::MessageManager::getInstance()->isThisTheMessageThread());

    const juce::String normalizedName = normaliseSourceDisplayName(properties);
    if (sourceDisplayName_ == normalizedName)
        return;

    MULTISCOPER_LOG(PLUGIN, "updateTrackProperties: name=" << normalizedName << " (was " << sourceDisplayName_ << ")");
    sourceDisplayName_ = normalizedName;

    if (!sourceId_.isValid())
        return;

    instanceRegistry_.updateSource(sourceId_, sourceDisplayName_, getTotalNumInputChannels(),
                                   currentSampleRate_.load(std::memory_order_relaxed));
}

void MultiScoperPluginProcessor::releaseResources()
{
    // Nothing to release
}

bool MultiScoperPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // CRITICAL: Reject disabled buses - this prevents hosts from passing 0-channel buffers
    // Without this check, some hosts (FL Studio, Ableton) may disable audio routing entirely
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled() ||
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    // Support mono and stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output for effect plugins
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void MultiScoperPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals const noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // Safety check
    if (numSamples == 0 || numChannels == 0)
        return;

    // Invariant: captureBuffer_ is created in the constructor and never null
    jassert(captureBuffer_ != nullptr);

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
    // DecimatingCaptureBuffer handles downsampling and real-time safety internally
    // (uses tryLock on its buffer swap lock — drops frames rather than blocking audio thread)
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

    updateCpuUsage(startTime, numSamples);
}

void MultiScoperPluginProcessor::updateCpuUsage(int64_t startTicks, int numSamples)
{
    double const sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
    if (sampleRate <= 0.0)
        return;

    auto endTime = juce::Time::getHighResolutionTicks();
    double const elapsed = static_cast<double>(endTime - startTicks) * ticksToSecondsScale_;
    double const available = static_cast<double>(numSamples) / sampleRate;
    auto const usage = static_cast<float>(elapsed / available * 100.0);

    float const current = cpuUsage_.load(std::memory_order_relaxed);
    cpuUsage_.store((current * 0.9f) + (usage * 0.1f), std::memory_order_relaxed);
}

bool MultiScoperPluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MultiScoperPluginProcessor::createEditor()
{
    MULTISCOPER_LOG(PLUGIN, "createEditor: sourceId=" << sourceId_.id << " trackId=" << trackIdentifier_);
    // Use unique_ptr for exception safety - if constructor throws, memory is cleaned up
    auto editor = std::make_unique<MultiScoperPluginEditor>(*this);
    // JUCE API requires raw pointer (framework takes ownership and deletes the editor)
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return editor.release();
}

// getStateInformation, updateCachedState, and setStateInformation
// are implemented in PluginProcessorState.cpp

std::shared_ptr<SharedCaptureBuffer> MultiScoperPluginProcessor::getCaptureBuffer() const
{
    return captureBuffer_->getInternalBuffer();
}

SourceId MultiScoperPluginProcessor::getSourceId() const { return sourceId_; }

MultiScoperState& MultiScoperPluginProcessor::getState() { return state_; }

int MultiScoperPluginProcessor::getCaptureRate() const
{
    if (captureBuffer_)
        return captureBuffer_->getCaptureRate();
    // Fall back to standard capture rate if buffer not yet configured
    return CaptureRate::STANDARD;
}

TimingEngine& MultiScoperPluginProcessor::getTimingEngine() { return timingEngine_; }

IInstanceRegistry& MultiScoperPluginProcessor::getInstanceRegistry() { return instanceRegistry_; }

IThemeService& MultiScoperPluginProcessor::getThemeService() { return themeService_; }

ShaderRegistry& MultiScoperPluginProcessor::getShaderRegistry() { return shaderRegistry_; }

PresetManager& MultiScoperPluginProcessor::getPresetManager() { return presetManager_; }

std::shared_ptr<IAudioBuffer> MultiScoperPluginProcessor::getBuffer(const SourceId& sourceId)
{
    // If the requested source is this processor's own source, return local buffer
    if (sourceId == sourceId_)
    {
        return captureBuffer_;
    }

    // Otherwise, look up in registry
    return instanceRegistry_.getCaptureBuffer(sourceId);
}

void MultiScoperPluginProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    // Check if CaptureQuality settings changed
    if (tree.hasType(StateIds::CaptureQuality) ||
        (tree.getParent().isValid() && tree.getParent().hasType(StateIds::CaptureQuality)))
    {
        MULTISCOPER_LOG(PLUGIN, "captureQuality property changed, deferring reconfigure");
        // CRITICAL: Always defer to message thread to guarantee real-time safety
        // ValueTree listeners should be called on message thread, but we verify defensively.
        // The state read MUST happen on message thread to avoid race conditions.
        // Use WeakReference to handle case where processor is destroyed before callback runs.
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<MultiScoperPluginProcessor>(this)]() {
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

CaptureQualityConfig MultiScoperPluginProcessor::getCaptureQualityConfig() const
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    return cachedCaptureConfig_;
}

void MultiScoperPluginProcessor::setCaptureQualityConfig(const CaptureQualityConfig& config)
{
    const juce::SpinLock::ScopedLockType sl(captureConfigLock_);
    cachedCaptureConfig_ = config;
}

} // namespace multiscoper

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return multiscoper::PluginFactory::getInstance().createPluginProcessor().release();
}
