/*
    Oscil - Plugin Processor Header
    Main audio plugin processor
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "InstanceRegistry.h"
#include "SharedCaptureBuffer.h"
#include "OscilState.h"
#include "dsp/TimingEngine.h"
#include "IInstanceRegistry.h"
#include "IAudioDataProvider.h"
#include "ui/IThemeService.h"
#include <memory>

namespace oscil
{

class OscilPluginProcessor : public juce::AudioProcessor,
                             public IAudioDataProvider
{
public:
    OscilPluginProcessor();
    ~OscilPluginProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Oscil-specific methods
    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const;
    SourceId getSourceId() const;
    TimingEngine& getTimingEngine();

    // Service access for dependency injection
    // UI components should use these instead of accessing singletons directly
    IInstanceRegistry& getInstanceRegistry();
    IThemeService& getThemeService();

    // IAudioDataProvider interface
    std::shared_ptr<IAudioBuffer> getBuffer(const SourceId& sourceId) override;
    OscilState& getState() override;
    float getCpuUsage() const override { return cpuUsage_.load(std::memory_order_relaxed); }
    double getSampleRate() const override { return currentSampleRate_; }

private:
    std::shared_ptr<SharedCaptureBuffer> captureBuffer_;
    SourceId sourceId_;
    juce::String trackIdentifier_;
    OscilState state_;
    TimingEngine timingEngine_;

    double currentSampleRate_ = 44100.0;
    int currentBlockSize_ = 512;

    // CPU usage tracking
    std::atomic<float> cpuUsage_{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilPluginProcessor)
};

} // namespace oscil
