/*
    Oscil - Decimating Capture Buffer
    Capture buffer with configurable sample rate decimation and anti-aliasing
    Reduces memory usage while maintaining visual fidelity for waveform display
*/

#pragma once

#include "core/SharedCaptureBuffer.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/interfaces/IAudioBuffer.h"

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace oscil
{

/**
 * Anti-aliasing lowpass filter for decimation
 * Uses JUCE's FIR::Filter with Kaiser window design for optimal anti-aliasing
 *
 * Design principles:
 * - Linear phase response (critical for accurate waveform display)
 * - -60dB stopband attenuation (eliminates visible aliasing artifacts)
 * - Narrow transition band for flat passband response
 * - Kaiser window provides best control over stopband/transition tradeoff
 */
class DecimationFilter
{
public:
    // -60dB stopband ensures aliased frequencies are inaudible and invisible
    static constexpr float STOPBAND_ATTENUATION_DB = -60.0f;

    // Transition width as fraction of Nyquist (0.1 = 10%)
    // Narrower = more taps but flatter passband
    static constexpr float TRANSITION_WIDTH_RATIO = 0.1f;

    /// Create a default decimation filter with no decimation (ratio = 1).
    DecimationFilter();

    /**
     * Configure filter for given decimation parameters
     * Uses JUCE's FilterDesign::designFIRLowpassKaiserMethod for optimal coefficients
     *
     * @param decimationRatio The decimation factor (2 = half rate, 4 = quarter rate, etc.)
     * @param sourceRate Source sample rate in Hz
     */
    void configure(int decimationRatio, double sourceRate);

    /**
     * Process a single sample through the filter
     * @param sample Input sample
     * @return Filtered sample
     */
    float processSample(float sample);

    /**
     * Reset filter state (call when audio stream restarts)
     */
    void reset();

    /**
     * Get current decimation ratio
     */
    [[nodiscard]] int getDecimationRatio() const { return decimationRatio_; }

    /**
     * Check if decimation is needed (ratio > 1)
     */
    [[nodiscard]] bool isActive() const { return decimationRatio_ > 1; }

    /**
     * Get filter order (number of taps - 1)
     */
    [[nodiscard]] size_t getFilterOrder() const;

    /**
     * Get memory usage in bytes
     */
    [[nodiscard]] size_t getMemoryUsageBytes() const;

private:
    int decimationRatio_ = 1;
    juce::dsp::FIR::Filter<float> filter_;
    juce::dsp::FIR::Coefficients<float>::Ptr coefficients_;
};

/**
 * Capture buffer with built-in decimation for memory efficiency
 *
 * Wraps SharedCaptureBuffer and adds:
 * - Configurable decimation (reduce sample rate for storage)
 * - Anti-aliasing filter (prevent aliasing artifacts)
 * - Dynamic buffer resizing based on quality settings
 * - Memory usage tracking for dashboard display
 *
 * Thread safety: Same guarantees as SharedCaptureBuffer
 * - write() safe from audio thread
 * - read() safe from any thread
 */
class DecimatingCaptureBuffer : public IAudioBuffer
{
public:
    /**
     * Create a decimating capture buffer with default settings
     */
    DecimatingCaptureBuffer();

    /**
     * Create a decimating capture buffer with specific quality config
     * @param config Quality configuration (preset, duration, budget)
     * @param sourceRate Source audio sample rate
     */
    DecimatingCaptureBuffer(const CaptureQualityConfig& config, int sourceRate);

    ~DecimatingCaptureBuffer() override = default;

    // Non-copyable, non-movable
    DecimatingCaptureBuffer(const DecimatingCaptureBuffer&) = delete;
    DecimatingCaptureBuffer& operator=(const DecimatingCaptureBuffer&) = delete;
    DecimatingCaptureBuffer(DecimatingCaptureBuffer&&) = delete;
    DecimatingCaptureBuffer& operator=(DecimatingCaptureBuffer&&) = delete;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * Configure the buffer for a specific quality and source rate
     * This may reallocate the internal buffer - do NOT call from audio thread
     * @param config Quality configuration
     * @param sourceRate Source audio sample rate in Hz
     */
    void configure(const CaptureQualityConfig& config, int sourceRate);

    /**
     * Update quality preset only (less disruptive than full configure)
     * @param preset New quality preset
     * @param sourceRate Source audio sample rate
     */
    void setQualityPreset(QualityPreset preset, int sourceRate);

    /**
     * Get current configuration
     */
    [[nodiscard]] const CaptureQualityConfig& getConfig() const { return config_; }

    /**
     * Get current source sample rate
     */
    [[nodiscard]] int getSourceRate() const { return sourceRate_.load(std::memory_order_relaxed); }

    /**
     * Get effective capture rate (after decimation)
     */
    [[nodiscard]] int getCaptureRate() const { return captureRate_.load(std::memory_order_relaxed); }

    /**
     * Get current decimation ratio
     */
    [[nodiscard]] int getDecimationRatio() const { return decimationRatio_.load(std::memory_order_relaxed); }

    //==========================================================================
    // Write Interface (Audio Thread)
    //==========================================================================

    /**
     * Write audio samples with decimation
     * Safe to call from audio thread (uses tryLock internally)
     *
     * @param buffer Audio buffer containing samples
     * @param metadata Frame metadata
     */
    void write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata);

    /**
     * Write raw sample data with decimation
     * Safe to call from audio thread (uses tryLock internally)
     *
     * @param samples Channel pointers to sample data
     * @param numSamples Number of samples per channel
     * @param numChannels Number of channels
     * @param metadata Frame metadata
     */
    void write(const float* const* samples, int numSamples, int numChannels, const CaptureFrameMetadata& metadata);

    //==========================================================================
    // Read Interface (Any Thread)
    //==========================================================================

    /**
     * Read the most recent samples
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read (at capture rate)
     * @param channel Channel index
     * @return Actual number of samples read
     */
    [[nodiscard]] int read(float* output, int numSamples, int channel = 0) const override;

    /**
     * Read into span
     */
    [[nodiscard]] int read(std::span<float> output, int channel = 0) const;

    /**
     * Read all channels
     */
    [[nodiscard]] int read(juce::AudioBuffer<float>& output, int numSamples) const override;

    /**
     * Get the latest metadata
     */
    [[nodiscard]] CaptureFrameMetadata getLatestMetadata() const override;

    /**
     * Get buffer capacity in samples (at capture rate)
     */
    [[nodiscard]] size_t getCapacity() const override;

    /**
     * Get available samples (at capture rate)
     */
    [[nodiscard]] size_t getAvailableSamples() const override;

    /**
     * Get peak level for metering
     */
    [[nodiscard]] float getPeakLevel(int channel, int numSamples = 1024) const override;

    /**
     * Get RMS level for metering
     */
    [[nodiscard]] float getRMSLevel(int channel, int numSamples = 1024) const override;

    //==========================================================================
    // Memory Management
    //==========================================================================

    /**
     * Get current memory usage in bytes
     * Includes internal buffer and filter state
     */
    [[nodiscard]] size_t getMemoryUsageBytes() const;

    /**
     * Get memory usage as a formatted string (e.g., "1.2 MB")
     */
    [[nodiscard]] juce::String getMemoryUsageString() const;

    /**
     * Clear all buffer contents
     */
    void clear();

    //==========================================================================
    // Direct Buffer Access (for compatibility)
    //==========================================================================

    /**
     * Get the underlying capture buffer
     * Use with caution - bypasses decimation for reads
     */
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getInternalBuffer() const;

    /**
     * Explicitly clean up old buffers
     * Should be called from Message Thread periodically or during shutdown
     */
    void cleanUpGarbage();

private:
    // Thread-safe processing context
    // Encapsulates all state required for the audio thread write operation
    struct ProcessingContext
    {
        std::array<DecimationFilter, SharedCaptureBuffer::MAX_CHANNELS> filters;
        std::array<int, SharedCaptureBuffer::MAX_CHANNELS> decimationCounters{};
        std::vector<float> scratchBuffer;
        size_t filterMemoryBytes = 0;
    };

    void reconfigure();
    void processAndWriteDecimated(const std::shared_ptr<SharedCaptureBuffer>& buf,
                                  const std::shared_ptr<ProcessingContext>& ctx, const float* const* samples,
                                  int numSamples, int numChannels, const CaptureFrameMetadata& metadata);
    int decimateChannel(const float* src, float* dest, DecimationFilter& filter, int& counter, int numSamples) const;
    std::shared_ptr<ProcessingContext> createProcessingContext();

    // Configuration — config_ is message-thread-only (protected by jassert).
    // sourceRate_, captureRate_, decimationRatio_ are written on message thread
    // and read on audio thread, so they use relaxed atomics for data-race freedom.
    CaptureQualityConfig config_;
    std::atomic<int> sourceRate_{44100};
    std::atomic<int> captureRate_{CaptureRate::STANDARD};
    std::atomic<int> decimationRatio_{1};

    // Internal storage protected by spin lock for reconfiguration safety
    // SpinLock is used because std::atomic<shared_ptr> is not yet portable
    std::shared_ptr<SharedCaptureBuffer> buffer_;
    mutable juce::SpinLock bufferSwapLock_;
    std::shared_ptr<ProcessingContext> context_;

    // Safety mechanism to prevent audio thread from deleting shared resources
    struct GraveyardItem
    {
        std::shared_ptr<SharedCaptureBuffer> buffer;
        std::shared_ptr<ProcessingContext> context;
        double timestampMs;
    };
    std::vector<GraveyardItem> graveyard_;
};

} // namespace oscil