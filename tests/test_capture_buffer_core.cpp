/*
    Oscil - Capture Buffer Tests: Core Operations
    Tests for basic buffer operations, capacity, peak/RMS, and metadata
*/

#include <gtest/gtest.h>
#include "helpers/AudioBufferBuilder.h"
#include "core/SharedCaptureBuffer.h"

using namespace oscil;
using namespace oscil::test;

class CaptureBufferCoreTest : public ::testing::Test
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

// Test: Ring buffer wrapping
TEST_F(CaptureBufferCoreTest, RingBufferWrapping)
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
    testBuffer.setSample(0, 50, 0.8f);  // Peak in left channel

    CaptureFrameMetadata metadata;
    buffer->write(testBuffer, metadata);

    float peak = buffer->getPeakLevel(0, 100);
    EXPECT_NEAR(peak, 0.8f, 0.001f);
}

// Test: RMS level calculation
TEST_F(CaptureBufferCoreTest, RMSLevel)
{
    auto testBuffer = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(100)
        .withDC(0.5f)
        .build();

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
    auto testBuf = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(1)
        .withSineWave(440.0f, 0.5f)
        .build();

    CaptureFrameMetadata meta;
    smallBuffer->write(testBuf, meta);

    std::vector<float> output(1);
    int read = smallBuffer->read(output.data(), 1, 0);
    EXPECT_EQ(read, 1);
}

// Test: Capacity of 1 (edge case)
TEST_F(CaptureBufferCoreTest, CapacityOne)
{
    auto tinyBuffer = std::make_unique<SharedCaptureBuffer>(1);
    // Should be at least 1 (may round up to minimum power of 2)
    EXPECT_GE(tinyBuffer->getCapacity(), 1u);

    // Should not crash on operations
    auto testBuf = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(1)
        .withDC(0.5f)
        .build();

    CaptureFrameMetadata meta;
    tinyBuffer->write(testBuf, meta);
}

// Test: Large capacity
TEST_F(CaptureBufferCoreTest, LargeCapacity)
{
    // 1MB worth of samples (262144 samples at 4 bytes each)
    auto largeBuffer = std::make_unique<SharedCaptureBuffer>(262144);
    EXPECT_EQ(largeBuffer->getCapacity(), 262144u);

    // Should handle large writes
    auto largeBuf = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(8192)
        .withDC(0.5f)
        .build();

    CaptureFrameMetadata meta;
    largeBuffer->write(largeBuf, meta);

    EXPECT_EQ(largeBuffer->getAvailableSamples(), 8192u);
}

// Test: Write with mono buffer (1 channel)
TEST_F(CaptureBufferCoreTest, WriteMonoBuffer)
{
    auto monoBuf = AudioBufferBuilder()
        .withChannels(1)
        .withSamples(100)
        .withDC(0.7f)
        .build();

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
    auto initialStereo = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(static_cast<int>(buffer->getCapacity()))
        .withDC(0.9f)
        .build();

    CaptureFrameMetadata meta;
    buffer->write(initialStereo, meta);

    std::vector<float> left(64, 0.25f);
    const float* channels[2] = { left.data(), nullptr };
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

// Test: Write exceeds capacity
TEST_F(CaptureBufferCoreTest, WriteExceedsCapacity)
{
    // Buffer is 1024 samples, write 2048
    auto largeBuf = AudioBufferBuilder()
        .withChannels(2)
        .withSamples(2048)
        .withRamp(0.0f, 1.0f)
        .build();

    CaptureFrameMetadata meta;
    buffer->write(largeBuf, meta);

    // Available samples capped at capacity
    EXPECT_LE(buffer->getAvailableSamples(), buffer->getCapacity());
}

// Test: Read from empty buffer
TEST_F(CaptureBufferCoreTest, ReadFromEmptyBuffer)
{
    std::vector<float> output(100);
    int read = buffer->read(output.data(), 100, 0);

    // Should return 0 or handle gracefully
    EXPECT_GE(read, 0);
}

// Test: Read more samples than available
TEST_F(CaptureBufferCoreTest, ReadMoreThanAvailable)
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

// Test: Read from invalid channel index
TEST_F(CaptureBufferCoreTest, ReadFromInvalidChannel)
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

