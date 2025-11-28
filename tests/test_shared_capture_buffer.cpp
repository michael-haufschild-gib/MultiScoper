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

            // Small delay to allow reads - use yield to avoid busy-wait
            std::this_thread::yield();
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
