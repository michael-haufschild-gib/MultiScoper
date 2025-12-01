/*
    Oscil - Shared Capture Buffer
    Lock-free ring buffer for real-time audio capture and visualization
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <cstring>
#include "IAudioBuffer.h"

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
 *
 * Write pattern: increment sequence (odd), write fields, increment sequence (even)
 * Read pattern: read sequence, read fields, read sequence again, retry if changed/odd
 */
struct AtomicMetadata
{
    std::atomic<uint32_t> sequence{0};  // SeqLock sequence counter
    std::atomic<double> sampleRate{44100.0};
    std::atomic<int> numChannels{2};
    std::atomic<int64_t> timestamp{0};
    std::atomic<int> numSamples{0};
    std::atomic<bool> isPlaying{false};
    std::atomic<double> bpm{120.0};

    /**
     * Write metadata from audio thread (lock-free)
     */
    void write(const CaptureFrameMetadata& meta)
    {
        // Increment sequence to odd (signals write in progress)
        sequence.fetch_add(1, std::memory_order_release);

        // Write all fields
        sampleRate.store(meta.sampleRate, std::memory_order_relaxed);
        numChannels.store(meta.numChannels, std::memory_order_relaxed);
        timestamp.store(meta.timestamp, std::memory_order_relaxed);
        numSamples.store(meta.numSamples, std::memory_order_relaxed);
        isPlaying.store(meta.isPlaying, std::memory_order_relaxed);
        bpm.store(meta.bpm, std::memory_order_relaxed);

        // Increment sequence to even (signals write complete)
        sequence.fetch_add(1, std::memory_order_release);
    }

    /**
     * Read metadata from any thread (lock-free, may retry)
     *
     * Uses a bounded retry strategy to prevent infinite loops in case the
     * writer thread crashes mid-update (leaving sequence odd). After MAX_RETRIES,
     * returns current atomic values as best-effort fallback.
     *
     * No yield() is used because the write operation is extremely fast (just a few
     * atomic stores). Yielding can make contention worse by giving up the CPU.
     */
    CaptureFrameMetadata read() const
    {
        CaptureFrameMetadata result;
        uint32_t seq1, seq2;

        // Low retry limit since writes are very fast (nanoseconds)
        // If writer crashes mid-update, we don't want to spin long on UI thread
        // Increased to 100 to handle thread preemption in high-contention CI environments
        static constexpr int MAX_RETRIES = 100;
        int retries = 0;

        for (;;) {
            seq1 = sequence.load(std::memory_order_acquire);

            // If sequence is odd, writer is in progress - retry with limit
            if (seq1 & 1) {
                if (++retries > MAX_RETRIES) {
                    // Writer may have crashed mid-update; return best-effort values
                    // Using relaxed loads since consistency cannot be guaranteed anyway
                    result.sampleRate = sampleRate.load(std::memory_order_relaxed);
                    result.numChannels = numChannels.load(std::memory_order_relaxed);
                    result.timestamp = timestamp.load(std::memory_order_relaxed);
                    result.numSamples = numSamples.load(std::memory_order_relaxed);
                    result.isPlaying = isPlaying.load(std::memory_order_relaxed);
                    result.bpm = bpm.load(std::memory_order_relaxed);
                    return result;
                }
                // Spin without yielding - write operation is nanoseconds
                continue;
            }

            // Read all fields
            result.sampleRate = sampleRate.load(std::memory_order_relaxed);
            result.numChannels = numChannels.load(std::memory_order_relaxed);
            result.timestamp = timestamp.load(std::memory_order_relaxed);
            result.numSamples = numSamples.load(std::memory_order_relaxed);
            result.isPlaying = isPlaying.load(std::memory_order_relaxed);
            result.bpm = bpm.load(std::memory_order_relaxed);

            seq2 = sequence.load(std::memory_order_acquire);

            // If sequence hasn't changed, read was consistent
            if (seq1 == seq2) {
                break;
            }

            // Writer modified during our read - retry with limit
            if (++retries > MAX_RETRIES) {
                // Excessive contention or issue - return what we have
                return result;
            }
        }

        return result;
    }
};

/**
 * Lock-free ring buffer for real-time audio capture.
 * Designed for single-producer (audio thread) / multiple-consumer (UI threads) usage.
 *
 * Memory budget: ≤1MB per instance as per PRD requirements.
 *
 * Implements IAudioBuffer interface for read-only access abstraction.
 */
class SharedCaptureBuffer : public IAudioBuffer
{
public:
    // Default buffer capacity: ~1 second at 192kHz stereo = 192000 * 2 * 4 bytes = 1.5MB
    // We'll use a smaller window for visualization purposes
    static constexpr size_t DEFAULT_BUFFER_SAMPLES = 65536; // ~1.5s at 44.1kHz
    static constexpr size_t MAX_CHANNELS = 2;

    /**
     * Create a capture buffer with specified capacity
     * @param bufferSamples Number of samples per channel to store
     */
    explicit SharedCaptureBuffer(size_t bufferSamples = DEFAULT_BUFFER_SAMPLES);

    /**
     * Write audio samples from the audio thread.
     * This is lock-free and safe to call from the real-time audio thread.
     *
     * @param buffer Audio buffer containing samples
     * @param metadata Frame metadata (sample rate, timestamp, etc.)
     */
    void write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata);

    /**
     * Write raw sample data from the audio thread.
     * @param samples Pointer to interleaved sample data
     * @param numSamples Number of samples per channel
     * @param numChannels Number of channels
     * @param metadata Frame metadata
     */
    void write(const float* const* samples, int numSamples, int numChannels,
               const CaptureFrameMetadata& metadata);

    /**
     * Read the most recent samples into a buffer.
     * Safe to call from any thread (UI thread typically).
     *
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read
     * @param channel Channel index (0 = left, 1 = right)
     * @return Actual number of samples read
     */
    int read(float* output, int numSamples, int channel = 0) const override;

    /**
     * Read the most recent samples for all channels.
     * @param output Audio buffer to write into
     * @param numSamples Number of samples to read
     * @return Actual number of samples read
     */
    int read(juce::AudioBuffer<float>& output, int numSamples) const override;

    /**
     * Get the latest metadata
     */
    CaptureFrameMetadata getLatestMetadata() const override;

    /**
     * Get the current write position
     */
    size_t getWritePosition() const { return writePos_.load(std::memory_order_acquire); }

    /**
     * Get the buffer capacity in samples
     */
    size_t getCapacity() const override { return capacity_; }

    /**
     * Get the number of available samples
     */
    size_t getAvailableSamples() const override;

    /**
     * Clear the buffer
     */
    void clear();

    /**
     * Get the peak level for a channel (recent samples)
     * @param channel Channel index
     * @param numSamples Number of recent samples to analyze
     */
    float getPeakLevel(int channel, int numSamples = 1024) const override;

    /**
     * Get RMS level for a channel (recent samples)
     * @param channel Channel index
     * @param numSamples Number of recent samples to analyze
     */
    float getRMSLevel(int channel, int numSamples = 1024) const override;

private:
    size_t capacity_;
    std::vector<std::vector<float>> buffers_; // One vector per channel
    std::atomic<size_t> writePos_{ 0 };
    std::atomic<size_t> samplesWritten_{ 0 };

    // Lock-free metadata using SeqLock pattern (no SpinLock needed)
    mutable AtomicMetadata metadata_;

    // Helper to wrap position within buffer using fast bitwise AND
    // Requires capacity to be power of 2 (enforced in constructor)
    size_t wrapPosition(size_t pos) const
    {
        jassert(capacity_ > 0);
        jassert((capacity_ & (capacity_ - 1)) == 0);  // Assert power of 2
        return pos & (capacity_ - 1);
    }
};

} // namespace oscil
