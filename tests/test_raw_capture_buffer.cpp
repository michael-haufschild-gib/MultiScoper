/*
    Oscil - Raw Capture Buffer Tests
    Unit tests for the lock-free SPSC ring buffer
*/

#include <gtest/gtest.h>
#include "core/RawCaptureBuffer.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

namespace oscil
{
namespace test
{

class RawCaptureBufferTest : public ::testing::Test
{
protected:
    RawCaptureBuffer buffer;
};

// Basic write and read
TEST_F(RawCaptureBufferTest, BasicWriteRead)
{
    // Create test data
    std::vector<float> leftIn(512, 0.5f);
    std::vector<float> rightIn(512, -0.5f);

    // Write to buffer
    buffer.write(leftIn.data(), rightIn.data(), 512, 44100.0);

    // Read back
    std::vector<float> leftOut(512);
    std::vector<float> rightOut(512);
    int samplesRead = buffer.read(leftOut.data(), rightOut.data(), 512);

    EXPECT_EQ(samplesRead, 512);
    
    // Verify data
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(leftOut[i], 0.5f) << "Left channel mismatch at " << i;
        EXPECT_FLOAT_EQ(rightOut[i], -0.5f) << "Right channel mismatch at " << i;
    }
}

// Sample rate is stored and retrievable
TEST_F(RawCaptureBufferTest, SampleRateStorage)
{
    std::vector<float> data(256, 0.0f);
    
    buffer.write(data.data(), data.data(), 256, 96000.0);
    EXPECT_DOUBLE_EQ(buffer.getSampleRate(), 96000.0);
    
    buffer.write(data.data(), data.data(), 256, 48000.0);
    EXPECT_DOUBLE_EQ(buffer.getSampleRate(), 48000.0);
}

// Available samples tracking
TEST_F(RawCaptureBufferTest, AvailableSamplesTracking)
{
    EXPECT_EQ(buffer.getAvailableSamples(), 0u);

    std::vector<float> data(1024, 0.0f);
    buffer.write(data.data(), data.data(), 1024, 44100.0);
    EXPECT_EQ(buffer.getAvailableSamples(), 1024u);

    // Read some samples
    std::vector<float> out(512);
    buffer.read(out.data(), out.data(), 512);
    EXPECT_EQ(buffer.getAvailableSamples(), 512u);

    // Read remaining
    buffer.read(out.data(), out.data(), 512);
    EXPECT_EQ(buffer.getAvailableSamples(), 0u);
}

// Mono write copies to both channels
TEST_F(RawCaptureBufferTest, MonoWriteCopies)
{
    std::vector<float> mono(256, 0.75f);
    buffer.writeMono(mono.data(), 256, 44100.0);

    std::vector<float> leftOut(256);
    std::vector<float> rightOut(256);
    buffer.read(leftOut.data(), rightOut.data(), 256);

    for (int i = 0; i < 256; ++i)
    {
        EXPECT_FLOAT_EQ(leftOut[i], 0.75f);
        EXPECT_FLOAT_EQ(rightOut[i], 0.75f);
    }
}

// Wrap-around handling
TEST_F(RawCaptureBufferTest, WrapAround)
{
    const int chunkSize = 8192;
    const int numChunks = 10; // Exceeds capacity, should wrap

    std::vector<float> data(chunkSize);

    // Write multiple chunks
    for (int chunk = 0; chunk < numChunks; ++chunk)
    {
        // Fill with identifiable values
        for (int i = 0; i < chunkSize; ++i)
        {
            data[i] = static_cast<float>(chunk * chunkSize + i);
        }
        buffer.write(data.data(), data.data(), chunkSize, 44100.0);
    }

    // Should still be able to read most recent data
    size_t available = buffer.getAvailableSamples();
    EXPECT_GT(available, 0u);
    EXPECT_LE(available, RawCaptureBuffer::CAPACITY);
}

// Reset clears buffer
TEST_F(RawCaptureBufferTest, ResetClearsBuffer)
{
    std::vector<float> data(1024, 1.0f);
    buffer.write(data.data(), data.data(), 1024, 44100.0);
    EXPECT_EQ(buffer.getAvailableSamples(), 1024u);

    buffer.reset();
    EXPECT_EQ(buffer.getAvailableSamples(), 0u);
}

// Null pointer handling
TEST_F(RawCaptureBufferTest, NullPointerHandling)
{
    // Write with null left channel should zero-fill left and copy to right
    std::vector<float> right(256, 0.5f);
    buffer.write(nullptr, right.data(), 256, 44100.0);

    std::vector<float> leftOut(256);
    std::vector<float> rightOut(256);
    buffer.read(leftOut.data(), rightOut.data(), 256);

    // Left should be zeros
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_FLOAT_EQ(leftOut[i], 0.0f);
    }
}

// Read with partial availability
TEST_F(RawCaptureBufferTest, PartialRead)
{
    std::vector<float> data(256, 0.5f);
    buffer.write(data.data(), data.data(), 256, 44100.0);

    // Try to read more than available
    std::vector<float> out(1024);
    int samplesRead = buffer.read(out.data(), out.data(), 1024);

    EXPECT_EQ(samplesRead, 256);
}

// Concurrent producer/consumer (SPSC pattern)
TEST_F(RawCaptureBufferTest, ConcurrentSPSC)
{
    const int totalSamples = 100000;
    const int blockSize = 512;
    std::atomic<bool> producerDone{false};
    std::atomic<int> samplesWritten{0};
    std::atomic<int> samplesRead{0};

    // Producer thread (simulates audio thread)
    std::thread producer([&]() {
        std::vector<float> data(blockSize);
        int written = 0;
        
        while (written < totalSamples)
        {
            // Fill with identifiable values
            for (int i = 0; i < blockSize; ++i)
            {
                data[i] = static_cast<float>(written + i);
            }
            
            buffer.write(data.data(), data.data(), blockSize, 44100.0);
            written += blockSize;
            samplesWritten.store(written, std::memory_order_relaxed);
            
            // Simulate audio callback timing (~1ms for 44.1kHz @ 512 samples)
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        producerDone.store(true, std::memory_order_release);
    });

    // Consumer thread (simulates capture thread)
    std::thread consumer([&]() {
        std::vector<float> left(blockSize);
        std::vector<float> right(blockSize);
        int totalRead = 0;
        
        while (!producerDone.load(std::memory_order_acquire) || buffer.getAvailableSamples() > 0)
        {
            int read = buffer.read(left.data(), right.data(), blockSize);
            if (read > 0)
            {
                totalRead += read;
                samplesRead.store(totalRead, std::memory_order_relaxed);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
    });

    producer.join();
    consumer.join();

    // Consumer should have read a significant portion
    // (may not be exact due to ring buffer overwrite)
    EXPECT_GT(samplesRead.load(), totalSamples / 2);
}

// Write timing test (should be < 10μs for typical block sizes)
TEST_F(RawCaptureBufferTest, WriteTiming)
{
    const int iterations = 1000;
    const int blockSize = 512;
    std::vector<float> left(blockSize, 0.5f);
    std::vector<float> right(blockSize, -0.5f);

    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i)
    {
        buffer.write(left.data(), right.data(), blockSize, 44100.0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avgTimeUs = static_cast<double>(durationNs) / (iterations * 1000.0);

    // Should be well under 10μs average
    EXPECT_LT(avgTimeUs, 10.0) << "Average write time: " << avgTimeUs << " μs";

    // Log actual timing for reference
    std::cout << "Average write time: " << avgTimeUs << " μs" << std::endl;
}

} // namespace test
} // namespace oscil













