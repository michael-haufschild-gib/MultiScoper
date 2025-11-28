/*
    Oscil - Plugin Processor Implementation
    Main audio plugin processor
*/

#include "core/PluginProcessor.h"
#include "ui/PluginEditor.h"
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/ThemeManager.h"

namespace oscil
{

OscilPluginProcessor::OscilPluginProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Create capture buffer
    captureBuffer_ = std::make_shared<SharedCaptureBuffer>();

    // Generate unique track identifier
    trackIdentifier_ = juce::Uuid().toString();
}

OscilPluginProcessor::~OscilPluginProcessor()
{
    // Unregister from instance registry
    if (sourceId_.isValid())
    {
        InstanceRegistry::getInstance().unregisterInstance(sourceId_);
    }
}

const juce::String OscilPluginProcessor::getName() const
{
    return "Oscil";
}

bool OscilPluginProcessor::acceptsMidi() const { return false; }
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

    timingEngine_.setSampleRate(sampleRate);

    // Register with instance registry
    sourceId_ = InstanceRegistry::getInstance().registerInstance(
        trackIdentifier_,
        captureBuffer_,
        "Oscil Track",
        getTotalNumInputChannels(),
        sampleRate);

    if (!sourceId_.isValid())
    {
        DBG("OscilPluginProcessor: Failed to register instance - max sources reached");
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
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    // Support mono and stereo only
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output for effect plugins
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void OscilPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
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
    captureBuffer_->write(buffer, metadata);

    // Process trigger detection (read-only operation)
    timingEngine_.processBlock(buffer);

    // Audio passes through unchanged - this is a visualization plugin

    // Calculate CPU usage
    auto endTime = juce::Time::getHighResolutionTicks();
    double elapsedSeconds = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
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
    destData.replaceAll(xmlString.toRawUTF8(), xmlString.getNumBytesAsUTF8());
}

void OscilPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xmlString = juce::String::createStringFromData(data, sizeInBytes);
    state_.fromXmlString(xmlString);

    // Apply restored state
    timingEngine_.fromValueTree(state_.getState().getChildWithName(StateIds::Timing));
}

std::shared_ptr<SharedCaptureBuffer> OscilPluginProcessor::getCaptureBuffer() const
{
    return captureBuffer_;
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
    return InstanceRegistry::getInstance();
}

IThemeService& OscilPluginProcessor::getThemeService()
{
    return ThemeManager::getInstance();
}

std::shared_ptr<IAudioBuffer> OscilPluginProcessor::getBuffer(const SourceId& sourceId)
{
    // If the requested source is this processor's own source, return local buffer
    if (sourceId == sourceId_)
    {
        return captureBuffer_;
    }

    // Otherwise, look up in registry
    return InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
}

} // namespace oscil

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new oscil::OscilPluginProcessor();
}
