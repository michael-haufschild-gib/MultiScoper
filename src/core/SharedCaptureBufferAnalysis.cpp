/*
    MultiScoper - Shared Capture Buffer Analysis
    Peak and RMS level analysis over the SeqLock-protected ring buffer.
    Split from SharedCaptureBuffer.cpp: the read/write hot path is one
    concept, peak/RMS telemetry is a second concern that reuses only the
    epoch-bracketed read protocol.
*/

#include "core/SharedCaptureBuffer.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace multiscoper
{

float SharedCaptureBuffer::getPeakLevel(int channel, int numSamples) const
{
    if (channel < 0 || std::cmp_greater_equal(channel, MAX_CHANNELS) || numSamples <= 0)
        return 0.0f;

    for (;;)
    {
        uint32_t const epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if ((epoch1 & 1) != 0u)
        {
            std::this_thread::yield();
            continue;
        }

        size_t const available = getAvailableSamples();
        auto const requestedSamples = static_cast<size_t>(numSamples);
        int const samplesToAnalyze = static_cast<int>(std::min(requestedSamples, available));

        if (samplesToAnalyze <= 0)
            return 0.0f;

        size_t const writePos = writePos_.load(std::memory_order_acquire);
        size_t const readStart = (writePos + capacity_ - static_cast<size_t>(samplesToAnalyze)) & (capacity_ - 1);
        size_t const channelOffset = static_cast<size_t>(channel) * capacity_;

        float peak = 0.0f;

        auto analyzeChunk = [&](size_t start, size_t count) {
            for (size_t i = 0; i < count; ++i)
            {
                float const val = std::abs(buffer_[channelOffset + start + i]);
                peak = std::max(val, peak);
            }
        };

        std::atomic_thread_fence(std::memory_order_acquire);

        size_t const firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
        analyzeChunk(readStart, firstChunk);

        if (std::cmp_less(firstChunk, samplesToAnalyze))
        {
            analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t const epoch2 = writeEpoch_.load(std::memory_order_acquire);

        if (epoch1 == epoch2)
            return peak;
    }
}

float SharedCaptureBuffer::getRMSLevel(int channel, int numSamples) const
{
    if (channel < 0 || std::cmp_greater_equal(channel, MAX_CHANNELS) || numSamples <= 0)
        return 0.0f;

    for (;;)
    {
        uint32_t const epoch1 = writeEpoch_.load(std::memory_order_acquire);
        if ((epoch1 & 1) != 0u)
        {
            std::this_thread::yield();
            continue;
        }

        size_t const available = getAvailableSamples();
        auto const requestedSamples = static_cast<size_t>(numSamples);
        int const samplesToAnalyze = static_cast<int>(std::min(requestedSamples, available));

        if (samplesToAnalyze <= 0)
            return 0.0f;

        size_t const writePos = writePos_.load(std::memory_order_acquire);
        size_t const readStart = (writePos + capacity_ - static_cast<size_t>(samplesToAnalyze)) & (capacity_ - 1);
        size_t const channelOffset = static_cast<size_t>(channel) * capacity_;

        double sumSquares = 0.0;

        auto analyzeChunk = [&](size_t start, size_t count) {
            for (size_t i = 0; i < count; ++i)
            {
                float const val = buffer_[channelOffset + start + i];
                sumSquares += val * val;
            }
        };

        std::atomic_thread_fence(std::memory_order_acquire);

        size_t const firstChunk = std::min(static_cast<size_t>(samplesToAnalyze), capacity_ - readStart);
        analyzeChunk(readStart, firstChunk);

        if (std::cmp_less(firstChunk, samplesToAnalyze))
        {
            analyzeChunk(0, static_cast<size_t>(samplesToAnalyze) - firstChunk);
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t const epoch2 = writeEpoch_.load(std::memory_order_acquire);

        if (epoch1 == epoch2)
            return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samplesToAnalyze)));
    }
}

} // namespace multiscoper
