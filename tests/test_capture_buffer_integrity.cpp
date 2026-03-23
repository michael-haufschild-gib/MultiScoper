/*
    Oscil - Capture Buffer Tests: Data Integrity
    Verifies exact sample values after ring wraps, channel separation,
    clear-then-write sequences, and metadata update correctness.
*/

#include "core/SharedCaptureBuffer.h"

#include "helpers/AudioBufferBuilder.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class CaptureBufferIntegrityTest : public ::testing::Test
{
protected:
    std::unique_ptr<SharedCaptureBuffer> buffer;

    void SetUp() override { buffer = std::make_unique<SharedCaptureBuffer>(1024); }

    juce::AudioBuffer<float> generateDC(int numSamples, float value)
    {
        return AudioBufferBuilder().withChannels(2).withSamples(numSamples).withDC(value).build();
    }
};

TEST_F(CaptureBufferIntegrityTest, MultipleWrapsPreserveExactSampleValues)
{
    // Off-by-one in wrapPosition after multiple wraps
    constexpr int kBlock = 300; // not factor of 1024 → split writes
    float lastDC = 0.0f;
    for (int b = 0; b < 10; ++b)
    {
        lastDC = static_cast<float>(b) * 0.1f + 0.05f;
        buffer->write(generateDC(kBlock, lastDC), CaptureFrameMetadata{});
    }

    std::vector<float> out(kBlock);
    ASSERT_EQ(buffer->read(out.data(), kBlock, 0), kBlock);
    for (int i = 0; i < kBlock; ++i)
        EXPECT_NEAR(out[i], lastDC, 0.001f) << "Sample " << i << " wrong after wraps";
}

TEST_F(CaptureBufferIntegrityTest, StereoChannelSeparationAllSamples)
{
    // Channel offset error causing L/R crosstalk
    constexpr int kN = 200;
    juce::AudioBuffer<float> buf(2, kN);
    for (int i = 0; i < kN; ++i)
    {
        buf.setSample(0, i, 0.3f);
        buf.setSample(1, i, -0.4f);
    }
    buffer->write(buf, CaptureFrameMetadata{});

    std::vector<float> L(kN), R(kN);
    ASSERT_EQ(buffer->read(L.data(), kN, 0), kN);
    ASSERT_EQ(buffer->read(R.data(), kN, 1), kN);

    for (int i = 0; i < kN; ++i)
    {
        EXPECT_NEAR(L[i], 0.3f, 0.001f) << "L[" << i << "] crosstalk";
        EXPECT_NEAR(R[i], -0.4f, 0.001f) << "R[" << i << "] crosstalk";
    }
}

TEST_F(CaptureBufferIntegrityTest, ClearProducesSilenceOnRead)
{
    // clear() resets counters but doesn't zero buffer → stale data bleeds through
    buffer->write(generateDC(512, 0.9f), CaptureFrameMetadata{});
    buffer->clear();

    std::vector<float> out(512, -999.0f);
    EXPECT_EQ(buffer->read(out.data(), 512, 0), 0);

    // Write new data and verify no bleed-through
    CaptureFrameMetadata m;
    buffer->write(generateDC(100, 0.1f), m);

    std::vector<float> fresh(100);
    ASSERT_EQ(buffer->read(fresh.data(), 100, 0), 100);
    for (int i = 0; i < 100; ++i)
        EXPECT_NEAR(fresh[i], 0.1f, 0.001f) << "Stale data at " << i;
}

TEST_F(CaptureBufferIntegrityTest, MetadataUpdatedByWrite)
{
    // numSamples/numChannels not updated by write
    CaptureFrameMetadata m;
    m.sampleRate = 48000.0;
    m.numChannels = 99; // wrong — write should clamp
    m.numSamples = 0;   // wrong — write should set

    buffer->write(generateDC(256, 0.5f), m);
    auto ret = buffer->getLatestMetadata();

    EXPECT_EQ(ret.numSamples, 256);
    EXPECT_EQ(ret.numChannels, 2);
    EXPECT_EQ(ret.sampleRate, 48000.0);
}

// ============================================================================
// Peak/RMS accuracy across ring wrap boundary
// ============================================================================

TEST_F(CaptureBufferIntegrityTest, PeakLevelAccurateAcrossRingWrap)
{
    // Bug caught: getPeakLevel reads the wrong chunk when readStart + count
    // wraps around the ring buffer boundary, causing stale data to be analyzed.
    constexpr int kBlock = 300; // not a factor of 1024, forces wraps

    // Fill buffer by writing multiple blocks so writePos wraps
    for (int i = 0; i < 5; ++i)
        buffer->write(generateDC(kBlock, 0.1f), CaptureFrameMetadata{});

    // Now write a block with a known peak
    juce::AudioBuffer<float> peakBuf(2, kBlock);
    peakBuf.clear();
    peakBuf.setSample(0, kBlock / 2, 0.95f); // single spike
    buffer->write(peakBuf, CaptureFrameMetadata{});

    float peak = buffer->getPeakLevel(0, kBlock);
    EXPECT_NEAR(peak, 0.95f, 0.001f) << "Peak missed after ring wrap";
}

TEST_F(CaptureBufferIntegrityTest, RMSAccurateForKnownSignal)
{
    // RMS of a DC signal at level L should be exactly L
    constexpr float kLevel = 0.7f;
    buffer->write(generateDC(512, kLevel), CaptureFrameMetadata{});

    float rms = buffer->getRMSLevel(0, 512);
    EXPECT_NEAR(rms, kLevel, 0.001f) << "RMS of DC signal should equal the DC level";
}

TEST_F(CaptureBufferIntegrityTest, PeakLevelAllNegativeSignal)
{
    // Bug caught: peak calculation uses max() instead of abs(), missing negative peaks
    juce::AudioBuffer<float> negBuf(2, 100);
    for (int i = 0; i < 100; ++i)
    {
        negBuf.setSample(0, i, -0.6f);
        negBuf.setSample(1, i, -0.6f);
    }
    buffer->write(negBuf, CaptureFrameMetadata{});

    float peak = buffer->getPeakLevel(0, 100);
    EXPECT_NEAR(peak, 0.6f, 0.001f) << "Peak must use absolute value for negative signals";
}

// ============================================================================
// Write with excess channels — only MAX_CHANNELS stored
// ============================================================================

TEST_F(CaptureBufferIntegrityTest, WriteExcessChannelsCapsAtMax)
{
    // Bug caught: writing a 4-channel buffer into a 2-channel ring buffer
    // causes out-of-bounds access in the channel loop
    juce::AudioBuffer<float> fourCh(4, 64);
    for (int ch = 0; ch < 4; ++ch)
        for (int i = 0; i < 64; ++i)
            fourCh.setSample(ch, i, static_cast<float>(ch) * 0.25f);

    CaptureFrameMetadata meta{};
    buffer->write(fourCh, meta);

    // Only channels 0 and 1 should be stored
    std::vector<float> ch0(64), ch1(64);
    ASSERT_EQ(buffer->read(ch0.data(), 64, 0), 64);
    ASSERT_EQ(buffer->read(ch1.data(), 64, 1), 64);

    EXPECT_NEAR(ch0[0], 0.0f, 0.001f);  // channel 0: 0 * 0.25
    EXPECT_NEAR(ch1[0], 0.25f, 0.001f); // channel 1: 1 * 0.25

    // Metadata should report capped channel count
    auto retMeta = buffer->getLatestMetadata();
    EXPECT_EQ(retMeta.numChannels, 2);
}

// ============================================================================
// Sequential writes accumulate correctly in available samples
// ============================================================================

TEST_F(CaptureBufferIntegrityTest, AvailableSamplesAccumulatesCorrectly)
{
    // Bug caught: samplesWritten_ overflow or incorrect capping
    for (int i = 0; i < 5; ++i)
        buffer->write(generateDC(100, 0.5f), CaptureFrameMetadata{});

    EXPECT_EQ(buffer->getAvailableSamples(), 500u);

    // Write enough to exceed capacity
    buffer->write(generateDC(600, 0.5f), CaptureFrameMetadata{});

    // Available should be capped at capacity
    EXPECT_EQ(buffer->getAvailableSamples(), buffer->getCapacity());
}

// ============================================================================
// AudioBuffer read returns consistent cross-channel data
// ============================================================================

TEST_F(CaptureBufferIntegrityTest, AudioBufferReadCrossChannelConsistency)
{
    // Bug caught: individual per-channel reads may span different write epochs,
    // causing L/R desync. The AudioBuffer overload must read all channels
    // within a single epoch validation.
    constexpr int kN = 256;

    // Write stereo data where L and R have a known relationship
    juce::AudioBuffer<float> stereo(2, kN);
    for (int i = 0; i < kN; ++i)
    {
        float val = static_cast<float>(i) / static_cast<float>(kN);
        stereo.setSample(0, i, val);
        stereo.setSample(1, i, 1.0f - val); // L + R = 1.0 at every sample
    }
    buffer->write(stereo, CaptureFrameMetadata{});

    juce::AudioBuffer<float> output(2, kN);
    int read = buffer->read(output, kN);
    ASSERT_EQ(read, kN);

    for (int i = 0; i < kN; ++i)
    {
        float sum = output.getSample(0, i) + output.getSample(1, i);
        EXPECT_NEAR(sum, 1.0f, 0.001f) << "Cross-channel invariant violated at sample " << i
                                       << " (L=" << output.getSample(0, i) << " R=" << output.getSample(1, i) << ")";
    }
}
