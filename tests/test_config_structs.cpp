#include "core/HostInfo.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <gtest/gtest.h>
#include <limits>

using namespace oscil;

// --- HostInfo Tests ---

TEST(HostInfoTest, BPMHelpers)
{
    HostInfo host;
    host.bpm = 120.0f;

    // 120 BPM = 2 beats per sec = 500ms per beat
    EXPECT_DOUBLE_EQ(host.getMsPerBeat(), 500.0);

    host.timeSignature.numerator = 4;
    // 4 beats per bar -> 2000ms per bar
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0);
}

TEST(HostInfoTest, PositionHelpers)
{
    HostInfo host;
    host.timeSignature.numerator = 4;
    host.ppqPosition = 4.5; // Bar 1 (start at 0), Beat 0.5

    EXPECT_DOUBLE_EQ(host.getBarPosition(), 1.125); // 4.5 / 4
    EXPECT_DOUBLE_EQ(host.getBeatInBar(), 0.5);     // 4.5 % 4
}

TEST(HostInfoTest, NonFiniteHelpersReturnSafeDefaults)
{
    HostInfo host;
    host.timeSignature.numerator = 4;

    host.bpm = std::numeric_limits<float>::quiet_NaN();
    EXPECT_DOUBLE_EQ(host.getMsPerBeat(), 500.0);
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0);

    host.bpm = std::numeric_limits<float>::infinity();
    EXPECT_DOUBLE_EQ(host.getMsPerBeat(), 500.0);
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0);

    host.ppqPosition = std::numeric_limits<double>::quiet_NaN();
    EXPECT_DOUBLE_EQ(host.getBarPosition(), 0.0);
    EXPECT_DOUBLE_EQ(host.getBeatInBar(), 0.0);
}

TEST(HostInfoTest, NegativePpqPositionWrapsToPositiveBeat)
{
    HostInfo host;
    host.timeSignature.numerator = 4;

    host.ppqPosition = -1.5;                    // Before the start
    EXPECT_DOUBLE_EQ(host.getBeatInBar(), 2.5); // -1.5 mod 4 = -1.5 + 4 = 2.5

    host.ppqPosition = -4.0; // Exactly one bar back
    EXPECT_DOUBLE_EQ(host.getBeatInBar(), 0.0);
}

TEST(HostInfoTest, ZeroBpmReturnsSafeDefault)
{
    HostInfo host;
    host.bpm = 0.0f;
    EXPECT_DOUBLE_EQ(host.getMsPerBeat(), 500.0); // Default 120 BPM fallback
}

TEST(HostInfoTest, InvalidNumeratorMsPerBarDefaultsTo4_4)
{
    HostInfo host;
    host.bpm = 120.0f;
    host.timeSignature.numerator = 0;
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0); // 500ms * 4 (default 4/4)

    host.timeSignature.numerator = -1;
    EXPECT_DOUBLE_EQ(host.getMsPerBar(), 2000.0);
}

TEST(HostInfoTest, UpdateFromPlayHeadIgnoresNonFiniteBpm)
{
    HostInfo host;
    host.bpm = 130.0f;

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(std::numeric_limits<double>::quiet_NaN());

    host.updateFromPlayHead(posInfo);
    EXPECT_FLOAT_EQ(host.bpm, 130.0f);
}
