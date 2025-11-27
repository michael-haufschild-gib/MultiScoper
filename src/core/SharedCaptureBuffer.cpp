/*
    Oscil - Shared Capture Buffer Implementation
*/

#include "core/SharedCaptureBuffer.h"
#include <cmath>
#include <algorithm>

namespace oscil
{

// Round up to next power of 2 for fast bitwise modulo
static size_t nextPowerOfTwo(size_t n)
{
    if (n == 0) return 1;
    // Check if already power of 2
    if ((n & (n - 1)) == 0) return n;
    // Round up
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

SharedCaptureBuffer::SharedCaptureBuffer(size_t bufferSamples)
    : capacity_(nextPowerOfTwo(bufferSamples))
{
    buffers_.resize(MAX_CHANNELS);
    for (auto& buffer : buffers_)
    {
        buffer.resize(capacity_, 0.0f);
    }
}

void SharedCaptureBuffer::write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata)
{
    const int numChannels = std::min(buffer.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    // Get channel pointers
    const float* channels[MAX_CHANNELS] = { nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[ch] = buffer.getReadPointer(ch);
    }

    write(channels, numSamples, numChannels, metadata);
}

void SharedCaptureBuffer::write(const float* const* samples, int numSamples, int numChannels,
                                 const CaptureFrameMetadata& metadata)
{
    if (numSamples <= 0 || numChannels <= 0)
        return;

    numChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));

    // Get current write position
    size_t writePos = writePos_.load(std::memory_order_relaxed);

    // Write samples to ring buffer
    for (int i = 0; i < numSamples; ++i)
    {
        size_t pos = wrapPosition(writePos + i);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (samples[ch] != nullptr)
            {
                buffers_[ch][pos] = samples[ch][i];
            }
        }

        // Zero out unused channels
        for (int ch = numChannels; ch < static_cast<int>(MAX_CHANNELS); ++ch)
        {
            buffers_[ch][pos] = 0.0f;
        }
    }

    // Update write position atomically
    writePos_.store(wrapPosition(writePos + numSamples), std::memory_order_release);

    // Update total samples written
    samplesWritten_.fetch_add(numSamples, std::memory_order_relaxed);

    // Update metadata using lock-free SeqLock pattern
    CaptureFrameMetadata meta = metadata;
    meta.numSamples = numSamples;
    meta.numChannels = numChannels;
    metadata_.write(meta);
}

int SharedCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    if (output == nullptr || numSamples <= 0 || channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0;

    size_t available = getAvailableSamples();
    int samplesToRead = std::min(static_cast<size_t>(numSamples), available);

    if (samplesToRead <= 0)
        return 0;

    // Read from the most recent samples (before write position)
    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - samplesToRead) % capacity_;

    for (int i = 0; i < samplesToRead; ++i)
    {
        size_t pos = wrapPosition(readStart + i);
        output[i] = buffers_[channel][pos];
    }

    return samplesToRead;
}

int SharedCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    int numChannels = std::min(output.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    int samplesRead = 0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        int read = this->read(output.getWritePointer(ch), numSamples, ch);
        if (ch == 0)
            samplesRead = read;
    }

    return samplesRead;
}

CaptureFrameMetadata SharedCaptureBuffer::getLatestMetadata() const
{
    return metadata_.read();
}

size_t SharedCaptureBuffer::getAvailableSamples() const
{
    size_t written = samplesWritten_.load(std::memory_order_acquire);
    return std::min(written, capacity_);
}

void SharedCaptureBuffer::clear()
{
    for (auto& buffer : buffers_)
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    writePos_.store(0, std::memory_order_release);
    samplesWritten_.store(0, std::memory_order_release);
}

float SharedCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    size_t available = getAvailableSamples();
    int samplesToAnalyze = std::min(static_cast<size_t>(numSamples), available);

    if (samplesToAnalyze <= 0)
        return 0.0f;

    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - samplesToAnalyze) % capacity_;

    float peak = 0.0f;
    for (int i = 0; i < samplesToAnalyze; ++i)
    {
        size_t pos = wrapPosition(readStart + i);
        float absVal = std::abs(buffers_[channel][pos]);
        if (absVal > peak)
            peak = absVal;
    }

    return peak;
}

float SharedCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    size_t available = getAvailableSamples();
    int samplesToAnalyze = std::min(static_cast<size_t>(numSamples), available);

    if (samplesToAnalyze <= 0)
        return 0.0f;

    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - samplesToAnalyze) % capacity_;

    double sumSquares = 0.0;
    for (int i = 0; i < samplesToAnalyze; ++i)
    {
        size_t pos = wrapPosition(readStart + i);
        float val = buffers_[channel][pos];
        sumSquares += val * val;
    }

    return static_cast<float>(std::sqrt(sumSquares / samplesToAnalyze));
}

} // namespace oscil
