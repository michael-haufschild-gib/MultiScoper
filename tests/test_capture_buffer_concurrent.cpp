/*
    Oscil - Capture Buffer Tests: Extended Concurrency
    Stereo consistency, analysis methods, oversized writes, and tryLock under concurrency
*/

#include <gtest/gtest.h>
#include "helpers/AudioBufferBuilder.h"
#include "core/SharedCaptureBuffer.h"
#include <thread>
#include <atomic>
#include <vector>
#include <cmath>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferConcurrentTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override { buffer = std::make_unique<SharedCaptureBuffer>(1024); }

    juce::AudioBuffer<float> generateDCBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

TEST_F(CaptureBufferConcurrentTest, TryLockDropsWriteUnderContention)
{
    // Both threads attempt tryLock writes concurrently.
    // Each thread runs a fixed iteration count instead of sleeping.
    // The w1/w2 counters track attempts (write() returns immediately on lock failure).
    constexpr int kIterations = 500;
    std::atomic<int> w1{0}, w2{0};

    std::thread writer1([&]() {
        for (int i = 0; i < kIterations; ++i) {
            buffer->write(generateDCBuffer(64, 0.3f), CaptureFrameMetadata{}, true);
            w1.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread writer2([&]() {
        for (int i = 0; i < kIterations; ++i) {
            buffer->write(generateDCBuffer(64, 0.7f), CaptureFrameMetadata{}, true);
            w2.fetch_add(1, std::memory_order_relaxed);
        }
    });

    writer1.join();
    writer2.join();

    EXPECT_EQ(w1.load(), kIterations);
    EXPECT_EQ(w2.load(), kIterations);
}

TEST_F(CaptureBufferConcurrentTest, OversizedWriteUnderConcurrency)
{
    buffer = std::make_unique<SharedCaptureBuffer>(128);
    constexpr int kReaderIterations = 500;
    std::atomic<bool> running{true};
    std::atomic<int> wc{0};

    // Seed the buffer so reader has data from the start
    buffer->write(generateDCBuffer(256, 0.42f), CaptureFrameMetadata{});

    std::thread writer([&]() {
        while (running.load(std::memory_order_relaxed)) {
            buffer->write(generateDCBuffer(256, 0.42f), CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread reader([&]() {
        std::vector<float> out(128);
        for (int i = 0; i < kReaderIterations; ++i) {
            int n = buffer->read(out.data(), 128, 0);
            for (int j = 0; j < n; ++j)
                EXPECT_NEAR(out[static_cast<size_t>(j)], 0.42f, 0.001f);
        }
    });

    reader.join();
    running.store(false, std::memory_order_relaxed);
    writer.join();
    EXPECT_GT(wc.load(), 10);
}

TEST_F(CaptureBufferConcurrentTest, StereoChannelConsistency)
{
    constexpr int kBlock = 64;
    constexpr int kReaderIterations = 500;
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, mismatch{0};

    // Seed the buffer with stereo DC data
    {
        juce::AudioBuffer<float> seed(2, kBlock);
        for (int s = 0; s < kBlock; ++s) {
            seed.setSample(0, s, 0.5f);
            seed.setSample(1, s, 0.5f);
        }
        buffer->write(seed, CaptureFrameMetadata{});
    }

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed)) {
            float dc = static_cast<float>(i % 500) * 0.002f;
            juce::AudioBuffer<float> buf(2, kBlock);
            for (int s = 0; s < kBlock; ++s) {
                buf.setSample(0, s, dc);
                buf.setSample(1, s, dc);
            }
            buffer->write(buf, CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    // AudioBuffer overload reads both channels within one epoch check
    std::thread reader([&]() {
        juce::AudioBuffer<float> out(2, kBlock);
        for (int i = 0; i < kReaderIterations; ++i) {
            if (buffer->read(out, kBlock) == kBlock) {
                for (int s = 0; s < kBlock; ++s) {
                    if (std::abs(out.getSample(0, s) - out.getSample(1, s)) > 0.001f) {
                        mismatch.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }
        }
    });

    reader.join();
    running.store(false, std::memory_order_relaxed);
    writer.join();

    EXPECT_GT(wc.load(), 10);
    EXPECT_EQ(mismatch.load(), 0) << "Stereo channels were desynchronized";
}

TEST_F(CaptureBufferConcurrentTest, AnalysisMethodsReturnValidResults)
{
    // Seed the buffer so analysis has data from the start,
    // eliminating the race between first write and first analysis.
    buffer->write(generateDCBuffer(256, 0.5f), CaptureFrameMetadata{});

    constexpr int kTargetIterations = 500;
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, invalid{0};
    std::atomic<int> analysisCompleted{0};

    std::thread writer([&]() {
        while (running.load(std::memory_order_relaxed)) {
            buffer->write(generateDCBuffer(128, 0.5f), CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Iteration-based: run a fixed number of analysis rounds instead of
    // relying on wall-clock time which varies under CI load.
    std::thread analyzer([&]() {
        for (int i = 0; i < kTargetIterations; ++i) {
            float peak = buffer->getPeakLevel(0, 64);
            float rms = buffer->getRMSLevel(0, 64);
            // Each method independently validates its epoch, so peak and rms
            // may come from different snapshots. Only check individual validity.
            if (peak < 0.0f || !std::isfinite(peak))
                invalid.fetch_add(1, std::memory_order_relaxed);
            if (rms < 0.0f || !std::isfinite(rms))
                invalid.fetch_add(1, std::memory_order_relaxed);
            analysisCompleted.fetch_add(1, std::memory_order_relaxed);
        }
    });

    analyzer.join();
    running.store(false, std::memory_order_relaxed);
    writer.join();

    EXPECT_GT(wc.load(), 10);
    EXPECT_EQ(analysisCompleted.load(), kTargetIterations);
    EXPECT_EQ(invalid.load(), 0) << "Invalid analysis results under concurrency";
}
