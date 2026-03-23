/*
    Oscil - Quality Preset Definitions
    Capture rate constants and quality/buffer duration presets
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

//==============================================================================
// Capture Rate Constants
//==============================================================================

namespace CaptureRate
{
static constexpr int ECO = 11025;      // ~11kHz - minimal memory, suitable for 20+ tracks
static constexpr int STANDARD = 22050; // ~22kHz - balanced quality/memory (default)
static constexpr int HIGH = 44100;     // ~44kHz - high quality visualization
static constexpr int SOURCE = 0;       // Use source sample rate (Ultra mode)

// Maximum supported source rate for buffer calculations
static constexpr int MAX_SOURCE_RATE = 192000;
} // namespace CaptureRate

//==============================================================================
// Quality Preset Enum
//==============================================================================

/**
 * Quality presets for waveform capture resolution
 *
 * Eco:      11kHz capture - Minimal memory (~180KB/track for 4s)
 *           Best for: Laptop users, 20+ tracks, limited RAM
 *
 * Standard: 22kHz capture - Balanced quality/memory (~360KB/track for 4s)
 *           Best for: Most users, default setting
 *
 * High:     44kHz capture - High quality (~720KB/track for 4s)
 *           Best for: Critical listening, fewer tracks
 *
 * Ultra:    Source rate capture - Maximum quality (variable memory)
 *           Best for: Forensic analysis, professional studios
 */
enum class QualityPreset
{
    Eco,
    Standard,
    High,
    Ultra
};

inline juce::String qualityPresetToString(QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Eco:
            return "Eco";
        case QualityPreset::Standard:
            return "Standard";
        case QualityPreset::High:
            return "High";
        case QualityPreset::Ultra:
            return "Ultra";
    }
    return "Standard";
}

inline QualityPreset stringToQualityPreset(const juce::String& str)
{
    if (str == "Eco")
        return QualityPreset::Eco;
    if (str == "High")
        return QualityPreset::High;
    if (str == "Ultra")
        return QualityPreset::Ultra;
    return QualityPreset::Standard;
}

inline juce::String qualityPresetToDisplayName(QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Eco:
            return "Eco (11 kHz)";
        case QualityPreset::Standard:
            return "Standard (22 kHz)";
        case QualityPreset::High:
            return "High (44 kHz)";
        case QualityPreset::Ultra:
            return "Ultra (Source Rate)";
    }
    return "Standard (22 kHz)";
}

//==============================================================================
// Buffer Duration Presets
//==============================================================================

/**
 * Maximum buffer duration options
 * Determines how much history is stored for visualization
 */
enum class BufferDuration
{
    Short,   // 1 second - minimal memory
    Medium,  // 4 seconds - covers most timing intervals (default)
    Long,    // 10 seconds - extended history
    VeryLong // 30 seconds - maximum history
};

inline float bufferDurationToSeconds(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:
            return 1.0f;
        case BufferDuration::Medium:
            return 4.0f;
        case BufferDuration::Long:
            return 10.0f;
        case BufferDuration::VeryLong:
            return 30.0f;
    }
    return 4.0f;
}

inline juce::String bufferDurationToString(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:
            return "1s";
        case BufferDuration::Medium:
            return "4s";
        case BufferDuration::Long:
            return "10s";
        case BufferDuration::VeryLong:
            return "30s";
    }
    return "4s";
}

inline BufferDuration stringToBufferDuration(const juce::String& str)
{
    if (str == "1s")
        return BufferDuration::Short;
    if (str == "10s")
        return BufferDuration::Long;
    if (str == "30s")
        return BufferDuration::VeryLong;
    return BufferDuration::Medium;
}

inline juce::String bufferDurationToDisplayName(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:
            return "1 second";
        case BufferDuration::Medium:
            return "4 seconds";
        case BufferDuration::Long:
            return "10 seconds";
        case BufferDuration::VeryLong:
            return "30 seconds";
    }
    return "4 seconds";
}

} // namespace oscil
