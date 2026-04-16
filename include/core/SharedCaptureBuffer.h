/*
    Oscil - Shared Capture Buffer
    Lock-free ring buffer for real-time audio capture and visualization
*/

#pragma once

#include "core/SeqLock.h"
#include "core/interfaces/IAudioBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <atomic>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace oscil
{

/**
 * Metadata for a captured audio frame
 */
struct CaptureFrameMetadata
{
    double sampleRate = 44100.0;
    int numChannels = 2;
    int64_t timestamp = 0; // Sample position in DAW timeline
    int numSamples = 0;
    bool isPlaying = false;
    double bpm = 120.0;
};

/**
 * Lock-free ring buffer for real-time audio capture.
 * Designed for single-producer (audio thread) / multiple-consumer (UI threads) usage.
 *
 * Memory budget: ~8MB per instance (2 channels × 4 bytes × 1M samples).
 *
 * Implements IAudioBuffer interface for read-only access abstraction.
 */
class SharedCaptureBuffer : public IAudioBuffer
{
public:
    // Buffer capacity must accommodate MAX_TIME_INTERVAL_MS (4000ms) at max sample rate (192kHz)
    // Calculation: 4000ms × 192kHz = 768,000 samples minimum
    // Using next power of 2: 1,048,576 (2^20) = ~5.5s @ 192kHz, ~24s @ 44.1kHz
    // Memory: 2 channels × 4 bytes × 1M samples = ~8MB per buffer
    static constexpr size_t DEFAULT_BUFFER_SAMPLES = 1048576;
    static constexpr size_t MAX_CHANNELS = 2;

    /**
     * Create a capture buffer with specified capacity
     * @param bufferSamples Number of samples per channel to store
     */
    explicit SharedCaptureBuffer(size_t bufferSamples = DEFAULT_BUFFER_SAMPLES);

    /**
     * Write audio samples to the buffer.
     *
     * @param buffer Audio buffer containing samples
     * @param metadata Frame metadata (sample rate, timestamp, etc.)
     * @param tryLock If true, attempts to lock and returns immediately if failed (for real-time thread).
     *                If false, blocks until lock is acquired (for non-real-time injection).
     */
    void write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata, bool tryLock = false);

    /**
     * Write raw sample data to the buffer.
     *
     * @param samples Pointer to interleaved sample data
     * @param numSamples Number of samples per channel
     * @param numChannels Number of channels
     * @param metadata Frame metadata
     * @param tryLock If true, attempts to lock and returns immediately if failed.
     */
    void write(const float* const* samples, int numSamples, int numChannels, const CaptureFrameMetadata& metadata,
               bool tryLock = false);

    /**
     * Blocking read (UI/message thread only — NOT real-time safe).
     * Spins with yield until a consistent snapshot is observed.
     *
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read
     * @param channel Channel index (0 = left, 1 = right)
     * @return Actual number of samples read
     */
    [[nodiscard]] int readBlocking(float* output, int numSamples, int channel = 0) const override;

    /**
     * Blocking read into a span (UI/message thread only).
     *
     * @param output Span to write samples into
     * @param channel Channel index (0 = left, 1 = right)
     * @return Actual number of samples read
     */
    [[nodiscard]] int readBlocking(std::span<float> output, int channel = 0) const;

    /**
     * Blocking multi-channel read (UI/message thread only).
     * @param output Audio buffer to write into
     * @param numSamples Number of samples to read
     * @return Actual number of samples read
     */
    [[nodiscard]] int readBlocking(juce::AudioBuffer<float>& output, int numSamples) const override;

    /**
     * Real-time-safe single-pass snapshot read. Returns std::nullopt if
     * a concurrent writer produced a torn read. Never yields.
     */
    [[nodiscard]] std::optional<int> readSnapshot(float* output, int numSamples, int channel = 0) const override;

    /**
     * Real-time-safe snapshot read into a span. Returns std::nullopt on torn read.
     */
    [[nodiscard]] std::optional<int> readSnapshot(std::span<float> output, int channel = 0) const;

    /**
     * Real-time-safe multi-channel snapshot read. Returns std::nullopt on torn read.
     */
    [[nodiscard]] std::optional<int> readSnapshot(juce::AudioBuffer<float>& output, int numSamples) const override;

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
    void writeInternal(const float* const* samples, int numSamples, int numChannels,
                       const CaptureFrameMetadata& metadata);
    void publishChannelWrites(const float* const* samples, int actualChannels, size_t srcOffset, size_t maskedWritePos,
                              size_t firstCount, size_t secondCount);
    void publishMetadata(const CaptureFrameMetadata& metadata, int numSamples, int actualChannels);
    size_t capacity_;
    std::vector<float> buffer_; // Flat buffer: [Channel 0][Channel 1]...
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> samplesWritten_{0};

    // SpinLock for thread-safe writing from multiple sources (e.g. audio thread + test injector)
    juce::SpinLock writeLock_;

    // Epoch counter for torn-read detection in read(). The writer increments
    // to odd before memcpy, then to even after. Readers bracket their copy
    // with two epoch loads and retry if they differ (concurrent write detected).
    std::atomic<uint32_t> writeEpoch_{0};

    // Lock-free metadata using SeqLock pattern (no SpinLock needed)
    mutable SeqLock<CaptureFrameMetadata> metadata_;

    // Helper to wrap position within buffer using fast bitwise AND
    // Requires capacity to be power of 2 (enforced in constructor)
    size_t wrapPosition(size_t pos) const
    {
        jassert(capacity_ > 0);
        jassert((capacity_ & (capacity_ - 1)) == 0); // Assert power of 2
        // Runtime safety check in case capacity_ is 0 (should never happen)
        if (capacity_ == 0)
            return 0;
        return pos & (capacity_ - 1);
    }
};

} // namespace oscil
