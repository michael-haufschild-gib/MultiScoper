/*
    Oscil - Timing Engine
    Timing and synchronization system for waveform display
    Aligned with PRD TimingConfig specification
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <mutex>

// Include entity-level TimingConfig for integration
#include "TimingConfig.h"

namespace oscil
{

// Forward declare entity types (already defined in TimingConfig.h)
// Using separate naming to distinguish engine-internal types from entity types
// Entity types: TimingMode, NoteInterval, TriggerMode (from TimingConfig.h)
// Engine types: EngineNoteInterval, WaveformTriggerMode (internal to TimingEngine)

/**
 * Note interval options for MELODIC mode (engine-internal representation)
 * Use conversion functions to map to/from entity NoteInterval
 */
enum class EngineNoteInterval
{
    NOTE_1_32ND,        // 1/32 note
    NOTE_1_16TH,        // 1/16 note
    NOTE_1_12TH,        // 1/12 note (triplet)
    NOTE_1_8TH,         // 1/8 note
    NOTE_1_4TH,         // 1/4 note (quarter)
    NOTE_1_2,           // 1/2 note (half)
    NOTE_1_1,           // Whole note
    NOTE_2_1,           // 2 bars
    NOTE_3_1,           // 3 bars
    NOTE_4_1,           // 4 bars
    NOTE_8_1,           // 8 bars
    NOTE_DOTTED_1_8TH,  // Dotted 1/8
    NOTE_TRIPLET_1_4TH, // Triplet 1/4
    NOTE_DOTTED_1_4TH,  // Dotted 1/4
    NOTE_TRIPLET_1_2,   // Triplet 1/2
    NOTE_DOTTED_1_2,    // Dotted 1/2
    NOTE_TRIPLET_1_8TH  // Triplet 1/8
};

/**
 * Waveform trigger modes (distinct from entity TriggerMode which is for sync)
 */
enum class WaveformTriggerMode
{
    None,           // No triggering (free running)
    RisingEdge,     // Trigger on rising edge crossing threshold
    FallingEdge,    // Trigger on falling edge crossing threshold
    Level,          // Trigger when level exceeds threshold
    Manual          // Manual trigger only
};

/**
 * Convert EngineNoteInterval to beats (quarter notes)
 * Uses precise fractional calculations for triplets
 */
inline double engineNoteIntervalToBeats(EngineNoteInterval interval)
{
    switch (interval)
    {
        case EngineNoteInterval::NOTE_1_32ND:        return 0.125;
        case EngineNoteInterval::NOTE_1_16TH:        return 0.25;
        case EngineNoteInterval::NOTE_1_12TH:        return 1.0 / 3.0;    // Triplet precision
        case EngineNoteInterval::NOTE_1_8TH:         return 0.5;
        case EngineNoteInterval::NOTE_1_4TH:         return 1.0;
        case EngineNoteInterval::NOTE_1_2:           return 2.0;
        case EngineNoteInterval::NOTE_1_1:           return 4.0;
        case EngineNoteInterval::NOTE_2_1:           return 8.0;
        case EngineNoteInterval::NOTE_3_1:           return 12.0;
        case EngineNoteInterval::NOTE_4_1:           return 16.0;
        case EngineNoteInterval::NOTE_8_1:           return 32.0;
        case EngineNoteInterval::NOTE_DOTTED_1_8TH:  return 0.75;
        case EngineNoteInterval::NOTE_TRIPLET_1_4TH: return 2.0 / 3.0;    // Triplet precision
        case EngineNoteInterval::NOTE_DOTTED_1_4TH:  return 1.5;
        case EngineNoteInterval::NOTE_TRIPLET_1_2:   return 4.0 / 3.0;    // Triplet precision
        case EngineNoteInterval::NOTE_DOTTED_1_2:    return 3.0;
        case EngineNoteInterval::NOTE_TRIPLET_1_8TH: return 1.0 / 3.0;    // Triplet precision
        default:                                      return 1.0;
    }
}

/**
 * Convert EngineNoteInterval to display string
 */
inline juce::String engineNoteIntervalToString(EngineNoteInterval interval)
{
    switch (interval)
    {
        case EngineNoteInterval::NOTE_1_32ND:        return "1/32";
        case EngineNoteInterval::NOTE_1_16TH:        return "1/16";
        case EngineNoteInterval::NOTE_1_12TH:        return "1/12";
        case EngineNoteInterval::NOTE_1_8TH:         return "1/8";
        case EngineNoteInterval::NOTE_1_4TH:         return "1/4";
        case EngineNoteInterval::NOTE_1_2:           return "1/2";
        case EngineNoteInterval::NOTE_1_1:           return "1 Bar";
        case EngineNoteInterval::NOTE_2_1:           return "2 Bars";
        case EngineNoteInterval::NOTE_3_1:           return "3 Bars";
        case EngineNoteInterval::NOTE_4_1:           return "4 Bars";
        case EngineNoteInterval::NOTE_8_1:           return "8 Bars";
        case EngineNoteInterval::NOTE_DOTTED_1_8TH:  return "1/8.";
        case EngineNoteInterval::NOTE_TRIPLET_1_4TH: return "1/4T";
        case EngineNoteInterval::NOTE_DOTTED_1_4TH:  return "1/4.";
        case EngineNoteInterval::NOTE_TRIPLET_1_2:   return "1/2T";
        case EngineNoteInterval::NOTE_DOTTED_1_2:    return "1/2.";
        case EngineNoteInterval::NOTE_TRIPLET_1_8TH: return "1/8T";
        default:                                      return "1/4";
    }
}

/**
 * Convert entity NoteInterval to engine EngineNoteInterval
 */
inline EngineNoteInterval entityToEngineNoteInterval(NoteInterval interval)
{
    switch (interval)
    {
        case NoteInterval::THIRTY_SECOND:   return EngineNoteInterval::NOTE_1_32ND;
        case NoteInterval::SIXTEENTH:       return EngineNoteInterval::NOTE_1_16TH;
        case NoteInterval::TWELFTH:         return EngineNoteInterval::NOTE_1_12TH;
        case NoteInterval::EIGHTH:          return EngineNoteInterval::NOTE_1_8TH;
        case NoteInterval::QUARTER:         return EngineNoteInterval::NOTE_1_4TH;
        case NoteInterval::HALF:            return EngineNoteInterval::NOTE_1_2;
        case NoteInterval::WHOLE:           return EngineNoteInterval::NOTE_1_1;
        case NoteInterval::TWO_BARS:        return EngineNoteInterval::NOTE_2_1;
        case NoteInterval::THREE_BARS:      return EngineNoteInterval::NOTE_3_1;
        case NoteInterval::FOUR_BARS:       return EngineNoteInterval::NOTE_4_1;
        case NoteInterval::EIGHT_BARS:      return EngineNoteInterval::NOTE_8_1;
        case NoteInterval::DOTTED_EIGHTH:   return EngineNoteInterval::NOTE_DOTTED_1_8TH;
        case NoteInterval::DOTTED_QUARTER:  return EngineNoteInterval::NOTE_DOTTED_1_4TH;
        case NoteInterval::DOTTED_HALF:     return EngineNoteInterval::NOTE_DOTTED_1_2;
        case NoteInterval::TRIPLET_EIGHTH:  return EngineNoteInterval::NOTE_TRIPLET_1_8TH;
        case NoteInterval::TRIPLET_QUARTER: return EngineNoteInterval::NOTE_TRIPLET_1_4TH;
        case NoteInterval::TRIPLET_HALF:    return EngineNoteInterval::NOTE_TRIPLET_1_2;
        default:                            return EngineNoteInterval::NOTE_1_4TH;
    }
}

/**
 * Convert engine EngineNoteInterval to entity NoteInterval
 */
inline NoteInterval engineToEntityNoteInterval(EngineNoteInterval interval)
{
    switch (interval)
    {
        case EngineNoteInterval::NOTE_1_32ND:        return NoteInterval::THIRTY_SECOND;
        case EngineNoteInterval::NOTE_1_16TH:        return NoteInterval::SIXTEENTH;
        case EngineNoteInterval::NOTE_1_12TH:        return NoteInterval::TWELFTH;
        case EngineNoteInterval::NOTE_1_8TH:         return NoteInterval::EIGHTH;
        case EngineNoteInterval::NOTE_1_4TH:         return NoteInterval::QUARTER;
        case EngineNoteInterval::NOTE_1_2:           return NoteInterval::HALF;
        case EngineNoteInterval::NOTE_1_1:           return NoteInterval::WHOLE;
        case EngineNoteInterval::NOTE_2_1:           return NoteInterval::TWO_BARS;
        case EngineNoteInterval::NOTE_3_1:           return NoteInterval::THREE_BARS;
        case EngineNoteInterval::NOTE_4_1:           return NoteInterval::FOUR_BARS;
        case EngineNoteInterval::NOTE_8_1:           return NoteInterval::EIGHT_BARS;
        case EngineNoteInterval::NOTE_DOTTED_1_8TH:  return NoteInterval::DOTTED_EIGHTH;
        case EngineNoteInterval::NOTE_TRIPLET_1_4TH: return NoteInterval::TRIPLET_QUARTER;
        case EngineNoteInterval::NOTE_DOTTED_1_4TH:  return NoteInterval::DOTTED_QUARTER;
        case EngineNoteInterval::NOTE_TRIPLET_1_2:   return NoteInterval::TRIPLET_HALF;
        case EngineNoteInterval::NOTE_DOTTED_1_2:    return NoteInterval::DOTTED_HALF;
        case EngineNoteInterval::NOTE_TRIPLET_1_8TH: return NoteInterval::TRIPLET_EIGHTH;
        default:                                      return NoteInterval::QUARTER;
    }
}

/**
 * Engine-internal timing configuration
 * Distinct from entity-level TimingConfig (oscil::TimingConfig)
 */
struct EngineTimingConfig
{
    // Mode selection (uses entity TimingMode)
    TimingMode timingMode = TimingMode::TIME;

    // Host sync
    bool hostSyncEnabled = false;       // Sync with host start/stop events
    bool syncToPlayhead = false;        // Align resets with host playhead

    // TIME mode settings
    float timeIntervalMs = 500.0f;      // Time interval in milliseconds (0.1-4000)

    // MELODIC mode settings (uses engine-internal note interval)
    EngineNoteInterval noteInterval = EngineNoteInterval::NOTE_1_4TH;

    // BPM settings
    float internalBPM = 120.0f;         // User-specified BPM (used when hostSyncEnabled = false)
    float hostBPM = 120.0f;             // Current host BPM from DAW (used when hostSyncEnabled = true)

    // Calculated value
    float actualIntervalMs = 500.0f;    // Computed interval duration

    // Trigger settings (uses engine-internal waveform trigger mode)
    WaveformTriggerMode triggerMode = WaveformTriggerMode::None;
    float triggerThreshold = 0.1f;      // Trigger threshold (0.0 - 1.0)
    int triggerChannel = 0;             // Channel for triggering
    float triggerHysteresis = 0.01f;    // Hysteresis to prevent retriggering

    // Sync state
    double lastSyncTimestamp = 0.0;     // Sample timestamp of last sync point

    // Constraints (same as entity for consistency)
    static constexpr float MIN_TIME_INTERVAL_MS = 0.1f;
    static constexpr float MAX_TIME_INTERVAL_MS = 4000.0f;
    static constexpr float DEFAULT_TIME_INTERVAL_MS = 500.0f;
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 300.0f;
};

/**
 * Host timing information from DAW
 */
struct HostTimingInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;           // Position in quarter notes
    bool isPlaying = false;
    double sampleRate = 44100.0;
    int64_t timeInSamples = 0;

    // Time signature
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;

    // Transport state
    enum class TransportState { STOPPED, PLAYING, RECORDING, PAUSED };
    TransportState transportState = TransportState::STOPPED;
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
     * Request a manual trigger
     */
    void requestManualTrigger();

    /**
     * Get the current engine configuration
     */
    [[nodiscard]] const EngineTimingConfig& getConfig() const { return config_; }

    /**
     * Set the engine configuration
     */
    void setConfig(const EngineTimingConfig& config);

    /**
     * Get the current host timing info
     */
    [[nodiscard]] const HostTimingInfo& getHostInfo() const { return hostInfo_; }

    /**
     * Set sample rate (thread-safe)
     */
    void setSampleRate(double sampleRate)
    {
        atomicSampleRate_.store(sampleRate, std::memory_order_relaxed);
        hostInfo_.sampleRate = sampleRate;
    }

    // Configuration setters
    void setTimingMode(TimingMode mode);
    void setHostSyncEnabled(bool enabled);
    void setTimeIntervalMs(float ms);
    void setNoteInterval(EngineNoteInterval interval);
    void setInternalBPM(float bpm);  // Set user-specified BPM for free-running mode
    void setWaveformTriggerMode(WaveformTriggerMode mode)
    {
        config_.triggerMode = mode;
        atomicTriggerMode_.store(static_cast<int>(mode), std::memory_order_relaxed);
    }
    void setTriggerThreshold(float threshold)
    {
        threshold = juce::jlimit(0.0f, 1.0f, threshold);
        config_.triggerThreshold = threshold;
        atomicTriggerThreshold_.store(threshold, std::memory_order_relaxed);
    }
    void setTriggerChannel(int channel)
    {
        config_.triggerChannel = channel;
        atomicTriggerChannel_.store(channel, std::memory_order_relaxed);
    }
    void setTriggerHysteresis(float hysteresis)
    {
        config_.triggerHysteresis = hysteresis;
        atomicTriggerHysteresis_.store(hysteresis, std::memory_order_relaxed);
    }
    void setSyncToPlayhead(bool enabled) { config_.syncToPlayhead = enabled; }

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
        return engineToEntityNoteInterval(config_.noteInterval);
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
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    EngineTimingConfig config_;
    HostTimingInfo hostInfo_;

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
    std::atomic<double> atomicSampleRate_{ 44100.0 };

    // Trigger state
    std::atomic<bool> manualTrigger_{ false };
    bool previousTriggerState_ = false;
    float previousSample_ = 0.0f;

    // Previous BPM for change detection
    float previousBPM_ = 120.0f;

    juce::ListenerList<Listener> listeners_;
    std::mutex listenerMutex_;  // Protects listener add/remove (call() is already thread-safe)

    // Pending update flags for lock-free notification
    std::atomic<bool> pendingTimingModeChange_{ false };
    std::atomic<bool> pendingIntervalChange_{ false };
    std::atomic<bool> pendingHostBPMChange_{ false };
    std::atomic<bool> pendingHostSyncChange_{ false };

    // Trigger detection
    bool detectTrigger(const float* samples, int numSamples);
    bool detectRisingEdge(float sample);
    bool detectFallingEdge(float sample);
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
}

} // namespace oscil
