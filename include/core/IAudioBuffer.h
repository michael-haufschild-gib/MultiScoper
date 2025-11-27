/*
    Oscil - Audio Buffer Interface
    Abstract interface for read-only audio buffer access
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace oscil
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
 */
class IAudioBuffer
{
public:
    virtual ~IAudioBuffer() = default;

    /**
     * Read the most recent samples into a buffer.
     * Safe to call from any thread.
     *
     * @param output Buffer to write samples into
     * @param numSamples Number of samples to read
     * @param channel Channel index (0 = left, 1 = right)
     * @return Actual number of samples read
     */
    virtual int read(float* output, int numSamples, int channel = 0) const = 0;

    /**
     * Read the most recent samples for all channels.
     * @param output Audio buffer to write into
     * @param numSamples Number of samples to read
     * @return Actual number of samples read
     */
    virtual int read(juce::AudioBuffer<float>& output, int numSamples) const = 0;

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

} // namespace oscil
