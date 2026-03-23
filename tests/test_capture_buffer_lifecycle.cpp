/*
    Oscil - Capture Buffer Tests: Lifecycle
    Tests for buffer allocation, deallocation, clear, and edge cases
*/

#include "core/SharedCaptureBuffer.h"

#include "helpers/AudioBufferBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferLifecycleTest : public ::testing::Test
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

// Test: Write after clear
TEST_F(CaptureBufferLifecycleTest, WriteAfterClear)
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
TEST_F(CaptureBufferLifecycleTest, MultipleClears)
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
