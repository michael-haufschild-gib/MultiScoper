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
#include "core/AudioCapturePool.h"
#include "core/CaptureThread.h"
#include "core/dsp/TimingEngine.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/theme/IThemeService.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

namespace oscil
{

class ShaderRegistry;

/**
 * Main audio processor for the Oscil plugin.
 *
 * Thread Safety:
 * - Audio callback methods (processBlock, prepareToPlay, releaseResources) run on audio thread
 * - Most UI callbacks and getters should only be called from message thread
 * - getCaptureBuffer/getDecimatingCaptureBuffer are thread-safe (lock-free path available)
 * - getCpuUsage/getSampleRate/getCaptureRate are thread-safe (use atomics)
 * - State serialization (get/setStateInformation) may be called from audio thread by some
 *   hosts (Pro Tools, Reaper); uses cached state with lock-free path for safety
 *
 * Lifecycle:
 * - Constructor initializes services and registers with InstanceRegistry
 * - prepareToPlay called before audio processing begins
 * - releaseResources called when audio stops
 * - Destructor unregisters from InstanceRegistry
 */
class OscilPluginProcessor : public juce::AudioProcessor
    , public IAudioDataProvider
    , public juce::ValueTree::Listener
{
public:
    // Constructor with dependency injection
    // AudioCapturePool and CaptureThread are centralized (owned by PluginFactory)
    OscilPluginProcessor(IInstanceRegistry& instanceRegistry, 
                         IThemeService& themeService, 
                         ShaderRegistry& shaderRegistry, 
                         MemoryBudgetManager& memoryBudgetManager,
                         AudioCapturePool& audioCapturePool,
                         CaptureThread& captureThread);
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

    // Track properties from host (provides track name)
    void updateTrackProperties(const TrackProperties& properties) override;

    // Oscil-specific methods
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer() const;
    [[nodiscard]] std::shared_ptr<DecimatingCaptureBuffer> getDecimatingCaptureBuffer() const;
    [[nodiscard]] SourceId getSourceId() const;
    [[nodiscard]] const juce::String& getInstanceUUID() const { return instanceUUID_; }
    [[nodiscard]] TimingEngine& getTimingEngine();

    // Lock-free capture pool access
    [[nodiscard]] int getCaptureSlotIndex() const { return captureSlotIndex_.load(std::memory_order_acquire); }
    [[nodiscard]] bool usesLockFreeCapture() const { return lockFreeReady_.load(std::memory_order_acquire); }
    void setUseLockFreeCapture(bool enable);

    // Access to pool and capture thread (for registry integration)
    [[nodiscard]] AudioCapturePool& getCapturePool() { return capturePool_; }
    [[nodiscard]] CaptureThread& getCaptureThread() { return captureThread_; }

    // Check if lock-free path is fully initialized and ready
    [[nodiscard]] bool isLockFreePathReady() const { return lockFreeReady_.load(std::memory_order_acquire); }

    // Service access for dependency injection
    // UI components should use these instead of accessing singletons directly
    IInstanceRegistry& getInstanceRegistry();
    IThemeService& getThemeService();
    ShaderRegistry& getShaderRegistry();

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
    IInstanceRegistry& instanceRegistry_;       // Injected dependency
    IThemeService& themeService_;               // Injected dependency
    ShaderRegistry& shaderRegistry_;            // Injected dependency
    MemoryBudgetManager& memoryBudgetManager_;  // Injected dependency

    // Legacy capture buffer (backward compatibility)
    std::shared_ptr<DecimatingCaptureBuffer> captureBuffer_;
    std::shared_ptr<AnalysisEngine> analysisEngine_;

    // Lock-free capture system (new architecture)
    // CENTRALIZED: Pool and thread are owned by PluginFactory, shared across all instances
    AudioCapturePool& capturePool_;             // Reference to centralized pool
    CaptureThread& captureThread_;              // Reference to centralized thread
    std::atomic<int> captureSlotIndex_{AudioCapturePool::INVALID_SLOT}; // Slot in pool for this instance
    std::atomic<bool> lockFreeReady_{false};    // True when slot is allocated and configured
    std::atomic<bool> useLockFreeCapture_{true}; // User preference for lock-free vs legacy path

    // Unique identifier for this instance
    juce::String trackIdentifier_;
    juce::String instanceUUID_;  // Persistent UUID (survives DAW session reload)
    SourceId sourceId_;

    // Track name from host (updated via updateTrackProperties)
    juce::String trackName_;

    OscilState state_;
    TimingEngine timingEngine_;

    std::atomic<double> currentSampleRate_{44100.0};
    std::atomic<int> currentBlockSize_{512};

    // H48 FIX: Reentrancy guard for prepareToPlay
    // Some hosts may call prepareToPlay reentrantly
    std::atomic<bool> preparingToPlay_{false};

    // CPU usage tracking
    std::atomic<float> cpuUsage_{0.0f};

    // Pre-calculated conversion factor for CPU timing (real-time safe)
    double ticksToSecondsScale_{0.0};

    // Cached state for real-time safe getStateInformation()
    // Stores pre-converted UTF-8 bytes to avoid any allocation on audio thread.
    // Updated on message thread, read by audio thread when needed.
    std::vector<char> cachedStateUtf8_;
    mutable juce::SpinLock cachedStateLock_;

    // Pre-allocated buffer for real-time safe setStateInformation()
    // When setStateInformation is called from audio thread, we copy to this buffer
    // and defer processing to message thread - no allocation on audio thread.
    static constexpr size_t MAX_STATE_SIZE = 64 * 1024;  // 64KB should cover any preset
    std::array<char, MAX_STATE_SIZE> pendingStateBuffer_;
    std::atomic<int> pendingStateSize_{0};  // 0 = no pending state

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
