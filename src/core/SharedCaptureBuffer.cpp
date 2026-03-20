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
    // Prevent overflow: if n is too large, return max representable power of 2
    constexpr size_t maxPowerOf2 = size_t{1} << (sizeof(size_t) * 8 - 1);
    if (n > maxPowerOf2) return maxPowerOf2;
    // Round up using bit manipulation
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    // Only apply 32-bit shift on 64-bit platforms to avoid UB
    // (shifting by >= width of type is undefined behavior)
    if constexpr (sizeof(size_t) > 4)
    {
        n |= n >> 32;
    }
    return n + 1;
}

SharedCaptureBuffer::SharedCaptureBuffer(size_t bufferSamples)
    : capacity_(nextPowerOfTwo(bufferSamples))
{
    // Allocate flat buffer for all channels
    // Layout: [Channel 0 Data] [Channel 1 Data] ...
    buffer_.resize(capacity_ * MAX_CHANNELS, 0.0f);
}

void SharedCaptureBuffer::write(const juce::AudioBuffer<float>& buffer, const CaptureFrameMetadata& metadata, bool tryLock)
{
    const int numChannels = std::min(buffer.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    // Get channel pointers
    const float* channels[MAX_CHANNELS] = { nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[ch] = buffer.getReadPointer(ch);
    }

    write(channels, numSamples, numChannels, metadata, tryLock);
}

void SharedCaptureBuffer::write(const float* const* samples, int numSamples, int numChannels,
                                 const CaptureFrameMetadata& metadata, bool tryLock)
{
    if (numSamples <= 0 || numChannels <= 0)
        return;

    // Handle locking based on tryLock parameter
    // For real-time audio (tryLock=true), we must not block.
    // If we can't get the lock, we drop the frame.
    if (tryLock)
    {
        const juce::SpinLock::ScopedTryLockType sl(writeLock_);
        if (!sl.isLocked())
            return;
            
        // Lock acquired, proceed
        // Clamp channels once
        const int actualChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));

        // Get current write position
        size_t writePos = writePos_.load(std::memory_order_relaxed);

        // Write samples to ring buffer
        // Optimization: Write all channels in the loop to maximize cache hits if possible,
        // but since we used planar layout, we write to distant memory regions.
        // However, flat buffer removes the pointer indirection overhead.
        
        for (int i = 0; i < numSamples; ++i)
        {
            size_t pos = wrapPosition(writePos + static_cast<size_t>(i));

            for (int ch = 0; ch < actualChannels; ++ch)
            {
                float sample = 0.0f;
                if (samples != nullptr && samples[ch] != nullptr)
                    sample = samples[ch][i];
                buffer_[static_cast<size_t>(ch) * capacity_ + pos] = sample;
            }

            // Zero out unused channels
            for (int ch = actualChannels; ch < static_cast<int>(MAX_CHANNELS); ++ch)
            {
                buffer_[static_cast<size_t>(ch) * capacity_ + pos] = 0.0f;
            }
        }

        // Update write position atomically
        writePos_.store(wrapPosition(writePos + static_cast<size_t>(numSamples)), std::memory_order_release);

        // Update total samples written
        samplesWritten_.fetch_add(static_cast<size_t>(numSamples), std::memory_order_release);

        // Update metadata
        CaptureFrameMetadata meta = metadata;
        meta.numSamples = numSamples;
        meta.numChannels = actualChannels;
        metadata_.write(meta);
    }
    else
    {
        // Non-real-time thread: block until lock acquired
        const juce::SpinLock::ScopedLockType sl(writeLock_);
        
        // Duplicate logic (could extract to private method, but keeping inline for perf)
        const int actualChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));
        size_t writePos = writePos_.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            size_t pos = wrapPosition(writePos + static_cast<size_t>(i));

            for (int ch = 0; ch < actualChannels; ++ch)
            {
                float sample = 0.0f;
                if (samples != nullptr && samples[ch] != nullptr)
                    sample = samples[ch][i];
                buffer_[static_cast<size_t>(ch) * capacity_ + pos] = sample;
            }

            for (int ch = actualChannels; ch < static_cast<int>(MAX_CHANNELS); ++ch)
            {
                buffer_[static_cast<size_t>(ch) * capacity_ + pos] = 0.0f;
            }
        }

        writePos_.store(wrapPosition(writePos + static_cast<size_t>(numSamples)), std::memory_order_release);
        samplesWritten_.fetch_add(static_cast<size_t>(numSamples), std::memory_order_release);
        
        CaptureFrameMetadata meta = metadata;
        meta.numSamples = numSamples;
        meta.numChannels = actualChannels;
        metadata_.write(meta);
    }
}

int SharedCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    if (output == nullptr || numSamples <= 0 || channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0;

    return read(std::span<float>(output, static_cast<size_t>(numSamples)), channel);
}

int SharedCaptureBuffer::read(std::span<float> output, int channel) const
{
    if (output.empty() || channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0;

    size_t available = getAvailableSamples();
    size_t requestedSamples = output.size();
    
    // Clamp read request to available samples
    size_t safeAvailable = std::min(available, capacity_);
    size_t samplesToRead = std::min(requestedSamples, safeAvailable);

    if (samplesToRead == 0)
        return 0;

    // Read from the most recent samples (before write position)
    size_t writePos = writePos_.load(std::memory_order_acquire);
    
    size_t readStart = (writePos + capacity_ - samplesToRead) & (capacity_ - 1);
    size_t channelOffset = static_cast<size_t>(channel) * capacity_;
    
    // Optimization: Split into two memcpy's if wrapping, or one if contiguous
    // Since we are reading a ring buffer, it might wrap around the end.
    
    size_t firstChunk = std::min(samplesToRead, capacity_ - readStart);
    size_t secondChunk = samplesToRead - firstChunk;

    // Copy first chunk
    std::memcpy(output.data(), &buffer_[channelOffset + readStart], firstChunk * sizeof(float));

    // Copy second chunk (wrapped)
    if (secondChunk > 0)
    {
        std::memcpy(output.data() + firstChunk, &buffer_[channelOffset], secondChunk * sizeof(float));
    }

    return static_cast<int>(samplesToRead);
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
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_.store(0, std::memory_order_release);
    samplesWritten_.store(0, std::memory_order_release);

    // Preserve transport fields but clear frame-size semantics after buffer reset.
    auto meta = metadata_.read();
    meta.numSamples = 0;
    metadata_.write(meta);
}

float SharedCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    size_t available = getAvailableSamples();
    size_t requestedSamples = static_cast<size_t>(numSamples);
    int samplesToAnalyze = static_cast<int>(std::min(requestedSamples, available));

    if (samplesToAnalyze <= 0)
        return 0.0f;

    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - static_cast<size_t>(samplesToAnalyze)) & (capacity_ - 1);
    size_t channelOffset = static_cast<size_t>(channel) * capacity_;

    float peak = 0.0f;
    
    // Analyze in chunks to handle wrap-around
    auto analyzeChunk = [&](size_t start, size_t count) {
        for (size_t i = 0; i < count; ++i)
        {
            float val = std::abs(buffer_[channelOffset + start + i]);
            if (val > peak) peak = val;
        }
    };

    size_t firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
    analyzeChunk(readStart, firstChunk);
    
    if (firstChunk < static_cast<size_t>(samplesToAnalyze))
    {
        analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
    }

    return peak;
}

float SharedCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    size_t available = getAvailableSamples();
    size_t requestedSamples = static_cast<size_t>(numSamples);
    int samplesToAnalyze = static_cast<int>(std::min(requestedSamples, available));

    if (samplesToAnalyze <= 0)
        return 0.0f;

    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - static_cast<size_t>(samplesToAnalyze)) & (capacity_ - 1);
    size_t channelOffset = static_cast<size_t>(channel) * capacity_;

    double sumSquares = 0.0;

    auto analyzeChunk = [&](size_t start, size_t count) {
        for (size_t i = 0; i < count; ++i)
        {
            float val = buffer_[channelOffset + start + i];
            sumSquares += val * val;
        }
    };

    size_t firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
    analyzeChunk(readStart, firstChunk);

    if (firstChunk < static_cast<size_t>(samplesToAnalyze))
    {
        analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
    }

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samplesToAnalyze)));
}

} // namespace oscil
