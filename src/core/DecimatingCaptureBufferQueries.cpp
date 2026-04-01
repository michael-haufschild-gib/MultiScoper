/*
    Oscil - Decimating Capture Buffer Read/Query Methods
    All read, query, and utility operations that forward to the internal buffer.

    Every read method acquires the swap lock, snapshots the buffer shared_ptr,
    then delegates. The withBuffer() helper eliminates this repeated pattern.
*/

#include "core/DecimatingCaptureBuffer.h"

namespace oscil
{

namespace
{

// Acquire the swap lock, snapshot the buffer, and invoke the callback.
// Returns defaultValue when the buffer is null.
template <typename Fn, typename T = std::invoke_result_t<Fn, SharedCaptureBuffer&>>
T withBuffer(const std::shared_ptr<SharedCaptureBuffer>& buffer, juce::SpinLock& lock, T defaultValue, Fn&& fn)
{
    std::shared_ptr<SharedCaptureBuffer> buf;
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        buf = buffer;
    }
    if (!buf)
        return defaultValue;
    return fn(*buf);
}

} // namespace

int DecimatingCaptureBuffer::read(float* output, int numSamples, int channel) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0,
                      [=](SharedCaptureBuffer& buf) { return buf.read(output, numSamples, channel); });
}

int DecimatingCaptureBuffer::read(std::span<float> output, int channel) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0, [=](SharedCaptureBuffer& buf) { return buf.read(output, channel); });
}

int DecimatingCaptureBuffer::read(juce::AudioBuffer<float>& output, int numSamples) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0,
                      [&](SharedCaptureBuffer& buf) { return buf.read(output, numSamples); });
}

CaptureFrameMetadata DecimatingCaptureBuffer::getLatestMetadata() const
{
    return withBuffer(buffer_, bufferSwapLock_, CaptureFrameMetadata{},
                      [](SharedCaptureBuffer& buf) { return buf.getLatestMetadata(); });
}

size_t DecimatingCaptureBuffer::getCapacity() const
{
    return withBuffer(buffer_, bufferSwapLock_, size_t{0}, [](SharedCaptureBuffer& buf) { return buf.getCapacity(); });
}

size_t DecimatingCaptureBuffer::getAvailableSamples() const
{
    return withBuffer(buffer_, bufferSwapLock_, size_t{0},
                      [](SharedCaptureBuffer& buf) { return buf.getAvailableSamples(); });
}

float DecimatingCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0.0f,
                      [=](SharedCaptureBuffer& buf) { return buf.getPeakLevel(channel, numSamples); });
}

float DecimatingCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0.0f,
                      [=](SharedCaptureBuffer& buf) { return buf.getRMSLevel(channel, numSamples); });
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

juce::String DecimatingCaptureBuffer::getMemoryUsageString() const { return formatBytes(getMemoryUsageBytes()); }

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
