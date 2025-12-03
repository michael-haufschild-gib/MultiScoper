/*
    Oscil - Plugin Processor Header
    Main audio plugin processor
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "core/OscilState.h"
#include "core/dsp/TimingEngine.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "ui/theme/IThemeService.h"
#include <memory>

namespace oscil
{

class OscilPluginProcessor : public juce::AudioProcessor,
                             public IAudioDataProvider
{
public:
    // Constructor with dependency injection
    OscilPluginProcessor(IInstanceRegistry& instanceRegistry, IThemeService& themeService);
    ~OscilPluginProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    [[nodiscard]] const juce::String getName() const override;

    [[nodiscard]] bool acceptsMidi() const override;
    [[nodiscard]] bool producesMidi() const override;
    [[nodiscard]] bool isMidiEffect() const override;
    [[nodiscard]] double getTailLengthSeconds() const override;

    [[nodiscard]] int getNumPrograms() override;
    [[nodiscard]] int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    [[nodiscard]] const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Oscil-specific methods
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const;
    [[nodiscard]] SourceId getSourceId() const;
    [[nodiscard]] TimingEngine& getTimingEngine();

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
    IInstanceRegistry& instanceRegistry_; // Injected dependency
    IThemeService& themeService_;          // Injected dependency

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
