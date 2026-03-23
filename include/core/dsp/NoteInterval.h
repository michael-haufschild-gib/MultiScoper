/*
    Oscil - Note Interval Types
    Musical note interval definitions for MELODIC timing mode
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

/**
 * Musical note intervals for MELODIC timing mode
 * PRD aligned with complete note interval options
 * Includes standard notes, dotted, triplets, and multi-bar options
 */
enum class NoteInterval
{
    // Standard notes
    THIRTY_SECOND, // 1/32 - Thirty-second note
    SIXTEENTH,     // 1/16 - Sixteenth note
    TWELFTH,       // 1/12 - Triplet eighth
    EIGHTH,        // 1/8 - Eighth note
    QUARTER,       // 1/4 - Quarter note
    HALF,          // 1/2 - Half note
    WHOLE,         // 1/1 - Whole note (1 bar in 4/4)

    // Multi-bar
    TWO_BARS,   // 2 bars
    THREE_BARS, // 3 bars
    FOUR_BARS,  // 4 bars
    EIGHT_BARS, // 8 bars

    // Dotted variants (1.5x duration)
    DOTTED_EIGHTH,  // Dotted eighth note
    DOTTED_QUARTER, // Dotted quarter note
    DOTTED_HALF,    // Dotted half note

    // Triplet variants (2/3x duration)
    TRIPLET_EIGHTH,  // Eighth note triplet
    TRIPLET_QUARTER, // Quarter note triplet
    TRIPLET_HALF     // Half note triplet
};

inline juce::String noteIntervalToString(NoteInterval interval)
{
    switch (interval)
    {
        case NoteInterval::THIRTY_SECOND:
            return "1/32";
        case NoteInterval::SIXTEENTH:
            return "1/16";
        case NoteInterval::TWELFTH:
            return "1/12";
        case NoteInterval::EIGHTH:
            return "1/8";
        case NoteInterval::QUARTER:
            return "1/4";
        case NoteInterval::HALF:
            return "1/2";
        case NoteInterval::WHOLE:
            return "1/1";
        case NoteInterval::TWO_BARS:
            return "2 Bars";
        case NoteInterval::THREE_BARS:
            return "3 Bars";
        case NoteInterval::FOUR_BARS:
            return "4 Bars";
        case NoteInterval::EIGHT_BARS:
            return "8 Bars";
        case NoteInterval::DOTTED_EIGHTH:
            return "1/8.";
        case NoteInterval::DOTTED_QUARTER:
            return "1/4.";
        case NoteInterval::DOTTED_HALF:
            return "1/2.";
        case NoteInterval::TRIPLET_EIGHTH:
            return "1/8T";
        case NoteInterval::TRIPLET_QUARTER:
            return "1/4T";
        case NoteInterval::TRIPLET_HALF:
            return "1/2T";
    }
    return "1/4";
}

inline juce::String noteIntervalToDisplayName(NoteInterval interval)
{
    switch (interval)
    {
        case NoteInterval::THIRTY_SECOND:
            return "32nd Note";
        case NoteInterval::SIXTEENTH:
            return "16th Note";
        case NoteInterval::TWELFTH:
            return "12th Note";
        case NoteInterval::EIGHTH:
            return "8th Note";
        case NoteInterval::QUARTER:
            return "Quarter Note";
        case NoteInterval::HALF:
            return "Half Note";
        case NoteInterval::WHOLE:
            return "Whole Note";
        case NoteInterval::TWO_BARS:
            return "2 Bars";
        case NoteInterval::THREE_BARS:
            return "3 Bars";
        case NoteInterval::FOUR_BARS:
            return "4 Bars";
        case NoteInterval::EIGHT_BARS:
            return "8 Bars";
        case NoteInterval::DOTTED_EIGHTH:
            return "Dotted 8th";
        case NoteInterval::DOTTED_QUARTER:
            return "Dotted Quarter";
        case NoteInterval::DOTTED_HALF:
            return "Dotted Half";
        case NoteInterval::TRIPLET_EIGHTH:
            return "8th Triplet";
        case NoteInterval::TRIPLET_QUARTER:
            return "Quarter Triplet";
        case NoteInterval::TRIPLET_HALF:
            return "Half Triplet";
    }
    return "Quarter Note";
}

inline NoteInterval stringToNoteInterval(const juce::String& str)
{
    // Standard notes
    if (str == "1/32")
        return NoteInterval::THIRTY_SECOND;
    if (str == "1/16")
        return NoteInterval::SIXTEENTH;
    if (str == "1/12")
        return NoteInterval::TWELFTH;
    if (str == "1/8")
        return NoteInterval::EIGHTH;
    if (str == "1/4")
        return NoteInterval::QUARTER;
    if (str == "1/2")
        return NoteInterval::HALF;
    if (str == "1/1")
        return NoteInterval::WHOLE;

    // Multi-bar
    if (str == "2 Bars")
        return NoteInterval::TWO_BARS;
    if (str == "3 Bars")
        return NoteInterval::THREE_BARS;
    if (str == "4 Bars")
        return NoteInterval::FOUR_BARS;
    if (str == "8 Bars")
        return NoteInterval::EIGHT_BARS;

    // Dotted
    if (str == "1/8.")
        return NoteInterval::DOTTED_EIGHTH;
    if (str == "1/4.")
        return NoteInterval::DOTTED_QUARTER;
    if (str == "1/2.")
        return NoteInterval::DOTTED_HALF;

    // Triplets
    if (str == "1/8T")
        return NoteInterval::TRIPLET_EIGHTH;
    if (str == "1/4T")
        return NoteInterval::TRIPLET_QUARTER;
    if (str == "1/2T")
        return NoteInterval::TRIPLET_HALF;

    return NoteInterval::QUARTER;
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
inline float getNoteIntervalMultiplier(NoteInterval interval, int timeSigNumerator = 4)
{
    switch (interval)
    {
        // Standard notes (independent of time signature)
        case NoteInterval::THIRTY_SECOND:
            return 0.125f;
        case NoteInterval::SIXTEENTH:
            return 0.25f;
        case NoteInterval::TWELFTH:
            return 1.0f / 3.0f; // Triplet eighth
        case NoteInterval::EIGHTH:
            return 0.5f;
        case NoteInterval::QUARTER:
            return 1.0f;
        case NoteInterval::HALF:
            return 2.0f;

        // Bar-based intervals (depend on time signature numerator)
        case NoteInterval::WHOLE:
            return static_cast<float>(timeSigNumerator); // 1 bar
        case NoteInterval::TWO_BARS:
            return static_cast<float>(timeSigNumerator) * 2.0f; // 2 bars
        case NoteInterval::THREE_BARS:
            return static_cast<float>(timeSigNumerator) * 3.0f; // 3 bars
        case NoteInterval::FOUR_BARS:
            return static_cast<float>(timeSigNumerator) * 4.0f; // 4 bars
        case NoteInterval::EIGHT_BARS:
            return static_cast<float>(timeSigNumerator) * 8.0f; // 8 bars

        // Dotted (1.5x duration, independent of time signature)
        case NoteInterval::DOTTED_EIGHTH:
            return 0.75f; // 0.5 * 1.5
        case NoteInterval::DOTTED_QUARTER:
            return 1.5f; // 1 * 1.5
        case NoteInterval::DOTTED_HALF:
            return 3.0f; // 2 * 1.5

        // Triplets (2/3x duration, independent of time signature)
        case NoteInterval::TRIPLET_EIGHTH:
            return 1.0f / 3.0f; // 0.5 * 2/3
        case NoteInterval::TRIPLET_QUARTER:
            return 2.0f / 3.0f; // 1 * 2/3
        case NoteInterval::TRIPLET_HALF:
            return 4.0f / 3.0f; // 2 * 2/3
    }
    return 1.0f;
}

} // namespace oscil
