/*
    Oscil - Capture Quality Configuration
    Manages waveform capture resolution and memory budget settings
    Enables adaptive quality based on track count and available memory
*/

#pragma once

#include "core/dsp/QualityPreset.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <cstddef>

namespace oscil
{

//==============================================================================
// Memory Budget Configuration
//==============================================================================

/**
 * Memory budget limits for waveform capture buffers
 */
struct MemoryBudget
{
    size_t totalBudgetBytes = 50 * 1024 * 1024; // 50 MB default total budget
    size_t perTrackMinBytes = 100 * 1024;       // 100 KB minimum per track
    size_t perTrackMaxBytes = 2 * 1024 * 1024;  // 2 MB maximum per track

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
        return totalBudgetBytes == other.totalBudgetBytes && perTrackMinBytes == other.perTrackMinBytes &&
               perTrackMaxBytes == other.perTrackMaxBytes;
    }

    bool operator!=(const MemoryBudget& other) const { return !(*this == other); }
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
    bool autoAdjustQuality = true; // Automatically reduce quality when tracks increase

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
            case QualityPreset::Eco:
                return CaptureRate::ECO;
            case QualityPreset::Standard:
                return CaptureRate::STANDARD;
            case QualityPreset::High:
                return CaptureRate::HIGH;
            case QualityPreset::Ultra:
                if (sourceRate <= 0)
                    return CaptureRate::HIGH;
                return juce::jmin(sourceRate, CaptureRate::MAX_SOURCE_RATE);
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
        return samples * 2 * sizeof(float); // 2 channels, 4 bytes per sample
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
        return static_cast<QualityPreset>(juce::jmin(static_cast<int>(qualityPreset), static_cast<int>(recommended)));
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
        static constexpr int DEFAULT_TOTAL_BUDGET_MB = 50;
        static constexpr int MAX_TOTAL_BUDGET_MB = 16 * 1024; // 16 GB safety ceiling

        if (tree.isValid())
        {
            config.qualityPreset = stringToQualityPreset(tree.getProperty("qualityPreset", "Standard").toString());
            config.bufferDuration = stringToBufferDuration(tree.getProperty("bufferDuration", "4s").toString());

            int totalMB = static_cast<int>(tree.getProperty("totalBudgetMB", DEFAULT_TOTAL_BUDGET_MB));
            if (totalMB <= 0 || totalMB > MAX_TOTAL_BUDGET_MB)
                totalMB = DEFAULT_TOTAL_BUDGET_MB;
            config.memoryBudget.totalBudgetBytes = static_cast<size_t>(totalMB) * 1024 * 1024;

            config.autoAdjustQuality = tree.getProperty("autoAdjustQuality", true);
        }

        return config;
    }

    bool operator==(const CaptureQualityConfig& other) const
    {
        return qualityPreset == other.qualityPreset && bufferDuration == other.bufferDuration &&
               memoryBudget == other.memoryBudget && autoAdjustQuality == other.autoAdjustQuality;
    }

    bool operator!=(const CaptureQualityConfig& other) const { return !(*this == other); }
};

} // namespace oscil

// Include QualityOverride for backward compatibility — consumers that included
// CaptureQualityConfig.h previously got QualityOverride types from the same file.
#include "core/dsp/QualityOverride.h"
