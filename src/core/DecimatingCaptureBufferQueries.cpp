/*
    Oscil - Decimating Capture Buffer Read/Query Methods
    All read, query, and utility operations that forward to the internal buffer
*/

#include "core/DecimatingCaptureBuffer.h"

namespace oscil
{

int DecimatingCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, numSamples, channel);
}

int DecimatingCaptureBuffer::read(std::span<float> output, int channel) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, channel);
}

int DecimatingCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->read(output, numSamples);
}

CaptureFrameMetadata DecimatingCaptureBuffer::getLatestMetadata() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return {};
    return buf->getLatestMetadata();
}

size_t DecimatingCaptureBuffer::getCapacity() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->getCapacity();
}

size_t DecimatingCaptureBuffer::getAvailableSamples() const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0;
    return buf->getAvailableSamples();
}

float DecimatingCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0.0f;
    return buf->getPeakLevel(channel, numSamples);
}

float DecimatingCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
    }
    if (!buf)
        return 0.0f;
    return buf->getRMSLevel(channel, numSamples);
}

size_t DecimatingCaptureBuffer::getMemoryUsageBytes() const
{
    size_t total = 0;
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);

    auto accumulateMemory = [&total](const std::shared_ptr<SharedCaptureBuffer>& buf,
                                     const std::shared_ptr<ProcessingContext>& ctx) {
        if (buf)
            total += buf->getCapacity() * SharedCaptureBuffer::MAX_CHANNELS * sizeof(float);

        if (ctx)
        {
            total += ctx->filterMemoryBytes;
            total += ctx->scratchBuffer.capacity() * sizeof(float);
        }
    };

    accumulateMemory(buffer_, context_);
    for (const auto& item : graveyard_)
        accumulateMemory(item.buffer, item.context);

    return total;
}

juce::String DecimatingCaptureBuffer::getMemoryUsageString() const
{
    size_t bytes = getMemoryUsageBytes();

    if (bytes >= 1024 * 1024)
    {
        float mb = static_cast<float>(bytes) / (1024.0f * 1024.0f);
        return juce::String(mb, 1) + " MB";
    }
    else if (bytes >= 1024)
    {
        float kb = static_cast<float>(bytes) / 1024.0f;
        return juce::String(kb, 0) + " KB";
    }
    else
    {
        return juce::String(bytes) + " B";
    }
}

void DecimatingCaptureBuffer::clear()
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    std::shared_ptr<ProcessingContext> ctx;
    {
        const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
        buf = buffer_;
        ctx = context_;
    }

    if (buf)
        buf->clear();

    if (ctx)
    {
        for (auto& filter : ctx->filters)
            filter.reset();

        std::fill(ctx->decimationCounters.begin(), ctx->decimationCounters.end(), 0);
    }
}

std::shared_ptr<SharedCaptureBuffer> DecimatingCaptureBuffer::getInternalBuffer() const
{
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
    return buffer_;
}

} // namespace oscil
