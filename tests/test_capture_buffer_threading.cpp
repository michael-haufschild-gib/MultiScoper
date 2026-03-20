/*
    Oscil - Capture Buffer Tests: Threading
    Tests for thread safety, concurrent access, and lock-free operations
*/

#include <gtest/gtest.h>
#include "helpers/AudioBufferBuilder.h"
#include "core/SharedCaptureBuffer.h"
#include <thread>
#include <atomic>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferThreadingTest : public ::testing::Test
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
        return AudioBufferBuilder()
            .withChannels(2)
            .withSamples(numSamples)
            .withDC(value)
            .build();
    }
};

// Test: Thread safety - concurrent write and read
TEST_F(CaptureBufferThreadingTest, ThreadSafetyConcurrentAccess)
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

// Test: SeqLock metadata consistency under concurrent access
// Verifies that metadata reads are always consistent (no torn reads)
TEST_F(CaptureBufferThreadingTest, SeqLockMetadataConsistency)
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
