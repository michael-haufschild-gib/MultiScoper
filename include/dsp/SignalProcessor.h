/*
    Oscil - Signal Processor
    Per-oscillator signal processing for visualization modes
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "core/Oscillator.h"
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
    std::vector<float> channel1;    // Primary channel (or Left for stereo)
    std::vector<float> channel2;    // Secondary channel (or Right for stereo, empty for mono modes)
    int numSamples = 0;
    bool isStereo = false;          // True if channel2 contains valid data

    void resize(int samples, bool stereo)
    {
        numSamples = samples;
        isStereo = stereo;
        channel1.resize(static_cast<size_t>(samples));
        if (stereo)
            channel2.resize(static_cast<size_t>(samples));
        else
            channel2.clear();
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
     * @param leftChannel  Pointer to left channel samples
     * @param rightChannel Pointer to right channel samples (can be nullptr for mono sources)
     * @param numSamples   Number of samples to process
     * @param mode         Processing mode to apply
     * @param output       Output structure to receive processed samples
     */
    void process(const float* leftChannel,
                 const float* rightChannel,
                 int numSamples,
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
     * @param leftChannel  Pointer to left channel samples
     * @param rightChannel Pointer to right channel samples
     * @param numSamples   Number of samples
     * @return Correlation coefficient (-1.0 to 1.0)
     */
    [[nodiscard]] static float calculateCorrelation(const float* leftChannel,
                                      const float* rightChannel,
                                      int numSamples);

    /**
     * Calculate peak level of samples
     */
    [[nodiscard]] static float calculatePeak(const float* samples, int numSamples);

    /**
     * Calculate RMS level of samples
     */
    [[nodiscard]] static float calculateRMS(const float* samples, int numSamples);

    /**
     * Decimate samples for display (reduce sample count while preserving visual characteristics)
     *
     * @param input        Input samples
     * @param inputLength  Number of input samples
     * @param output       Output buffer (must be pre-allocated)
     * @param outputLength Desired output length
     * @param preservePeaks If true, preserves peak values during decimation
     */
    static void decimate(const float* input,
                         int inputLength,
                         float* output,
                         int outputLength,
                         bool preservePeaks = true);

private:
    // Processing implementations for each mode
    void processFullStereo(const float* left, const float* right, int numSamples, ProcessedSignal& output) const;
    void processMono(const float* left, const float* right, int numSamples, ProcessedSignal& output) const;
    void processMid(const float* left, const float* right, int numSamples, ProcessedSignal& output) const;
    void processSide(const float* left, const float* right, int numSamples, ProcessedSignal& output) const;
    void processLeft(const float* left, int numSamples, ProcessedSignal& output) const;
    void processRight(const float* right, int numSamples, ProcessedSignal& output) const;
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
    void process(const float* input, int inputLength,
                 std::vector<float>& output) const;

    /**
     * Decimate with min/max envelope for peak preservation
     */
    void processWithEnvelope(const float* input, int inputLength,
                             std::vector<float>& minEnvelope,
                             std::vector<float>& maxEnvelope) const;

private:
    int displayWidth_ = 800;
    int targetSamples_ = 800;
};

} // namespace oscil
