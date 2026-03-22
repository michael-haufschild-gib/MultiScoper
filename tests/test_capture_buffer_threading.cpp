/*
    Oscil - Capture Buffer Tests: Threading
    Core thread safety: torn reads, data integrity, ring wrap, metadata consistency.
    Extended concurrency tests in test_capture_buffer_concurrent.cpp.
*/

#include <gtest/gtest.h>
#include "helpers/AudioBufferBuilder.h"
#include "core/SharedCaptureBuffer.h"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cmath>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferThreadingTest : public ::testing::Test
{
protected:
    static constexpr size_t kCapacity = 1024;
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override { buffer = std::make_unique<SharedCaptureBuffer>(kCapacity); }

    juce::AudioBuffer<float> generateDCBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

// ============================================================================
// Concurrent write + read — verify data integrity, not just "no crash"
// ============================================================================

TEST_F(CaptureBufferThreadingTest, ConcurrentWriteReadDataIntegrity)
{
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, rc{0}, torn{0};
    constexpr int kBlock = 64;

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed)) {
            float dc = static_cast<float>(i % 1000) * 0.001f;
            buffer->write(generateDCBuffer(kBlock, dc), CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    std::thread reader([&]() {
        std::vector<float> out(kBlock);
        while (running.load(std::memory_order_relaxed)) {
            if (buffer->read(out.data(), kBlock, 0) == kBlock) {
                float first = out[0];
                for (int i = 1; i < kBlock; ++i) {
                    if (std::abs(out[i] - first) > 0.0001f) {
                        torn.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(wc.load(), 100);
    EXPECT_GT(rc.load(), 100);
    EXPECT_EQ(torn.load(), 0) << "Torn reads: " << torn.load() << "/" << rc.load();
}

// ============================================================================
// Multiple readers concurrently — epoch check under contention
// ============================================================================

TEST_F(CaptureBufferThreadingTest, MultiReaderConcurrentAccess)
{
    constexpr int kReaders = 4, kBlock = 64;
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, totalRc{0}, torn{0};

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed)) {
            float dc = static_cast<float>(i % 500) * 0.002f;
            buffer->write(generateDCBuffer(kBlock, dc), CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    std::vector<std::thread> readers;
    for (int r = 0; r < kReaders; ++r) {
        readers.emplace_back([&]() {
            std::vector<float> out(kBlock);
            while (running.load(std::memory_order_relaxed)) {
                if (buffer->read(out.data(), kBlock, 0) == kBlock) {
                    float first = out[0];
                    for (int i = 1; i < kBlock; ++i)
                        if (std::abs(out[i] - first) > 0.0001f) {
                            torn.fetch_add(1, std::memory_order_relaxed);
                            break;
                        }
                }
                totalRc.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    for (auto& t : readers) t.join();

    EXPECT_GT(wc.load(), 100);
    EXPECT_GT(totalRc.load(), 100);
    EXPECT_EQ(torn.load(), 0) << "Multi-reader torn: " << torn.load() << "/" << totalRc.load();
}

// ============================================================================
// Metadata consistency (SeqLock) under rapid writes
// ============================================================================

TEST_F(CaptureBufferThreadingTest, MetadataConsistencyUnderConcurrency)
{
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, rc{0}, bad{0};

    std::thread writer([&]() {
        for (int i = 1; running.load(std::memory_order_relaxed); ++i) {
            CaptureFrameMetadata m{};
            m.sampleRate = 1000.0 * i;
            m.bpm = static_cast<double>(i);
            m.timestamp = static_cast<int64_t>(i) * 100;
            m.numChannels = 2;
            m.isPlaying = (i % 2 == 0);
            buffer->write(generateDCBuffer(64, 0.5f), m);
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::thread reader([&]() {
        while (running.load(std::memory_order_relaxed)) {
            auto m = buffer->getLatestMetadata();
            if (m.bpm >= 1.0 && m.bpm <= 1e7 && m.timestamp > 0) {
                bool ok = std::abs(m.sampleRate - 1000.0 * m.bpm) < 0.01
                       && m.timestamp == static_cast<int64_t>(m.bpm) * 100
                       && m.isPlaying == (static_cast<int>(m.bpm) % 2 == 0);
                if (!ok) bad.fetch_add(1, std::memory_order_relaxed);
            }
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(wc.load(), 100);
    EXPECT_GT(rc.load(), 100);
    EXPECT_EQ(bad.load(), 0) << "Metadata torn: " << bad.load() << "/" << rc.load();
}

// ============================================================================
// clear() racing with concurrent reads
// ============================================================================

TEST_F(CaptureBufferThreadingTest, ClearDuringConcurrentReads)
{
    buffer->write(generateDCBuffer(static_cast<int>(kCapacity), 0.75f), CaptureFrameMetadata{});
    std::atomic<bool> running{true};
    std::atomic<int> rc{0}, mixed{0};

    std::thread reader([&]() {
        std::vector<float> out(128);
        while (running.load(std::memory_order_relaxed)) {
            int n = buffer->read(out.data(), 128, 0);
            if (n > 0) {
                bool hasOrig = false, hasZero = false;
                for (int i = 0; i < n; ++i) {
                    if (std::abs(out[i] - 0.75f) < 0.001f) hasOrig = true;
                    else if (std::abs(out[i]) < 0.001f) hasZero = true;
                }
                if (hasOrig && hasZero) mixed.fetch_add(1, std::memory_order_relaxed);
            }
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for (int c = 0; c < 50; ++c) {
        buffer->clear();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        buffer->write(generateDCBuffer(static_cast<int>(kCapacity), 0.75f), CaptureFrameMetadata{});
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    running.store(false, std::memory_order_relaxed);
    reader.join();

    EXPECT_GT(rc.load(), 10);
    EXPECT_EQ(mixed.load(), 0) << "Clear-during-read mixed: " << mixed.load() << "/" << rc.load();
}

// ============================================================================
// Ring wrap-around data integrity under concurrency (small buffer)
// ============================================================================

TEST_F(CaptureBufferThreadingTest, RingWrapDataIntegrity)
{
    buffer = std::make_unique<SharedCaptureBuffer>(256);
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, rc{0}, err{0};
    constexpr int kBlock = 100; // not power-of-2 → split writes

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed)) {
            float dc = static_cast<float>(i % 200) * 0.005f;
            buffer->write(generateDCBuffer(kBlock, dc), CaptureFrameMetadata{});
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    std::thread reader([&]() {
        std::vector<float> out(kBlock);
        while (running.load(std::memory_order_relaxed)) {
            if (buffer->read(out.data(), kBlock, 0) == kBlock) {
                float first = out[0];
                for (int i = 1; i < kBlock; ++i)
                    if (std::abs(out[i] - first) > 0.0001f) { err.fetch_add(1, std::memory_order_relaxed); break; }
            }
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(wc.load(), 200);
    EXPECT_GT(rc.load(), 100);
    EXPECT_EQ(err.load(), 0) << "Ring wrap corruption: " << err.load() << "/" << rc.load();
}
