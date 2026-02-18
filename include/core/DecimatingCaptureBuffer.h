/*
    Oscil - Decimating Capture Buffer
    Lock-free capture buffer with SIMD-optimized decimation filtering
    
    Key features:
    - Zero locks on audio thread (truly lock-free SPSC design)
    - SIMD-accelerated anti-aliasing filter (NEON/SSE/AVX)
    - Epoch-based safe reconfiguration
    - Cache-line aligned state for optimal performance
*/

#pragma once

#include "core/SharedCaptureBuffer.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/interfaces/IAudioBuffer.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <memory>
#include <new>  // For std::hardware_destructive_interference_size

namespace oscil
{

// Cache line size for alignment (fallback if not available)
#ifdef __cpp_lib_hardware_interference_size
    inline constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
#else
    inline constexpr size_t kCacheLineSize = 64;
#endif

/**
 * SIMD-optimized anti-aliasing filter for decimation
 * Uses JUCE's block-based FIR processing for vectorized operations
 *
 * Design principles:
 * - Block processing enables SIMD (4-8x faster than sample-by-sample)
 * - Linear phase response (critical for accurate waveform display)
 * - -60dB stopband attenuation (eliminates visible aliasing artifacts)
 * - Kaiser window provides best control over stopband/transition tradeoff
 */
class SIMDDecimationFilter
{
public:
    static constexpr float STOPBAND_ATTENUATION_DB = -60.0f;
    static constexpr float TRANSITION_WIDTH_RATIO = 0.1f;
    static constexpr size_t MAX_CHANNELS = SharedCaptureBuffer::MAX_CHANNELS;

    SIMDDecimationFilter();

    /**
     * Configure filter for given decimation parameters
     * NOT real-time safe - call from message thread only
     */
    void configure(int decimationRatio, double sourceRate, int maxBlockSize);

    /**
     * Process a block of samples through the filter and decimate
     * SIMD-optimized, real-time safe (no allocations)
     *
     * @param input Input sample pointers per channel
     * @param output Output sample pointers per channel (decimated)
     * @param numInputSamples Number of input samples per channel
     * @param numChannels Number of channels to process
     * @return Number of output samples written
     */
    int processBlock(const float* const* input, float* const* output,
                     int numInputSamples, int numChannels);

    /**
     * Reset filter state (call when audio stream restarts)
     */
    void reset();

    [[nodiscard]] int getDecimationRatio() const { return decimationRatio_; }
    [[nodiscard]] bool isActive() const { return decimationRatio_ > 1; }
    [[nodiscard]] size_t getFilterOrder() const;
    [[nodiscard]] size_t getMemoryUsageBytes() const;

private:
    int decimationRatio_ = 1;
    int decimationPhase_ = 0;  // Tracks phase across block boundaries
    
    // Per-channel FIR filters for SIMD block processing
    std::array<juce::dsp::FIR::Filter<float>, MAX_CHANNELS> filters_;
    juce::dsp::FIR::Coefficients<float>::Ptr coefficients_;
    
    // Scratch buffer for in-place filtering (channel-interleaved for SIMD)
    juce::AudioBuffer<float> scratchBuffer_;
};

/**
 * Processing context containing all state needed for audio thread operations
 * Allocated once during configure, never modified during audio processing
 */
struct ProcessingContext
{
    SIMDDecimationFilter filter;
    
    // Output scratch buffer for decimated samples
    juce::AudioBuffer<float> decimatedBuffer;
    
    // Memory tracking
    size_t totalMemoryBytes = 0;
    
    void updateMemoryUsage()
    {
        totalMemoryBytes = filter.getMemoryUsageBytes() +
            static_cast<size_t>(decimatedBuffer.getNumSamples() * 
                               decimatedBuffer.getNumChannels()) * sizeof(float);
    }
};

/**
 * Cache-line aligned capture state for lock-free access
 * Contains all resources needed by audio thread
 */
struct alignas(kCacheLineSize) CaptureState
{
    // Ring buffer for captured samples
    std::unique_ptr<SharedCaptureBuffer> buffer;
    
    // Processing context (filter, scratch buffers)
    std::unique_ptr<ProcessingContext> context;
    
    // Epoch counter for safe reclamation
    // Audio thread increments on entry, decrements on exit
    // Message thread waits for zero before deletion
    std::atomic<uint32_t> epoch{0};
    
    // Configuration snapshot
    int decimationRatio = 1;
    int captureRate = CaptureRate::STANDARD;
    
    CaptureState() = default;
    ~CaptureState() = default;
    
    // Non-copyable, non-movable
    CaptureState(const CaptureState&) = delete;
    CaptureState& operator=(const CaptureState&) = delete;
    CaptureState(CaptureState&&) = delete;
    CaptureState& operator=(CaptureState&&) = delete;
};

/**
 * Lock-free capture buffer with SIMD-optimized decimation
 *
 * Thread safety guarantees:
 * - write() is lock-free and wait-free on audio thread
 * - read() is lock-free (may see slightly stale data during reconfigure)
 * - configure() must be called from message thread only
 *
 * Performance characteristics:
 * - Zero locks on audio thread
 * - SIMD filter processing (4-8x faster than scalar)
 * - Batched memcpy for ring buffer writes
 * - Safe reconfiguration without blocking audio
 */
class DecimatingCaptureBuffer : public IAudioBuffer
{
public:
    static constexpr size_t MAX_EXPECTED_BLOCK_SIZE = 8192;

    DecimatingCaptureBuffer();
    DecimatingCaptureBuffer(const CaptureQualityConfig& config, int sourceRate);
    ~DecimatingCaptureBuffer() override;

    // Non-copyable, non-movable
    DecimatingCaptureBuffer(const DecimatingCaptureBuffer&) = delete;
    DecimatingCaptureBuffer& operator=(const DecimatingCaptureBuffer&) = delete;
    DecimatingCaptureBuffer(DecimatingCaptureBuffer&&) = delete;
    DecimatingCaptureBuffer& operator=(DecimatingCaptureBuffer&&) = delete;

    //==========================================================================
    // Configuration (Message Thread Only)
    //==========================================================================

    /**
     * Configure the buffer for a specific quality and source rate
     * NOT real-time safe - must be called from message thread
     */
    void configure(const CaptureQualityConfig& config, int sourceRate);

    void setQualityPreset(QualityPreset preset, int sourceRate);

    [[nodiscard]] const CaptureQualityConfig& getConfig() const { return config_; }
    [[nodiscard]] int getSourceRate() const { return sourceRate_; }
    [[nodiscard]] int getCaptureRate() const;
    [[nodiscard]] int getDecimationRatio() const;

    //==========================================================================
    // Write Interface (Audio Thread - Lock Free)
    //==========================================================================

    /**
     * Write audio samples with decimation - LOCK FREE
     * Safe to call from audio thread (never blocks)
     */
    void write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata);

    void write(const float* const* samples, int numSamples, int numChannels,
               const CaptureFrameMetadata& metadata);

    //==========================================================================
    // Read Interface (Any Thread - Lock Free)
    //==========================================================================

    [[nodiscard]] int read(float* output, int numSamples, int channel = 0) const override;
    [[nodiscard]] int read(std::span<float> output, int channel = 0) const;
    [[nodiscard]] int read(juce::AudioBuffer<float>& output, int numSamples) const override;

    [[nodiscard]] CaptureFrameMetadata getLatestMetadata() const override;
    [[nodiscard]] size_t getCapacity() const override;
    [[nodiscard]] size_t getAvailableSamples() const override;
    [[nodiscard]] float getPeakLevel(int channel, int numSamples = 1024) const override;
    [[nodiscard]] float getRMSLevel(int channel, int numSamples = 1024) const override;

    //==========================================================================
    // Memory Management
    //==========================================================================

    [[nodiscard]] size_t getMemoryUsageBytes() const;
    [[nodiscard]] juce::String getMemoryUsageString() const;
    void clear();

    /**
     * Get the underlying capture buffer (for compatibility)
     * Returns buffer from current active state
     *
     * THREAD SAFETY: Must only be called from the message thread.
     * This method uses non-atomic caching and may allocate on first call
     * or when the active state changes.
     */
    [[nodiscard]] std::shared_ptr<SharedCaptureBuffer> getInternalBuffer() const;

    /**
     * Clean up retired states - call from message thread periodically
     */
    void cleanUpGarbage();

private:
    void reconfigure();
    std::unique_ptr<CaptureState> createNewState();

    // Configuration (only modified on message thread)
    CaptureQualityConfig config_;
    int sourceRate_ = 44100;

    // Active state pointer - atomically swapped during reconfiguration
    // Audio thread loads this without locking
    std::atomic<CaptureState*> activeState_{nullptr};

    // Retired states waiting for cleanup
    // Only accessed from message thread
    struct RetiredState
    {
        std::unique_ptr<CaptureState> state;
        double retiredTimeMs;
    };
    std::vector<RetiredState> retiredStates_;
    
    // Shared pointer wrapper for getInternalBuffer() compatibility
    // Lazily updated when needed
    mutable std::shared_ptr<SharedCaptureBuffer> cachedBufferPtr_;
    mutable std::atomic<CaptureState*> cachedBufferState_{nullptr};
};

// Keep old DecimationFilter for backward compatibility if needed elsewhere
using DecimationFilter = SIMDDecimationFilter;

} // namespace oscil
