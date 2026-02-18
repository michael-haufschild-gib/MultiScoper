/*
    Oscil - Capture Thread Tests
    Unit tests for the dedicated capture processing thread
*/

#include <gtest/gtest.h>
#include "core/CaptureThread.h"
#include "core/AudioCapturePool.h"
#include <thread>
#include <chrono>
#include <cmath>

namespace oscil
{
namespace test
{

class CaptureThreadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Allocate a test slot
        testSlot_ = pool_.allocateSlot(testSource_, 44100.0, 1);
        ASSERT_GE(testSlot_, 0);
    }

    void TearDown() override
    {
        // Stop thread if running
        thread_.stopCapturing();
        
        // Release slot
        if (testSlot_ >= 0)
        {
            pool_.releaseSlot(testSlot_);
        }
    }

    AudioCapturePool pool_;
    CaptureThread thread_{pool_};
    SourceId testSource_{"test-source"};
    int testSlot_ = -1;
};

// Start and stop capturing
TEST_F(CaptureThreadTest, StartStop)
{
    EXPECT_FALSE(thread_.isCapturing());
    
    thread_.startCapturing();
    EXPECT_TRUE(thread_.isCapturing());
    
    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    thread_.stopCapturing();
    EXPECT_FALSE(thread_.isCapturing());
}

// Get processed buffer
TEST_F(CaptureThreadTest, GetProcessedBuffer)
{
    auto buffer = thread_.getProcessedBuffer(testSlot_);
    EXPECT_NE(buffer, nullptr);
    
    // Invalid slot should return null
    EXPECT_EQ(thread_.getProcessedBuffer(-1), nullptr);
    EXPECT_EQ(thread_.getProcessedBuffer(AudioCapturePool::MAX_SOURCES), nullptr);
}

// Data flow through thread
TEST_F(CaptureThreadTest, DataFlow)
{
    thread_.startCapturing();
    
    // Write test data to raw buffer
    RawCaptureBuffer* rawBuffer = pool_.getRawBuffer(testSlot_);
    ASSERT_NE(rawBuffer, nullptr);
    
    const int numSamples = 1024;
    std::vector<float> testData(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        testData[i] = std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(i) / 44100.0f);
    }
    
    rawBuffer->write(testData.data(), testData.data(), numSamples, 44100.0);
    
    // Wait for capture thread to process
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Check processed buffer has data
    auto processedBuffer = thread_.getProcessedBuffer(testSlot_);
    ASSERT_NE(processedBuffer, nullptr);
    
    size_t available = processedBuffer->getAvailableSamples();
    EXPECT_GT(available, 0u);
    
    thread_.stopCapturing();
}

// Statistics tracking
TEST_F(CaptureThreadTest, Statistics)
{
    thread_.resetStats();
    
    EXPECT_DOUBLE_EQ(thread_.getAverageTickTimeUs(), 0.0);
    EXPECT_DOUBLE_EQ(thread_.getMaxTickTimeUs(), 0.0);
    
    thread_.startCapturing();
    
    // Let it run to accumulate stats
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    thread_.stopCapturing();
    
    // Should have some timing data now
    EXPECT_GT(thread_.getAverageTickTimeUs(), 0.0);
}

// Multiple active slots
TEST_F(CaptureThreadTest, MultipleSlots)
{
    // Allocate additional slots
    SourceId source2{"source-2"};
    SourceId source3{"source-3"};
    
    int slot2 = pool_.allocateSlot(source2, 48000.0, 2);
    int slot3 = pool_.allocateSlot(source3, 96000.0, 4);
    
    ASSERT_GE(slot2, 0);
    ASSERT_GE(slot3, 0);
    
    thread_.startCapturing();
    
    // Write to all buffers
    std::vector<float> data(512, 0.5f);
    
    pool_.getRawBuffer(testSlot_)->write(data.data(), data.data(), 512, 44100.0);
    pool_.getRawBuffer(slot2)->write(data.data(), data.data(), 512, 48000.0);
    pool_.getRawBuffer(slot3)->write(data.data(), data.data(), 512, 96000.0);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // All should have processed buffers
    EXPECT_NE(thread_.getProcessedBuffer(testSlot_), nullptr);
    EXPECT_NE(thread_.getProcessedBuffer(slot2), nullptr);
    EXPECT_NE(thread_.getProcessedBuffer(slot3), nullptr);
    
    thread_.stopCapturing();
    
    pool_.releaseSlot(slot2);
    pool_.releaseSlot(slot3);
}

// Interface access
TEST_F(CaptureThreadTest, InterfaceAccess)
{
    auto buffer = thread_.getProcessedBufferAsInterface(testSlot_);
    EXPECT_NE(buffer, nullptr);
    
    // Should be usable as IAudioBuffer
    EXPECT_GE(buffer->getCapacity(), 0u);
}

// Continuous data streaming
TEST_F(CaptureThreadTest, ContinuousStreaming)
{
    thread_.startCapturing();
    
    RawCaptureBuffer* rawBuffer = pool_.getRawBuffer(testSlot_);
    auto processedBuffer = thread_.getProcessedBuffer(testSlot_);
    
    ASSERT_NE(rawBuffer, nullptr);
    ASSERT_NE(processedBuffer, nullptr);
    
    const int iterations = 100;
    const int blockSize = 256;
    std::vector<float> data(blockSize, 0.5f);
    
    // Simulate continuous audio streaming
    for (int i = 0; i < iterations; ++i)
    {
        rawBuffer->write(data.data(), data.data(), blockSize, 44100.0);
        
        // Small delay to simulate audio timing
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    
    // Wait for final processing
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    // Should have accumulated samples
    size_t available = processedBuffer->getAvailableSamples();
    EXPECT_GT(available, 0u);
    
    thread_.stopCapturing();
}

// Thread safety stress test
TEST_F(CaptureThreadTest, StressTest)
{
    thread_.startCapturing();
    
    std::atomic<bool> producerDone{false};
    std::atomic<int> writeCount{0};
    std::atomic<int> readCount{0};
    
    RawCaptureBuffer* rawBuffer = pool_.getRawBuffer(testSlot_);
    auto processedBuffer = thread_.getProcessedBuffer(testSlot_);
    
    // Producer thread (simulates audio thread)
    std::thread producer([&]() {
        std::vector<float> data(512, 0.5f);
        
        for (int i = 0; i < 500; ++i)
        {
            rawBuffer->write(data.data(), data.data(), 512, 44100.0);
            writeCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        producerDone.store(true, std::memory_order_release);
    });
    
    // Consumer thread (simulates UI thread reading)
    std::thread consumer([&]() {
        std::vector<float> readData(1024);
        
        while (!producerDone.load(std::memory_order_acquire))
        {
            int read = processedBuffer->read(readData.data(), 1024, 0);
            if (read > 0)
            {
                readCount.fetch_add(1, std::memory_order_relaxed);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(writeCount.load(), 500);
    EXPECT_GT(readCount.load(), 0);
    
    thread_.stopCapturing();
}

} // namespace test
} // namespace oscil













