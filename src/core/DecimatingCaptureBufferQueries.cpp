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

int DecimatingCaptureBuffer::readBlocking(float* output, int numSamples, int channel) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0,
                      [=](SharedCaptureBuffer& buf) { return buf.readBlocking(output, numSamples, channel); });
}

int DecimatingCaptureBuffer::readBlocking(std::span<float> output, int channel) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0,
                      [=](SharedCaptureBuffer& buf) { return buf.readBlocking(output, channel); });
}

int DecimatingCaptureBuffer::readBlocking(juce::AudioBuffer<float>& output, int numSamples) const
{
    return withBuffer(buffer_, bufferSwapLock_, 0,
                      [&](SharedCaptureBuffer& buf) { return buf.readBlocking(output, numSamples); });
}

// Snapshot paths are real-time safe: they must NOT take the bufferSwapLock_
// SpinLock. Instead they use the lock-free publishedBuffer_ atomic (which
// is already used by the audio-thread write path). The graveyard keeps
// swapped-out buffers alive for ~2s so the raw pointer remains valid even
// if reconfigure() ran between the load and the dereference.
std::optional<int> DecimatingCaptureBuffer::readSnapshot(float* output, int numSamples, int channel) const
{
    const SharedCaptureBuffer* const buf = publishedBuffer_.load(std::memory_order_acquire);
    if (buf == nullptr)
        return std::nullopt;
    return buf->readSnapshot(output, numSamples, channel);
}

std::optional<int> DecimatingCaptureBuffer::readSnapshot(std::span<float> output, int channel) const
{
    const SharedCaptureBuffer* const buf = publishedBuffer_.load(std::memory_order_acquire);
    if (buf == nullptr)
        return std::nullopt;
    return buf->readSnapshot(output, channel);
}

std::optional<int> DecimatingCaptureBuffer::readSnapshot(juce::AudioBuffer<float>& output, int numSamples) const
{
    const SharedCaptureBuffer* const buf = publishedBuffer_.load(std::memory_order_acquire);
    if (buf == nullptr)
        return std::nullopt;
    return buf->readSnapshot(output, numSamples);
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
    // Delegate to reconfigure() which safely swaps in fresh buffer + context
    // via the graveyard pattern. The old approach mutated the existing
    // ProcessingContext (filters, counters) while the audio thread could be
    // using the same object through its shared_ptr copy — a data race.
    reconfigure();
}

std::shared_ptr<SharedCaptureBuffer> DecimatingCaptureBuffer::getInternalBuffer() const
{
    const juce::SpinLock::ScopedLockType sl(bufferSwapLock_);
    return buffer_;
}

} // namespace oscil
