/*
    Oscil - Shared Capture Buffer Tests
*/

#include <gtest/gtest.h>
#include "core/SharedCaptureBuffer.h"
#include <thread>
#include <atomic>

using namespace oscil;

class SharedCaptureBufferTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override
    {
        buffer = std::make_unique<SharedCaptureBuffer>(1024);
    }

    // Generate test audio buffer
    juce::AudioBuffer<float> generateTestBuffer(int numSamples, float value)
    {
        juce::AudioBuffer<float> buf(2, numSamples);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                buf.setSample(ch, i, value + ch * 0.1f);
            }
        }
        return buf;
    }
};

// Test: Basic write and read
TEST_F(SharedCaptureBufferTest, BasicWriteRead)
{
    auto testBuffer = generateTestBuffer(100, 0.5f);

    CaptureFrameMetadata metadata;
    metadata.sampleRate = 44100.0;
    metadata.numChannels = 2;

    buffer->write(testBuffer, metadata);

    std::vector<float> output(100);
    int samplesRead = buffer->read(output.data(), 100, 0);

    EXPECT_EQ(samplesRead, 100);
    EXPECT_NEAR(output[0], 0.5f, 0.001f);
}

// Test: Ring buffer wrapping
TEST_F(SharedCaptureBufferTest, RingBufferWrapping)
{
    // Write more than buffer capacity to test wrapping
    for (int i = 0; i < 20; ++i)
    {
        auto testBuffer = generateTestBuffer(100, static_cast<float>(i) * 0.05f);

        CaptureFrameMetadata metadata;
        metadata.sampleRate = 44100.0;

        buffer->write(testBuffer, metadata);
    }

    // Should still be able to read most recent samples
    std::vector<float> output(100);
    int samplesRead = buffer->read(output.data(), 100, 0);

    EXPECT_GT(samplesRead, 0);
}

// Test: Get available samples
TEST_F(SharedCaptureBufferTest, GetAvailableSamples)
{
    EXPECT_EQ(buffer->getAvailableSamples(), 0);

    auto testBuffer = generateTestBuffer(256, 0.5f);
    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    EXPECT_EQ(buffer->getAvailableSamples(), 256);

    // Write more
    buffer->write(testBuffer, metadata);
    EXPECT_EQ(buffer->getAvailableSamples(), 512);
}

// Test: Clear buffer
TEST_F(SharedCaptureBufferTest, ClearBuffer)
{
    auto testBuffer = generateTestBuffer(256, 0.5f);
    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    EXPECT_GT(buffer->getAvailableSamples(), 0);

    buffer->clear();

    EXPECT_EQ(buffer->getAvailableSamples(), 0);
}

// Test: Peak level calculation
TEST_F(SharedCaptureBufferTest, PeakLevel)
{
    juce::AudioBuffer<float> testBuffer(2, 100);
    testBuffer.clear();
    testBuffer.setSample(0, 50, 0.8f);  // Peak in left channel

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    float peak = buffer->getPeakLevel(0, 100);
    EXPECT_NEAR(peak, 0.8f, 0.001f);
}

// Test: RMS level calculation
TEST_F(SharedCaptureBufferTest, RMSLevel)
{
    juce::AudioBuffer<float> testBuffer(2, 100);
    for (int i = 0; i < 100; ++i)
    {
        testBuffer.setSample(0, i, 0.5f);
    }

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    float rms = buffer->getRMSLevel(0, 100);
    EXPECT_NEAR(rms, 0.5f, 0.001f);
}

// Test: Metadata persistence
TEST_F(SharedCaptureBufferTest, MetadataPersistence)
{
    auto testBuffer = generateTestBuffer(100, 0.5f);

    CaptureFrameMetadata metadata;
    metadata.sampleRate = 96000.0;
    metadata.numChannels = 2;
    metadata.bpm = 140.0;
    metadata.isPlaying = true;
    metadata.timestamp = 12345;

    buffer->write(testBuffer, metadata);

    auto retrieved = buffer->getLatestMetadata();

    EXPECT_EQ(retrieved.sampleRate, 96000.0);
    EXPECT_EQ(retrieved.numChannels, 2);
    EXPECT_EQ(retrieved.bpm, 140.0);
    EXPECT_TRUE(retrieved.isPlaying);
    EXPECT_EQ(retrieved.timestamp, 12345);
}

// Test: Stereo channel separation
TEST_F(SharedCaptureBufferTest, StereoChannelSeparation)
{
    juce::AudioBuffer<float> testBuffer(2, 100);
    for (int i = 0; i < 100; ++i)
    {
        testBuffer.setSample(0, i, 0.3f);  // Left
        testBuffer.setSample(1, i, 0.7f);  // Right
    }

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    std::vector<float> leftOutput(100);
    std::vector<float> rightOutput(100);

    buffer->read(leftOutput.data(), 100, 0);
    buffer->read(rightOutput.data(), 100, 1);

    EXPECT_NEAR(leftOutput[50], 0.3f, 0.001f);
    EXPECT_NEAR(rightOutput[50], 0.7f, 0.001f);
}

// Test: Thread safety - concurrent write and read
TEST_F(SharedCaptureBufferTest, ThreadSafetyConcurrentAccess)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

    // Writer thread (simulating audio thread)
    std::thread writer([this, &running, &writeCount]()
    {
        while (running)
        {
            auto testBuffer = generateTestBuffer(64, 0.5f);
            CaptureFrameMetadata metadata;
            buffer->write(testBuffer, metadata);
            writeCount++;
        }
    });

    // Reader thread (simulating UI thread)
    std::thread reader([this, &running, &readCount]()
    {
        std::vector<float> output(64);
        while (running)
        {
            buffer->read(output.data(), 64, 0);
            readCount++;
        }
    });

    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    reader.join();

    // Should have completed multiple operations without crashes
    EXPECT_GT(writeCount.load(), 0);
    EXPECT_GT(readCount.load(), 0);
}

// Test: Buffer capacity
TEST_F(SharedCaptureBufferTest, BufferCapacity)
{
    EXPECT_EQ(buffer->getCapacity(), 1024);

    auto largeBuffer = std::make_unique<SharedCaptureBuffer>(8192);
    EXPECT_EQ(largeBuffer->getCapacity(), 8192);
}

// Test: SeqLock metadata consistency under concurrent access
// Verifies that metadata reads are always consistent (no torn reads)
TEST_F(SharedCaptureBufferTest, SeqLockMetadataConsistency)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};
    std::atomic<int> inconsistentCount{0};

    // Writer thread (simulating audio thread) - writes with related values
    // Each write has sampleRate = 1000 * i, bpm = i, timestamp = i * 100
    // If reads are consistent, these relationships must hold
    std::thread writer([this, &running, &writeCount]()
    {
        int i = 1;
        while (running)
        {
            auto testBuffer = generateTestBuffer(64, 0.5f);
            CaptureFrameMetadata metadata;
            metadata.sampleRate = 1000.0 * i;
            metadata.bpm = static_cast<double>(i);
            metadata.timestamp = i * 100;
            metadata.numChannels = 2;
            metadata.isPlaying = (i % 2 == 0);

            buffer->write(testBuffer, metadata);
            writeCount++;
            i++;

            // Small delay to allow reads and simulate realistic audio callback timing
            // Without this, the writer spams updates faster than the reader can complete a SeqLock loop
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Reader thread (simulating UI thread) - verifies consistency
    std::thread reader([this, &running, &readCount, &inconsistentCount]()
    {
        while (running)
        {
            auto meta = buffer->getLatestMetadata();

            // Check consistency: sampleRate should be 1000 * bpm
            // and timestamp should be bpm * 100
            // Only check if bpm >= 1.0 (writer starts at i=1, so bpm starts at 1.0)
            // This excludes the initial default state (bpm=120.0 doesn't match our pattern)
            if (meta.bpm >= 1.0 && meta.bpm < 100.0)  // Writer sets bpm = i where i starts at 1
            {
                double expectedSampleRate = 1000.0 * meta.bpm;
                int64_t expectedTimestamp = static_cast<int64_t>(meta.bpm) * 100;

                bool sampleRateConsistent = std::abs(meta.sampleRate - expectedSampleRate) < 0.01;
                bool timestampConsistent = meta.timestamp == expectedTimestamp;

                if (!sampleRateConsistent || !timestampConsistent)
                {
                    inconsistentCount++;
                }
            }
            readCount++;
        }
    });

    // Run for enough time to stress test
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;

    writer.join();
    reader.join();

    // Should have completed many operations
    EXPECT_GT(writeCount.load(), 100);
    EXPECT_GT(readCount.load(), 100);

    // CRITICAL: No inconsistent reads should occur with SeqLock
    EXPECT_EQ(inconsistentCount.load(), 0)
        << "SeqLock failed: " << inconsistentCount.load() << " inconsistent reads detected";
}

// =============================================================================
// Edge Case Tests - Capacity and Buffer Size
// =============================================================================

// Test: Non-power-of-2 capacity is adjusted to power of 2
TEST_F(SharedCaptureBufferTest, NonPowerOfTwoCapacity)
{
    // Constructor should round up to nearest power of 2
    auto buffer100 = std::make_unique<SharedCaptureBuffer>(100);
    // 100 rounds up to 128
    EXPECT_GE(buffer100->getCapacity(), 100u);
    // Should be a power of 2
    size_t cap = buffer100->getCapacity();
    EXPECT_EQ(cap & (cap - 1), 0u) << "Capacity " << cap << " is not a power of 2";

    auto buffer1000 = std::make_unique<SharedCaptureBuffer>(1000);
    cap = buffer1000->getCapacity();
    EXPECT_GE(cap, 1000u);
    EXPECT_EQ(cap & (cap - 1), 0u) << "Capacity " << cap << " is not a power of 2";
}

// Test: Very small capacity
TEST_F(SharedCaptureBufferTest, SmallCapacity)
{
    // Capacity of 2 (minimum power of 2 > 1)
    auto smallBuffer = std::make_unique<SharedCaptureBuffer>(2);
    EXPECT_GE(smallBuffer->getCapacity(), 2u);

    // Write and read should still work
    juce::AudioBuffer<float> testBuf(2, 1);
    testBuf.setSample(0, 0, 0.5f);
    testBuf.setSample(1, 0, 0.6f);

    CaptureFrameMetadata meta;
    smallBuffer->write(testBuf, meta);

    std::vector<float> output(1);
    int read = smallBuffer->read(output.data(), 1, 0);
    EXPECT_EQ(read, 1);
    EXPECT_NEAR(output[0], 0.5f, 0.001f);
}

// Test: Capacity of 1 (edge case)
TEST_F(SharedCaptureBufferTest, CapacityOne)
{
    auto tinyBuffer = std::make_unique<SharedCaptureBuffer>(1);
    // Should be at least 1 (may round up to minimum power of 2)
    EXPECT_GE(tinyBuffer->getCapacity(), 1u);

    // Should not crash on operations
    juce::AudioBuffer<float> testBuf(2, 1);
    testBuf.setSample(0, 0, 0.5f);
    CaptureFrameMetadata meta;
    tinyBuffer->write(testBuf, meta);
}

// Test: Large capacity
TEST_F(SharedCaptureBufferTest, LargeCapacity)
{
    // 1MB worth of samples (262144 samples at 4 bytes each)
    auto largeBuffer = std::make_unique<SharedCaptureBuffer>(262144);
    EXPECT_EQ(largeBuffer->getCapacity(), 262144u);

    // Should handle large writes
    juce::AudioBuffer<float> largeBuf(2, 8192);
    for (int i = 0; i < 8192; ++i)
    {
        largeBuf.setSample(0, i, 0.5f);
        largeBuf.setSample(1, i, 0.5f);
    }

    CaptureFrameMetadata meta;
    largeBuffer->write(largeBuf, meta);

    EXPECT_EQ(largeBuffer->getAvailableSamples(), 8192u);
}

// =============================================================================
// Edge Case Tests - Write Operations
// =============================================================================

// Test: Write with mono buffer (1 channel)
TEST_F(SharedCaptureBufferTest, WriteMonoBuffer)
{
    juce::AudioBuffer<float> monoBuf(1, 100);
    for (int i = 0; i < 100; ++i)
    {
        monoBuf.setSample(0, i, 0.7f);
    }

    CaptureFrameMetadata meta;
    meta.numChannels = 1;
    buffer->write(monoBuf, meta);

    std::vector<float> output(100);
    int read = buffer->read(output.data(), 100, 0);
    EXPECT_EQ(read, 100);
    EXPECT_NEAR(output[50], 0.7f, 0.001f);
}

// Test: Write with empty buffer (0 samples)
TEST_F(SharedCaptureBufferTest, WriteEmptyBuffer)
{
    juce::AudioBuffer<float> emptyBuf(2, 0);
    CaptureFrameMetadata meta;

    // Should not crash
    buffer->write(emptyBuf, meta);

    // Available samples should remain 0
    EXPECT_EQ(buffer->getAvailableSamples(), 0u);
}

// Test: Write with 0 channels
TEST_F(SharedCaptureBufferTest, WriteZeroChannels)
{
    juce::AudioBuffer<float> noChannelBuf(0, 100);
    CaptureFrameMetadata meta;
    meta.numChannels = 0;

    // Should not crash
    buffer->write(noChannelBuf, meta);
}

// Test: Multiple rapid writes
TEST_F(SharedCaptureBufferTest, RapidWrites)
{
    CaptureFrameMetadata meta;

    for (int i = 0; i < 1000; ++i)
    {
        juce::AudioBuffer<float> buf(2, 16);
        float value = static_cast<float>(i % 100) / 100.0f;
        for (int s = 0; s < 16; ++s)
        {
            buf.setSample(0, s, value);
            buf.setSample(1, s, value);
        }
        buffer->write(buf, meta);
    }

    // Should have data (capped at capacity)
    EXPECT_GT(buffer->getAvailableSamples(), 0u);
    EXPECT_LE(buffer->getAvailableSamples(), buffer->getCapacity());
}

// Test: Write fills buffer exactly
TEST_F(SharedCaptureBufferTest, WriteFillsExactly)
{
    // Buffer is 1024 samples, write exactly 1024
    juce::AudioBuffer<float> exactBuf(2, 1024);
    for (int i = 0; i < 1024; ++i)
    {
        exactBuf.setSample(0, i, 0.5f);
        exactBuf.setSample(1, i, 0.5f);
    }

    CaptureFrameMetadata meta;
    buffer->write(exactBuf, meta);

    EXPECT_EQ(buffer->getAvailableSamples(), 1024u);
}

// Test: Write exceeds capacity
TEST_F(SharedCaptureBufferTest, WriteExceedsCapacity)
{
    // Buffer is 1024 samples, write 2048
    juce::AudioBuffer<float> largeBuf(2, 2048);
    for (int i = 0; i < 2048; ++i)
    {
        largeBuf.setSample(0, i, static_cast<float>(i) / 2048.0f);
        largeBuf.setSample(1, i, static_cast<float>(i) / 2048.0f);
    }

    CaptureFrameMetadata meta;
    buffer->write(largeBuf, meta);

    // Available samples capped at capacity
    EXPECT_LE(buffer->getAvailableSamples(), buffer->getCapacity());
}

// =============================================================================
// Edge Case Tests - Read Operations
// =============================================================================

// Test: Read from empty buffer
TEST_F(SharedCaptureBufferTest, ReadFromEmptyBuffer)
{
    std::vector<float> output(100);
    int read = buffer->read(output.data(), 100, 0);

    // Should return 0 or handle gracefully
    EXPECT_GE(read, 0);
}

// Test: Read more samples than available
TEST_F(SharedCaptureBufferTest, ReadMoreThanAvailable)
{
    auto testBuf = generateTestBuffer(50, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    // Try to read 100 when only 50 available
    std::vector<float> output(100);
    int read = buffer->read(output.data(), 100, 0);

    // Should read at most what's available
    EXPECT_LE(read, 50);
}

// Test: Read more samples than capacity
TEST_F(SharedCaptureBufferTest, ReadMoreThanCapacity)
{
    auto testBuf = generateTestBuffer(512, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    // Try to read more than capacity
    std::vector<float> output(2048);
    int read = buffer->read(output.data(), 2048, 0);

    // Should be limited to available samples
    EXPECT_LE(static_cast<size_t>(read), buffer->getCapacity());
}

// Test: Read from invalid channel index
TEST_F(SharedCaptureBufferTest, ReadFromInvalidChannel)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    std::vector<float> output(100);

    // Channel 5 doesn't exist (only 0 and 1)
    int read = buffer->read(output.data(), 100, 5);

    // Should handle gracefully (return 0 or clamp to valid channel)
    EXPECT_GE(read, 0);
}

// Test: Read with AudioBuffer overload
TEST_F(SharedCaptureBufferTest, ReadToAudioBuffer)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    juce::AudioBuffer<float> output(2, 100);
    int read = buffer->read(output, 100);

    EXPECT_EQ(read, 100);
    EXPECT_NEAR(output.getSample(0, 50), 0.5f, 0.01f);
}

// Test: Read with zero samples requested
TEST_F(SharedCaptureBufferTest, ReadZeroSamples)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    std::vector<float> output(10);
    int read = buffer->read(output.data(), 0, 0);

    EXPECT_EQ(read, 0);
}

// =============================================================================
// Edge Case Tests - Peak and RMS Calculations
// =============================================================================

// Test: Peak level on empty buffer
TEST_F(SharedCaptureBufferTest, PeakLevelEmptyBuffer)
{
    float peak = buffer->getPeakLevel(0, 100);
    // Should return 0 for empty buffer
    EXPECT_FLOAT_EQ(peak, 0.0f);
}

// Test: RMS level on empty buffer
TEST_F(SharedCaptureBufferTest, RMSLevelEmptyBuffer)
{
    float rms = buffer->getRMSLevel(0, 100);
    // Should return 0 for empty buffer
    EXPECT_FLOAT_EQ(rms, 0.0f);
}

// Test: Peak level with negative values
TEST_F(SharedCaptureBufferTest, PeakLevelNegativeValues)
{
    juce::AudioBuffer<float> testBuf(2, 100);
    testBuf.clear();
    testBuf.setSample(0, 50, -0.9f);  // Negative peak

    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    float peak = buffer->getPeakLevel(0, 100);
    // Peak should be absolute value
    EXPECT_NEAR(peak, 0.9f, 0.001f);
}

// Test: RMS with alternating positive/negative
TEST_F(SharedCaptureBufferTest, RMSAlternatingValues)
{
    juce::AudioBuffer<float> testBuf(2, 100);
    for (int i = 0; i < 100; ++i)
    {
        float value = (i % 2 == 0) ? 0.5f : -0.5f;
        testBuf.setSample(0, i, value);
    }

    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    float rms = buffer->getRMSLevel(0, 100);
    // RMS of alternating +/- 0.5 should be 0.5
    EXPECT_NEAR(rms, 0.5f, 0.01f);
}

// Test: Peak/RMS requesting more samples than available
TEST_F(SharedCaptureBufferTest, PeakRMSMoreThanAvailable)
{
    juce::AudioBuffer<float> testBuf(2, 50);
    for (int i = 0; i < 50; ++i)
    {
        testBuf.setSample(0, i, 0.5f);
    }

    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    // Request more samples than available
    float peak = buffer->getPeakLevel(0, 1000);
    float rms = buffer->getRMSLevel(0, 1000);

    // Should compute over available samples
    EXPECT_NEAR(peak, 0.5f, 0.01f);
    EXPECT_NEAR(rms, 0.5f, 0.01f);
}

// Test: Peak/RMS on invalid channel
TEST_F(SharedCaptureBufferTest, PeakRMSInvalidChannel)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    // Invalid channel
    float peak = buffer->getPeakLevel(10, 100);
    float rms = buffer->getRMSLevel(10, 100);

    // Should handle gracefully
    EXPECT_GE(peak, 0.0f);
    EXPECT_GE(rms, 0.0f);
}

// =============================================================================
// Edge Case Tests - Clear and Reset
// =============================================================================

// Test: Write after clear
TEST_F(SharedCaptureBufferTest, WriteAfterClear)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    buffer->clear();
    EXPECT_EQ(buffer->getAvailableSamples(), 0u);

    // Write new data
    auto testBuf2 = generateTestBuffer(50, 0.8f);
    buffer->write(testBuf2, meta);

    EXPECT_EQ(buffer->getAvailableSamples(), 50u);

    std::vector<float> output(50);
    buffer->read(output.data(), 50, 0);
    EXPECT_NEAR(output[0], 0.8f, 0.001f);
}

// Test: Multiple clears
TEST_F(SharedCaptureBufferTest, MultipleClears)
{
    for (int i = 0; i < 10; ++i)
    {
        auto testBuf = generateTestBuffer(100, 0.5f);
        CaptureFrameMetadata meta;
        buffer->write(testBuf, meta);
        buffer->clear();
    }

    EXPECT_EQ(buffer->getAvailableSamples(), 0u);
}

// =============================================================================
// Edge Case Tests - Metadata
// =============================================================================

// Test: Metadata with extreme values
TEST_F(SharedCaptureBufferTest, MetadataExtremeValues)
{
    auto testBuf = generateTestBuffer(100, 0.5f);

    CaptureFrameMetadata meta;
    meta.sampleRate = 192000.0;
    meta.numChannels = 64;  // This will be overwritten by actual buffer channels
    meta.bpm = 300.0;
    meta.isPlaying = true;
    meta.timestamp = std::numeric_limits<int64_t>::max();

    buffer->write(testBuf, meta);

    auto retrieved = buffer->getLatestMetadata();
    EXPECT_DOUBLE_EQ(retrieved.sampleRate, 192000.0);
    // numChannels is set from actual buffer, not from passed metadata
    EXPECT_EQ(retrieved.numChannels, testBuf.getNumChannels());
    EXPECT_DOUBLE_EQ(retrieved.bpm, 300.0);
    EXPECT_TRUE(retrieved.isPlaying);
    EXPECT_EQ(retrieved.timestamp, std::numeric_limits<int64_t>::max());
}

// Test: Metadata before any write
TEST_F(SharedCaptureBufferTest, MetadataBeforeWrite)
{
    auto meta = buffer->getLatestMetadata();

    // Should return default values
    EXPECT_GT(meta.sampleRate, 0.0);
}

// =============================================================================
// Edge Case Tests - Write Position
// =============================================================================

// Test: Get write position
TEST_F(SharedCaptureBufferTest, WritePositionTracking)
{
    size_t initialPos = buffer->getWritePosition();
    EXPECT_EQ(initialPos, 0u);

    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    size_t newPos = buffer->getWritePosition();
    EXPECT_EQ(newPos, 100u);

    // Write more
    buffer->write(testBuf, meta);
    EXPECT_EQ(buffer->getWritePosition(), 200u);
}

// Test: Write position wraps at capacity
TEST_F(SharedCaptureBufferTest, WritePositionWrapping)
{
    // Write exactly capacity
    juce::AudioBuffer<float> fullBuf(2, 1024);
    for (int i = 0; i < 1024; ++i)
    {
        fullBuf.setSample(0, i, 0.5f);
        fullBuf.setSample(1, i, 0.5f);
    }

    CaptureFrameMetadata meta;
    buffer->write(fullBuf, meta);

    // Position should wrap to 0 (or be at capacity)
    size_t pos = buffer->getWritePosition();
    EXPECT_LE(pos, buffer->getCapacity());

    // Write one more sample
    juce::AudioBuffer<float> oneSample(2, 1);
    oneSample.setSample(0, 0, 0.9f);
    buffer->write(oneSample, meta);

    // Position should have advanced and possibly wrapped
    size_t newPos = buffer->getWritePosition();
    EXPECT_LE(newPos, buffer->getCapacity());
}
