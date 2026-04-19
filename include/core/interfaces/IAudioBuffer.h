/*
    MultiScoper - Audio Buffer Interface
    Abstract interface for read-only audio buffer access
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <optional>

namespace multiscoper
{

// Forward declaration
struct CaptureFrameMetadata;

/**
 * Abstract interface for read-only access to audio capture buffers.
 * Enables dependency injection and testability by decoupling
 * UI/visualization components from the concrete SharedCaptureBuffer.
 *
 * This interface only exposes read operations - write operations
 * are implementation-specific and not part of this abstraction.
 *
 * Two read variants are exposed to make thread-safety requirements
 * explicit at the call site:
 *   - readBlocking(...)  — spins/yields until a consistent snapshot is
 *                          observed. Must only be called from the
 *                          UI/message thread; NOT real-time safe.
 *   - readSnapshot(...)  — single-pass, real-time-safe. Returns
 *                          std::nullopt if a concurrent writer
 *                          produced a torn read. Safe from any thread,
 *                          including the audio thread.
 */
class IAudioBuffer
{
public:
    virtual ~IAudioBuffer() = default;

    /**
     * Read the most recent samples into a buffer, blocking (yielding)
     * until a consistent snapshot is observed.
     *
     * UI/message-thread only. NOT real-time safe — yields on writer
     * contention, which is a syscall.
     *
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read
     * @param channel Channel index (0 = left, 1 = right)
     * @return Actual number of samples read
     */
    [[nodiscard]] virtual int readBlocking(float* output, int numSamples, int channel = 0) const = 0;

    /**
     * Read the most recent samples for all channels, blocking (yielding)
     * until a consistent snapshot is observed across all channels.
     *
     * UI/message-thread only. NOT real-time safe.
     *
     * @param output Audio buffer to write into
     * @param numSamples Number of samples to read
     * @return Actual number of samples read
     */
    [[nodiscard]] virtual int readBlocking(juce::AudioBuffer<float>& output, int numSamples) const = 0;

    /**
     * Non-blocking, real-time-safe snapshot read for a single channel.
     * Performs one epoch-bracketed copy; returns std::nullopt if a
     * concurrent writer produced a torn read. Never yields.
     *
     * Safe from any thread, including the audio thread. Callers must
     * handle the std::nullopt case (typically: skip frame, reuse last
     * good result).
     *
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read
     * @param channel Channel index (0 = left, 1 = right)
     * @return Number of samples read, or std::nullopt if torn
     */
    [[nodiscard]] virtual std::optional<int> readSnapshot(float* output, int numSamples, int channel = 0) const = 0;

    /**
     * Non-blocking, real-time-safe snapshot read for all channels.
     * See readSnapshot(float*, ...) above.
     *
     * @param output Audio buffer to write into
     * @param numSamples Number of samples to read
     * @return Number of samples read, or std::nullopt if torn
     */
    [[nodiscard]] virtual std::optional<int> readSnapshot(juce::AudioBuffer<float>& output, int numSamples) const = 0;

    /**
     * Get the latest metadata (sample rate, timestamp, etc.)
     */
    virtual CaptureFrameMetadata getLatestMetadata() const = 0;

    /**
     * Get the number of available samples in the buffer
     */
    virtual size_t getAvailableSamples() const = 0;

    /**
     * Get the buffer capacity in samples
     */
    virtual size_t getCapacity() const = 0;

    /**
     * Get the peak level for a channel (recent samples)
     * @param channel Channel index
     * @param numSamples Number of recent samples to analyze
     */
    virtual float getPeakLevel(int channel, int numSamples = 1024) const = 0;

    /**
     * Get RMS level for a channel (recent samples)
     * @param channel Channel index
     * @param numSamples Number of recent samples to analyze
     */
    virtual float getRMSLevel(int channel, int numSamples = 1024) const = 0;
};

} // namespace multiscoper
