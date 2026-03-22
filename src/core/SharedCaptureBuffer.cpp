/*
    Oscil - Shared Capture Buffer Implementation
*/

#include "core/SharedCaptureBuffer.h"
#include <cmath>
#include <algorithm>
#include <thread>

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

void SharedCaptureBuffer::writeInternal(const float* const* samples, int numSamples, int numChannels,
                                         const CaptureFrameMetadata& metadata)
{
    if (numSamples <= 0)
        return;

    const int actualChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));
    const auto totalSamples = static_cast<size_t>(numSamples);
    size_t writePos = writePos_.load(std::memory_order_relaxed);

    // If writing more than capacity, only the last capacity_ samples survive
    size_t srcOffset = 0;
    size_t effectiveSamples = totalSamples;
    if (effectiveSamples > capacity_)
    {
        srcOffset = effectiveSamples - capacity_;
        effectiveSamples = capacity_;
    }

    const size_t maskedWritePos = wrapPosition(writePos + srcOffset);
    // First segment: from maskedWritePos to end of buffer (or fewer if effectiveSamples fits)
    const size_t firstCount = std::min(effectiveSamples, capacity_ - maskedWritePos);
    const size_t secondCount = effectiveSamples - firstCount;

    // Increment epoch to odd → signals "write in progress" to readers
    writeEpoch_.fetch_add(1, std::memory_order_acq_rel);

    for (int ch = 0; ch < static_cast<int>(MAX_CHANNELS); ++ch)
    {
        float* dst = buffer_.data() + static_cast<size_t>(ch) * capacity_;

        if (ch < actualChannels && samples != nullptr && samples[ch] != nullptr)
        {
            const float* src = samples[ch] + srcOffset;
            std::memcpy(dst + maskedWritePos, src, firstCount * sizeof(float));
            if (secondCount > 0)
                std::memcpy(dst, src + firstCount, secondCount * sizeof(float));
        }
        else
        {
            std::memset(dst + maskedWritePos, 0, firstCount * sizeof(float));
            if (secondCount > 0)
                std::memset(dst, 0, secondCount * sizeof(float));
        }
    }

    writePos_.store(wrapPosition(writePos + totalSamples), std::memory_order_release);
    samplesWritten_.fetch_add(totalSamples, std::memory_order_release);

    CaptureFrameMetadata meta = metadata;
    meta.numSamples = numSamples;
    meta.numChannels = actualChannels;
    metadata_.write(meta);

    // Fence: ensure all data stores are visible before epoch even signal
    std::atomic_thread_fence(std::memory_order_release);
    writeEpoch_.fetch_add(1, std::memory_order_release);
}

void SharedCaptureBuffer::write(const float* const* samples, int numSamples, int numChannels,
                                 const CaptureFrameMetadata& metadata, bool tryLock)
{
    if (numSamples <= 0 || numChannels <= 0)
        return;

    if (tryLock)
    {
        const juce::SpinLock::ScopedTryLockType sl(writeLock_);
        if (!sl.isLocked())
            return;
        writeInternal(samples, numSamples, numChannels, metadata);
    }
    else
    {
        const juce::SpinLock::ScopedLockType sl(writeLock_);
        writeInternal(samples, numSamples, numChannels, metadata);
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

    // Torn-read detection: retry if a write occurred during our read.
    // The writer brackets data copies with odd/even epoch increments.
    // We spin with yield until we observe a consistent (untorn) snapshot.
    for (;;)
    {
        uint32_t epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if (epoch1 & 1)
        {
            std::this_thread::yield();
            continue;
        }

        size_t available = getAvailableSamples();
        size_t requestedSamples = output.size();

        size_t safeAvailable = std::min(available, capacity_);
        size_t samplesToRead = std::min(requestedSamples, safeAvailable);

        if (samplesToRead == 0)
            return 0;

        size_t writePos = writePos_.load(std::memory_order_acquire);

        size_t readStart = (writePos + capacity_ - samplesToRead) & (capacity_ - 1);
        size_t channelOffset = static_cast<size_t>(channel) * capacity_;

        size_t firstChunk = std::min(samplesToRead, capacity_ - readStart);
        size_t secondChunk = samplesToRead - firstChunk;

        // Fence: ensure buffer reads happen strictly after epoch1 load
        std::atomic_thread_fence(std::memory_order_acquire);

        std::memcpy(output.data(), &buffer_[channelOffset + readStart], firstChunk * sizeof(float));

        if (secondChunk > 0)
        {
            std::memcpy(output.data() + firstChunk, &buffer_[channelOffset], secondChunk * sizeof(float));
        }

        // Fence: ensure buffer reads complete before epoch2 load
        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t epoch2 = writeEpoch_.load(std::memory_order_acquire);

        if (epoch1 == epoch2)
            return static_cast<int>(samplesToRead);
    }
}

int SharedCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    int numChannels = std::min(output.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    if (numChannels <= 0 || numSamples <= 0)
        return 0;

    // Read all channels within a single epoch validation to guarantee
    // cross-channel consistency. The per-channel read() method validates
    // epochs independently, which can yield channels from different writes.
    for (;;)
    {
        uint32_t epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if (epoch1 & 1)
        {
            std::this_thread::yield();
            continue;
        }

        size_t available = getAvailableSamples();
        size_t safeAvailable = std::min(available, capacity_);
        size_t samplesToRead = std::min(static_cast<size_t>(numSamples), safeAvailable);

        if (samplesToRead == 0)
            return 0;

        size_t writePos = writePos_.load(std::memory_order_acquire);
        size_t readStart = (writePos + capacity_ - samplesToRead) & (capacity_ - 1);
        size_t firstChunk = std::min(samplesToRead, capacity_ - readStart);
        size_t secondChunk = samplesToRead - firstChunk;

        std::atomic_thread_fence(std::memory_order_acquire);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            size_t channelOffset = static_cast<size_t>(ch) * capacity_;
            float* dst = output.getWritePointer(ch);

            std::memcpy(dst, &buffer_[channelOffset + readStart], firstChunk * sizeof(float));
            if (secondChunk > 0)
                std::memcpy(dst + firstChunk, &buffer_[channelOffset], secondChunk * sizeof(float));
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t epoch2 = writeEpoch_.load(std::memory_order_acquire);
        if (epoch1 == epoch2)
            return static_cast<int>(samplesToRead);
    }
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
    // Acquire writeLock_ to prevent concurrent audio-thread writes
    // during the buffer reset.  Bracket with writeEpoch_ increments
    // so readers can detect the concurrent modification.
    const juce::SpinLock::ScopedLockType sl(writeLock_);

    writeEpoch_.fetch_add(1, std::memory_order_acq_rel);   // odd → modification in progress

    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_.store(0, std::memory_order_release);
    samplesWritten_.store(0, std::memory_order_release);

    auto meta = metadata_.read();
    meta.numSamples = 0;
    metadata_.write(meta);

    std::atomic_thread_fence(std::memory_order_release);
    writeEpoch_.fetch_add(1, std::memory_order_release);   // even → modification complete
}

float SharedCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    for (;;)
    {
        uint32_t epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if (epoch1 & 1)
        {
            std::this_thread::yield();
            continue;
        }

        size_t available = getAvailableSamples();
        size_t requestedSamples = static_cast<size_t>(numSamples);
        int samplesToAnalyze = static_cast<int>(std::min(requestedSamples, available));

        if (samplesToAnalyze <= 0)
            return 0.0f;

        size_t writePos = writePos_.load(std::memory_order_acquire);
        size_t readStart = (writePos + capacity_ - static_cast<size_t>(samplesToAnalyze)) & (capacity_ - 1);
        size_t channelOffset = static_cast<size_t>(channel) * capacity_;

        float peak = 0.0f;

        auto analyzeChunk = [&](size_t start, size_t count) {
            for (size_t i = 0; i < count; ++i)
            {
                float val = std::abs(buffer_[channelOffset + start + i]);
                if (val > peak) peak = val;
            }
        };

        std::atomic_thread_fence(std::memory_order_acquire);

        size_t firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
        analyzeChunk(readStart, firstChunk);

        if (firstChunk < static_cast<size_t>(samplesToAnalyze))
        {
            analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t epoch2 = writeEpoch_.load(std::memory_order_acquire);

        if (epoch1 == epoch2)
            return peak;
    }
}

float SharedCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    for (;;)
    {
        uint32_t epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if (epoch1 & 1)
        {
            std::this_thread::yield();
            continue;
        }

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

        std::atomic_thread_fence(std::memory_order_acquire);

        size_t firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
        analyzeChunk(readStart, firstChunk);

        if (firstChunk < static_cast<size_t>(samplesToAnalyze))
        {
            analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
        }

        float rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samplesToAnalyze)));

        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t epoch2 = writeEpoch_.load(std::memory_order_acquire);

        if (epoch1 == epoch2)
            return rms;
    }
}

} // namespace oscil
