/*
    Oscil - Capture Buffer Tests: Core Operations
    Tests for basic buffer operations, capacity, peak/RMS, and metadata
*/

#include "core/SharedCaptureBuffer.h"

#include "helpers/AudioBufferBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferCoreTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override { buffer = std::make_unique<SharedCaptureBuffer>(1024); }

    // Generate test audio buffer
    juce::AudioBuffer<float> generateTestBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

// Test: Basic write and read
TEST_F(CaptureBufferCoreTest, BasicWriteRead)
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

// Test: Ring buffer wrapping — last write's data must survive
TEST_F(CaptureBufferCoreTest, RingBufferWrappingPreservesLatestData)
{
    float lastValue = 0.0f;
    for (int i = 0; i < 20; ++i)
    {
        lastValue = static_cast<float>(i) * 0.05f;
        auto testBuffer = generateTestBuffer(100, lastValue);

        CaptureFrameMetadata metadata;
        metadata.sampleRate = 44100.0;
        buffer->write(testBuffer, metadata);
    }

    // Read the most recent 100 samples — they must match the last write's value
    std::vector<float> output(100);
    int samplesRead = buffer->read(output.data(), 100, 0);

    ASSERT_EQ(samplesRead, 100);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_NEAR(output[i], lastValue, 0.001f) << "Sample " << i << " has stale data after ring wrap";
    }
}

// Test: Get available samples
TEST_F(CaptureBufferCoreTest, GetAvailableSamples)
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
TEST_F(CaptureBufferCoreTest, ClearBuffer)
{
    auto testBuffer = generateTestBuffer(256, 0.5f);
    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    EXPECT_GT(buffer->getAvailableSamples(), 0);

    buffer->clear();

    EXPECT_EQ(buffer->getAvailableSamples(), 0);
}

// Test: Peak level calculation
TEST_F(CaptureBufferCoreTest, PeakLevel)
{
    juce::AudioBuffer<float> testBuffer(2, 100);
    testBuffer.clear();
    testBuffer.setSample(0, 50, 0.8f); // Peak in left channel

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    float peak = buffer->getPeakLevel(0, 100);
    EXPECT_NEAR(peak, 0.8f, 0.001f);
}

// Test: RMS level calculation
TEST_F(CaptureBufferCoreTest, RMSLevel)
{
    auto testBuffer = AudioBufferBuilder().withChannels(2).withSamples(100).withDC(0.5f).build();

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    float rms = buffer->getRMSLevel(0, 100);
    EXPECT_NEAR(rms, 0.5f, 0.001f);
}

// Test: Metadata persistence
TEST_F(CaptureBufferCoreTest, MetadataPersistence)
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
TEST_F(CaptureBufferCoreTest, StereoChannelSeparation)
{
    juce::AudioBuffer<float> testBuffer(2, 100);
    for (int i = 0; i < 100; ++i)
    {
        testBuffer.setSample(0, i, 0.3f); // Left
        testBuffer.setSample(1, i, 0.7f); // Right
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

// Test: Buffer capacity
TEST_F(CaptureBufferCoreTest, BufferCapacity)
{
    EXPECT_EQ(buffer->getCapacity(), 1024);

    auto largeBuffer = std::make_unique<SharedCaptureBuffer>(8192);
    EXPECT_EQ(largeBuffer->getCapacity(), 8192);
}

// Test: Non-power-of-2 capacity is adjusted to power of 2
TEST_F(CaptureBufferCoreTest, NonPowerOfTwoCapacity)
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
TEST_F(CaptureBufferCoreTest, SmallCapacity)
{
    // Capacity of 2 (minimum power of 2 > 1)
    auto smallBuffer = std::make_unique<SharedCaptureBuffer>(2);
    EXPECT_GE(smallBuffer->getCapacity(), 2u);

    // Write and read should still work
    auto testBuf = AudioBufferBuilder().withChannels(2).withSamples(1).withSineWave(440.0f, 0.5f).build();

    CaptureFrameMetadata meta;
    smallBuffer->write(testBuf, meta);

    std::vector<float> output(1);
    int read = smallBuffer->read(output.data(), 1, 0);
    EXPECT_EQ(read, 1);
}

// Test: Capacity of 1 — write and read back correctly
TEST_F(CaptureBufferCoreTest, CapacityOne)
{
    // Bug caught: capacity=1 with power-of-2 masking causes division by zero
    // or infinite loop in wrapPosition
    auto tinyBuffer = std::make_unique<SharedCaptureBuffer>(1);
    EXPECT_GE(tinyBuffer->getCapacity(), 1u);

    auto testBuf = AudioBufferBuilder().withChannels(2).withSamples(1).withDC(0.5f).build();

    CaptureFrameMetadata meta;
    tinyBuffer->write(testBuf, meta);

    // Verify we can read back the written value
    std::vector<float> output(1);
    int read = tinyBuffer->read(output.data(), 1, 0);
    EXPECT_EQ(read, 1);
    EXPECT_NEAR(output[0], 0.5f, 0.001f);
}

// Test: Large capacity
TEST_F(CaptureBufferCoreTest, LargeCapacity)
{
    // 1MB worth of samples (262144 samples at 4 bytes each)
    auto largeBuffer = std::make_unique<SharedCaptureBuffer>(262144);
    EXPECT_EQ(largeBuffer->getCapacity(), 262144u);

    // Should handle large writes
    auto largeBuf = AudioBufferBuilder().withChannels(2).withSamples(8192).withDC(0.5f).build();

    CaptureFrameMetadata meta;
    largeBuffer->write(largeBuf, meta);

    EXPECT_EQ(largeBuffer->getAvailableSamples(), 8192u);
}

// Test: Write with mono buffer (1 channel)
TEST_F(CaptureBufferCoreTest, WriteMonoBuffer)
{
    auto monoBuf = AudioBufferBuilder().withChannels(1).withSamples(100).withDC(0.7f).build();

    CaptureFrameMetadata meta;
    meta.numChannels = 1;
    buffer->write(monoBuf, meta);

    std::vector<float> output(100);
    int read = buffer->read(output.data(), 100, 0);
    EXPECT_EQ(read, 100);
    EXPECT_NEAR(output[50], 0.7f, 0.001f);
}

// Test: Raw write with null channel pointer writes silence for that channel
TEST_F(CaptureBufferCoreTest, WriteNullChannelPointerWritesSilence)
{
    auto initialStereo =
        AudioBufferBuilder().withChannels(2).withSamples(static_cast<int>(buffer->getCapacity())).withDC(0.9f).build();

    CaptureFrameMetadata meta;
    buffer->write(initialStereo, meta);

    std::vector<float> left(64, 0.25f);
    const float* channels[2] = {left.data(), nullptr};
    buffer->write(channels, 64, 2, meta);

    std::vector<float> rightOutput(64, -1.0f);
    const int read = buffer->read(rightOutput.data(), 64, 1);
    ASSERT_EQ(read, 64);

    for (float sample : rightOutput)
    {
        EXPECT_FLOAT_EQ(sample, 0.0f);
    }
}

// Test: Write with empty buffer (0 samples)
TEST_F(CaptureBufferCoreTest, WriteEmptyBuffer)
{
    juce::AudioBuffer<float> emptyBuf(2, 0);
    CaptureFrameMetadata meta;

    // Should not crash
    buffer->write(emptyBuf, meta);

    // Available samples should remain 0
    EXPECT_EQ(buffer->getAvailableSamples(), 0u);
}

// Test: Write with 0 channels does not modify buffer state
TEST_F(CaptureBufferCoreTest, WriteZeroChannels)
{
    auto availBefore = buffer->getAvailableSamples();

    juce::AudioBuffer<float> noChannelBuf(0, 100);
    CaptureFrameMetadata meta;
    meta.numChannels = 0;

    buffer->write(noChannelBuf, meta);

    // Zero-channel write should not add any samples
    EXPECT_EQ(buffer->getAvailableSamples(), availBefore);
}

// Test: Multiple rapid writes
TEST_F(CaptureBufferCoreTest, RapidWrites)
{
    CaptureFrameMetadata meta;

    for (int i = 0; i < 1000; ++i)
    {
        auto buf = generateTestBuffer(16, static_cast<float>(i % 100) / 100.0f);
        buffer->write(buf, meta);
    }

    // Should have data (capped at capacity)
    EXPECT_GT(buffer->getAvailableSamples(), 0u);
    EXPECT_LE(buffer->getAvailableSamples(), buffer->getCapacity());
}

// Test: Write fills buffer exactly
TEST_F(CaptureBufferCoreTest, WriteFillsExactly)
{
    // Buffer is 1024 samples, write exactly 1024
    auto exactBuf = generateTestBuffer(1024, 0.5f);

    CaptureFrameMetadata meta;
    buffer->write(exactBuf, meta);

    EXPECT_EQ(buffer->getAvailableSamples(), 1024u);
}

// Test: Write exceeds capacity — only the last capacity_ samples survive
TEST_F(CaptureBufferCoreTest, WriteExceedsCapacityPreservesLastSamples)
{
    // Bug caught: srcOffset calculation error causing the wrong samples to
    // be stored when write size > capacity
    // Buffer is 1024 samples, write 2048 with a ramp from 0.0 to 1.0
    // The last 1024 samples should be the upper half of the ramp (0.5 to 1.0)
    auto largeBuf = AudioBufferBuilder().withChannels(2).withSamples(2048).withRamp(0.0f, 1.0f).build();

    CaptureFrameMetadata meta;
    buffer->write(largeBuf, meta);

    EXPECT_EQ(buffer->getAvailableSamples(), buffer->getCapacity());

    // Read back and verify we got the LAST 1024 samples (the upper half)
    std::vector<float> output(1024);
    int read = buffer->read(output.data(), 1024, 0);
    ASSERT_EQ(read, 1024);

    // The last 1024 samples of a 2048-sample ramp from 0.0 to 1.0
    // correspond to samples[1024..2047], which span approximately 0.5 to 1.0
    EXPECT_GT(output[0], 0.45f) << "First readable sample should be from upper half of ramp";
    EXPECT_NEAR(output[1023], 1.0f, 0.01f) << "Last readable sample should be near end of ramp";
}

// Test: Read from empty buffer returns 0 and does not modify output
TEST_F(CaptureBufferCoreTest, ReadFromEmptyBuffer)
{
    std::vector<float> output(100, -999.0f);
    int read = buffer->read(output.data(), 100, 0);

    EXPECT_EQ(read, 0);
    // Output should be untouched since no data was available
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ(output[i], -999.0f) << "Read from empty buffer modified output at index " << i;
    }
}

// Test: Read more samples than available — returns exact count with correct data
TEST_F(CaptureBufferCoreTest, ReadMoreThanAvailable)
{
    auto testBuf = generateTestBuffer(50, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    std::vector<float> output(100, -999.0f);
    int read = buffer->read(output.data(), 100, 0);

    EXPECT_EQ(read, 50);
    // First 50 samples should have the written data
    for (int i = 0; i < 50; ++i)
    {
        EXPECT_NEAR(output[i], 0.5f, 0.001f) << "Sample " << i << " incorrect in partial read";
    }
}

// Test: Read more samples than capacity
TEST_F(CaptureBufferCoreTest, ReadMoreThanCapacity)
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

// Test: Read from invalid channel returns 0 samples
TEST_F(CaptureBufferCoreTest, ReadFromInvalidChannel)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    std::vector<float> output(100, -999.0f);

    // Channel 5 doesn't exist (only 0 and 1), should return 0
    int read = buffer->read(output.data(), 100, 5);
    EXPECT_EQ(read, 0);

    // Negative channel should also return 0
    read = buffer->read(output.data(), 100, -1);
    EXPECT_EQ(read, 0);
}

// Test: Read with AudioBuffer overload
TEST_F(CaptureBufferCoreTest, ReadToAudioBuffer)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    juce::AudioBuffer<float> output(2, 100);
    int read = buffer->read(output, 100);

    EXPECT_EQ(read, 100);
    EXPECT_NEAR(output.getSample(0, 50), 0.5f, 0.01f);
}

// Extended data integrity tests in test_capture_buffer_integrity.cpp

// Test: Read with zero samples requested
TEST_F(CaptureBufferCoreTest, ReadZeroSamples)
{
    auto testBuf = generateTestBuffer(100, 0.5f);
    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    std::vector<float> output(10);
    int read = buffer->read(output.data(), 0, 0);

    EXPECT_EQ(read, 0);
}
