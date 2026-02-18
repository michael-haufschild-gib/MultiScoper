/*
    Oscil - Signal Processor
    Per-oscillator signal processing for visualization modes
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include "core/Oscillator.h"
#include <vector>
#include <span>
#include <atomic>

namespace oscil
{

/**
 * Result of signal processing containing processed samples.
 *
 * WARNING: Uses std::vector which performs dynamic memory allocation.
 * This struct is designed for UI thread use only (e.g., WaveformComponent).
 * NEVER use this struct in the audio thread (processBlock) as memory
 * allocation is not real-time safe and can cause audio glitches.
 */
struct ProcessedSignal
{
    std::vector<float> channel1;    // Primary channel (or Left for stereo)
    std::vector<float> channel2;    // Secondary channel (or Right for stereo, empty for mono modes)
    int numSamples = 0;
    bool isStereo = false;          // True if channel2 contains valid data

    void resize(int samples, bool stereo)
    {
        // ProcessedSignal is for UI visualization only.
        // Allocating memory on the audio thread is strictly forbidden.
        // This check works in both debug AND release builds for safety.
        if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            jassertfalse; // Trigger in debug for investigation
            return;       // Fail safe in release - don't resize from wrong thread
        }

        // Optimization: reserve capacity to avoid reallocations if possible
        if (channel1.capacity() < static_cast<size_t>(samples))
             channel1.reserve(static_cast<size_t>(samples));

        numSamples = samples;
        isStereo = stereo;
        channel1.resize(static_cast<size_t>(samples));
        
        if (stereo)
        {
            if (channel2.capacity() < static_cast<size_t>(samples))
                 channel2.reserve(static_cast<size_t>(samples));
            channel2.resize(static_cast<size_t>(samples));
        }
        else
        {
            channel2.clear();
        }
    }

    void clear()
    {
        std::fill(channel1.begin(), channel1.end(), 0.0f);
        if (isStereo)
            std::fill(channel2.begin(), channel2.end(), 0.0f);
    }
};

/**
 * Stateless signal processor for per-oscillator audio transforms.
 * Applies processing modes (FullStereo, Mono, Mid, Side, Left, Right)
 * to raw audio data for visualization purposes.
 *
 * Thread safety: All methods are thread-safe (stateless design).
 */
class SignalProcessor
{
public:
    SignalProcessor() = default;

    /**
     * Process audio samples according to the specified mode.
     *
     * @param leftChannel  Span of left channel samples
     * @param rightChannel Span of right channel samples (can be empty for mono sources)
     * @param mode         Processing mode to apply
     * @param output       Output structure to receive processed samples
     */
    void process(std::span<const float> leftChannel,
                 std::span<const float> rightChannel,
                 ProcessingMode mode,
                 ProcessedSignal& output) const;

    /**
     * Process audio from a JUCE AudioBuffer
     */
    void process(const juce::AudioBuffer<float>& input,
                 ProcessingMode mode,
                 ProcessedSignal& output) const;

    /**
     * Calculate stereo correlation coefficient for the given samples.
     * Returns a value from -1.0 (out of phase) to 1.0 (in phase).
     *
     * @param leftChannel  Span of left channel samples
     * @param rightChannel Span of right channel samples
     * @return Correlation coefficient (-1.0 to 1.0)
     */
    [[nodiscard]] static float calculateCorrelation(std::span<const float> leftChannel,
                                                    std::span<const float> rightChannel);

    /**
     * Calculate peak level of samples
     */
    [[nodiscard]] static float calculatePeak(std::span<const float> samples);

    /**
     * Calculate RMS level of samples
     */
    [[nodiscard]] static float calculateRMS(std::span<const float> samples);

    /**
     * Decimate samples for display (reduce sample count while preserving visual characteristics)
     *
     * @param input        Input samples
     * @param output       Output buffer (must be pre-allocated)
     * @param preservePeaks If true, preserves peak values during decimation
     */
    static void decimate(std::span<const float> input,
                         std::span<float> output,
                         bool preservePeaks = true);

private:
    // Processing implementations for each mode
    void processFullStereo(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processMono(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processMid(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processSide(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processLeft(std::span<const float> left, ProcessedSignal& output) const;
    void processRight(std::span<const float> right, ProcessedSignal& output) const;
};

//==============================================================================
// Level-of-Detail (LOD) System
//==============================================================================

/**
 * LOD quality tiers based on visible waveform width.
 * Each tier defines the target sample count for optimal visual fidelity
 * without excessive GPU/CPU overhead.
 *
 * Width Thresholds:
 *   > 800px  -> Full (2048 samples)
 *   400-800px -> High (1024 samples)
 *   200-400px -> Medium (512 samples)
 *   < 200px   -> Preview (256 samples)
 */
enum class LODTier
{
    Full,       // 2048 samples - maximum quality for large displays
    High,       // 1024 samples - high quality for medium displays
    Medium,     // 512 samples  - balanced quality for small displays
    Preview     // 256 samples  - preview quality for tiny displays
};

/**
 * Get the sample count for a given LOD tier.
 */
[[nodiscard]] inline int getLODSampleCount(LODTier tier)
{
    switch (tier)
    {
        case LODTier::Full:    return 2048;
        case LODTier::High:    return 1024;
        case LODTier::Medium:  return 512;
        case LODTier::Preview: return 256;
    }
    return 1024; // Default fallback
}

/**
 * Get a human-readable name for an LOD tier.
 */
[[nodiscard]] inline const char* getLODTierName(LODTier tier)
{
    switch (tier)
    {
        case LODTier::Full:    return "Full";
        case LODTier::High:    return "High";
        case LODTier::Medium:  return "Medium";
        case LODTier::Preview: return "Preview";
    }
    return "Unknown";
}

/**
 * Result of decimation with min/max envelope for accurate peak visualization.
 * This struct preserves peak transients even at low LOD levels.
 */
struct DecimatedWaveform
{
    std::vector<float> samples;      // Decimated sample values
    std::vector<float> minEnvelope;  // Minimum values per decimation segment
    std::vector<float> maxEnvelope;  // Maximum values per decimation segment
    LODTier tier = LODTier::Full;    // Current LOD tier
    
    void resize(size_t count)
    {
        samples.resize(count);
        minEnvelope.resize(count);
        maxEnvelope.resize(count);
    }
    
    void clear()
    {
        samples.clear();
        minEnvelope.clear();
        maxEnvelope.clear();
    }
    
    [[nodiscard]] bool empty() const { return samples.empty(); }
    [[nodiscard]] size_t size() const { return samples.size(); }
};

/**
 * Adaptive decimator for display optimization.
 * Reduces sample count based on display width while preserving visual fidelity.
 * Uses tiered LOD with hysteresis to prevent flickering at tier boundaries.
 */
class AdaptiveDecimator
{
public:
    // Width thresholds for LOD tier transitions
    // Hysteresis zone of 20px prevents flickering at boundaries
    static constexpr int THRESHOLD_FULL_HIGH = 800;
    static constexpr int THRESHOLD_HIGH_MEDIUM = 400;
    static constexpr int THRESHOLD_MEDIUM_PREVIEW = 200;
    static constexpr int HYSTERESIS_PIXELS = 20;

    AdaptiveDecimator() = default;

    /**
     * Configure the decimator for a given display width.
     * Applies hysteresis to prevent tier flickering at boundaries.
     */
    void setDisplayWidth(int widthPixels);

    /**
     * Get the current LOD tier.
     * Thread-safe: uses relaxed atomic load.
     */
    [[nodiscard]] LODTier getCurrentTier() const { return currentTier_.load(std::memory_order_relaxed); }

    /**
     * Get the recommended sample count for display.
     * Thread-safe: uses relaxed atomic load.
     */
    [[nodiscard]] int getTargetSampleCount() const { return targetSamples_.load(std::memory_order_relaxed); }

    /**
     * Get the current display width.
     * Thread-safe: uses relaxed atomic load.
     */
    [[nodiscard]] int getDisplayWidth() const { return displayWidth_.load(std::memory_order_relaxed); }

    /**
     * Decimate samples for display (simple mode).
     */
    void process(std::span<const float> input,
                 std::vector<float>& output) const;

    /**
     * Decimate with min/max envelope for peak preservation (legacy API).
     */
    void processWithEnvelope(std::span<const float> input,
                             std::vector<float>& minEnvelope,
                             std::vector<float>& maxEnvelope) const;

    /**
     * Decimate with full peak-preserving envelope (new API).
     * Populates a DecimatedWaveform with samples and min/max envelopes.
     */
    void processWithPeaks(std::span<const float> input,
                          DecimatedWaveform& output) const;

private:
    /**
     * Calculate LOD tier for a given width with hysteresis.
     * @param width Current display width in pixels
     * @param currentTier Current tier (for hysteresis calculation)
     * @return The appropriate LOD tier
     */
    [[nodiscard]] LODTier calculateTierWithHysteresis(int width, LODTier currentTier) const;

    // Thread-safe atomic members for cross-thread access
    // setDisplayWidth() may be called from UI thread while process() runs on another
    std::atomic<int> displayWidth_{800};
    std::atomic<int> targetSamples_{2048};
    std::atomic<LODTier> currentTier_{LODTier::Full};
};

} // namespace oscil
