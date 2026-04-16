/*
    Oscil - IAudioBuffer Thread-Safety Tests (readSnapshot RT-safety)

    Verifies the contract that SharedCaptureBuffer::readSnapshot() and
    DecimatingCaptureBuffer::readSnapshot() NEVER yield or otherwise
    block when a writer is mid-write — they return std::nullopt on any
    torn read or writer-in-progress observation.

    These tests are complementary to test_capture_buffer_concurrent and
    test_capture_buffer_threading, which exercise the blocking variant.

    Key invariants exercised here:
      1. Every snapshot result is either std::nullopt OR a fully
         self-consistent sample count (never torn).
      2. Under heavy writer contention the reader observes a meaningful
         fraction of non-nullopt results (the path is not degenerate).
      3. A single snapshot call executes in bounded wall-clock time
         (proxy for "no yield/syscall under contention").
*/

#include "core/DecimatingCaptureBuffer.h"
#include "core/SharedCaptureBuffer.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace oscil;

namespace
{

// Write a ramp with all channels set to the same integer-valued sample so
// a self-consistent read has all samples equal (easy torn detection).
void writeConstantBlock(SharedCaptureBuffer& buffer, float value, int numSamples)
{
    std::vector<float> left(static_cast<size_t>(numSamples), value);
    std::vector<float> right(static_cast<size_t>(numSamples), value);
    const float* channels[2] = {left.data(), right.data()};
    CaptureFrameMetadata meta;
    meta.sampleRate = 48000.0;
    meta.numChannels = 2;
    meta.numSamples = numSamples;
    buffer.write(channels, numSamples, 2, meta, /*tryLock=*/true);
}

} // namespace

TEST(AudioBufferSnapshotThreadSafety, SnapshotNeverTornUnderContention)
{
    SharedCaptureBuffer buffer(4096);

    std::atomic<bool> running{true};
    std::atomic<int> writerIters{0};
    std::atomic<int> snapshotOk{0};
    std::atomic<int> snapshotNullopt{0};
    std::atomic<int> tornDetected{0};

    constexpr int kBlockSize = 128;

    std::thread writer([&]() {
        float value = 1.0f;
        while (running.load(std::memory_order_relaxed))
        {
            writeConstantBlock(buffer, value, kBlockSize);
            writerIters.fetch_add(1, std::memory_order_relaxed);
            value += 1.0f;
            if (value > 1000.0f)
                value = 1.0f;
        }
    });

    std::thread reader([&]() {
        std::vector<float> out(static_cast<size_t>(kBlockSize));
        while (running.load(std::memory_order_relaxed))
        {
            auto result = buffer.readSnapshot(std::span<float>(out), 0);
            if (!result.has_value())
            {
                snapshotNullopt.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            snapshotOk.fetch_add(1, std::memory_order_relaxed);

            int const n = *result;
            if (n <= 0)
                continue;
            // Self-consistency: within a single snapshot, the most
            // recent writer value is a single constant; ALL returned
            // samples must be equal. A torn read from concurrent
            // writers would yield a mix.
            float const first = out[0];
            for (int i = 1; i < n; ++i)
            {
                if (out[static_cast<size_t>(i)] != first)
                {
                    tornDetected.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(writerIters.load(), 100);
    EXPECT_GT(snapshotOk.load(), 100);
    EXPECT_EQ(tornDetected.load(), 0) << "readSnapshot returned torn data (" << tornDetected.load() << " torn of "
                                      << snapshotOk.load() << ")";
}

TEST(AudioBufferSnapshotThreadSafety, SnapshotCallBoundedWallClock)
{
    // Proxy for "no yield/syscall under contention": each readSnapshot
    // call must complete in a single-pass bounded time, even while
    // writers hammer the epoch counter. We tolerate a generous upper
    // bound to avoid flakiness on loaded CI, but a yield/spin loop
    // would violate this by orders of magnitude on torn contention.
    SharedCaptureBuffer buffer(4096);

    std::atomic<bool> running{true};
    std::thread writer([&]() {
        float value = 1.0f;
        while (running.load(std::memory_order_relaxed))
        {
            writeConstantBlock(buffer, value, 128);
            value += 1.0f;
        }
    });

    // Warm-up.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::vector<float> out(128);
    std::vector<long long> timingsNs;
    timingsNs.reserve(2000);

    for (int i = 0; i < 2000; ++i)
    {
        auto const t0 = std::chrono::steady_clock::now();
        (void) buffer.readSnapshot(std::span<float>(out), 0);
        auto const t1 = std::chrono::steady_clock::now();
        timingsNs.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    running.store(false, std::memory_order_relaxed);
    writer.join();

    // Compute max observation. On any sane machine without yield,
    // readSnapshot is a single memcpy of 128 floats plus epoch loads —
    // well under 100 microseconds even on a loaded system.
    // A yielding implementation on torn contention would easily hit
    // milliseconds on scheduler latency.
    long long maxNs = 0;
    for (auto t : timingsNs)
        maxNs = std::max(maxNs, t);

    // 5ms is very generous — the actual path should be well under 1ms.
    EXPECT_LT(maxNs, 5'000'000LL) << "readSnapshot took " << maxNs << "ns (> 5ms); likely yielded";
}

namespace
{

struct DecSnapshotStats
{
    std::atomic<int> writerIters{0};
    std::atomic<int> snapshotOk{0};
    std::atomic<int> snapshotNullopt{0};
    std::atomic<int> outOfRangeCount{0};
    std::atomic<int> nonFiniteCount{0};
    std::atomic<long long> maxCallNs{0};
};

void decSnapshotWriterLoop(DecimatingCaptureBuffer& decBuffer, std::atomic<bool>& running, DecSnapshotStats& stats,
                           int blockSize)
{
    float value = 1.0f;
    while (running.load(std::memory_order_relaxed))
    {
        std::vector<float> left(static_cast<size_t>(blockSize), value);
        std::vector<float> right(static_cast<size_t>(blockSize), value);
        const float* channels[2] = {left.data(), right.data()};
        CaptureFrameMetadata meta;
        meta.sampleRate = 48000.0;
        meta.numChannels = 2;
        meta.numSamples = blockSize;
        decBuffer.write(channels, blockSize, 2, meta);
        stats.writerIters.fetch_add(1, std::memory_order_relaxed);
        value += 1.0f;
        if (value > 1000.0f)
            value = 1.0f;
    }
}

void decSnapshotReaderLoop(const DecimatingCaptureBuffer& decBuffer, std::atomic<bool>& running,
                           DecSnapshotStats& stats, int blockSize)
{
    std::vector<float> out(static_cast<size_t>(blockSize));
    while (running.load(std::memory_order_relaxed))
    {
        auto const t0 = std::chrono::steady_clock::now();
        auto result = decBuffer.readSnapshot(out.data(), blockSize, 0);
        auto const t1 = std::chrono::steady_clock::now();
        long long const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        long long prev = stats.maxCallNs.load(std::memory_order_relaxed);
        while (ns > prev && !stats.maxCallNs.compare_exchange_weak(prev, ns, std::memory_order_relaxed))
        {
        }

        if (!result.has_value())
        {
            stats.snapshotNullopt.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        stats.snapshotOk.fetch_add(1, std::memory_order_relaxed);

        int const n = *result;
        if (n < 0 || n > blockSize)
        {
            stats.outOfRangeCount.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        for (int i = 0; i < n; ++i)
        {
            if (!std::isfinite(out[static_cast<size_t>(i)]))
            {
                stats.nonFiniteCount.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }
    }
}

} // namespace

TEST(AudioBufferSnapshotThreadSafety, DecimatingSnapshotBoundedAndNonBlocking)
{
    // DecimatingCaptureBuffer applies an FIR anti-aliasing filter to every
    // sample, so within-block sample equality is NOT a valid torn-read
    // probe (the filter smears adjacent values across block boundaries).
    //
    // Instead we verify the contract that actually matters for RT safety:
    //   1. readSnapshot returns in bounded wall-clock time under writer
    //      contention (no yield on torn read).
    //   2. The returned sample count (when non-nullopt) is always within
    //      [0, numSamples].
    //   3. The call never crashes, never reads freed memory.
    //   4. Every returned value is a finite float (not NaN/Inf from a
    //      torn read across filter state swaps).
    DecimatingCaptureBuffer decBuffer;
    std::atomic<bool> running{true};
    DecSnapshotStats stats;
    constexpr int kBlockSize = 128;

    std::thread writer([&]() { decSnapshotWriterLoop(decBuffer, running, stats, kBlockSize); });
    std::thread reader([&]() { decSnapshotReaderLoop(decBuffer, running, stats, kBlockSize); });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(stats.writerIters.load(), 100);
    EXPECT_GT(stats.snapshotOk.load(), 100);
    EXPECT_EQ(stats.outOfRangeCount.load(), 0);
    EXPECT_EQ(stats.nonFiniteCount.load(), 0);
    EXPECT_LT(stats.maxCallNs.load(), 5'000'000LL)
        << "readSnapshot took " << stats.maxCallNs.load() << "ns; likely yielded";
}

TEST(AudioBufferSnapshotThreadSafety, SnapshotReturnsValueWithoutContention)
{
    // Sanity: with no concurrent writer, readSnapshot must always
    // succeed (never nullopt).
    SharedCaptureBuffer buffer(4096);
    writeConstantBlock(buffer, 0.5f, 256);

    std::vector<float> out(256);
    auto result = buffer.readSnapshot(out.data(), 256, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 256);
    for (float v : out)
        EXPECT_FLOAT_EQ(v, 0.5f);
}
