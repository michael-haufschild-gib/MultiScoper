/*
    Oscil - Decimating Capture Buffer Concurrent Tests
    Thread safety of the DecimatingCaptureBuffer under concurrent write + read
    and concurrent write + reconfigure scenarios.

    Bug targets:
    - Torn read when reconfigure() swaps buffer_ while write() is in progress
    - Use-after-free when graveyard cleanup races with audio thread read
    - Data corruption when decimation ratio changes mid-write
    - SpinLock tryLock path drops frames silently without corrupting state
    - Stale context_ pointer used after reconfigure() replaces it
*/

#include "core/DecimatingCaptureBuffer.h"

#include "helpers/AudioBufferBuilder.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>

using namespace oscil;
using namespace oscil::test;

class DecimatingCaptureBufferConcurrentTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;

    juce::AudioBuffer<float> generateDCBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

// ============================================================================
// Concurrent write + read: data read back must be internally consistent
// (all samples in a DC block should be the same value within tolerance)
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, ConcurrentWriteReadDataIntegrity)
{
    // Use passthrough (ratio 1) so DC values are preserved exactly
    config.qualityPreset = QualityPreset::Ultra;
    DecimatingCaptureBuffer buffer(config, 44100);

    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, rc{0}, torn{0};
    constexpr int kBlock = 64;

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed))
        {
            float dc = static_cast<float>(i % 1000) * 0.001f;
            auto buf = generateDCBuffer(kBlock, dc);
            CaptureFrameMetadata meta{};
            meta.sampleRate = 44100.0;
            meta.numChannels = 2;
            meta.numSamples = kBlock;
            buffer.write(buf, meta);
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    std::thread reader([&]() {
        std::vector<float> out(kBlock);
        while (running.load(std::memory_order_relaxed))
        {
            int n = buffer.read(out.data(), kBlock, 0);
            if (n == kBlock)
            {
                float first = out[0];
                for (int i = 1; i < kBlock; ++i)
                {
                    if (std::abs(out[i] - first) > 0.002f)
                    {
                        torn.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                }
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
    EXPECT_EQ(torn.load(), 0) << "Torn reads: " << torn.load() << "/" << rc.load();
}

// ============================================================================
// Concurrent write + read with active decimation (ratio > 1)
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, ConcurrentWriteReadWithDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    DecimatingCaptureBuffer buffer(config, 44100);

    // With decimation, DC values get filtered, but the filter converges
    // to the DC level. We just verify no crash and metadata consistency.
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, rc{0}, metaBad{0};

    std::thread writer([&]() {
        for (int i = 1; running.load(std::memory_order_relaxed); ++i)
        {
            CaptureFrameMetadata meta{};
            meta.sampleRate = 44100.0;
            meta.numChannels = 2;
            meta.numSamples = 256;
            meta.bpm = static_cast<double>(i);
            meta.timestamp = static_cast<int64_t>(i) * 100;
            auto buf = generateDCBuffer(256, 0.5f);
            buffer.write(buf, meta);
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::thread reader([&]() {
        while (running.load(std::memory_order_relaxed))
        {
            auto m = buffer.getLatestMetadata();
            // If metadata has been written at least once, verify consistency
            if (m.bpm >= 1.0 && m.timestamp > 0)
            {
                // Timestamp should be correlated with bpm (both derived from i)
                // But timestamp is scaled by capture rate, so just verify finite
                if (!std::isfinite(m.bpm) || !std::isfinite(m.sampleRate))
                    metaBad.fetch_add(1, std::memory_order_relaxed);
            }
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    reader.join();

    EXPECT_GT(wc.load(), 50);
    EXPECT_GT(rc.load(), 50);
    EXPECT_EQ(metaBad.load(), 0);
}

// ============================================================================
// Write continues safely when reconfigure() is called concurrently
// The audio thread write() uses tryLock, so it drops frames when
// reconfigure() holds the lock. This test verifies:
// 1. No crash or use-after-free
// 2. Buffer remains functional after reconfiguration
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, ReconfigureDuringConcurrentWriteDoesNotCorrupt)
{
    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer buffer(config, 44100);

    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, reconfigCount{0};

    // Writer thread simulates audio thread
    std::thread writer([&]() {
        while (running.load(std::memory_order_relaxed))
        {
            auto buf = generateDCBuffer(256, 0.5f);
            CaptureFrameMetadata meta{};
            meta.sampleRate = 44100.0;
            meta.numChannels = 2;
            buffer.write(buf, meta);
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Main thread reconfigures repeatedly (message thread operation)
    QualityPreset presets[] = {QualityPreset::Eco, QualityPreset::Standard, QualityPreset::High, QualityPreset::Ultra};

    for (int i = 0; i < 20; ++i)
    {
        auto preset = presets[i % 4];
        config.qualityPreset = preset;
        buffer.configure(config, 44100);
        reconfigCount.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    running.store(false, std::memory_order_relaxed);
    writer.join();

    EXPECT_GT(wc.load(), 50) << "Writer should have processed many frames";
    EXPECT_EQ(reconfigCount.load(), 20);

    // Buffer must still be functional after all reconfigurations
    auto buf = generateDCBuffer(256, 0.3f);
    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    buffer.write(buf, meta);

    EXPECT_GT(buffer.getAvailableSamples(), 0u);
}

// ============================================================================
// Read during reconfigure: reader must never see partial buffer state
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, ReadDuringReconfigureDoesNotCrash)
{
    // Bug caught: read() using a dangling shared_ptr to old buffer after
    // reconfigure() swaps it out, causing use-after-free.
    //
    // Note: After reconfigure, reads may return partial/empty data because
    // the new buffer starts empty. This test verifies no crash or segfault,
    // not data consistency across reconfigurations.
    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer buffer(config, 44100);

    auto initialBuf = generateDCBuffer(1024, 0.7f);
    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    buffer.write(initialBuf, meta);

    std::atomic<bool> running{true};
    std::atomic<int> rc{0};

    std::thread reader([&]() {
        std::vector<float> out(256);
        while (running.load(std::memory_order_relaxed))
        {
            // These calls must not crash, even during reconfigure
            (void) buffer.read(out.data(), 256, 0);
            (void) buffer.getLatestMetadata();
            (void) buffer.getAvailableSamples();
            (void) buffer.getCapacity();
            rc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for (int i = 0; i < 20; ++i)
    {
        config.qualityPreset = (i % 2 == 0) ? QualityPreset::Eco : QualityPreset::Standard;
        buffer.configure(config, 44100);

        auto refillBuf = generateDCBuffer(1024, 0.7f);
        buffer.write(refillBuf, meta);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    running.store(false, std::memory_order_relaxed);
    reader.join();

    EXPECT_GT(rc.load(), 100) << "Reader should have completed many iterations";
}

// ============================================================================
// Multiple readers concurrent with writer (multi-pane scenario)
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, MultipleReadersWithConcurrentWriter)
{
    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer buffer(config, 44100);

    constexpr int kReaders = 4;
    std::atomic<bool> running{true};
    std::atomic<int> wc{0}, totalRc{0};

    std::thread writer([&]() {
        int i = 0;
        while (running.load(std::memory_order_relaxed))
        {
            auto buf = generateDCBuffer(128, static_cast<float>(i % 100) * 0.01f);
            CaptureFrameMetadata meta{};
            meta.sampleRate = 44100.0;
            meta.numChannels = 2;
            buffer.write(buf, meta);
            wc.fetch_add(1, std::memory_order_relaxed);
            ++i;
        }
    });

    std::vector<std::thread> readers;
    for (int r = 0; r < kReaders; ++r)
    {
        readers.emplace_back([&]() {
            std::vector<float> out(128);
            while (running.load(std::memory_order_relaxed))
            {
                (void) buffer.read(out.data(), 128, 0);
                (void) buffer.read(out.data(), 128, 1);
                (void) buffer.getLatestMetadata();
                (void) buffer.getPeakLevel(0, 128);
                (void) buffer.getRMSLevel(0, 128);
                totalRc.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running.store(false, std::memory_order_relaxed);
    writer.join();
    for (auto& t : readers)
        t.join();

    EXPECT_GT(wc.load(), 100);
    EXPECT_GT(totalRc.load(), 100);
}

// ============================================================================
// Clear racing with concurrent writes: must not leave buffer in corrupt state
// ============================================================================

TEST_F(DecimatingCaptureBufferConcurrentTest, ClearDuringConcurrentWriteIsSafe)
{
    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer buffer(config, 44100);

    std::atomic<bool> running{true};
    std::atomic<int> wc{0};

    std::thread writer([&]() {
        while (running.load(std::memory_order_relaxed))
        {
            auto buf = generateDCBuffer(128, 0.5f);
            CaptureFrameMetadata meta{};
            meta.sampleRate = 44100.0;
            meta.numChannels = 2;
            buffer.write(buf, meta);
            wc.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for (int i = 0; i < 30; ++i)
    {
        buffer.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    running.store(false, std::memory_order_relaxed);
    writer.join();

    EXPECT_GT(wc.load(), 50);

    // Buffer must be functional after clear storm
    auto buf = generateDCBuffer(256, 0.3f);
    CaptureFrameMetadata meta{};
    buffer.write(buf, meta);
    EXPECT_GT(buffer.getAvailableSamples(), 0u);
}
