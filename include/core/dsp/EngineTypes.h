/*
    Oscil - Engine Types
    Internal types for DSP engine
*/

#pragma once

#include <juce_core/juce_core.h>
#include "core/dsp/TimingConfig.h"

namespace oscil
{

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
    Midi,           // Trigger on MIDI Note On
    Manual          // Manual trigger only
};

/**
 * Convert EngineNoteInterval to beats (quarter notes)
 * Uses precise fractional calculations for triplets
 */
inline double engineNoteIntervalToBeats(EngineNoteInterval interval, int timeSigNumerator = 4)
{
    switch (interval)
    {
        // Standard note intervals (independent of time signature)
        case EngineNoteInterval::NOTE_1_32ND:        return 0.125;
        case EngineNoteInterval::NOTE_1_16TH:        return 0.25;
        case EngineNoteInterval::NOTE_1_12TH:        return 1.0 / 3.0;    // Triplet precision
        case EngineNoteInterval::NOTE_1_8TH:         return 0.5;
        case EngineNoteInterval::NOTE_1_4TH:         return 1.0;
        case EngineNoteInterval::NOTE_1_2:           return 2.0;

        // Bar-based intervals (depend on time signature numerator)
        case EngineNoteInterval::NOTE_1_1:           return static_cast<double>(timeSigNumerator);           // 1 bar
        case EngineNoteInterval::NOTE_2_1:           return static_cast<double>(timeSigNumerator) * 2.0;     // 2 bars
        case EngineNoteInterval::NOTE_3_1:           return static_cast<double>(timeSigNumerator) * 3.0;     // 3 bars
        case EngineNoteInterval::NOTE_4_1:           return static_cast<double>(timeSigNumerator) * 4.0;     // 4 bars
        case EngineNoteInterval::NOTE_8_1:           return static_cast<double>(timeSigNumerator) * 8.0;     // 8 bars

        // Dotted notes (independent of time signature)
        case EngineNoteInterval::NOTE_DOTTED_1_8TH:  return 0.75;
        case EngineNoteInterval::NOTE_DOTTED_1_4TH:  return 1.5;
        case EngineNoteInterval::NOTE_DOTTED_1_2:    return 3.0;

        // Triplet notes (independent of time signature)
        case EngineNoteInterval::NOTE_TRIPLET_1_4TH: return 2.0 / 3.0;    // Triplet precision
        case EngineNoteInterval::NOTE_TRIPLET_1_2:   return 4.0 / 3.0;    // Triplet precision
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

} // namespace oscil
