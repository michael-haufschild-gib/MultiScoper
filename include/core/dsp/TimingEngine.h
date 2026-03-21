/*
    Oscil - Timing Engine
    Timing and synchronization system for waveform display
    Aligned with PRD TimingConfig specification
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#include "core/SeqLock.h"
#include "TimingEngineTypes.h"

namespace oscil
{

/**
 * UI-thread-writable timing configuration fields.
 * Published via SeqLock for consistent cross-thread reads.
 */
struct TimingConfigData
{
    TimingMode timingMode = TimingMode::TIME;
    bool hostSyncEnabled = false;
    bool syncToPlayhead = false;
    float timeIntervalMs = 500.0f;
    EngineNoteInterval noteInterval = EngineNoteInterval::NOTE_1_4TH;
    float internalBPM = 120.0f;
    WaveformTriggerMode triggerMode = WaveformTriggerMode::None;
    float triggerThreshold = 0.1f;
    int triggerChannel = 0;
    float triggerHysteresis = 0.01f;
    int midiTriggerNote = -1;
    int midiTriggerChannel = 0;
};

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
     * Returns a consistent snapshot assembled from SeqLock-protected data
     */
    [[nodiscard]] EngineTimingConfig getConfig() const;

    /**
     * Set the engine configuration
     */
    void setConfig(const EngineTimingConfig& config);

    /**
     * Get the current host timing info
     * Returns a consistent snapshot from the host info SeqLock
     */
    [[nodiscard]] HostTimingInfo getHostInfo() const;

    /**
     * Set sample rate (thread-safe)
     */
    void setSampleRate(double sampleRate);

    // Configuration setters (UI thread only)
    void setTimingMode(TimingMode mode);
    void setHostSyncEnabled(bool enabled);
    void setTimeIntervalMs(float ms);
    void setNoteInterval(EngineNoteInterval interval);
    void setInternalBPM(float bpm);
    void setWaveformTriggerMode(WaveformTriggerMode mode);
    void setTriggerThreshold(float threshold);
    void setTriggerChannel(int channel);
    void setTriggerHysteresis(float hysteresis);
    void setMidiTriggerNote(int note);
    void setMidiTriggerChannel(int channel);
    void setSyncToPlayhead(bool enabled);

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
        auto cfg = configLock_.read();
        return engineToEntityNoteInterval(cfg.noteInterval);
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

    // --- SeqLock-protected cross-thread state ---

    // UI-thread-written config (read by audio thread via SeqLock)
    SeqLock<TimingConfigData> configLock_;

    // Audio-thread-written host info (read by UI thread via SeqLock)
    SeqLock<HostTimingInfo> hostInfoLock_;

    // Audio-thread-local mirror of host info for direct access during processBlock.
    // The audio thread updates this first, then publishes via hostInfoLock_.
    HostTimingInfo audioThreadHostInfo_;

    // --- Standalone atomics for values written by both threads or with exchange semantics ---

    // Computed by recalculateInterval() from both threads
    std::atomic<float> atomicActualIntervalMs_{500.0f};

    // Written by audio thread in multiple locations
    std::atomic<double> atomicLastSyncTimestamp_{0.0};

    // Trigger state (thread-safe, uses exchange semantics)
    std::atomic<bool> manualTrigger_{false};
    std::atomic<bool> triggered_{false};
    std::atomic<bool> resetTriggerHistoryPending_{false};

    // Audio-thread-only state (not synchronized — accessed only from processBlock)
    bool previousTriggerState_ = false;
    float previousSample_ = 0.0f;
    float previousBPM_ = 120.0f;

    juce::ListenerList<Listener> listeners_;

    // Pending update flags for lock-free notification (exchange semantics)
    std::atomic<bool> pendingTimingModeChange_{false};
    std::atomic<bool> pendingIntervalChange_{false};
    std::atomic<bool> pendingHostBPMChange_{false};
    std::atomic<bool> pendingHostSyncChange_{false};
    std::atomic<bool> pendingTimeSignatureChange_{false};

    // Trigger detection
    bool detectTrigger(const float* samples, int numSamples);
    bool detectRisingEdge(float sample);
    bool detectFallingEdge(float sample);
    bool detectBothEdges(float sample);
    bool detectLevel(float sample);

    // Notification helpers
    void notifyTimingModeChanged();
    void notifyIntervalChanged();
    void notifyHostBPMChanged();
    void notifyHostSyncStateChanged();
};

// ValueTree identifiers for TimingConfig
namespace TimingIds
{
    inline const juce::Identifier Timing{"Timing"};
    inline const juce::Identifier TimingMode{"timingMode"};
    inline const juce::Identifier HostSyncEnabled{"hostSyncEnabled"};
    inline const juce::Identifier SyncToPlayhead{"syncToPlayhead"};
    inline const juce::Identifier TimeIntervalMs{"timeIntervalMs"};
    inline const juce::Identifier NoteInterval{"noteInterval"};
    inline const juce::Identifier TriggerMode{"triggerMode"};
    inline const juce::Identifier TriggerThreshold{"triggerThreshold"};
    inline const juce::Identifier MidiTriggerNote{"midiTriggerNote"};
    inline const juce::Identifier MidiTriggerChannel{"midiTriggerChannel"};
    inline const juce::Identifier InternalBPM{"internalBPM"};
    inline const juce::Identifier TriggerChannel{"triggerChannel"};
    inline const juce::Identifier TriggerHysteresis{"triggerHysteresis"};
}

} // namespace oscil
