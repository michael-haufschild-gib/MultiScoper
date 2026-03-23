/*
    Oscil - Capture Buffer Tests: Metadata, Peak/RMS Edge Cases, Write Position
*/

#include "core/SharedCaptureBuffer.h"

#include "helpers/AudioBufferBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferMetadataTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override { buffer = std::make_unique<SharedCaptureBuffer>(1024); }

    juce::AudioBuffer<float> generateTestBuffer(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

// Test: Peak level on empty buffer
TEST_F(CaptureBufferMetadataTest, PeakLevelEmptyBuffer)
{
    float peak = buffer->getPeakLevel(0, 100);
    // Should return 0 for empty buffer
    EXPECT_FLOAT_EQ(peak, 0.0f);
}

// Test: RMS level on empty buffer
TEST_F(CaptureBufferMetadataTest, RMSLevelEmptyBuffer)
{
    float rms = buffer->getRMSLevel(0, 100);
    // Should return 0 for empty buffer
    EXPECT_FLOAT_EQ(rms, 0.0f);
}

// Test: Peak level with negative values
TEST_F(CaptureBufferMetadataTest, PeakLevelNegativeValues)
{
    juce::AudioBuffer<float> testBuf(2, 100);
    testBuf.clear();
    testBuf.setSample(0, 50, -0.9f); // Negative peak

    CaptureFrameMetadata meta;
    buffer->write(testBuf, meta);

    float peak = buffer->getPeakLevel(0, 100);
    // Peak should be absolute value
    EXPECT_NEAR(peak, 0.9f, 0.001f);
}

// Test: RMS with alternating positive/negative
TEST_F(CaptureBufferMetadataTest, RMSAlternatingValues)
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
TEST_F(CaptureBufferMetadataTest, PeakRMSMoreThanAvailable)
{
    auto testBuf = AudioBufferBuilder().withChannels(2).withSamples(50).withDC(0.5f).build();

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
TEST_F(CaptureBufferMetadataTest, PeakRMSInvalidChannel)
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

// Test: Metadata with extreme values
TEST_F(CaptureBufferMetadataTest, MetadataExtremeValues)
{
    auto testBuf = generateTestBuffer(100, 0.5f);

    CaptureFrameMetadata meta;
    meta.sampleRate = 192000.0;
    meta.numChannels = 64; // This will be overwritten by actual buffer channels
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
TEST_F(CaptureBufferMetadataTest, MetadataBeforeWrite)
{
    auto meta = buffer->getLatestMetadata();

    // Should return default values
    EXPECT_GT(meta.sampleRate, 0.0);
}

// Test: Get write position
TEST_F(CaptureBufferMetadataTest, WritePositionTracking)
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
TEST_F(CaptureBufferMetadataTest, WritePositionWrapping)
{
    // Write exactly capacity
    auto fullBuf = generateTestBuffer(1024, 0.5f);

    CaptureFrameMetadata meta;
    buffer->write(fullBuf, meta);

    // Position should wrap to 0 (or be at capacity)
    size_t pos = buffer->getWritePosition();
    EXPECT_LE(pos, buffer->getCapacity());

    // Write one more sample
    auto oneSample = AudioBufferBuilder().withChannels(2).withSamples(1).withDC(0.9f).build();
    buffer->write(oneSample, meta);

    // Position should have advanced and possibly wrapped
    size_t newPos = buffer->getWritePosition();
    EXPECT_LE(newPos, buffer->getCapacity());
}
