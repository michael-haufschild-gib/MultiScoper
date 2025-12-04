/*
    Oscil - Capture Quality Configuration
    Manages waveform capture resolution and memory budget settings
    Enables adaptive quality based on track count and available memory
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <cstddef>

namespace oscil
{

//==============================================================================
// Capture Rate Constants
//==============================================================================

namespace CaptureRate
{
    static constexpr int ECO = 11025;        // ~11kHz - minimal memory, suitable for 20+ tracks
    static constexpr int STANDARD = 22050;   // ~22kHz - balanced quality/memory (default)
    static constexpr int HIGH = 44100;       // ~44kHz - high quality visualization
    static constexpr int SOURCE = 0;         // Use source sample rate (Ultra mode)

    // Maximum supported source rate for buffer calculations
    static constexpr int MAX_SOURCE_RATE = 192000;
}

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
        case QualityPreset::Eco:      return "Eco";
        case QualityPreset::Standard: return "Standard";
        case QualityPreset::High:     return "High";
        case QualityPreset::Ultra:    return "Ultra";
    }
    return "Standard";
}

inline QualityPreset stringToQualityPreset(const juce::String& str)
{
    if (str == "Eco")      return QualityPreset::Eco;
    if (str == "High")     return QualityPreset::High;
    if (str == "Ultra")    return QualityPreset::Ultra;
    return QualityPreset::Standard;
}

inline juce::String qualityPresetToDisplayName(QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Eco:      return "Eco (11 kHz)";
        case QualityPreset::Standard: return "Standard (22 kHz)";
        case QualityPreset::High:     return "High (44 kHz)";
        case QualityPreset::Ultra:    return "Ultra (Source Rate)";
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
    Short,      // 1 second - minimal memory
    Medium,     // 4 seconds - covers most timing intervals (default)
    Long,       // 10 seconds - extended history
    VeryLong    // 30 seconds - maximum history
};

inline float bufferDurationToSeconds(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:     return 1.0f;
        case BufferDuration::Medium:    return 4.0f;
        case BufferDuration::Long:      return 10.0f;
        case BufferDuration::VeryLong:  return 30.0f;
    }
    return 4.0f;
}

inline juce::String bufferDurationToString(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:     return "1s";
        case BufferDuration::Medium:    return "4s";
        case BufferDuration::Long:      return "10s";
        case BufferDuration::VeryLong:  return "30s";
    }
    return "4s";
}

inline BufferDuration stringToBufferDuration(const juce::String& str)
{
    if (str == "1s")  return BufferDuration::Short;
    if (str == "10s") return BufferDuration::Long;
    if (str == "30s") return BufferDuration::VeryLong;
    return BufferDuration::Medium;
}

inline juce::String bufferDurationToDisplayName(BufferDuration duration)
{
    switch (duration)
    {
        case BufferDuration::Short:     return "1 second";
        case BufferDuration::Medium:    return "4 seconds";
        case BufferDuration::Long:      return "10 seconds";
        case BufferDuration::VeryLong:  return "30 seconds";
    }
    return "4 seconds";
}

//==============================================================================
// Memory Budget Configuration
//==============================================================================

/**
 * Memory budget limits for waveform capture buffers
 */
struct MemoryBudget
{
    size_t totalBudgetBytes = 50 * 1024 * 1024;     // 50 MB default total budget
    size_t perTrackMinBytes = 100 * 1024;            // 100 KB minimum per track
    size_t perTrackMaxBytes = 2 * 1024 * 1024;       // 2 MB maximum per track

    /**
     * Calculate optimal per-track budget based on track count
     * Returns bytes per track, clamped to min/max limits
     */
    [[nodiscard]] size_t calculatePerTrackBudget(int numTracks) const
    {
        if (numTracks <= 0)
            return perTrackMaxBytes;

        size_t perTrack = totalBudgetBytes / static_cast<size_t>(numTracks);
        return juce::jlimit(perTrackMinBytes, perTrackMaxBytes, perTrack);
    }

    /**
     * Calculate recommended quality preset based on track count and budget
     */
    [[nodiscard]] QualityPreset calculateRecommendedPreset(int numTracks, float bufferDurationSec) const
    {
        size_t perTrack = calculatePerTrackBudget(numTracks);

        // Calculate required bytes for each quality level at given duration
        // Formula: captureRate * duration * 2 channels * 4 bytes per sample
        auto bytesRequired = [bufferDurationSec](int captureRate) -> size_t {
            return static_cast<size_t>(captureRate * bufferDurationSec * 2 * sizeof(float));
        };

        // Find highest quality that fits in budget
        if (perTrack >= bytesRequired(CaptureRate::HIGH))
            return QualityPreset::High;
        if (perTrack >= bytesRequired(CaptureRate::STANDARD))
            return QualityPreset::Standard;
        return QualityPreset::Eco;
    }

    bool operator==(const MemoryBudget& other) const
    {
        return totalBudgetBytes == other.totalBudgetBytes &&
               perTrackMinBytes == other.perTrackMinBytes &&
               perTrackMaxBytes == other.perTrackMaxBytes;
    }

    bool operator!=(const MemoryBudget& other) const
    {
        return !(*this == other);
    }
};

//==============================================================================
// Capture Quality Configuration
//==============================================================================

/**
 * Complete capture quality configuration
 * Combines quality preset, buffer duration, and memory budget
 */
struct CaptureQualityConfig
{
    QualityPreset qualityPreset = QualityPreset::Standard;
    BufferDuration bufferDuration = BufferDuration::Medium;
    MemoryBudget memoryBudget;
    bool autoAdjustQuality = true;  // Automatically reduce quality when tracks increase

    //==========================================================================
    // Calculation Helpers
    //==========================================================================

    /**
     * Get the capture sample rate for current preset
     * @param sourceRate The source audio sample rate (used for Ultra mode)
     * @return Capture rate in Hz
     */
    [[nodiscard]] int getCaptureRate(int sourceRate) const
    {
        return calculateCaptureRateForPreset(qualityPreset, sourceRate);
    }

    /**
     * Calculate capture rate for a given preset
     * @param preset The quality preset
     * @param sourceRate The source audio sample rate
     * @return Capture rate in Hz
     */
    [[nodiscard]] static int calculateCaptureRateForPreset(QualityPreset preset, int sourceRate)
    {
        switch (preset)
        {
            case QualityPreset::Eco:      return CaptureRate::ECO;
            case QualityPreset::Standard: return CaptureRate::STANDARD;
            case QualityPreset::High:     return CaptureRate::HIGH;
            case QualityPreset::Ultra:    return sourceRate > 0 ? sourceRate : CaptureRate::HIGH;
        }
        return CaptureRate::STANDARD;
    }

    /**
     * Calculate decimation ratio from source to capture rate
     * @param sourceRate Source sample rate in Hz
     * @return Decimation ratio (1 = no decimation, 2 = half rate, etc.)
     */
    [[nodiscard]] int getDecimationRatio(int sourceRate) const
    {
        int captureRate = getCaptureRate(sourceRate);
        if (captureRate <= 0 || sourceRate <= 0)
            return 1;

        int ratio = sourceRate / captureRate;
        return juce::jmax(1, ratio);
    }

    /**
     * Calculate required buffer size in samples
     * @param captureRate The capture sample rate
     * @return Buffer size in samples (per channel)
     */
    [[nodiscard]] size_t calculateBufferSizeSamples(int captureRate) const
    {
        float durationSec = bufferDurationToSeconds(bufferDuration);
        return static_cast<size_t>(captureRate * durationSec);
    }

    /**
     * Calculate required buffer size in samples for a specific duration
     * @param captureRate The capture sample rate
     * @param durationMs Duration in milliseconds
     * @return Buffer size in samples (per channel)
     */
    [[nodiscard]] static size_t calculateBufferSizeForDuration(int captureRate, float durationMs)
    {
        return static_cast<size_t>((captureRate * durationMs) / 1000.0f);
    }

    /**
     * Calculate memory usage for a single buffer
     * @param sourceRate Source sample rate (for Ultra mode calculation)
     * @return Memory usage in bytes (both channels)
     */
    [[nodiscard]] size_t calculateMemoryUsageBytes(int sourceRate) const
    {
        int captureRate = getCaptureRate(sourceRate);
        size_t samples = calculateBufferSizeSamples(captureRate);
        return samples * 2 * sizeof(float);  // 2 channels, 4 bytes per sample
    }

    /**
     * Get effective quality considering auto-adjustment
     * @param numTracks Current number of tracks
     * @param sourceRate Source sample rate
     * @return Effective quality preset (may be lower than configured if auto-adjust enabled)
     */
    [[nodiscard]] QualityPreset getEffectiveQuality(int numTracks, int /*sourceRate*/) const
    {
        if (!autoAdjustQuality)
            return qualityPreset;

        float durationSec = bufferDurationToSeconds(bufferDuration);
        QualityPreset recommended = memoryBudget.calculateRecommendedPreset(numTracks, durationSec);

        // Return the lower of configured and recommended
        return static_cast<QualityPreset>(
            juce::jmin(static_cast<int>(qualityPreset), static_cast<int>(recommended)));
    }

    //==========================================================================
    // Serialization
    //==========================================================================

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("CaptureQuality");
        tree.setProperty("qualityPreset", qualityPresetToString(qualityPreset), nullptr);
        tree.setProperty("bufferDuration", bufferDurationToString(bufferDuration), nullptr);
        tree.setProperty("totalBudgetMB", static_cast<int>(memoryBudget.totalBudgetBytes / (1024 * 1024)), nullptr);
        tree.setProperty("autoAdjustQuality", autoAdjustQuality, nullptr);
        return tree;
    }

    static CaptureQualityConfig fromValueTree(const juce::ValueTree& tree)
    {
        CaptureQualityConfig config;

        if (tree.isValid())
        {
            config.qualityPreset = stringToQualityPreset(
                tree.getProperty("qualityPreset", "Standard").toString());
            config.bufferDuration = stringToBufferDuration(
                tree.getProperty("bufferDuration", "4s").toString());

            int totalMB = tree.getProperty("totalBudgetMB", 50);
            config.memoryBudget.totalBudgetBytes = static_cast<size_t>(totalMB) * 1024 * 1024;

            config.autoAdjustQuality = tree.getProperty("autoAdjustQuality", true);
        }

        return config;
    }

    bool operator==(const CaptureQualityConfig& other) const
    {
        return qualityPreset == other.qualityPreset &&
               bufferDuration == other.bufferDuration &&
               memoryBudget == other.memoryBudget &&
               autoAdjustQuality == other.autoAdjustQuality;
    }

    bool operator!=(const CaptureQualityConfig& other) const
    {
        return !(*this == other);
    }
};

//==============================================================================
// Per-Oscillator Quality Override
//==============================================================================

/**
 * Quality override for individual oscillators
 * Allows specific tracks to use different quality than global setting
 */
enum class QualityOverride
{
    UseGlobal,  // Use the global CaptureQualityConfig setting
    Eco,
    Standard,
    High,
    Ultra
};

inline juce::String qualityOverrideToString(QualityOverride override)
{
    switch (override)
    {
        case QualityOverride::UseGlobal: return "UseGlobal";
        case QualityOverride::Eco:       return "Eco";
        case QualityOverride::Standard:  return "Standard";
        case QualityOverride::High:      return "High";
        case QualityOverride::Ultra:     return "Ultra";
    }
    return "UseGlobal";
}

inline QualityOverride stringToQualityOverride(const juce::String& str)
{
    if (str == "Eco")      return QualityOverride::Eco;
    if (str == "Standard") return QualityOverride::Standard;
    if (str == "High")     return QualityOverride::High;
    if (str == "Ultra")    return QualityOverride::Ultra;
    return QualityOverride::UseGlobal;
}

inline juce::String qualityOverrideToDisplayName(QualityOverride override)
{
    switch (override)
    {
        case QualityOverride::UseGlobal: return "Use Global";
        case QualityOverride::Eco:       return "Eco (11 kHz)";
        case QualityOverride::Standard:  return "Standard (22 kHz)";
        case QualityOverride::High:      return "High (44 kHz)";
        case QualityOverride::Ultra:     return "Ultra (Source Rate)";
    }
    return "Use Global";
}

/**
 * Resolve override to actual quality preset
 * @param override The per-oscillator override setting
 * @param globalPreset The global quality preset (used when override is UseGlobal)
 * @return The resolved quality preset
 */
inline QualityPreset resolveQualityOverride(QualityOverride override, QualityPreset globalPreset)
{
    switch (override)
    {
        case QualityOverride::UseGlobal: return globalPreset;
        case QualityOverride::Eco:       return QualityPreset::Eco;
        case QualityOverride::Standard:  return QualityPreset::Standard;
        case QualityOverride::High:      return QualityPreset::High;
        case QualityOverride::Ultra:     return QualityPreset::Ultra;
    }
    return globalPreset;
}

} // namespace oscil
