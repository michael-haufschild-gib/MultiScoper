/*
    Oscil - Plugin Processor Header
    Main audio plugin processor
*/

#pragma once

#include "core/DecimatingCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/OscilState.h"
#include "core/SharedCaptureBuffer.h"
#include "core/dsp/TimingEngine.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/theme/IThemeService.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace oscil
{

class ShaderRegistry;
class PresetManager;

/**
 * Aggregates the injected dependencies for OscilPluginProcessor construction.
 */
struct PluginProcessorConfig
{
    IInstanceRegistry& instanceRegistry;
    IThemeService& themeService;
    ShaderRegistry& shaderRegistry;
    PresetManager& presetManager;
    MemoryBudgetManager& memoryBudgetManager;
};

class OscilPluginProcessor : public juce::AudioProcessor
    , public IAudioDataProvider
    , public juce::ValueTree::Listener
{
public:
    // Primary constructor with aggregated config
    explicit OscilPluginProcessor(const PluginProcessorConfig& config);

    // Legacy constructor — delegates to the config constructor
    OscilPluginProcessor(IInstanceRegistry& instanceRegistry, IThemeService& themeService,
                         ShaderRegistry& shaderRegistry, PresetManager& presetManager,
                         MemoryBudgetManager& memoryBudgetManager);
    ~OscilPluginProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void updateTrackProperties(const TrackProperties& properties) override;

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
    ShaderRegistry& getShaderRegistry();
    PresetManager& getPresetManager();

    // IAudioDataProvider interface
    std::shared_ptr<IAudioBuffer> getBuffer(const SourceId& sourceId) override;
    OscilState& getState() override;
    float getCpuUsage() const override { return cpuUsage_.load(std::memory_order_relaxed); }
    double getSampleRate() const override { return currentSampleRate_.load(std::memory_order_relaxed); }
    int getCaptureRate() const override;

    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*child*/) override {}
    void valueTreeChildRemoved(juce::ValueTree& /*parentTree*/, juce::ValueTree& /*child*/, int /*index*/) override {}
    void valueTreeChildOrderChanged(juce::ValueTree& /*parentTree*/, int /*oldIndex*/, int /*newIndex*/) override {}
    void valueTreeParentChanged(juce::ValueTree& /*tree*/) override {}

private:
    void deferRegistration(double sampleRate);
    void updateCpuUsage(int64_t startTicks, int numSamples);

    IInstanceRegistry& instanceRegistry_;       // Injected dependency
    IThemeService& themeService_;               // Injected dependency
    ShaderRegistry& shaderRegistry_;            // Injected dependency
    PresetManager& presetManager_;              // Injected dependency
    MemoryBudgetManager& memoryBudgetManager_;  // Injected dependency

    // Capture buffer
    std::shared_ptr<DecimatingCaptureBuffer> captureBuffer_;
    std::shared_ptr<AnalysisEngine> analysisEngine_;

    // Unique identifier for this instance
    juce::String trackIdentifier_;
    juce::String sourceDisplayName_{"Oscil Track"};
    SourceId sourceId_;

    OscilState state_;
    TimingEngine timingEngine_;

    std::atomic<double> currentSampleRate_{44100.0};
    std::atomic<int> currentBlockSize_{512};

    // CPU usage tracking
    std::atomic<float> cpuUsage_{0.0f};

    // Pre-calculated conversion factor for CPU timing (real-time safe)
    double ticksToSecondsScale_{0.0};

    // Cached state for real-time safe getStateInformation()
    // Stores pre-converted UTF-8 bytes to avoid any allocation on audio thread.
    // Updated on message thread, read by audio thread when needed.
    std::vector<char> cachedStateUtf8_;
    mutable juce::SpinLock cachedStateLock_;

    // Helper to update cached state (call from message thread only)
    void updateCachedState();

    // Thread-safe cache of capture quality config
    // Prevents race conditions when prepareToPlay reads state while UI modifies it
    CaptureQualityConfig cachedCaptureConfig_;
    mutable juce::SpinLock captureConfigLock_;

    // Helper to thread-safely get current capture config
    CaptureQualityConfig getCaptureQualityConfig() const;
    
    // Helper to thread-safely set current capture config
    void setCaptureQualityConfig(const CaptureQualityConfig& config);

    JUCE_DECLARE_WEAK_REFERENCEABLE(OscilPluginProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilPluginProcessor)
};

} // namespace oscil
