/*
    Oscil - Raw Capture Buffer
    Ultra-minimal SPSC ring buffer for lock-free audio thread writes
    
    Design goals:
    - ~1μs write latency (just memcpy + atomic store)
    - Zero allocations on audio thread
    - Zero locks - pure SPSC pattern
    - Cache-line aligned for optimal performance
    - Power-of-2 capacity for fast modulo via bitwise AND
*/

#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstring>
#include <new> // For std::hardware_destructive_interference_size
#include <thread>

namespace oscil
{

// Cache line size for alignment (fallback if not available)
#ifdef __cpp_lib_hardware_interference_size
    inline constexpr size_t kRawBufferCacheLineSize = std::hardware_destructive_interference_size;
#else
    inline constexpr size_t kRawBufferCacheLineSize = 64;
#endif

/**
 * Ultra-minimal SPSC ring buffer for real-time audio capture.
 * 
 * Thread safety model:
 * - Single Producer (audio thread): write() - lock-free, wait-free
 * - Single Consumer (capture thread): read() - lock-free
 * 
 * Memory layout:
 * - Separate arrays for L/R channels (better cache locality during write)
 * - Cache-line aligned write/read positions to prevent false sharing
 * - Power-of-2 capacity for fast modulo
 * 
 * Performance characteristics:
 * - write(): ~1μs for typical block sizes (just memcpy + atomic store)
 * - read(): Lock-free, may return partial data if producer is slow
 * - No allocations, no locks, no system calls
 */
class RawCaptureBuffer
{
public:
    // Power-of-2 capacity for fast bitwise modulo
    // 65536 samples = ~1.5 seconds at 44.1kHz
    static constexpr size_t CAPACITY = 65536;
    static constexpr size_t CAPACITY_MASK = CAPACITY - 1;
    static constexpr size_t MAX_CHANNELS = 2;

    RawCaptureBuffer();
    ~RawCaptureBuffer() = default;

    // Non-copyable, non-movable (contains atomics and large buffers)
    RawCaptureBuffer(const RawCaptureBuffer&) = delete;
    RawCaptureBuffer& operator=(const RawCaptureBuffer&) = delete;
    RawCaptureBuffer(RawCaptureBuffer&&) = delete;
    RawCaptureBuffer& operator=(RawCaptureBuffer&&) = delete;

    //==========================================================================
    // Audio Thread Interface - LOCK FREE, WAIT FREE (~1μs)
    //==========================================================================

    /**
     * Write stereo audio samples to the ring buffer.
     * 
     * REAL-TIME SAFE: No locks, no allocations, just memcpy + atomic store.
     * 
     * @param left Pointer to left channel samples (may be nullptr for mono)
     * @param right Pointer to right channel samples (may be nullptr for mono)
     * @param numSamples Number of samples to write per channel
     * @param sampleRate Current sample rate (stored for consumer reference)
     */
    void write(const float* left, const float* right, int numSamples, double sampleRate = 44100.0);

    /**
     * Write mono audio sample to both channels.
     * 
     * REAL-TIME SAFE: No locks, no allocations.
     * 
     * @param mono Pointer to mono samples
     * @param numSamples Number of samples to write
     * @param sampleRate Current sample rate
     */
    void writeMono(const float* mono, int numSamples, double sampleRate = 44100.0);

    //==========================================================================
    // Capture Thread Interface - LOCK FREE
    //==========================================================================

    /**
     * Read available samples from the ring buffer.
     * 
     * Called by CaptureThread only. Returns number of samples actually read.
     * May return fewer samples than requested if producer hasn't written enough.
     * 
     * @param left Output buffer for left channel
     * @param right Output buffer for right channel
     * @param maxSamples Maximum samples to read per channel
     * @return Number of samples actually read (same for both channels)
     */
    [[nodiscard]] int read(float* left, float* right, int maxSamples);

    /**
     * Get number of samples available to read.
     * Lock-free, may be slightly stale.
     */
    [[nodiscard]] size_t getAvailableSamples() const;

    /**
     * Get the current sample rate (set by last write).
     * Lock-free atomic load.
     */
    [[nodiscard]] double getSampleRate() const;

    /**
     * Reset the buffer (clear all data).
     * NOT real-time safe - call from message thread only.
     */
    void reset();

    //==========================================================================
    // Multi-Producer Detection (Defense-in-Depth)
    //==========================================================================

    /**
     * Check if multi-producer access was detected.
     * 
     * Some DAWs (Pro Tools HDX, Reaper with certain configs) may use multiple
     * audio threads. If detected, the plugin should fall back to the legacy path.
     * 
     * Lock-free atomic load.
     */
    [[nodiscard]] bool wasMultiProducerDetected() const;

    /**
     * Get the first producer thread ID (for debugging).
     * Only valid after first write.
     */
    [[nodiscard]] std::thread::id getProducerThreadId() const;

    /**
     * Clear the multi-producer flag (e.g., after fallback to legacy path).
     * NOT real-time safe - call from message thread only.
     */
    void clearMultiProducerFlag();

private:
    // Separate channel buffers for better cache locality during write
    // Each channel is a contiguous block, avoiding interleaved access patterns
    alignas(kRawBufferCacheLineSize) std::array<float, CAPACITY> bufferL_;
    alignas(kRawBufferCacheLineSize) std::array<float, CAPACITY> bufferR_;

    // Write position - only modified by producer (audio thread)
    // Cache-line aligned to prevent false sharing with read position
    alignas(kRawBufferCacheLineSize) std::atomic<size_t> writePos_{0};

    // Read position - only modified by consumer (capture thread)
    // Cache-line aligned to prevent false sharing with write position
    alignas(kRawBufferCacheLineSize) std::atomic<size_t> readPos_{0};

    // Sample rate - written by producer, read by consumer
    alignas(kRawBufferCacheLineSize) std::atomic<double> sampleRate_{44100.0};

    // Total samples written (for statistics/debugging)
    std::atomic<size_t> totalSamplesWritten_{0};

    // Multi-producer detection (defense-in-depth)
    // Some DAWs may use multiple audio threads; we detect and warn
    std::atomic<std::thread::id> producerThreadId_{};  // First thread to write
    std::atomic<bool> producerIdSet_{false};            // Whether producerThreadId_ is valid
    std::atomic<bool> multiProducerDetected_{false};    // Set if different thread writes
};

} // namespace oscil

