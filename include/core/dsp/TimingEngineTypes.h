/*
    Oscil - Timing Engine Types
    Enums, conversion functions, and configuration structs for the TimingEngine
*/

#pragma once

#include <juce_core/juce_core.h>
#include "TimingConfig.h"

namespace oscil
{

/**
 * Note interval options for MELODIC mode (engine-internal representation)
 * Use conversion functions to map to/from entity NoteInterval
 */
enum class EngineNoteInterval
{
    NOTE_1_32ND,
    NOTE_1_16TH,
    NOTE_1_12TH,
    NOTE_1_8TH,
    NOTE_1_4TH,
    NOTE_1_2,
    NOTE_1_1,
    NOTE_2_1,
    NOTE_3_1,
    NOTE_4_1,
    NOTE_8_1,
    NOTE_DOTTED_1_8TH,
    NOTE_TRIPLET_1_4TH,
    NOTE_DOTTED_1_4TH,
    NOTE_TRIPLET_1_2,
    NOTE_DOTTED_1_2,
    NOTE_TRIPLET_1_8TH
};

/**
 * Waveform trigger modes (distinct from entity TriggerMode which is for sync)
 */
enum class WaveformTriggerMode
{
    None,
    RisingEdge,
    FallingEdge,
    Level,
    Midi,
    Manual,
    BothEdges
};

/**
 * Convert EngineNoteInterval to beats (quarter notes).
 * Bar-based intervals scale with time signature numerator.
 */
inline double engineNoteIntervalToBeats(EngineNoteInterval interval, int timeSigNumerator = 4)
{
    switch (interval)
    {
        case EngineNoteInterval::NOTE_1_32ND:        return 0.125;
        case EngineNoteInterval::NOTE_1_16TH:        return 0.25;
        case EngineNoteInterval::NOTE_1_12TH:        return 1.0 / 3.0;
        case EngineNoteInterval::NOTE_1_8TH:         return 0.5;
        case EngineNoteInterval::NOTE_1_4TH:         return 1.0;
        case EngineNoteInterval::NOTE_1_2:           return 2.0;
        case EngineNoteInterval::NOTE_1_1:           return static_cast<double>(timeSigNumerator);
        case EngineNoteInterval::NOTE_2_1:           return static_cast<double>(timeSigNumerator) * 2.0;
        case EngineNoteInterval::NOTE_3_1:           return static_cast<double>(timeSigNumerator) * 3.0;
        case EngineNoteInterval::NOTE_4_1:           return static_cast<double>(timeSigNumerator) * 4.0;
        case EngineNoteInterval::NOTE_8_1:           return static_cast<double>(timeSigNumerator) * 8.0;
        case EngineNoteInterval::NOTE_DOTTED_1_8TH:  return 0.75;
        case EngineNoteInterval::NOTE_DOTTED_1_4TH:  return 1.5;
        case EngineNoteInterval::NOTE_DOTTED_1_2:    return 3.0;
        case EngineNoteInterval::NOTE_TRIPLET_1_4TH: return 2.0 / 3.0;
        case EngineNoteInterval::NOTE_TRIPLET_1_2:   return 4.0 / 3.0;
        case EngineNoteInterval::NOTE_TRIPLET_1_8TH: return 1.0 / 3.0;
    }
    return 1.0;
}

/** Convert EngineNoteInterval to display string */
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
    }
    return "1/4";
}

/** Convert entity NoteInterval to engine EngineNoteInterval */
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
    }
    return EngineNoteInterval::NOTE_1_4TH;
}

/** Convert engine EngineNoteInterval to entity NoteInterval */
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
    }
    return NoteInterval::QUARTER;
}

/**
 * Engine-internal timing configuration.
 * Distinct from entity-level TimingConfig (oscil::TimingConfig).
 */
struct EngineTimingConfig
{
    TimingMode timingMode = TimingMode::TIME;
    bool hostSyncEnabled = false;
    bool syncToPlayhead = false;
    float timeIntervalMs = 500.0f;
    EngineNoteInterval noteInterval = EngineNoteInterval::NOTE_1_4TH;
    float internalBPM = 120.0f;
    float hostBPM = 120.0f;
    float actualIntervalMs = 500.0f;
    WaveformTriggerMode triggerMode = WaveformTriggerMode::None;
    float triggerThreshold = 0.1f;
    int triggerChannel = 0;
    float triggerHysteresis = 0.01f;
    int midiTriggerNote = -1;
    int midiTriggerChannel = 0;
    double lastSyncTimestamp = 0.0;

    static constexpr float MIN_TIME_INTERVAL_MS = 0.1f;
    static constexpr float MAX_TIME_INTERVAL_MS = 4000.0f;
    static constexpr float DEFAULT_TIME_INTERVAL_MS = 500.0f;
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 300.0f;
};

/**
 * Host timing information from DAW.
 */
struct HostTimingInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool isPlaying = false;
    double sampleRate = 44100.0;
    int64_t timeInSamples = 0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;

    enum class TransportState { STOPPED, PLAYING, RECORDING, PAUSED };
    TransportState transportState = TransportState::STOPPED;
};

} // namespace oscil
