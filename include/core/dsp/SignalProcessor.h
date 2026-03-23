/*
    Oscil - Signal Processor
    Per-oscillator signal processing for visualization modes
*/

#pragma once

#include "core/Oscillator.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

#include <span>
#include <vector>

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
    std::vector<float> channel1; // Primary channel (or Left for stereo)
    std::vector<float> channel2; // Secondary channel (or Right for stereo, empty for mono modes)
    int numSamples = 0;
    bool isStereo = false; // True if channel2 contains valid data

    void resize(int samples, bool stereo)
    {
        // ProcessedSignal is for UI visualization only.
        // Allocating memory on the audio thread is strictly forbidden.
        // This check works in both debug AND release builds for safety.
        // Use getInstanceWithoutCreating() to avoid creating a MessageManager
        // in non-JUCE contexts (e.g. unit tests without a JUCE event loop).
        auto* mm = juce::MessageManager::getInstanceWithoutCreating();
        if (mm != nullptr && !mm->isThisTheMessageThread())
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
    void process(std::span<const float> leftChannel, std::span<const float> rightChannel, ProcessingMode mode,
                 ProcessedSignal& output) const;

    /**
     * Process audio from a JUCE AudioBuffer
     */
    void process(const juce::AudioBuffer<float>& input, ProcessingMode mode, ProcessedSignal& output) const;

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
    static void decimate(std::span<const float> input, std::span<float> output, bool preservePeaks = true);

private:
    static void upsample(std::span<const float> input, std::span<float> output);
    // Processing implementations for each mode
    void processFullStereo(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processMono(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processMid(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processSide(std::span<const float> left, std::span<const float> right, ProcessedSignal& output) const;
    void processLeft(std::span<const float> left, ProcessedSignal& output) const;
    void processRight(std::span<const float> right, ProcessedSignal& output) const;
};

/**
 * Adaptive decimator for display optimization.
 * Reduces sample count based on display width while preserving visual fidelity.
 */
class AdaptiveDecimator
{
public:
    AdaptiveDecimator() = default;

    /**
     * Configure the decimator for a given display width
     */
    void setDisplayWidth(int widthPixels);

    /**
     * Get the recommended sample count for display
     */
    [[nodiscard]] int getTargetSampleCount() const { return targetSamples_; }

    /**
     * Decimate samples for display
     */
    void process(std::span<const float> input, std::vector<float>& output) const;

    /**
     * Decimate with min/max envelope for peak preservation
     */
    void processWithEnvelope(std::span<const float> input, std::vector<float>& minEnvelope,
                             std::vector<float>& maxEnvelope) const;

private:
    int displayWidth_ = 800;
    int targetSamples_ = 800;
};

} // namespace oscil
