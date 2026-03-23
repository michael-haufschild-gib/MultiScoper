/*
    Oscil - Timing Configuration Entity
    Manages timing modes, intervals, and host synchronization
    PRD aligned: Entities -> TimingConfig
*/

#pragma once

#include "core/dsp/NoteInterval.h"
#include "core/dsp/TriggerTypes.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <cmath>

namespace oscil
{

/**
 * Timing mode for display window calculation
 * TIME: User specifies interval in milliseconds
 * MELODIC: User selects musical note interval, calculated from host BPM
 */
enum class TimingMode
{
    TIME,   // Millisecond-based timing
    MELODIC // Musical note-based timing (requires BPM)
};

inline juce::String timingModeToString(TimingMode mode) { return mode == TimingMode::TIME ? "TIME" : "MELODIC"; }

inline TimingMode stringToTimingMode(const juce::String& str)
{
    return str == "MELODIC" ? TimingMode::MELODIC : TimingMode::TIME;
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
    float timeIntervalMs = 500.0f; // 0.1-4000.0 ms (default 500ms)

    // MELODIC mode settings
    NoteInterval noteInterval = NoteInterval::QUARTER;

    // Host sync settings
    bool hostSyncEnabled = false;
    float hostBPM = 120.0f;      // 20.0-300.0 BPM (from host)
    bool syncToPlayhead = false; // Align display to host playhead position

    // Trigger settings
    float triggerThreshold = -20.0f;               // dBFS trigger level
    TriggerEdge triggerEdge = TriggerEdge::Rising; // Edge detection mode
    int midiTriggerNote = -1;                      // -1 for any note, 0-127 for specific note
    int midiTriggerChannel = 0;                    // 0 for omni, 1-16 for specific channel

    // Computed values (read-only, updated by calculateActualInterval)
    float actualIntervalMs = 500.0f; // Final calculated interval in ms

    // Constraints from PRD
    static constexpr float MIN_TIME_INTERVAL_MS = 0.1f;
    static constexpr float MAX_TIME_INTERVAL_MS = 4000.0f;
    static constexpr float DEFAULT_TIME_INTERVAL_MS = 500.0f;
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 300.0f;
    static constexpr float DEFAULT_BPM = 120.0f;

    /**
     * Get the multiplier for a note interval relative to a quarter note.
     * Static forwarder to the free function in NoteInterval.h for backward compatibility.
     */
    static float getNoteIntervalMultiplier(NoteInterval interval, int timeSigNumerator = 4)
    {
        return oscil::getNoteIntervalMultiplier(interval, timeSigNumerator);
    }

    /**
     * Calculate the actual display interval in milliseconds
     * For TIME mode: returns timeIntervalMs directly
     * For MELODIC mode: calculates from noteInterval and hostBPM
     */
    void calculateActualInterval()
    {
        auto sanitizeNaN = [](float value, float fallback) { return std::isnan(value) ? fallback : value; };

        if (timingMode == TimingMode::TIME)
        {
            float safeTimeIntervalMs = sanitizeNaN(timeIntervalMs, DEFAULT_TIME_INTERVAL_MS);
            actualIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS, safeTimeIntervalMs);
        }
        else // MELODIC mode
        {
            float safeHostBpm = sanitizeNaN(hostBPM, DEFAULT_BPM);
            float effectiveBpm = juce::jlimit(MIN_BPM, MAX_BPM, safeHostBpm);
            float msPerBeat = 60000.0f / effectiveBpm; // ms per quarter note

            float multiplier = getNoteIntervalMultiplier(noteInterval);
            if (!std::isfinite(multiplier) || multiplier <= 0.0f)
            {
                multiplier = 1.0f;
            }

            actualIntervalMs = msPerBeat * multiplier;

            // Clamp to valid range
            actualIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS,
                                            sanitizeNaN(actualIntervalMs, DEFAULT_TIME_INTERVAL_MS));
        }
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
        float safeBpm = std::isnan(bpm) ? DEFAULT_BPM : bpm;
        hostBPM = juce::jlimit(MIN_BPM, MAX_BPM, safeBpm);
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
        float safeMs = std::isnan(ms) ? DEFAULT_TIME_INTERVAL_MS : ms;
        timeIntervalMs = juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS, safeMs);
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
    bool isHostSyncAvailable() const { return hostBPM >= MIN_BPM && hostBPM <= MAX_BPM; }

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
        tree.setProperty("midiTriggerNote", midiTriggerNote, nullptr);
        tree.setProperty("midiTriggerChannel", midiTriggerChannel, nullptr);
        return tree;
    }

    /**
     * Load from ValueTree
     */
    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid())
            return;

        auto sanitizeFloat = [](float value, float fallback) { return std::isfinite(value) ? value : fallback; };

        timingMode = stringToTimingMode(tree.getProperty("timingMode", "TIME").toString());
        triggerMode = stringToTriggerMode(tree.getProperty("triggerMode", "FREE_RUNNING").toString());
        timeIntervalMs =
            juce::jlimit(MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS,
                         sanitizeFloat(static_cast<float>(tree.getProperty("timeIntervalMs", DEFAULT_TIME_INTERVAL_MS)),
                                       DEFAULT_TIME_INTERVAL_MS));
        noteInterval = stringToNoteInterval(tree.getProperty("noteInterval", "1/4").toString());
        hostSyncEnabled = static_cast<bool>(tree.getProperty("hostSyncEnabled", false));
        hostBPM = juce::jlimit(
            MIN_BPM, MAX_BPM, sanitizeFloat(static_cast<float>(tree.getProperty("hostBPM", DEFAULT_BPM)), DEFAULT_BPM));
        syncToPlayhead = static_cast<bool>(tree.getProperty("syncToPlayhead", false));
        triggerThreshold = sanitizeFloat(static_cast<float>(tree.getProperty("triggerThreshold", -20.0f)), -20.0f);
        triggerEdge = stringToTriggerEdge(tree.getProperty("triggerEdge", "Rising").toString());
        midiTriggerNote = juce::jlimit(-1, 127, static_cast<int>(tree.getProperty("midiTriggerNote", -1)));
        midiTriggerChannel = juce::jlimit(0, 16, static_cast<int>(tree.getProperty("midiTriggerChannel", 0)));

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
