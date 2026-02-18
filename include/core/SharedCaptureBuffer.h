/*
    Oscil - Shared Capture Buffer
    Lock-free SPSC ring buffer for real-time audio capture and visualization
    
    Optimizations:
    - Lock-free write for audio thread (SPSC pattern)
    - Batched memcpy instead of sample-by-sample copy
    - Power-of-2 capacity for fast modulo via bitwise AND
    - SeqLock pattern for metadata (already lock-free)
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <thread>
#include <vector>
#include <cstring>
#include <span>
#include "core/interfaces/IAudioBuffer.h"

namespace oscil
{

/**
 * Metadata for a captured audio frame
 */
struct CaptureFrameMetadata
{
    double sampleRate = 44100.0;
    int numChannels = 2;
    int64_t timestamp = 0;      // Sample position in DAW timeline
    int numSamples = 0;
    bool isPlaying = false;
    double bpm = 120.0;
};

/**
 * Lock-free atomic metadata using SeqLock pattern.
 * Single-producer (audio thread) / multiple-consumer (UI threads) safe.
 */
struct AtomicMetadata
{
    std::atomic<uint32_t> sequence{0};
    std::atomic<double> sampleRate{44100.0};
    std::atomic<int> numChannels{2};
    std::atomic<int64_t> timestamp{0};
    std::atomic<int> numSamples{0};
    std::atomic<bool> isPlaying{false};
    std::atomic<double> bpm{120.0};

    void write(const CaptureFrameMetadata& meta)
    {
        // SeqLock write pattern: increment sequence (odd = write in progress),
        // then fence, then writes, then fence, then increment (even = write complete).
        // CRITICAL: Use fences to prevent reordering on ARM/Apple Silicon.
        sequence.fetch_add(1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);  // Fence BEFORE writes

        sampleRate.store(meta.sampleRate, std::memory_order_relaxed);
        numChannels.store(meta.numChannels, std::memory_order_relaxed);
        timestamp.store(meta.timestamp, std::memory_order_relaxed);
        numSamples.store(meta.numSamples, std::memory_order_relaxed);
        isPlaying.store(meta.isPlaying, std::memory_order_relaxed);
        bpm.store(meta.bpm, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_release);  // Fence AFTER writes
        sequence.fetch_add(1, std::memory_order_relaxed);
    }

    CaptureFrameMetadata read() const
    {
        CaptureFrameMetadata result;
        uint32_t seq1, seq2;

        static constexpr int YIELD_THRESHOLD = 10;
        static constexpr int MAX_RETRIES = 100;
        int retries = 0;

        for (;;) {
            seq1 = sequence.load(std::memory_order_acquire);

            if (seq1 & 1) {
                if (++retries > MAX_RETRIES) {
                    result.sampleRate = sampleRate.load(std::memory_order_relaxed);
                    result.numChannels = numChannels.load(std::memory_order_relaxed);
                    result.timestamp = timestamp.load(std::memory_order_relaxed);
                    result.numSamples = numSamples.load(std::memory_order_relaxed);
                    result.isPlaying = isPlaying.load(std::memory_order_relaxed);
                    result.bpm = bpm.load(std::memory_order_relaxed);
                    return result;
                }
                if (retries > YIELD_THRESHOLD)
                    std::this_thread::yield();
                continue;
            }

            result.sampleRate = sampleRate.load(std::memory_order_relaxed);
            result.numChannels = numChannels.load(std::memory_order_relaxed);
            result.timestamp = timestamp.load(std::memory_order_relaxed);
            result.numSamples = numSamples.load(std::memory_order_relaxed);
            result.isPlaying = isPlaying.load(std::memory_order_relaxed);
            result.bpm = bpm.load(std::memory_order_relaxed);

            seq2 = sequence.load(std::memory_order_acquire);

            if (seq1 == seq2) {
                break;
            }

            if (++retries > MAX_RETRIES) {
                return result;
            }
            if (retries > YIELD_THRESHOLD)
                std::this_thread::yield();
        }

        return result;
    }
};

/**
 * Lock-free SPSC ring buffer for real-time audio capture.
 * 
 * Thread safety model:
 * - SPSC (Single Producer Single Consumer) for optimal performance
 * - Audio thread: Use writeLockFree() - zero locks, zero blocking
 * - UI thread: Use read() - lock-free atomic reads
 * - Test injection: Use write() with tryLock=false for blocking write
 */
class SharedCaptureBuffer : public IAudioBuffer
{
public:
    static constexpr size_t DEFAULT_BUFFER_SAMPLES = 1048576;
    static constexpr size_t MAX_CHANNELS = 2;

    explicit SharedCaptureBuffer(size_t bufferSamples = DEFAULT_BUFFER_SAMPLES);

    //==========================================================================
    // Lock-Free Write (Audio Thread - SPSC)
    //==========================================================================

    /**
     * Write audio samples - LOCK FREE, WAIT FREE
     * Use this from audio thread for maximum performance.
     * Uses batched memcpy instead of sample-by-sample copy.
     *
     * IMPORTANT: Only safe when there is a single producer (audio thread).
     * For test injection from another thread, use write() with tryLock=false.
     */
    void writeLockFree(const float* const* samples, int numSamples, int numChannels,
                       const CaptureFrameMetadata& metadata);

    void writeLockFree(const juce::AudioBuffer<float>& buffer, 
                       const CaptureFrameMetadata& metadata);

    //==========================================================================
    // Legacy Write Interface (with locking for multi-producer scenarios)
    //==========================================================================

    /**
     * Write with optional locking for thread safety.
     * Use tryLock=true for audio thread (non-blocking, may drop frames).
     * Use tryLock=false for test injection (blocking).
     */
    void write(const juce::AudioBuffer<float>& buffer, 
               const CaptureFrameMetadata& metadata, bool tryLock = false);

    void write(const float* const* samples, int numSamples, int numChannels,
               const CaptureFrameMetadata& metadata, bool tryLock = false);

    //==========================================================================
    // Read Interface (Any Thread - Lock Free)
    //==========================================================================

    int read(float* output, int numSamples, int channel = 0) const override;
    int read(std::span<float> output, int channel = 0) const;
    int read(juce::AudioBuffer<float>& output, int numSamples) const override;

    CaptureFrameMetadata getLatestMetadata() const override;
    size_t getWritePosition() const { return writePos_.load(std::memory_order_acquire); }
    size_t getCapacity() const override { return capacity_; }
    size_t getAvailableSamples() const override;
    void clear();

    float getPeakLevel(int channel, int numSamples = 1024) const override;
    float getRMSLevel(int channel, int numSamples = 1024) const override;

private:
    size_t capacity_;
    std::vector<float> buffer_;  // Flat buffer: [Channel 0][Channel 1]...
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> samplesWritten_{0};

    // Overtake detection: tracks how many times writer has wrapped around the buffer.
    // Readers can detect if data has been overwritten by comparing wrap counts.
    // This provides visibility into write pressure without adding synchronization overhead.
    std::atomic<uint64_t> writeWrapCount_{0};

    // SpinLock only for multi-producer scenarios (test injection)
    // Not used in SPSC audio path
    juce::SpinLock writeLock_;

    mutable AtomicMetadata metadata_;

    size_t wrapPosition(size_t pos) const
    {
        return pos & (capacity_ - 1);
    }

    // Internal implementation for batched memcpy write
    void writeInternal(const float* const* samples, int numSamples, int numChannels,
                       const CaptureFrameMetadata& metadata);
};

} // namespace oscil
