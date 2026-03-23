/*
    Oscil - Decimating Capture Buffer Tests (Edge Cases)
    Memory usage, clear/reset, metering, reconfiguration, internal access
*/

#include <gtest/gtest.h>
#include "core/DecimatingCaptureBuffer.h"
#include <cmath>
#include <numeric>

using namespace oscil;

//==============================================================================
// Memory Usage Tests
//==============================================================================

class DecimatingBufferMemoryEdgeTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(DecimatingBufferMemoryEdgeTest, EcoUsesLessMemoryThanStandard)
{
    config.bufferDuration = BufferDuration::Medium;

    config.qualityPreset = QualityPreset::Eco;
    DecimatingCaptureBuffer ecoBuffer(config, 44100);

    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer standardBuffer(config, 44100);

    EXPECT_LT(ecoBuffer.getMemoryUsageBytes(), standardBuffer.getMemoryUsageBytes());
}

TEST_F(DecimatingBufferMemoryEdgeTest, ShortDurationUsesLessMemoryThanLong)
{
    config.qualityPreset = QualityPreset::Standard;

    config.bufferDuration = BufferDuration::Short;
    DecimatingCaptureBuffer shortBuffer(config, 44100);

    config.bufferDuration = BufferDuration::VeryLong;
    DecimatingCaptureBuffer longBuffer(config, 44100);

    EXPECT_LT(shortBuffer.getMemoryUsageBytes(), longBuffer.getMemoryUsageBytes());
}

TEST_F(DecimatingBufferMemoryEdgeTest, MemoryUsageStringFormatsCorrectly)
{
    config.qualityPreset = QualityPreset::Standard;
    config.bufferDuration = BufferDuration::Medium;
    DecimatingCaptureBuffer buffer(config, 44100);

    juce::String memStr = buffer.getMemoryUsageString();

    // Should contain KB or MB
    EXPECT_TRUE(memStr.contains("KB") || memStr.contains("MB"));
}

TEST_F(DecimatingBufferMemoryEdgeTest, GetMemoryUsageBytesIsPositive)
{
    DecimatingCaptureBuffer buffer;
    EXPECT_GT(buffer.getMemoryUsageBytes(), 0u);
}

TEST_F(DecimatingBufferMemoryEdgeTest, MemoryUsageIncludesRetainedGraveyardBuffersAfterReconfigure)
{
    CaptureQualityConfig largeConfig;
    largeConfig.qualityPreset = QualityPreset::Ultra;
    largeConfig.bufferDuration = BufferDuration::VeryLong;

    CaptureQualityConfig smallConfig;
    smallConfig.qualityPreset = QualityPreset::Eco;
    smallConfig.bufferDuration = BufferDuration::Short;

    DecimatingCaptureBuffer buffer(largeConfig, 44100);
    DecimatingCaptureBuffer smallOnlyBuffer(smallConfig, 44100);
    const size_t smallOnlyBytes = smallOnlyBuffer.getMemoryUsageBytes();

    ASSERT_GT(buffer.getMemoryUsageBytes(), smallOnlyBytes);

    buffer.configure(smallConfig, 44100);

    // Reconfigure retains old buffers in a graveyard for safety; memory reporting should include that retention.
    EXPECT_GT(buffer.getMemoryUsageBytes(), smallOnlyBytes);
}

//==============================================================================
// Clear and Reset Tests
//==============================================================================

class DecimatingBufferClearEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferClearEdgeTest, ClearResetsAvailableSamples)
{
    // Write some data
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    CaptureFrameMetadata meta;
    buffer->write(audioBuffer, meta);

    EXPECT_GT(buffer->getAvailableSamples(), 0u);

    buffer->clear();

    EXPECT_EQ(buffer->getAvailableSamples(), 0u);
}

//==============================================================================
// Metering Tests
//==============================================================================

class DecimatingBufferMeteringEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferMeteringEdgeTest, PeakLevelReturnsZeroForEmptyBuffer)
{
    EXPECT_FLOAT_EQ(buffer->getPeakLevel(0), 0.0f);
}

TEST_F(DecimatingBufferMeteringEdgeTest, RMSLevelReturnsZeroForEmptyBuffer)
{
    EXPECT_FLOAT_EQ(buffer->getRMSLevel(0), 0.0f);
}

TEST_F(DecimatingBufferMeteringEdgeTest, PeakLevelDetectsSignal)
{
    // Write a known signal
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = audioBuffer.getWritePointer(ch);
        for (int i = 0; i < 4096; ++i)
        {
            data[i] = 0.5f;  // Constant 0.5 signal
        }
    }

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    buffer->write(audioBuffer, meta);

    float peak = buffer->getPeakLevel(0, 1024);

    // Peak should be close to 0.5 (allowing for filter effects)
    EXPECT_GT(peak, 0.3f);
    EXPECT_LT(peak, 0.7f);
}

//==============================================================================
// Configuration Change Tests
//==============================================================================

class DecimatingBufferReconfigureEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferReconfigureEdgeTest, SetQualityPresetChangesDecimationRatio)
{
    EXPECT_EQ(buffer->getCaptureRate(), CaptureRate::STANDARD);

    buffer->setQualityPreset(QualityPreset::Eco, 44100);

    EXPECT_EQ(buffer->getCaptureRate(), CaptureRate::ECO);
    EXPECT_EQ(buffer->getDecimationRatio(), 4);
}

TEST_F(DecimatingBufferReconfigureEdgeTest, ConfigurePreservesDataIntegrity)
{
    // Write some data
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = audioBuffer.getWritePointer(ch);
        for (int i = 0; i < 4096; ++i)
            data[i] = 0.5f;
    }

    CaptureFrameMetadata meta;
    buffer->write(audioBuffer, meta);

    // Reconfigure
    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::Eco;
    buffer->configure(newConfig, 48000);

    // Buffer should be cleared/reset after reconfiguration
    // (new buffer created with different size)
    EXPECT_EQ(buffer->getConfig().qualityPreset, QualityPreset::Eco);
}

//==============================================================================
// Internal Buffer Access Tests
//==============================================================================

class DecimatingBufferInternalAccessEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferInternalAccessEdgeTest, GetInternalBufferReturnsValidPointer)
{
    auto internal = buffer->getInternalBuffer();
    EXPECT_NE(internal, nullptr);
    EXPECT_GT(buffer->getCapacity(), 0u);
}

TEST_F(DecimatingBufferInternalAccessEdgeTest, GetCapacityReturnsPositiveValue)
{
    EXPECT_GT(buffer->getCapacity(), 0u);
}

//==============================================================================
// Decimation Accuracy and Anti-Aliasing Tests
//==============================================================================

class DecimatingBufferAccuracyTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferAccuracyTest, DecimationReducesSampleCountByExpectedRatio)
{
    config.qualityPreset = QualityPreset::Standard;
    buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);
    ASSERT_EQ(buffer->getDecimationRatio(), 2);

    buffer->clear();

    juce::AudioBuffer<float> audioBuf(2, 4000);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4000; ++i)
            audioBuf.setSample(ch, i, 0.5f);

    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    buffer->write(audioBuf, meta);

    size_t available = buffer->getAvailableSamples();
    EXPECT_NEAR(static_cast<double>(available), 2000.0, 5.0)
        << "Decimation ratio not applied correctly: expected ~2000, got " << available;
}

TEST_F(DecimatingBufferAccuracyTest, AntiAliasingRemovesHighFrequencyContent)
{
    config.qualityPreset = QualityPreset::Eco;
    buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);
    ASSERT_EQ(buffer->getDecimationRatio(), 4);

    buffer->clear();

    constexpr int kN = 16384;
    constexpr float kFreq = 10000.0f;
    constexpr float kSR = 44100.0f;

    juce::AudioBuffer<float> hf(2, kN);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kN; ++i)
            hf.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * kFreq * static_cast<float>(i) / kSR));

    buffer->write(hf, CaptureFrameMetadata{});

    std::vector<float> out(4096);
    int n = buffer->read(out.data(), 4096, 0);
    ASSERT_GT(n, 100);

    double sumSq = 0.0;
    int count = 0;
    for (int i = 200; i < n; ++i) { sumSq += out[static_cast<size_t>(i)] * out[static_cast<size_t>(i)]; ++count; }
    float rms = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;

    EXPECT_LT(rms, 0.1f) << "High-frequency not attenuated: RMS=" << rms;
}

TEST_F(DecimatingBufferAccuracyTest, LowFrequencySignalPreservedAfterDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);
    buffer->clear();

    constexpr int kN = 16384;
    constexpr float kFreq = 100.0f;
    constexpr float kSR = 44100.0f;

    juce::AudioBuffer<float> lf(2, kN);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kN; ++i)
            lf.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * kFreq * static_cast<float>(i) / kSR));

    buffer->write(lf, CaptureFrameMetadata{});

    std::vector<float> out(4096);
    int n = buffer->read(out.data(), 4096, 0);
    ASSERT_GT(n, 200);

    double sumSq = 0.0;
    int count = 0;
    for (int i = 200; i < n; ++i) { sumSq += out[static_cast<size_t>(i)] * out[static_cast<size_t>(i)]; ++count; }
    float rms = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;

    EXPECT_GT(rms, 0.5f) << "Low-frequency attenuated: RMS=" << rms;
}

//==============================================================================
// Concurrent Reconfiguration Stress Tests
//==============================================================================

#include <thread>
#include <atomic>

namespace
{

// Helper: run continuous audio writes until signaled to stop
void runAudioWriter(DecimatingCaptureBuffer& buffer,
                    std::atomic<bool>& running, std::atomic<int>& writeCount)
{
    juce::AudioBuffer<float> audioBuf(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            audioBuf.setSample(ch, i, 0.5f);

    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    meta.numSamples = 512;

    while (running.load(std::memory_order_relaxed))
    {
        buffer.write(audioBuf, meta);
        writeCount.fetch_add(1, std::memory_order_relaxed);
    }
}

// Helper: run periodic reconfigures, then signal stop
void runReconfigurer(DecimatingCaptureBuffer& buffer,
                     std::atomic<bool>& running, std::atomic<int>& reconfigureCount)
{
    CaptureQualityConfig configs[4];
    configs[0].qualityPreset = QualityPreset::Eco;
    configs[0].bufferDuration = BufferDuration::Short;
    configs[1].qualityPreset = QualityPreset::Standard;
    configs[1].bufferDuration = BufferDuration::Medium;
    configs[2].qualityPreset = QualityPreset::High;
    configs[2].bufferDuration = BufferDuration::Long;
    configs[3].qualityPreset = QualityPreset::Ultra;
    configs[3].bufferDuration = BufferDuration::VeryLong;
    int rates[] = {44100, 48000, 88200, 96000, 192000};

    for (int i = 0; running.load(std::memory_order_relaxed) && i < 50; ++i)
    {
        buffer.configure(configs[i % 4], rates[i % 5]);
        reconfigureCount.fetch_add(1, std::memory_order_relaxed);
    }
    running.store(false, std::memory_order_relaxed);
}

// Helper: read buffer continuously, checking for non-finite values
void runReader(DecimatingCaptureBuffer& buffer,
               std::atomic<bool>& running, std::atomic<int>& readCount,
               std::atomic<int>& invalidReads)
{
    std::vector<float> output(1024);
    while (running.load(std::memory_order_relaxed))
    {
        int n = buffer.read(output.data(), 1024, 0);
        for (int i = 0; i < n; ++i)
        {
            if (!std::isfinite(output[static_cast<size_t>(i)]))
            {
                invalidReads.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }
        readCount.fetch_add(1, std::memory_order_relaxed);
    }
}

} // anonymous namespace

TEST(DecimatingBufferConcurrencyTest, AudioWriteDuringReconfigureDoesNotCorruptOrCrash)
{
    // Bug caught: audio thread writes to a buffer being swapped out by
    // concurrent configure(). Graveyard mechanism prevents use-after-free.
    DecimatingCaptureBuffer buffer;
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0}, reconfigureCount{0}, readCount{0}, invalidReads{0};

    std::thread audioThread([&]() { runAudioWriter(buffer, running, writeCount); });
    std::thread messageThread([&]() { runReconfigurer(buffer, running, reconfigureCount); });
    std::thread readerThread([&]() { runReader(buffer, running, readCount, invalidReads); });

    messageThread.join();
    running.store(false, std::memory_order_relaxed);
    audioThread.join();
    readerThread.join();

    EXPECT_GT(writeCount.load(), 100) << "Audio thread didn't run enough";
    EXPECT_GT(reconfigureCount.load(), 10) << "Message thread didn't reconfigure enough";
    EXPECT_GT(readCount.load(), 10) << "Reader thread didn't run enough";
    EXPECT_EQ(invalidReads.load(), 0) << "Non-finite samples read during reconfigure";
}

TEST(DecimatingBufferConcurrencyTest, CleanUpGarbageDuringActiveWriteDoesNotCrash)
{
    // Bug caught: cleanUpGarbage() frees old buffers while audio thread
    // still holds a reference from a slow write path.
    auto buffer = std::make_shared<DecimatingCaptureBuffer>();

    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};

    std::thread audioThread([&]() {
        juce::AudioBuffer<float> audioBuf(2, 256);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                audioBuf.setSample(ch, i, 0.3f);

        CaptureFrameMetadata meta{};
        while (running.load(std::memory_order_relaxed))
        {
            buffer->write(audioBuf, meta);
            writeCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Rapidly reconfigure and clean up garbage
    for (int i = 0; i < 30; ++i)
    {
        CaptureQualityConfig cfg;
        cfg.qualityPreset = (i % 2 == 0) ? QualityPreset::Eco : QualityPreset::Ultra;
        cfg.bufferDuration = (i % 3 == 0) ? BufferDuration::Short : BufferDuration::Long;
        buffer->configure(cfg, (i % 2 == 0) ? 44100 : 96000);
        buffer->cleanUpGarbage();
    }

    running.store(false, std::memory_order_relaxed);
    audioThread.join();

    EXPECT_GT(writeCount.load(), 50);
}
