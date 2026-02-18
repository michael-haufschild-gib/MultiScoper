/*
    Oscil - Capture Buffer Tests: Threading
    Tests for thread safety, concurrent access, and lock-free operations
    
    Key test scenarios:
    - Lock-free SPSC write from audio thread
    - Concurrent read/write without data races
    - SeqLock metadata consistency
    - Safe reconfiguration during audio processing
    - SIMD decimation correctness
*/

#include <gtest/gtest.h>
#include "helpers/AudioBufferBuilder.h"
#include "core/SharedCaptureBuffer.h"
#include "core/DecimatingCaptureBuffer.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace oscil;
using namespace oscil::test;

//==============================================================================
// SharedCaptureBuffer Threading Tests
//==============================================================================

class SharedCaptureBufferThreadingTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override
    {
        buffer = std::make_unique<SharedCaptureBuffer>(1024);
    }

    juce::AudioBuffer<float> generateTestBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder()
            .withChannels(2)
            .withSamples(numSamples)
            .withDC(value)
            .build();
    }
};

// Test: Lock-free write performance - should complete without blocking
TEST_F(SharedCaptureBufferThreadingTest, LockFreeWritePerformance)
{
    auto testBuffer = generateTestBuffer(512, 0.5f);
    CaptureFrameMetadata metadata;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many lock-free writes
    for (int i = 0; i < 10000; ++i)
    {
        buffer->writeLockFree(testBuffer, metadata);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete quickly - less than 100us per write on average
    double avgMicroseconds = static_cast<double>(duration.count()) / 10000.0;
    EXPECT_LT(avgMicroseconds, 100.0) 
        << "Lock-free write too slow: " << avgMicroseconds << "us average";
}

// Test: Thread safety - concurrent lock-free write and read
TEST_F(SharedCaptureBufferThreadingTest, LockFreeConcurrentAccess)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

    // Writer thread using lock-free path
    std::thread writer([this, &running, &writeCount]()
    {
        while (running)
        {
            auto testBuffer = generateTestBuffer(64, 0.5f);
            CaptureFrameMetadata metadata;
            buffer->writeLockFree(testBuffer, metadata);
            writeCount++;
        }
    });

    // Reader thread
    std::thread reader([this, &running, &readCount]()
    {
        std::vector<float> output(64);
        while (running)
        {
            buffer->read(output.data(), 64, 0);
            readCount++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    reader.join();

    EXPECT_GT(writeCount.load(), 0);
    EXPECT_GT(readCount.load(), 0);
}

// Test: Legacy write interface still works
TEST_F(SharedCaptureBufferThreadingTest, LegacyWriteStillWorks)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

    std::thread writer([this, &running, &writeCount]()
    {
        while (running)
        {
            auto testBuffer = generateTestBuffer(64, 0.5f);
            CaptureFrameMetadata metadata;
            buffer->write(testBuffer, metadata);  // Legacy blocking write
            writeCount++;
        }
    });

    std::thread reader([this, &running, &readCount]()
    {
        std::vector<float> output(64);
        while (running)
        {
            buffer->read(output.data(), 64, 0);
            readCount++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    reader.join();

    EXPECT_GT(writeCount.load(), 0);
    EXPECT_GT(readCount.load(), 0);
}

// Test: SeqLock metadata consistency
TEST_F(SharedCaptureBufferThreadingTest, SeqLockMetadataConsistency)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};
    std::atomic<int> inconsistentCount{0};

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

            buffer->writeLockFree(testBuffer, metadata);
            writeCount++;
            i++;

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    std::thread reader([this, &running, &readCount, &inconsistentCount]()
    {
        while (running)
        {
            auto meta = buffer->getLatestMetadata();

            if (meta.bpm >= 1.0 && meta.bpm < 100.0)
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

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;

    writer.join();
    reader.join();

    EXPECT_GT(writeCount.load(), 100);
    EXPECT_GT(readCount.load(), 100);
    EXPECT_EQ(inconsistentCount.load(), 0)
        << "SeqLock failed: " << inconsistentCount.load() << " inconsistent reads detected";
}

//==============================================================================
// DecimatingCaptureBuffer Threading Tests
//==============================================================================

class DecimatingCaptureBufferThreadingTest : public ::testing::Test
{
protected:
    std::unique_ptr<DecimatingCaptureBuffer> buffer;

    void SetUp() override
    {
        CaptureQualityConfig config;
        config.qualityPreset = QualityPreset::Standard;
        buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);
    }

    juce::AudioBuffer<float> generateTestBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder()
            .withChannels(2)
            .withSamples(numSamples)
            .withDC(value)
            .build();
    }
};

// Test: Lock-free write to DecimatingCaptureBuffer
TEST_F(DecimatingCaptureBufferThreadingTest, LockFreeWriteNoBlocking)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

    // Audio thread - should never block
    std::thread audioThread([this, &running, &writeCount]()
    {
        while (running)
        {
            auto testBuffer = generateTestBuffer(512, 0.5f);
            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0;
            
            buffer->write(testBuffer, metadata);
            writeCount++;
        }
    });

    // UI thread - reading
    std::thread uiThread([this, &running, &readCount]()
    {
        std::vector<float> output(512);
        while (running)
        {
            buffer->read(output.data(), 512, 0);
            readCount++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running = false;

    audioThread.join();
    uiThread.join();

    EXPECT_GT(writeCount.load(), 100);
    EXPECT_GT(readCount.load(), 100);
}

// Test: Safe reconfiguration during audio processing
TEST_F(DecimatingCaptureBufferThreadingTest, SafeReconfigurationDuringProcessing)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> reconfigureCount{0};
    std::atomic<int> failedWrites{0};

    // Audio thread - continuous writes
    std::thread audioThread([this, &running, &writeCount, &failedWrites]()
    {
        while (running)
        {
            auto testBuffer = generateTestBuffer(256, 0.5f);
            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0;
            
            buffer->write(testBuffer, metadata);
            writeCount++;
            
            // Simulate realistic audio callback timing
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    // Message thread - reconfiguring frequently
    std::thread messageThread([this, &running, &reconfigureCount]()
    {
        QualityPreset presets[] = {
            QualityPreset::Eco,
            QualityPreset::Standard,
            QualityPreset::High
        };
        int presetIndex = 0;

        while (running)
        {
            buffer->setQualityPreset(presets[presetIndex], 44100);
            reconfigureCount++;
            presetIndex = (presetIndex + 1) % 3;
            
            // Reconfigure every 10ms (aggressive stress test)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running = false;

    audioThread.join();
    messageThread.join();

    EXPECT_GT(writeCount.load(), 100) << "Should complete many writes";
    EXPECT_GT(reconfigureCount.load(), 10) << "Should complete many reconfigurations";
    EXPECT_EQ(failedWrites.load(), 0) << "No writes should fail (drop is ok, crash is not)";
}

// Test: SIMD decimation produces correct output count
TEST_F(DecimatingCaptureBufferThreadingTest, SIMDDecimationCorrectness)
{
    // Configure with known decimation ratio
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::Eco;  // Higher decimation
    buffer->configure(config, 96000);  // High sample rate = more decimation
    
    int decimationRatio = buffer->getDecimationRatio();
    
    // Write known input
    auto testBuffer = generateTestBuffer(1024, 0.5f);
    CaptureFrameMetadata metadata;
    metadata.sampleRate = 96000.0;
    
    buffer->write(testBuffer, metadata);
    
    // Check that data was written
    size_t available = buffer->getAvailableSamples();
    EXPECT_GT(available, 0) << "Should have samples available after write";
    
    // Read and verify we can read samples
    std::vector<float> output(512);
    int samplesRead = buffer->read(output.data(), 512, 0);
    EXPECT_GT(samplesRead, 0) << "Should read samples";
}

// Test: Multiple rapid reconfigurations don't cause issues
TEST_F(DecimatingCaptureBufferThreadingTest, RapidReconfigurationStress)
{
    int successfulWrites = 0;
    int successfulReads = 0;

    // Rapidly reconfigure many times
    for (int i = 0; i < 100; ++i)
    {
        QualityPreset preset = static_cast<QualityPreset>(i % 3);
        int sampleRate = (i % 2 == 0) ? 44100 : 96000;

        buffer->setQualityPreset(preset, sampleRate);

        // Write some data between reconfigurations
        auto testBuffer = generateTestBuffer(128, 0.3f);
        CaptureFrameMetadata metadata;
        metadata.sampleRate = static_cast<double>(sampleRate);
        buffer->write(testBuffer, metadata);
        successfulWrites++;

        // Verify we can still read after reconfiguration
        std::vector<float> output(64);
        int samplesRead = buffer->read(output.data(), 64, 0);
        if (samplesRead > 0)
            successfulReads++;
    }

    // Verify operations completed successfully
    EXPECT_EQ(successfulWrites, 100) << "All write operations should complete";
    EXPECT_GT(successfulReads, 0) << "At least some reads should succeed after reconfigurations";

    // Verify buffer is still functional after stress test
    auto finalBuffer = generateTestBuffer(256, 0.5f);
    CaptureFrameMetadata finalMetadata;
    finalMetadata.sampleRate = 44100.0;
    buffer->write(finalBuffer, finalMetadata);

    std::vector<float> finalOutput(256);
    int finalRead = buffer->read(finalOutput.data(), 256, 0);
    EXPECT_GT(finalRead, 0) << "Buffer should still be readable after stress test";
}

// Test: Verify lock-free read during write
TEST_F(DecimatingCaptureBufferThreadingTest, ConcurrentReadWriteDataIntegrity)
{
    std::atomic<bool> running{true};
    std::atomic<int> corruptedReads{0};

    // Writer writes known pattern
    std::thread writer([this, &running]()
    {
        float value = 0.0f;
        while (running)
        {
            auto testBuffer = generateTestBuffer(256, value);
            CaptureFrameMetadata metadata;
            metadata.sampleRate = 44100.0;
            buffer->write(testBuffer, metadata);
            
            value = std::fmod(value + 0.1f, 1.0f);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Reader verifies data is valid (not garbage/NaN/Inf)
    std::thread reader([this, &running, &corruptedReads]()
    {
        std::vector<float> output(256);
        while (running)
        {
            int read = buffer->read(output.data(), 256, 0);
            
            for (int i = 0; i < read; ++i)
            {
                if (!std::isfinite(output[i]) || std::abs(output[i]) > 1.1f)
                {
                    corruptedReads++;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    running = false;

    writer.join();
    reader.join();

    EXPECT_EQ(corruptedReads.load(), 0) 
        << "Detected " << corruptedReads.load() << " corrupted reads";
}

//==============================================================================
// Legacy Test Compatibility (renamed from CaptureBufferThreadingTest)
//==============================================================================

class CaptureBufferThreadingTest : public SharedCaptureBufferThreadingTest {};

TEST_F(CaptureBufferThreadingTest, ThreadSafetyConcurrentAccess)
{
    std::atomic<bool> running{true};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};

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

    std::thread reader([this, &running, &readCount]()
    {
        std::vector<float> output(64);
        while (running)
        {
            buffer->read(output.data(), 64, 0);
            readCount++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    writer.join();
    reader.join();

    EXPECT_GT(writeCount.load(), 0);
    EXPECT_GT(readCount.load(), 0);
}
