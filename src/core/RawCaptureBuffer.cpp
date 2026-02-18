/*
    Oscil - Raw Capture Buffer Implementation
    Ultra-minimal SPSC ring buffer for lock-free audio thread writes
*/

#include "core/RawCaptureBuffer.h"
#include <algorithm>
#include <cmath>

namespace oscil
{

RawCaptureBuffer::RawCaptureBuffer()
{
    // Zero-initialize buffers
    bufferL_.fill(0.0f);
    bufferR_.fill(0.0f);
}

//==============================================================================
// Audio Thread Interface - LOCK FREE, WAIT FREE (~1μs)
//==============================================================================

void RawCaptureBuffer::write(const float* left, const float* right, int numSamples, double sampleRate)
{
    if (numSamples <= 0)
        return;

    // ===== MULTI-PRODUCER DETECTION (Defense-in-Depth) =====
    // SPSC assumes single producer. If DAW uses multiple audio threads, detect it.
    // This is real-time safe: just atomic compare-exchange, no logging.
    std::thread::id currentThread = std::this_thread::get_id();
    bool wasSet = producerIdSet_.load(std::memory_order_relaxed);
    if (!wasSet)
    {
        // First write - record the thread ID
        // Use compare-exchange to handle race between multiple first writes
        bool expected = false;
        if (producerIdSet_.compare_exchange_strong(expected, true, std::memory_order_release))
        {
            producerThreadId_.store(currentThread, std::memory_order_release);
        }
    }
    else
    {
        // Subsequent write - check if it's the same thread
        std::thread::id expectedThread = producerThreadId_.load(std::memory_order_relaxed);
        if (currentThread != expectedThread)
        {
            // Different thread detected! Mark for fallback to legacy path.
            // Don't block or log here - let the capture thread handle it.
            multiProducerDetected_.store(true, std::memory_order_release);
        }
    }

    // Clamp to capacity (handles case where block size > buffer)
    const size_t samplesToWrite = static_cast<size_t>(std::min(numSamples, static_cast<int>(CAPACITY)));

    // Get current write position (relaxed load - we're the only writer)
    const size_t currentPos = writePos_.load(std::memory_order_relaxed);

    // Calculate how many samples fit before wrap
    const size_t firstChunk = std::min(samplesToWrite, CAPACITY - currentPos);
    const size_t secondChunk = samplesToWrite - firstChunk;

    // Write left channel
    if (left != nullptr)
    {
        std::memcpy(&bufferL_[currentPos], left, firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memcpy(&bufferL_[0], left + firstChunk, secondChunk * sizeof(float));
        }
    }
    else
    {
        // Zero-fill if no left channel provided
        std::memset(&bufferL_[currentPos], 0, firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memset(&bufferL_[0], 0, secondChunk * sizeof(float));
        }
    }

    // Write right channel
    if (right != nullptr)
    {
        std::memcpy(&bufferR_[currentPos], right, firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memcpy(&bufferR_[0], right + firstChunk, secondChunk * sizeof(float));
        }
    }
    else if (left != nullptr)
    {
        // Copy left to right if only mono provided
        std::memcpy(&bufferR_[currentPos], &bufferL_[currentPos], firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memcpy(&bufferR_[0], &bufferL_[0], secondChunk * sizeof(float));
        }
    }
    else
    {
        // Zero-fill if no channels provided
        std::memset(&bufferR_[currentPos], 0, firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memset(&bufferR_[0], 0, secondChunk * sizeof(float));
        }
    }

    // Memory fence ensures all writes are visible before updating position
    std::atomic_thread_fence(std::memory_order_release);

    // Update write position atomically (wrapping via mask)
    const size_t newPos = (currentPos + samplesToWrite) & CAPACITY_MASK;
    writePos_.store(newPos, std::memory_order_release);

    // Update sample rate (relaxed - consumers will see it eventually)
    sampleRate_.store(sampleRate, std::memory_order_relaxed);

    // Update statistics
    totalSamplesWritten_.fetch_add(samplesToWrite, std::memory_order_relaxed);
}

void RawCaptureBuffer::writeMono(const float* mono, int numSamples, double sampleRate)
{
    // Write mono to both channels
    write(mono, mono, numSamples, sampleRate);
}

//==============================================================================
// Capture Thread Interface - LOCK FREE
//==============================================================================

int RawCaptureBuffer::read(float* left, float* right, int maxSamples)
{
    if (maxSamples <= 0 || (left == nullptr && right == nullptr))
        return 0;

    // Get current positions (acquire ensures we see all writes before writePos)
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_relaxed);

    // Calculate available samples (handles wrap-around)
    size_t available;
    if (writePos >= readPos)
    {
        available = writePos - readPos;
    }
    else
    {
        // Wrapped around
        available = CAPACITY - readPos + writePos;
    }

    // Limit to requested amount
    const size_t samplesToRead = std::min(available, static_cast<size_t>(maxSamples));

    if (samplesToRead == 0)
        return 0;

    // Calculate chunks (handle wrap-around)
    const size_t firstChunk = std::min(samplesToRead, CAPACITY - readPos);
    const size_t secondChunk = samplesToRead - firstChunk;

    // Read left channel
    if (left != nullptr)
    {
        std::memcpy(left, &bufferL_[readPos], firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memcpy(left + firstChunk, &bufferL_[0], secondChunk * sizeof(float));
        }
    }

    // Read right channel
    if (right != nullptr)
    {
        std::memcpy(right, &bufferR_[readPos], firstChunk * sizeof(float));
        if (secondChunk > 0)
        {
            std::memcpy(right + firstChunk, &bufferR_[0], secondChunk * sizeof(float));
        }
    }

    // Update read position atomically
    const size_t newReadPos = (readPos + samplesToRead) & CAPACITY_MASK;
    readPos_.store(newReadPos, std::memory_order_release);

    return static_cast<int>(samplesToRead);
}

size_t RawCaptureBuffer::getAvailableSamples() const
{
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_relaxed);

    if (writePos >= readPos)
    {
        return writePos - readPos;
    }
    else
    {
        return CAPACITY - readPos + writePos;
    }
}

double RawCaptureBuffer::getSampleRate() const
{
    return sampleRate_.load(std::memory_order_relaxed);
}

void RawCaptureBuffer::reset()
{
    // Reset positions atomically
    // NOTE: We intentionally do NOT zero the buffer data here because:
    // 1. The audio thread may still be writing (data race / undefined behavior)
    // 2. It's unnecessary - resetting positions makes old data unreachable
    // 3. New writes will overwrite old data anyway
    writePos_.store(0, std::memory_order_release);
    readPos_.store(0, std::memory_order_release);
    totalSamplesWritten_.store(0, std::memory_order_relaxed);

    // Reset multi-producer detection
    producerIdSet_.store(false, std::memory_order_release);
    multiProducerDetected_.store(false, std::memory_order_release);
}

//==============================================================================
// Multi-Producer Detection
//==============================================================================

bool RawCaptureBuffer::wasMultiProducerDetected() const
{
    return multiProducerDetected_.load(std::memory_order_acquire);
}

std::thread::id RawCaptureBuffer::getProducerThreadId() const
{
    return producerThreadId_.load(std::memory_order_acquire);
}

void RawCaptureBuffer::clearMultiProducerFlag()
{
    multiProducerDetected_.store(false, std::memory_order_release);
}

} // namespace oscil

