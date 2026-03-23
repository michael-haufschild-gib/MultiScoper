/*
    Oscil - TimingConfig Entity Tests (Edge Cases)
    NaN/infinity, sample rate, multi-bar, extreme BPM, mode switching, multipliers
*/

#include "core/dsp/TimingConfig.h"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

using namespace oscil;

class TimingConfigEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// EDGE CASE TESTS - NaN and Infinity Values
// =============================================================================

TEST_F(TimingConfigEdgeTest, TimeIntervalWithNaN)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    float nan = std::numeric_limits<float>::quiet_NaN();
    config.setTimeInterval(nan);

    // jlimit with NaN behavior - document what happens
    float result = config.timeIntervalMs;
    // NaN propagates through jlimit (implementation detail)
    EXPECT_TRUE(std::isnan(result) || std::isfinite(result));
}

TEST_F(TimingConfigEdgeTest, TimeIntervalWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigEdgeTest, TimeIntervalWithNegativeInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(-std::numeric_limits<float>::infinity());

    // Should be clamped to minimum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigEdgeTest, BPMWithNaN)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setNoteInterval(NoteInterval::QUARTER);

    float nan = std::numeric_limits<float>::quiet_NaN();
    config.setHostBPM(nan);

    // Check behavior with NaN BPM
    float result = config.hostBPM;
    EXPECT_TRUE(std::isnan(result) || std::isfinite(result));
}

TEST_F(TimingConfigEdgeTest, NonFiniteSetterInputsFallbackToDefaults)
{
    TimingConfig config;

    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::DEFAULT_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::DEFAULT_TIME_INTERVAL_MS);
    EXPECT_TRUE(std::isfinite(config.actualIntervalMs));

    config.setTimingMode(TimingMode::MELODIC);
    config.setNoteInterval(NoteInterval::QUARTER);
    config.setHostBPM(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::DEFAULT_BPM);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);
    EXPECT_TRUE(std::isfinite(config.actualIntervalMs));
}

TEST_F(TimingConfigEdgeTest, BPMWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);

    config.setHostBPM(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MAX_BPM);
}

TEST_F(TimingConfigEdgeTest, BPMWithNegativeInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);

    config.setHostBPM(-std::numeric_limits<float>::infinity());

    // Should be clamped to minimum
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MIN_BPM);
}

// =============================================================================
// EDGE CASE TESTS - Sample Rate Edge Cases
// =============================================================================

TEST_F(TimingConfigEdgeTest, IntervalInSamplesZeroSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // With 0 sample rate, result is 0 samples
    EXPECT_EQ(config.getIntervalInSamples(0.0f), 0);
}

TEST_F(TimingConfigEdgeTest, IntervalInSamplesNegativeSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // Negative sample rate produces negative samples
    int result = config.getIntervalInSamples(-44100.0f);
    EXPECT_LT(result, 0);
}

TEST_F(TimingConfigEdgeTest, IntervalInSamplesVeryHighSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f); // 100ms

    // At 192000 Hz, 100ms = 19200 samples
    EXPECT_EQ(config.getIntervalInSamples(192000.0f), 19200);
}

// =============================================================================
// EDGE CASE TESTS - Multi-bar Intervals
// =============================================================================

TEST_F(TimingConfigEdgeTest, MultiBarIntervalTwoBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TWO_BARS);

    // At 120 BPM, 2 bars (8 quarter notes) = 4000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 4000.0f);
}

TEST_F(TimingConfigEdgeTest, MultiBarIntervalFourBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::FOUR_BARS);

    // At 120 BPM, 4 bars (16 quarter notes) = 8000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigEdgeTest, MultiBarIntervalEightBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 120 BPM, 8 bars (32 quarter notes) = 16000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigEdgeTest, MultiBarIntervalClampedToMax)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(20.0f); // Very slow
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 20 BPM, 8 bars = 96000ms, but clamped to 60000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

// =============================================================================
// EDGE CASE TESTS - Extreme BPM Values
// =============================================================================

TEST_F(TimingConfigEdgeTest, VerySlowBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MIN_BPM); // 20 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 20 BPM, quarter note = 3000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 3000.0f);
}

TEST_F(TimingConfigEdgeTest, VeryFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MAX_BPM); // 300 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 300 BPM, quarter note = 200ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 200.0f);
}

TEST_F(TimingConfigEdgeTest, VeryFastNoteWithFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(300.0f); // Max BPM
    config.setNoteInterval(NoteInterval::THIRTY_SECOND);

    // At 300 BPM, 32nd note = 200 * 0.125 = 25ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 25.0f);
}

TEST_F(TimingConfigEdgeTest, VeryFastNoteClampedToMin)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(300.0f);
    config.setNoteInterval(NoteInterval::THIRTY_SECOND);

    // 25ms is above 1ms minimum, so no clamping
    EXPECT_GE(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
}

// =============================================================================
// EDGE CASE TESTS - Twelve Note Interval
// =============================================================================

TEST_F(TimingConfigEdgeTest, TwelfthNoteInterval)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TWELFTH);

    // 1/12 = 1/3 of a quarter note = 500 * (1/3) = 166.67ms
    float expected = 500.0f / 3.0f;
    EXPECT_NEAR(config.actualIntervalMs, expected, 0.01f);
}

// =============================================================================
// EDGE CASE TESTS - Mode Switching
// =============================================================================

TEST_F(TimingConfigEdgeTest, SwitchFromTimeToMelodic)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 100.0f);

    // Switch to MELODIC
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);

    // Should now use melodic calculation
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);
}

TEST_F(TimingConfigEdgeTest, SwitchFromMelodicToTime)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);

    // Switch to TIME
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // Should now use time-based calculation
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 100.0f);
}

TEST_F(TimingConfigEdgeTest, TimeIntervalChangeInMelodicModeIgnored)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);
    float melodicInterval = config.actualIntervalMs;

    // Change time interval while in MELODIC mode
    config.setTimeInterval(100.0f);

    // actualIntervalMs should not change (still using melodic)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, melodicInterval);
}

TEST_F(TimingConfigEdgeTest, NoteIntervalChangeInTimeModeIgnored)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);
    float timeInterval = config.actualIntervalMs;

    // Change note interval while in TIME mode
    config.setNoteInterval(NoteInterval::WHOLE);

    // actualIntervalMs should not change (still using time)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, timeInterval);
}

// =============================================================================
// EDGE CASE TESTS - All Note Interval Multipliers
// =============================================================================

TEST_F(TimingConfigEdgeTest, AllMultiBarMultipliers)
{
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWO_BARS), 8.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::THREE_BARS), 12.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::FOUR_BARS), 16.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::EIGHT_BARS), 32.0f);
}

TEST_F(TimingConfigEdgeTest, TwelfthNoteMultiplier)
{
    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWELFTH), 1.0f / 3.0f, 0.001f);
}

// =============================================================================
// EDGE CASE TESTS - All Note Interval String Conversions
// =============================================================================

TEST_F(TimingConfigEdgeTest, AllNoteIntervalStrings)
{
    // Standard notes
    EXPECT_EQ(noteIntervalToString(NoteInterval::THIRTY_SECOND), "1/32");
    EXPECT_EQ(noteIntervalToString(NoteInterval::TWELFTH), "1/12");
    EXPECT_EQ(noteIntervalToString(NoteInterval::SIXTEENTH), "1/16");
    EXPECT_EQ(noteIntervalToString(NoteInterval::HALF), "1/2");
    EXPECT_EQ(noteIntervalToString(NoteInterval::WHOLE), "1/1");

    // Multi-bar
    EXPECT_EQ(noteIntervalToString(NoteInterval::TWO_BARS), "2 Bars");
    EXPECT_EQ(noteIntervalToString(NoteInterval::THREE_BARS), "3 Bars");
    EXPECT_EQ(noteIntervalToString(NoteInterval::FOUR_BARS), "4 Bars");
    EXPECT_EQ(noteIntervalToString(NoteInterval::EIGHT_BARS), "8 Bars");

    // Dotted
    EXPECT_EQ(noteIntervalToString(NoteInterval::DOTTED_EIGHTH), "1/8.");
    EXPECT_EQ(noteIntervalToString(NoteInterval::DOTTED_HALF), "1/2.");

    // Triplet
    EXPECT_EQ(noteIntervalToString(NoteInterval::TRIPLET_EIGHTH), "1/8T");
    EXPECT_EQ(noteIntervalToString(NoteInterval::TRIPLET_HALF), "1/2T");
}

TEST_F(TimingConfigEdgeTest, AllNoteIntervalFromStrings)
{
    EXPECT_EQ(stringToNoteInterval("1/32"), NoteInterval::THIRTY_SECOND);
    EXPECT_EQ(stringToNoteInterval("1/12"), NoteInterval::TWELFTH);
    EXPECT_EQ(stringToNoteInterval("1/1"), NoteInterval::WHOLE);
    EXPECT_EQ(stringToNoteInterval("2 Bars"), NoteInterval::TWO_BARS);
    EXPECT_EQ(stringToNoteInterval("3 Bars"), NoteInterval::THREE_BARS);
    EXPECT_EQ(stringToNoteInterval("4 Bars"), NoteInterval::FOUR_BARS);
    EXPECT_EQ(stringToNoteInterval("8 Bars"), NoteInterval::EIGHT_BARS);
    EXPECT_EQ(stringToNoteInterval("1/8."), NoteInterval::DOTTED_EIGHTH);
    EXPECT_EQ(stringToNoteInterval("1/2."), NoteInterval::DOTTED_HALF);
    EXPECT_EQ(stringToNoteInterval("1/8T"), NoteInterval::TRIPLET_EIGHTH);
    EXPECT_EQ(stringToNoteInterval("1/2T"), NoteInterval::TRIPLET_HALF);
}

TEST_F(TimingConfigEdgeTest, AllNoteIntervalDisplayNames)
{
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::THIRTY_SECOND), "32nd Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::SIXTEENTH), "16th Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TWELFTH), "12th Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::HALF), "Half Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::WHOLE), "Whole Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TWO_BARS), "2 Bars");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::THREE_BARS), "3 Bars");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::FOUR_BARS), "4 Bars");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::EIGHT_BARS), "8 Bars");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::DOTTED_EIGHTH), "Dotted 8th");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::DOTTED_HALF), "Dotted Half");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TRIPLET_EIGHTH), "8th Triplet");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TRIPLET_HALF), "Half Triplet");
}
