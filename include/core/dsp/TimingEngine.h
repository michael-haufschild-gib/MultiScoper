/*
    Oscil - Timing Engine
    Timing and synchronization system for waveform display
    Aligned with PRD TimingConfig specification
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#include "TimingEngineTypes.h"

namespace oscil
{

/**
 * Timing engine for waveform display synchronization.
 * Handles TIME/MELODIC modes, triggering, and DAW synchronization.
 */
class TimingEngine
{
public:
    TimingEngine();
    ~TimingEngine();

    /**
     * Update host timing information from DAW
     * Called from processBlock
     */
    void updateHostInfo(const juce::AudioPlayHead::PositionInfo& positionInfo);

    /**
     * Process a block of audio for trigger detection
     * @param buffer Audio buffer to analyze
     * @return True if trigger condition met
     */
    [[nodiscard]] bool processBlock(const juce::AudioBuffer<float>& buffer);

    /**
     * Process MIDI buffer for trigger detection
     * @param midiMessages MIDI buffer to analyze
     * @return True if trigger condition met
     */
    [[nodiscard]] bool processMidi(const juce::MidiBuffer& midiMessages);

    /**
     * Get the number of samples to display based on current settings
     */
    [[nodiscard]] int getDisplaySampleCount(double sampleRate) const;

    /**
     * Get the actual interval in milliseconds based on current settings
     */
    [[nodiscard]] float getActualIntervalMs() const;

    /**
     * Get the window size in seconds based on current settings
     */
    [[nodiscard]] double getWindowSizeSeconds() const;

    /**
     * Check if a manual trigger was requested and clear the flag
     */
    [[nodiscard]] bool checkAndClearManualTrigger();

    /**
     * Check if any trigger (Audio, MIDI, Manual) occurred and clear the flag
     */
    [[nodiscard]] bool checkAndClearTrigger();

    /**
     * Request a manual trigger
     */
    void requestManualTrigger();

    /**
     * Get the current engine configuration
     * Constructs configuration from atomic values for thread safety
     */
    [[nodiscard]] EngineTimingConfig getConfig() const;

    /**
     * Set the engine configuration
     */
    void setConfig(const EngineTimingConfig& config);

    /**
     * Get the current host timing info
     * Constructs info from atomic values for thread safety
     */
    [[nodiscard]] HostTimingInfo getHostInfo() const;

    /**
     * Set sample rate (thread-safe)
     */
    void setSampleRate(double sampleRate)
    {
        atomicSampleRate_.store(sampleRate, std::memory_order_relaxed);
    }

    // Configuration setters
    void setTimingMode(TimingMode mode);
    void setHostSyncEnabled(bool enabled);
    void setTimeIntervalMs(float ms);
    void setNoteInterval(EngineNoteInterval interval);
    void setInternalBPM(float bpm);  // Set user-specified BPM for free-running mode
    void setWaveformTriggerMode(WaveformTriggerMode mode)
    {
        atomicTriggerMode_.store(static_cast<int>(mode), std::memory_order_relaxed);
    }
    void setTriggerThreshold(float threshold)
    {
        threshold = juce::jlimit(0.0f, 1.0f, threshold);
        atomicTriggerThreshold_.store(threshold, std::memory_order_relaxed);
    }
    void setTriggerChannel(int channel)
    {
        atomicTriggerChannel_.store(channel, std::memory_order_relaxed);
    }
    void setTriggerHysteresis(float hysteresis)
    {
        atomicTriggerHysteresis_.store(hysteresis, std::memory_order_relaxed);
    }
    void setMidiTriggerNote(int note)
    {
        atomicMidiTriggerNote_.store(note, std::memory_order_relaxed);
    }
    void setMidiTriggerChannel(int channel)
    {
        atomicMidiTriggerChannel_.store(channel, std::memory_order_relaxed);
    }
    void setSyncToPlayhead(bool enabled) { atomicSyncToPlayhead_.store(enabled, std::memory_order_relaxed); }

    /**
     * Recalculate actual interval based on current mode and BPM
     */
    void recalculateInterval();

    // === Entity Integration Methods ===

    /**
     * Apply settings from entity-level TimingConfig
     * Converts entity types to engine-internal types
     */
    void applyEntityConfig(const TimingConfig& entityConfig);

    /**
     * Export current settings as entity-level TimingConfig
     * Converts engine-internal types to entity types
     */
    [[nodiscard]] TimingConfig toEntityConfig() const;

    /**
     * Set note interval from entity type (convenience method)
     */
    void setNoteIntervalFromEntity(NoteInterval interval)
    {
        setNoteInterval(entityToEngineNoteInterval(interval));
    }

    /**
     * Get note interval as entity type (convenience method)
     */
    [[nodiscard]] NoteInterval getNoteIntervalAsEntity() const
    {
        return engineToEntityNoteInterval(static_cast<EngineNoteInterval>(atomicNoteInterval_.load(std::memory_order_relaxed)));
    }

    // === Serialization ===

    /**
     * Serialize configuration to ValueTree
     */
    [[nodiscard]] juce::ValueTree toValueTree() const;

    /**
     * Load configuration from ValueTree
     */
    void fromValueTree(const juce::ValueTree& state);

    /**
     * Dispatch any pending updates to listeners.
     * MUST be called on the message thread (e.g. from a Timer).
     * This replaces the previous async callback mechanism to ensure real-time safety.
     */
    void dispatchPendingUpdates();

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void timingModeChanged(TimingMode /*mode*/) {}
        virtual void intervalChanged(float /*actualIntervalMs*/) {}
        virtual void hostBPMChanged(float /*bpm*/) {}
        virtual void hostSyncStateChanged(bool /*enabled*/) {}
        virtual void timeSignatureChanged(int /*numerator*/, int /*denominator*/) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    void updateHostBPM(const juce::AudioPlayHead::PositionInfo& positionInfo);
    void updateSyncState(bool wasPlaying, bool isPlaying,
                         const juce::Optional<int64_t>& timeInSamples,
                         int64_t previousTimeInSamples);
    void updateHostTimeSignature(const juce::AudioPlayHead::PositionInfo& positionInfo);
    void resetRuntimeStateForLoad();
    void loadTimingProperties(const juce::ValueTree& state);

    // Thread-safe copies of values for cross-thread access.
    // UI thread writes config via setters → stores to atomics
    // Audio thread reads from atomics in processBlock/updateHostInfo
    std::atomic<float> atomicHostBPM_{ 120.0f };
    std::atomic<float> atomicInternalBPM_{ 120.0f };
    std::atomic<float> atomicTimeIntervalMs_{ 500.0f };
    std::atomic<float> atomicActualIntervalMs_{ 500.0f };

    // Atomics for fields read from audio thread (written by UI thread setters)
    std::atomic<int> atomicTimingMode_{ static_cast<int>(TimingMode::TIME) };
    std::atomic<int> atomicNoteInterval_{ static_cast<int>(EngineNoteInterval::NOTE_1_4TH) };
    std::atomic<bool> atomicHostSyncEnabled_{ false };
    std::atomic<int> atomicTriggerMode_{ static_cast<int>(WaveformTriggerMode::None) };
    std::atomic<int> atomicTriggerChannel_{ 0 };
    std::atomic<float> atomicTriggerThreshold_{ 0.1f };
    std::atomic<float> atomicTriggerHysteresis_{ 0.01f };
    std::atomic<int> atomicMidiTriggerNote_{ -1 };
    std::atomic<int> atomicMidiTriggerChannel_{ 0 };
    std::atomic<double> atomicSampleRate_{ 44100.0 };
    std::atomic<bool> atomicSyncToPlayhead_{ false };
    std::atomic<double> atomicLastSyncTimestamp_{ 0.0 };

    // Host info atomics
    std::atomic<double> atomicPpqPosition_{ 0.0 };
    std::atomic<bool> atomicIsPlaying_{ false };
    std::atomic<int64_t> atomicTimeInSamples_{ 0 };
    std::atomic<int> atomicTimeSigNumerator_{ 4 };
    std::atomic<int> atomicTimeSigDenominator_{ 4 };
    std::atomic<int> atomicTransportState_{ static_cast<int>(HostTimingInfo::TransportState::STOPPED) };

    // Trigger state (thread-safe)
    std::atomic<bool> manualTrigger_{ false };
    std::atomic<bool> triggered_{ false };  // Unified trigger flag
    std::atomic<bool> resetTriggerHistoryPending_{ false };

    // Audio-thread-only state (accessed only from processBlock, not synchronized)
    bool previousTriggerState_ = false;
    float previousSample_ = 0.0f;
    float previousBPM_ = 120.0f;  // For change detection

    juce::ListenerList<Listener> listeners_;  // Message-thread-only (add/remove require jassert)

    // Pending update flags for lock-free notification
    std::atomic<bool> pendingTimingModeChange_{ false };
    std::atomic<bool> pendingIntervalChange_{ false };
    std::atomic<bool> pendingHostBPMChange_{ false };
    std::atomic<bool> pendingHostSyncChange_{ false };
    std::atomic<bool> pendingTimeSignatureChange_{ false };

    // Trigger detection
    bool detectTrigger(const float* samples, int numSamples);
    bool detectRisingEdge(float sample);
    bool detectFallingEdge(float sample);
    bool detectBothEdges(float sample);
    bool detectLevel(float sample);

    // Notification helpers (now just set flags)
    void notifyTimingModeChanged();
    void notifyIntervalChanged();
    void notifyHostBPMChanged();
    void notifyHostSyncStateChanged();
};

// ValueTree identifiers for TimingConfig
namespace TimingIds
{
    static const juce::Identifier Timing{ "Timing" };
    static const juce::Identifier TimingMode{ "timingMode" };
    static const juce::Identifier HostSyncEnabled{ "hostSyncEnabled" };
    static const juce::Identifier SyncToPlayhead{ "syncToPlayhead" };
    static const juce::Identifier TimeIntervalMs{ "timeIntervalMs" };
    static const juce::Identifier NoteInterval{ "noteInterval" };
    static const juce::Identifier TriggerMode{ "triggerMode" };
    static const juce::Identifier TriggerThreshold{ "triggerThreshold" };
    static const juce::Identifier MidiTriggerNote{ "midiTriggerNote" };
    static const juce::Identifier MidiTriggerChannel{ "midiTriggerChannel" };
    static const juce::Identifier InternalBPM{ "internalBPM" };
    static const juce::Identifier TriggerChannel{ "triggerChannel" };
    static const juce::Identifier TriggerHysteresis{ "triggerHysteresis" };
}

} // namespace oscil
