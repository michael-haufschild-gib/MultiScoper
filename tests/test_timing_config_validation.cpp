/*
    Oscil - TimingConfig Validation Tests
    Tests for configuration validation, bounds checking, and mode calculations
*/

#include <gtest/gtest.h>
#include "core/dsp/TimingConfig.h"
#include "helpers/TimingConfigBuilder.h"
#include <limits>
#include <cmath>

using namespace oscil;
using namespace oscil::test;

class TimingConfigValidationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

// === Default Values Tests ===

TEST_F(TimingConfigValidationTest, DefaultValues)
{
    TimingConfig config;

    EXPECT_EQ(config.timingMode, TimingMode::TIME);
    EXPECT_EQ(config.triggerMode, TriggerMode::FREE_RUNNING);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, 500.0f);  // New default: 500ms
    EXPECT_EQ(config.noteInterval, NoteInterval::QUARTER);
    EXPECT_FALSE(config.hostSyncEnabled);
    EXPECT_FLOAT_EQ(config.hostBPM, 120.0f);
}

// === TIME Mode Tests ===

TEST_F(TimingConfigValidationTest, TimeModeCalculation)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    EXPECT_FLOAT_EQ(config.actualIntervalMs, 100.0f);
}

TEST_F(TimingConfigValidationTest, TimeIntervalClamping)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    // Test minimum clamping
    config.setTimeInterval(0.1f);  // Below minimum
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);

    // Test maximum clamping
    config.setTimeInterval(100000.0f);  // Above maximum
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

// === MELODIC Mode Tests ===

TEST_F(TimingConfigValidationTest, MelodicModeQuarterNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 120 BPM, quarter note = 500ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeWholeNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::WHOLE);

    // At 120 BPM, whole note = 2000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 2000.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeHalfNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::HALF);

    // At 120 BPM, half note = 1000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 1000.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeEighthNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHTH);

    // At 120 BPM, eighth note = 250ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 250.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeSixteenthNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::SIXTEENTH);

    // At 120 BPM, sixteenth note = 125ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 125.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeDottedQuarter)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::DOTTED_QUARTER);

    // At 120 BPM, dotted quarter = 500 * 1.5 = 750ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 750.0f);
}

TEST_F(TimingConfigValidationTest, MelodicModeTripletQuarter)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TRIPLET_QUARTER);

    // At 120 BPM, quarter triplet = 500 * (2/3) = 333.33ms
    float expected = 500.0f * (2.0f / 3.0f);
    EXPECT_NEAR(config.actualIntervalMs, expected, 0.01f);
}

TEST_F(TimingConfigValidationTest, BPMChangeRecalculation)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setNoteInterval(NoteInterval::QUARTER);

    // 120 BPM -> 500ms
    config.setHostBPM(120.0f);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);

    // 60 BPM -> 1000ms
    config.setHostBPM(60.0f);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 1000.0f);

    // 240 BPM -> 250ms
    config.setHostBPM(240.0f);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 250.0f);
}

TEST_F(TimingConfigValidationTest, BPMClamping)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setNoteInterval(NoteInterval::QUARTER);

    // Below minimum
    config.setHostBPM(5.0f);
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MIN_BPM);

    // Above maximum
    config.setHostBPM(500.0f);
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MAX_BPM);
}

// === Samples Conversion Tests ===

TEST_F(TimingConfigValidationTest, IntervalInSamples)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);  // 100ms

    // At 44100 Hz, 100ms = 4410 samples
    EXPECT_EQ(config.getIntervalInSamples(44100.0f), 4410);

    // At 48000 Hz, 100ms = 4800 samples
    EXPECT_EQ(config.getIntervalInSamples(48000.0f), 4800);

    // At 96000 Hz, 100ms = 9600 samples
    EXPECT_EQ(config.getIntervalInSamples(96000.0f), 9600);
}

// === Note Interval Multiplier Tests ===

TEST_F(TimingConfigValidationTest, NoteIntervalMultipliers)
{
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::WHOLE), 4.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::HALF), 2.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::QUARTER), 1.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::EIGHTH), 0.5f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::SIXTEENTH), 0.25f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::THIRTY_SECOND), 0.125f);

    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::DOTTED_HALF), 3.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::DOTTED_QUARTER), 1.5f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::DOTTED_EIGHTH), 0.75f);

    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TRIPLET_HALF), 4.0f / 3.0f, 0.001f);
    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TRIPLET_QUARTER), 2.0f / 3.0f, 0.001f);
    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TRIPLET_EIGHTH), 1.0f / 3.0f, 0.001f);
}

// === Host Sync Availability Tests ===

TEST_F(TimingConfigValidationTest, HostSyncAvailability)
{
    TimingConfig config;

    config.hostBPM = 120.0f;
    EXPECT_TRUE(config.isHostSyncAvailable());

    config.hostBPM = 10.0f;  // Below minimum
    EXPECT_FALSE(config.isHostSyncAvailable());

    config.hostBPM = 400.0f;  // Above maximum
    EXPECT_FALSE(config.isHostSyncAvailable());
}

// === Multi-bar Interval Tests ===

TEST_F(TimingConfigValidationTest, MultiBarIntervalTwoBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TWO_BARS);

    // At 120 BPM, 2 bars (8 quarter notes) = 4000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 4000.0f);
}

TEST_F(TimingConfigValidationTest, MultiBarIntervalFourBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::FOUR_BARS);

    // At 120 BPM, 4 bars (16 quarter notes) = 8000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigValidationTest, MultiBarIntervalEightBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 120 BPM, 8 bars (32 quarter notes) = 16000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigValidationTest, MultiBarIntervalClampedToMax)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(20.0f);  // Very slow
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 20 BPM, 8 bars = 96000ms, but clamped to 60000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

// === Extreme BPM Tests ===

TEST_F(TimingConfigValidationTest, VerySlowBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MIN_BPM);  // 20 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 20 BPM, quarter note = 3000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 3000.0f);
}

TEST_F(TimingConfigValidationTest, VeryFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MAX_BPM);  // 300 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 300 BPM, quarter note = 200ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 200.0f);
}

TEST_F(TimingConfigValidationTest, VeryFastNoteWithFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(300.0f);  // Max BPM
    config.setNoteInterval(NoteInterval::THIRTY_SECOND);

    // At 300 BPM, 32nd note = 200 * 0.125 = 25ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 25.0f);
}

TEST_F(TimingConfigValidationTest, VeryFastNoteClampedToMin)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(300.0f);
    config.setNoteInterval(NoteInterval::THIRTY_SECOND);

    // 25ms is above 1ms minimum, so no clamping
    EXPECT_GE(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
}

// === Twelfth Note Interval Tests ===

TEST_F(TimingConfigValidationTest, TwelfthNoteInterval)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TWELFTH);

    // 1/12 = 1/3 of a quarter note = 500 * (1/3) = 166.67ms
    float expected = 500.0f / 3.0f;
    EXPECT_NEAR(config.actualIntervalMs, expected, 0.01f);
}

// === All Multi-bar Multipliers Tests ===

TEST_F(TimingConfigValidationTest, AllMultiBarMultipliers)
{
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWO_BARS), 8.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::THREE_BARS), 12.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::FOUR_BARS), 16.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::EIGHT_BARS), 32.0f);
}

TEST_F(TimingConfigValidationTest, TwelfthNoteMultiplier)
{
    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWELFTH), 1.0f / 3.0f, 0.001f);
}
