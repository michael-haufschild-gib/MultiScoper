/*
    Oscil - Shared Capture Buffer Implementation
    Lock-free SPSC ring buffer with batched memcpy writes
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
    if ((n & (n - 1)) == 0) return n;
    
    constexpr size_t maxPowerOf2 = size_t{1} << (sizeof(size_t) * 8 - 1);
    if (n > maxPowerOf2) return maxPowerOf2;
    
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if constexpr (sizeof(size_t) > 4)
    {
        n |= n >> 32;
    }
    return n + 1;
}

SharedCaptureBuffer::SharedCaptureBuffer(size_t bufferSamples)
    : capacity_(nextPowerOfTwo(bufferSamples))
{
    buffer_.resize(capacity_ * MAX_CHANNELS, 0.0f);
}

//==============================================================================
// Lock-Free Write (Audio Thread - SPSC)
//==============================================================================

void SharedCaptureBuffer::writeLockFree(const juce::AudioBuffer<float>& buffer,
                                         const CaptureFrameMetadata& metadata)
{
    const int numChannels = std::min(buffer.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

    const float* channels[MAX_CHANNELS] = { nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[ch] = buffer.getReadPointer(ch);
    }

    writeLockFree(channels, numSamples, numChannels, metadata);
}

void SharedCaptureBuffer::writeLockFree(const float* const* samples, int numSamples,
                                         int numChannels, const CaptureFrameMetadata& metadata)
{
    // CRITICAL FIX (C8): Runtime guards MUST come before jasserts
    // jassert is disabled in release builds, so null pointer would cause crash
    // These guards provide defense-in-depth for release builds
    if (samples == nullptr || numSamples <= 0 || numChannels <= 0)
        return;

    // Debug-only assertions to catch programming errors during development
    jassert(samples != nullptr);
    jassert(numSamples >= 0);
    jassert(numChannels >= 0);

    writeInternal(samples, numSamples, numChannels, metadata);
}

void SharedCaptureBuffer::writeInternal(const float* const* samples, int numSamples,
                                         int numChannels, const CaptureFrameMetadata& metadata)
{
    // NOTE: No logging allowed here - this runs on audio thread
    // DBG() was removed to avoid blocking (see styleguide rule: no DBG on audio thread)

    const int actualChannels = std::min(numChannels, static_cast<int>(MAX_CHANNELS));
    const size_t mask = capacity_ - 1;
    size_t writePos = writePos_.load(std::memory_order_relaxed);

    // Batched memcpy per channel - much faster than sample-by-sample loop
    for (int ch = 0; ch < actualChannels; ++ch)
    {
        const size_t channelOffset = static_cast<size_t>(ch) * capacity_;
        const float* src = samples[ch];
        
        if (src == nullptr)
        {
            // Zero-fill if source is null
            const size_t firstChunk = std::min(static_cast<size_t>(numSamples), capacity_ - writePos);
            std::memset(&buffer_[channelOffset + writePos], 0, firstChunk * sizeof(float));
            
            if (firstChunk < static_cast<size_t>(numSamples))
            {
                std::memset(&buffer_[channelOffset], 0, 
                           (static_cast<size_t>(numSamples) - firstChunk) * sizeof(float));
            }
            continue;
        }

        // Calculate chunks for wrap-around handling
        const size_t firstChunk = std::min(static_cast<size_t>(numSamples), capacity_ - writePos);
        
        // Copy first chunk (before wrap)
        std::memcpy(&buffer_[channelOffset + writePos], src, firstChunk * sizeof(float));
        
        // Copy second chunk (after wrap) if needed
        if (firstChunk < static_cast<size_t>(numSamples))
        {
            const size_t secondChunk = static_cast<size_t>(numSamples) - firstChunk;
            std::memcpy(&buffer_[channelOffset], src + firstChunk, secondChunk * sizeof(float));
        }
    }

    // Zero-fill unused channels
    for (int ch = actualChannels; ch < static_cast<int>(MAX_CHANNELS); ++ch)
    {
        const size_t channelOffset = static_cast<size_t>(ch) * capacity_;
        const size_t firstChunk = std::min(static_cast<size_t>(numSamples), capacity_ - writePos);
        
        std::memset(&buffer_[channelOffset + writePos], 0, firstChunk * sizeof(float));
        
        if (firstChunk < static_cast<size_t>(numSamples))
        {
            std::memset(&buffer_[channelOffset], 0,
                       (static_cast<size_t>(numSamples) - firstChunk) * sizeof(float));
        }
    }

    // Memory fence ensures all writes are visible before updating position
    std::atomic_thread_fence(std::memory_order_release);

    // Update write position atomically
    const size_t newWritePos = (writePos + static_cast<size_t>(numSamples)) & mask;

    // Track buffer wrap-around for overtake detection
    // If newWritePos < writePos (due to wrap-around), increment wrap count
    if (newWritePos < writePos)
    {
        writeWrapCount_.fetch_add(1, std::memory_order_relaxed);
    }

    writePos_.store(newWritePos, std::memory_order_release);

    // Update samples written count
    samplesWritten_.fetch_add(static_cast<size_t>(numSamples), std::memory_order_relaxed);

    // Update metadata (already lock-free via SeqLock)
    CaptureFrameMetadata meta = metadata;
    meta.numSamples = numSamples;
    meta.numChannels = actualChannels;
    metadata_.write(meta);
}

//==============================================================================
// Legacy Write Interface (with locking)
//==============================================================================

void SharedCaptureBuffer::write(const juce::AudioBuffer<float>& buffer, 
                                 const CaptureFrameMetadata& metadata, bool tryLock)
{
    const int numChannels = std::min(buffer.getNumChannels(), static_cast<int>(MAX_CHANNELS));
    const int numSamples = buffer.getNumSamples();

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
    // CRITICAL FIX (C8): Runtime guards MUST come before jasserts
    // jassert is disabled in release builds, so null pointer would cause crash
    if (samples == nullptr || numSamples <= 0 || numChannels <= 0)
        return;

    // Debug-only assertions to catch programming errors during development
    jassert(samples != nullptr);
    jassert(numSamples >= 0);
    jassert(numChannels >= 0);

    if (tryLock)
    {
        const juce::SpinLock::ScopedTryLockType sl(writeLock_);
        if (!sl.isLocked())
            return;  // Drop frame rather than block
        writeInternal(samples, numSamples, numChannels, metadata);
    }
    else
    {
        const juce::SpinLock::ScopedLockType sl(writeLock_);
        writeInternal(samples, numSamples, numChannels, metadata);
    }
}

//==============================================================================
// Read Interface
//==============================================================================

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

    size_t safeAvailable = std::min(available, capacity_);
    size_t samplesToRead = std::min(requestedSamples, safeAvailable);

    // NOTE: No logging allowed here - read() may be called from audio thread context
    // DBG() was removed to avoid blocking (see styleguide rule: no DBG on audio thread)

    if (samplesToRead == 0)
        return 0;

    size_t writePos = writePos_.load(std::memory_order_acquire);
    size_t readStart = (writePos + capacity_ - samplesToRead) & (capacity_ - 1);
    size_t channelOffset = static_cast<size_t>(channel) * capacity_;
    
    // Batched memcpy for reading
    size_t firstChunk = std::min(samplesToRead, capacity_ - readStart);
    std::memcpy(output.data(), &buffer_[channelOffset + readStart], firstChunk * sizeof(float));

    if (firstChunk < samplesToRead)
    {
        size_t secondChunk = samplesToRead - firstChunk;
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
        int result = this->read(output.getWritePointer(ch), numSamples, ch);
        if (ch == 0)
            samplesRead = result;
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
    // CRITICAL FIX (C7): Synchronize clear() with concurrent read()/write() operations.
    // This method should ONLY be called from the message thread when no audio processing
    // is occurring (e.g., during initialization or when plugin is suspended).
    //
    // Memory ordering rationale:
    // 1. First reset atomics with release semantics to signal "clearing in progress"
    // 2. Use acquire fence to ensure we see all prior writes before clearing
    // 3. Clear buffer data
    // 4. Use release fence to ensure buffer clear completes before subsequent operations
    //
    // NOTE: For true lock-free safety during active processing, callers should use
    // the legacy write() path with locking or ensure clear() is only called when
    // the audio thread is suspended.

    // Step 1: Reset write position first to signal readers that data is stale
    // Using seq_cst for maximum safety during this non-real-time operation
    samplesWritten_.store(0, std::memory_order_seq_cst);
    writePos_.store(0, std::memory_order_seq_cst);
    writeWrapCount_.store(0, std::memory_order_seq_cst);

    // Step 2: Acquire fence ensures we see all prior writes before clearing
    std::atomic_thread_fence(std::memory_order_acquire);

    // Step 3: Clear the buffer
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);

    // Step 4: Release fence ensures buffer clear is visible before any subsequent writes
    std::atomic_thread_fence(std::memory_order_release);
}

float SharedCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    if (channel < 0 || channel >= static_cast<int>(MAX_CHANNELS))
        return 0.0f;

    size_t available = getAvailableSamples();
    int samplesToAnalyze = static_cast<int>(std::min(static_cast<size_t>(numSamples), available));

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
    int samplesToAnalyze = static_cast<int>(std::min(static_cast<size_t>(numSamples), available));

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
            sumSquares += static_cast<double>(val) * static_cast<double>(val);
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
