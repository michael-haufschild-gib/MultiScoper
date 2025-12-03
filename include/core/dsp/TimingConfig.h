/*
    Oscil - Timing Configuration Entity
    Manages timing modes, intervals, and host synchronization
    PRD aligned: Entities -> TimingConfig
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace oscil
{

/**
 * Timing mode for display window calculation
 * TIME: User specifies interval in milliseconds
 * MELODIC: User selects musical note interval, calculated from host BPM
 */
enum class TimingMode
{
    TIME,     // Millisecond-based timing
    MELODIC   // Musical note-based timing (requires BPM)
};

inline juce::String timingModeToString(TimingMode mode)
{
    return mode == TimingMode::TIME ? "TIME" : "MELODIC";
}

inline TimingMode stringToTimingMode(const juce::String& str)
{
    return str == "MELODIC" ? TimingMode::MELODIC : TimingMode::TIME;
}

/**
 * Musical note intervals for MELODIC timing mode
 * PRD aligned with complete note interval options
 * Includes standard notes, dotted, triplets, and multi-bar options
 */
enum class NoteInterval
{
    // Standard notes
    THIRTY_SECOND,   // 1/32 - Thirty-second note
    SIXTEENTH,       // 1/16 - Sixteenth note
    TWELFTH,         // 1/12 - Triplet eighth
    EIGHTH,          // 1/8 - Eighth note
    QUARTER,         // 1/4 - Quarter note
    HALF,            // 1/2 - Half note
    WHOLE,           // 1/1 - Whole note (1 bar in 4/4)

    // Multi-bar
    TWO_BARS,        // 2 bars
    THREE_BARS,      // 3 bars
    FOUR_BARS,       // 4 bars
    EIGHT_BARS,      // 8 bars

    // Dotted variants (1.5x duration)
    DOTTED_EIGHTH,   // Dotted eighth note
    DOTTED_QUARTER,  // Dotted quarter note
    DOTTED_HALF,     // Dotted half note

    // Triplet variants (2/3x duration)
    TRIPLET_EIGHTH,  // Eighth note triplet
    TRIPLET_QUARTER, // Quarter note triplet
    TRIPLET_HALF     // Half note triplet
};

inline juce::String noteIntervalToString(NoteInterval interval)
{
    switch (interval)
    {
        case NoteInterval::THIRTY_SECOND:   return "1/32";
        case NoteInterval::SIXTEENTH:       return "1/16";
        case NoteInterval::TWELFTH:         return "1/12";
        case NoteInterval::EIGHTH:          return "1/8";
        case NoteInterval::QUARTER:         return "1/4";
        case NoteInterval::HALF:            return "1/2";
        case NoteInterval::WHOLE:           return "1/1";
        case NoteInterval::TWO_BARS:        return "2 Bars";
        case NoteInterval::THREE_BARS:      return "3 Bars";
        case NoteInterval::FOUR_BARS:       return "4 Bars";
        case NoteInterval::EIGHT_BARS:      return "8 Bars";
        case NoteInterval::DOTTED_EIGHTH:   return "1/8.";
        case NoteInterval::DOTTED_QUARTER:  return "1/4.";
        case NoteInterval::DOTTED_HALF:     return "1/2.";
        case NoteInterval::TRIPLET_EIGHTH:  return "1/8T";
        case NoteInterval::TRIPLET_QUARTER: return "1/4T";
        case NoteInterval::TRIPLET_HALF:    return "1/2T";
    }
    return "1/4";
}

inline juce::String noteIntervalToDisplayName(NoteInterval interval)
{
    switch (interval)
    {
        case NoteInterval::THIRTY_SECOND:   return "32nd Note";
        case NoteInterval::SIXTEENTH:       return "16th Note";
        case NoteInterval::TWELFTH:         return "12th Note";
        case NoteInterval::EIGHTH:          return "8th Note";
        case NoteInterval::QUARTER:         return "Quarter Note";
        case NoteInterval::HALF:            return "Half Note";
        case NoteInterval::WHOLE:           return "Whole Note";
        case NoteInterval::TWO_BARS:        return "2 Bars";
        case NoteInterval::THREE_BARS:      return "3 Bars";
        case NoteInterval::FOUR_BARS:       return "4 Bars";
        case NoteInterval::EIGHT_BARS:      return "8 Bars";
        case NoteInterval::DOTTED_EIGHTH:   return "Dotted 8th";
        case NoteInterval::DOTTED_QUARTER:  return "Dotted Quarter";
        case NoteInterval::DOTTED_HALF:     return "Dotted Half";
        case NoteInterval::TRIPLET_EIGHTH:  return "8th Triplet";
        case NoteInterval::TRIPLET_QUARTER: return "Quarter Triplet";
        case NoteInterval::TRIPLET_HALF:    return "Half Triplet";
    }
    return "Quarter Note";
}

inline NoteInterval stringToNoteInterval(const juce::String& str)
{
    // Standard notes
    if (str == "1/32") return NoteInterval::THIRTY_SECOND;
    if (str == "1/16") return NoteInterval::SIXTEENTH;
    if (str == "1/12") return NoteInterval::TWELFTH;
    if (str == "1/8")  return NoteInterval::EIGHTH;
    if (str == "1/4")  return NoteInterval::QUARTER;
    if (str == "1/2")  return NoteInterval::HALF;
    if (str == "1/1")  return NoteInterval::WHOLE;

    // Multi-bar
    if (str == "2 Bars") return NoteInterval::TWO_BARS;
    if (str == "3 Bars") return NoteInterval::THREE_BARS;
    if (str == "4 Bars") return NoteInterval::FOUR_BARS;
    if (str == "8 Bars") return NoteInterval::EIGHT_BARS;

    // Dotted
    if (str == "1/8.") return NoteInterval::DOTTED_EIGHTH;
    if (str == "1/4.") return NoteInterval::DOTTED_QUARTER;
    if (str == "1/2.") return NoteInterval::DOTTED_HALF;

    // Triplets
    if (str == "1/8T") return NoteInterval::TRIPLET_EIGHTH;
    if (str == "1/4T") return NoteInterval::TRIPLET_QUARTER;
    if (str == "1/2T") return NoteInterval::TRIPLET_HALF;

    return NoteInterval::QUARTER;
}

/**
 * Trigger mode for timing synchronization
 */
enum class TriggerMode
{
    FREE_RUNNING,  // Continuous display without sync
    HOST_SYNC,     // Sync to host transport (play/stop)
    TRIGGERED      // Trigger on signal threshold
};

inline juce::String triggerModeToString(TriggerMode mode)
{
    switch (mode)
    {
        case TriggerMode::FREE_RUNNING: return "FREE_RUNNING";
        case TriggerMode::HOST_SYNC:    return "HOST_SYNC";
        case TriggerMode::TRIGGERED:    return "TRIGGERED";
        default:                        return "FREE_RUNNING";
    }
}

inline TriggerMode stringToTriggerMode(const juce::String& str)
{
    if (str == "HOST_SYNC")  return TriggerMode::HOST_SYNC;
    if (str == "TRIGGERED")  return TriggerMode::TRIGGERED;
    return TriggerMode::FREE_RUNNING;
}

/**
 * Edge detection mode for triggered timing
 */
enum class TriggerEdge
{
    Rising,
    Falling,
    Both
};

inline juce::String triggerEdgeToString(TriggerEdge edge)
{
    switch (edge)
    {
        case TriggerEdge::Rising:  return "Rising";
        case TriggerEdge::Falling: return "Falling";
        case TriggerEdge::Both:    return "Both";
    }
    return "Rising";
}

inline TriggerEdge stringToTriggerEdge(const juce::String& str)
{
    if (str == "Falling") return TriggerEdge::Falling;
    if (str == "Both")    return TriggerEdge::Both;
    return TriggerEdge::Rising;
}

/**
 * Waveform trigger/restart mode
 */
enum class WaveformMode
{
    FreeRunning,    // Waveforms keep running continuously
    RestartOnPlay,  // Waveforms reset when playback starts
    RestartOnNote   // Waveforms reset on MIDI note
};

inline juce::String waveformModeToString(WaveformMode mode)
{
    switch (mode)
    {
        case WaveformMode::FreeRunning:    return "Free Running";
        case WaveformMode::RestartOnPlay:  return "Restart on Play";
        case WaveformMode::RestartOnNote:  return "Restart on Note";
    }
    return "Free Running";
}

inline WaveformMode stringToWaveformMode(const juce::String& str)
{
    if (str == "Restart on Play")  return WaveformMode::RestartOnPlay;
    if (str == "Restart on Note")  return WaveformMode::RestartOnNote;
    return WaveformMode::FreeRunning;
}

/**
 * Timing configuration structure
 * PRD aligned: Entities -> TimingConfig
 */
struct TimingConfig
{
    // Mode selection
    TimingMode timingMode = TimingMode::TIME;
    TriggerMode triggerMode = TriggerMode::FREE_RUNNING;

    // TIME mode settings
    float timeIntervalMs = 500.0f;  // 0.1-4000.0 ms (default 500ms)

    // MELODIC mode settings
    NoteInterval noteInterval = NoteInterval::QUARTER;

    // Host sync settings
    bool hostSyncEnabled = false;
    float hostBPM = 120.0f;        // 20.0-300.0 BPM (from host)
    bool syncToPlayhead = false;   // Align display to host playhead position

    // Trigger settings
    float triggerThreshold = -20.0f;  // dBFS trigger level
    TriggerEdge triggerEdge = TriggerEdge::Rising;  // Edge detection mode

    // Computed values (read-only, updated by calculateActualInterval)
    float actualIntervalMs = 500.0f;   // Final calculated interval in ms

    // Constraints from PRD
    static constexpr float MIN_TIME_INTERVAL_MS = 0.1f;
    static constexpr float MAX_TIME_INTERVAL_MS = 4000.0f;
    static constexpr float DEFAULT_TIME_INTERVAL_MS = 500.0f;
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 300.0f;
    static constexpr float DEFAULT_BPM = 120.0f;

    /**
     * Calculate the actual display interval in milliseconds
     * For TIME mode: returns timeIntervalMs directly
     * For MELODIC mode: calculates from noteInterval and hostBPM
     */
    void calculateActualInterval()
    {
        if (timingMode == TimingMode::TIME)
        {
            actualIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS, timeIntervalMs);
        }
        else // MELODIC mode
        {
            float effectiveBpm = juce::jlimit(MIN_BPM, MAX_BPM, hostBPM);
            float msPerBeat = 60000.0f / effectiveBpm;  // ms per quarter note

            float multiplier = getNoteIntervalMultiplier(noteInterval);
            actualIntervalMs = msPerBeat * multiplier;

            // Clamp to valid range
            actualIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS, actualIntervalMs);
        }
    }

    /**
     * Get the multiplier for a note interval relative to a quarter note
     *
     * @param interval The note interval to convert
     * @param timeSigNumerator The time signature numerator (beats per bar), default 4
     * @return Multiplier relative to a quarter note
     *
     * For bar-based intervals (1 Bar, 2 Bars, etc.), the multiplier depends on
     * the time signature. For example, "1 Bar" in 3/4 time = 3 quarter notes.
     */
    static float getNoteIntervalMultiplier(NoteInterval interval, int timeSigNumerator = 4)
    {
        switch (interval)
        {
            // Standard notes (independent of time signature)
            case NoteInterval::THIRTY_SECOND:   return 0.125f;
            case NoteInterval::SIXTEENTH:       return 0.25f;
            case NoteInterval::TWELFTH:         return 1.0f / 3.0f;   // Triplet eighth
            case NoteInterval::EIGHTH:          return 0.5f;
            case NoteInterval::QUARTER:         return 1.0f;
            case NoteInterval::HALF:            return 2.0f;

            // Bar-based intervals (depend on time signature numerator)
            case NoteInterval::WHOLE:           return static_cast<float>(timeSigNumerator);           // 1 bar
            case NoteInterval::TWO_BARS:        return static_cast<float>(timeSigNumerator) * 2.0f;    // 2 bars
            case NoteInterval::THREE_BARS:      return static_cast<float>(timeSigNumerator) * 3.0f;    // 3 bars
            case NoteInterval::FOUR_BARS:       return static_cast<float>(timeSigNumerator) * 4.0f;    // 4 bars
            case NoteInterval::EIGHT_BARS:      return static_cast<float>(timeSigNumerator) * 8.0f;    // 8 bars

            // Dotted (1.5x duration, independent of time signature)
            case NoteInterval::DOTTED_EIGHTH:   return 0.75f;     // 0.5 * 1.5
            case NoteInterval::DOTTED_QUARTER:  return 1.5f;      // 1 * 1.5
            case NoteInterval::DOTTED_HALF:     return 3.0f;      // 2 * 1.5

            // Triplets (2/3x duration, independent of time signature)
            case NoteInterval::TRIPLET_EIGHTH:  return 1.0f / 3.0f;    // 0.5 * 2/3
            case NoteInterval::TRIPLET_QUARTER: return 2.0f / 3.0f;    // 1 * 2/3
            case NoteInterval::TRIPLET_HALF:    return 4.0f / 3.0f;    // 2 * 2/3
        }
        return 1.0f;
    }

    /**
     * Get the actual interval in samples for a given sample rate
     */
    int getIntervalInSamples(float sampleRate) const
    {
        return static_cast<int>((actualIntervalMs / 1000.0f) * sampleRate);
    }

    /**
     * Update host BPM and recalculate interval if in MELODIC mode
     */
    void setHostBPM(float bpm)
    {
        hostBPM = juce::jlimit(MIN_BPM, MAX_BPM, bpm);
        if (timingMode == TimingMode::MELODIC)
        {
            calculateActualInterval();
        }
    }

    /**
     * Set timing mode and recalculate interval
     */
    void setTimingMode(TimingMode mode)
    {
        timingMode = mode;
        calculateActualInterval();
    }

    /**
     * Set time interval (TIME mode) and recalculate
     */
    void setTimeInterval(float ms)
    {
        timeIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS, ms);
        if (timingMode == TimingMode::TIME)
        {
            calculateActualInterval();
        }
    }

    /**
     * Set note interval (MELODIC mode) and recalculate
     */
    void setNoteInterval(NoteInterval interval)
    {
        noteInterval = interval;
        if (timingMode == TimingMode::MELODIC)
        {
            calculateActualInterval();
        }
    }

    /**
     * Check if host sync is available (host must provide BPM)
     */
    bool isHostSyncAvailable() const
    {
        return hostBPM >= MIN_BPM && hostBPM <= MAX_BPM;
    }

    /**
     * Serialize to ValueTree
     */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("TimingConfig");
        tree.setProperty("timingMode", timingModeToString(timingMode), nullptr);
        tree.setProperty("triggerMode", triggerModeToString(triggerMode), nullptr);
        tree.setProperty("timeIntervalMs", timeIntervalMs, nullptr);
        tree.setProperty("noteInterval", noteIntervalToString(noteInterval), nullptr);
        tree.setProperty("hostSyncEnabled", hostSyncEnabled, nullptr);
        tree.setProperty("hostBPM", hostBPM, nullptr);
        tree.setProperty("syncToPlayhead", syncToPlayhead, nullptr);
        tree.setProperty("triggerThreshold", triggerThreshold, nullptr);
        tree.setProperty("triggerEdge", triggerEdgeToString(triggerEdge), nullptr);
        return tree;
    }

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid()) return;

        timingMode = stringToTimingMode(tree.getProperty("timingMode", "TIME").toString());
        triggerMode = stringToTriggerMode(tree.getProperty("triggerMode", "FREE_RUNNING").toString());
        timeIntervalMs = static_cast<float>(tree.getProperty("timeIntervalMs", 500.0f));
        noteInterval = stringToNoteInterval(tree.getProperty("noteInterval", "1/4").toString());
        hostSyncEnabled = static_cast<bool>(tree.getProperty("hostSyncEnabled", false));
        hostBPM = static_cast<float>(tree.getProperty("hostBPM", 120.0f));
        syncToPlayhead = static_cast<bool>(tree.getProperty("syncToPlayhead", false));
        triggerThreshold = static_cast<float>(tree.getProperty("triggerThreshold", -20.0f));
        triggerEdge = stringToTriggerEdge(tree.getProperty("triggerEdge", "Rising").toString());

        calculateActualInterval();
    }
};

/**
 * Configuration for the background grid display
 */
struct GridConfiguration
{
    bool enabled = true;
    TimingMode timingMode = TimingMode::TIME;
    float visibleDurationMs = 500.0f;
    
    // Musical mode specific
    NoteInterval noteInterval = NoteInterval::QUARTER;
    float bpm = 120.0f;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
};

} // namespace oscil
