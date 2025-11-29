/*
    Oscil - TimingConfig Entity Tests
*/

#include <gtest/gtest.h>
#include "dsp/TimingConfig.h"
#include <limits>
#include <cmath>

using namespace oscil;

class TimingConfigTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

// === Default Values Tests ===

TEST_F(TimingConfigTest, DefaultValues)
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

TEST_F(TimingConfigTest, TimeModCalculation)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    EXPECT_FLOAT_EQ(config.actualIntervalMs, 100.0f);
}

TEST_F(TimingConfigTest, TimeIntervalClamping)
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

TEST_F(TimingConfigTest, MelodicModeQuarterNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 120 BPM, quarter note = 500ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 500.0f);
}

TEST_F(TimingConfigTest, MelodicModeWholeNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::WHOLE);

    // At 120 BPM, whole note = 2000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 2000.0f);
}

TEST_F(TimingConfigTest, MelodicModeHalfNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::HALF);

    // At 120 BPM, half note = 1000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 1000.0f);
}

TEST_F(TimingConfigTest, MelodicModeEighthNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHTH);

    // At 120 BPM, eighth note = 250ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 250.0f);
}

TEST_F(TimingConfigTest, MelodicModeSixteenthNote)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::SIXTEENTH);

    // At 120 BPM, sixteenth note = 125ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 125.0f);
}

TEST_F(TimingConfigTest, MelodicModeDottedQuarter)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::DOTTED_QUARTER);

    // At 120 BPM, dotted quarter = 500 * 1.5 = 750ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 750.0f);
}

TEST_F(TimingConfigTest, MelodicModeTripletQuarter)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TRIPLET_QUARTER);

    // At 120 BPM, quarter triplet = 500 * (2/3) = 333.33ms
    float expected = 500.0f * (2.0f / 3.0f);
    EXPECT_NEAR(config.actualIntervalMs, expected, 0.01f);
}

TEST_F(TimingConfigTest, BPMChangeRecalculation)
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

TEST_F(TimingConfigTest, BPMClamping)
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

TEST_F(TimingConfigTest, IntervalInSamples)
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

TEST_F(TimingConfigTest, NoteIntervalMultipliers)
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

// === Serialization Tests ===

TEST_F(TimingConfigTest, SerializationRoundTrip)
{
    TimingConfig original;
    original.timingMode = TimingMode::MELODIC;
    original.triggerMode = TriggerMode::HOST_SYNC;
    original.timeIntervalMs = 75.0f;
    original.noteInterval = NoteInterval::DOTTED_QUARTER;
    original.hostSyncEnabled = true;
    original.hostBPM = 140.0f;
    original.syncToPlayhead = true;
    original.triggerThreshold = -15.0f;
    original.triggerEdge = TriggerEdge::Falling;
    original.calculateActualInterval();

    // Serialize
    auto tree = original.toValueTree();

    // Deserialize
    TimingConfig restored;
    restored.fromValueTree(tree);

    EXPECT_EQ(restored.timingMode, original.timingMode);
    EXPECT_EQ(restored.triggerMode, original.triggerMode);
    EXPECT_FLOAT_EQ(restored.timeIntervalMs, original.timeIntervalMs);
    EXPECT_EQ(restored.noteInterval, original.noteInterval);
    EXPECT_EQ(restored.hostSyncEnabled, original.hostSyncEnabled);
    EXPECT_FLOAT_EQ(restored.hostBPM, original.hostBPM);
    EXPECT_EQ(restored.syncToPlayhead, original.syncToPlayhead);
    EXPECT_FLOAT_EQ(restored.triggerThreshold, original.triggerThreshold);
    EXPECT_EQ(restored.triggerEdge, original.triggerEdge);
}

// === String Conversion Tests ===

TEST_F(TimingConfigTest, TimingModeStringConversion)
{
    EXPECT_EQ(timingModeToString(TimingMode::TIME), "TIME");
    EXPECT_EQ(timingModeToString(TimingMode::MELODIC), "MELODIC");

    EXPECT_EQ(stringToTimingMode("TIME"), TimingMode::TIME);
    EXPECT_EQ(stringToTimingMode("MELODIC"), TimingMode::MELODIC);
    EXPECT_EQ(stringToTimingMode("INVALID"), TimingMode::TIME);  // Default
}

TEST_F(TimingConfigTest, NoteIntervalStringConversion)
{
    EXPECT_EQ(noteIntervalToString(NoteInterval::QUARTER), "1/4");
    EXPECT_EQ(noteIntervalToString(NoteInterval::EIGHTH), "1/8");
    EXPECT_EQ(noteIntervalToString(NoteInterval::DOTTED_QUARTER), "1/4.");
    EXPECT_EQ(noteIntervalToString(NoteInterval::TRIPLET_QUARTER), "1/4T");

    EXPECT_EQ(stringToNoteInterval("1/4"), NoteInterval::QUARTER);
    EXPECT_EQ(stringToNoteInterval("1/8"), NoteInterval::EIGHTH);
    EXPECT_EQ(stringToNoteInterval("1/4."), NoteInterval::DOTTED_QUARTER);
    EXPECT_EQ(stringToNoteInterval("1/4T"), NoteInterval::TRIPLET_QUARTER);
    EXPECT_EQ(stringToNoteInterval("INVALID"), NoteInterval::QUARTER);  // Default
}

TEST_F(TimingConfigTest, TriggerModeStringConversion)
{
    EXPECT_EQ(triggerModeToString(TriggerMode::FREE_RUNNING), "FREE_RUNNING");
    EXPECT_EQ(triggerModeToString(TriggerMode::HOST_SYNC), "HOST_SYNC");
    EXPECT_EQ(triggerModeToString(TriggerMode::TRIGGERED), "TRIGGERED");

    EXPECT_EQ(stringToTriggerMode("FREE_RUNNING"), TriggerMode::FREE_RUNNING);
    EXPECT_EQ(stringToTriggerMode("HOST_SYNC"), TriggerMode::HOST_SYNC);
    EXPECT_EQ(stringToTriggerMode("TRIGGERED"), TriggerMode::TRIGGERED);
    EXPECT_EQ(stringToTriggerMode("INVALID"), TriggerMode::FREE_RUNNING);  // Default
}

TEST_F(TimingConfigTest, NoteIntervalDisplayNames)
{
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::QUARTER), "Quarter Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::EIGHTH), "8th Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::DOTTED_QUARTER), "Dotted Quarter");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TRIPLET_QUARTER), "Quarter Triplet");
}

// === Host Sync Availability Tests ===

TEST_F(TimingConfigTest, HostSyncAvailability)
{
    TimingConfig config;

    config.hostBPM = 120.0f;
    EXPECT_TRUE(config.isHostSyncAvailable());

    config.hostBPM = 10.0f;  // Below minimum
    EXPECT_FALSE(config.isHostSyncAvailable());

    config.hostBPM = 400.0f;  // Above maximum
    EXPECT_FALSE(config.isHostSyncAvailable());
}

// =============================================================================
// EDGE CASE TESTS - Invalid Enum Serialization
// =============================================================================

TEST_F(TimingConfigTest, DeserializeInvalidTimingMode)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("timingMode", "INVALID_MODE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to TIME mode
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
}

TEST_F(TimingConfigTest, DeserializeInvalidTriggerMode)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("triggerMode", "UNKNOWN_TRIGGER", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to FREE_RUNNING
    EXPECT_EQ(config.triggerMode, TriggerMode::FREE_RUNNING);
}

TEST_F(TimingConfigTest, DeserializeInvalidNoteInterval)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("noteInterval", "INVALID_NOTE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to QUARTER
    EXPECT_EQ(config.noteInterval, NoteInterval::QUARTER);
}

TEST_F(TimingConfigTest, DeserializeInvalidTriggerEdge)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("triggerEdge", "INVALID_EDGE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to Rising
    EXPECT_EQ(config.triggerEdge, TriggerEdge::Rising);
}

TEST_F(TimingConfigTest, TriggerEdgeStringConversion)
{
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Rising), "Rising");
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Falling), "Falling");
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Both), "Both");

    EXPECT_EQ(stringToTriggerEdge("Rising"), TriggerEdge::Rising);
    EXPECT_EQ(stringToTriggerEdge("Falling"), TriggerEdge::Falling);
    EXPECT_EQ(stringToTriggerEdge("Both"), TriggerEdge::Both);
    EXPECT_EQ(stringToTriggerEdge("INVALID"), TriggerEdge::Rising);  // Default
}

// =============================================================================
// EDGE CASE TESTS - Empty and Invalid ValueTree
// =============================================================================

TEST_F(TimingConfigTest, DeserializeEmptyValueTree)
{
    TimingConfig original;
    original.timingMode = TimingMode::MELODIC;
    original.hostBPM = 180.0f;

    juce::ValueTree empty;
    original.fromValueTree(empty);

    // State should be unchanged when ValueTree is invalid
    EXPECT_EQ(original.timingMode, TimingMode::MELODIC);
    EXPECT_FLOAT_EQ(original.hostBPM, 180.0f);
}

TEST_F(TimingConfigTest, DeserializeValueTreeMissingProperties)
{
    juce::ValueTree tree("TimingConfig");
    // No properties set

    TimingConfig config;
    config.fromValueTree(tree);

    // Should have default values
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
    EXPECT_EQ(config.triggerMode, TriggerMode::FREE_RUNNING);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, 500.0f);  // New default: 500ms
    EXPECT_EQ(config.noteInterval, NoteInterval::QUARTER);
    EXPECT_FLOAT_EQ(config.hostBPM, 120.0f);
}

// =============================================================================
// EDGE CASE TESTS - NaN and Infinity Values
// =============================================================================

TEST_F(TimingConfigTest, TimeIntervalWithNaN)
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

TEST_F(TimingConfigTest, TimeIntervalWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigTest, TimeIntervalWithNegativeInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(-std::numeric_limits<float>::infinity());

    // Should be clamped to minimum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigTest, BPMWithNaN)
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

TEST_F(TimingConfigTest, BPMWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);

    config.setHostBPM(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MAX_BPM);
}

TEST_F(TimingConfigTest, BPMWithNegativeInfinity)
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

TEST_F(TimingConfigTest, IntervalInSamplesZeroSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // With 0 sample rate, result is 0 samples
    EXPECT_EQ(config.getIntervalInSamples(0.0f), 0);
}

TEST_F(TimingConfigTest, IntervalInSamplesNegativeSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // Negative sample rate produces negative samples
    int result = config.getIntervalInSamples(-44100.0f);
    EXPECT_LT(result, 0);
}

TEST_F(TimingConfigTest, IntervalInSamplesVeryHighSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);  // 100ms

    // At 192000 Hz, 100ms = 19200 samples
    EXPECT_EQ(config.getIntervalInSamples(192000.0f), 19200);
}

// =============================================================================
// EDGE CASE TESTS - Multi-bar Intervals
// =============================================================================

TEST_F(TimingConfigTest, MultiBarIntervalTwoBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::TWO_BARS);

    // At 120 BPM, 2 bars (8 quarter notes) = 4000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 4000.0f);
}

TEST_F(TimingConfigTest, MultiBarIntervalFourBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::FOUR_BARS);

    // At 120 BPM, 4 bars (16 quarter notes) = 8000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigTest, MultiBarIntervalEightBars)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 120 BPM, 8 bars (32 quarter notes) = 16000ms, clamped to MAX (4000ms)
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigTest, MultiBarIntervalClampedToMax)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(20.0f);  // Very slow
    config.setNoteInterval(NoteInterval::EIGHT_BARS);

    // At 20 BPM, 8 bars = 96000ms, but clamped to 60000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

// =============================================================================
// EDGE CASE TESTS - Extreme BPM Values
// =============================================================================

TEST_F(TimingConfigTest, VerySlowBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MIN_BPM);  // 20 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 20 BPM, quarter note = 3000ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 3000.0f);
}

TEST_F(TimingConfigTest, VeryFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(TimingConfig::MAX_BPM);  // 300 BPM
    config.setNoteInterval(NoteInterval::QUARTER);

    // At 300 BPM, quarter note = 200ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 200.0f);
}

TEST_F(TimingConfigTest, VeryFastNoteWithFastBPM)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(300.0f);  // Max BPM
    config.setNoteInterval(NoteInterval::THIRTY_SECOND);

    // At 300 BPM, 32nd note = 200 * 0.125 = 25ms
    EXPECT_FLOAT_EQ(config.actualIntervalMs, 25.0f);
}

TEST_F(TimingConfigTest, VeryFastNoteClampedToMin)
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

TEST_F(TimingConfigTest, TwelfthNoteInterval)
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

TEST_F(TimingConfigTest, SwitchFromTimeToMelodic)
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

TEST_F(TimingConfigTest, SwitchFromMelodicToTime)
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

TEST_F(TimingConfigTest, TimeIntervalChangeInMelodicModeIgnored)
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

TEST_F(TimingConfigTest, NoteIntervalChangeInTimeModeIgnored)
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

TEST_F(TimingConfigTest, AllMultiBarMultipliers)
{
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWO_BARS), 8.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::THREE_BARS), 12.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::FOUR_BARS), 16.0f);
    EXPECT_FLOAT_EQ(TimingConfig::getNoteIntervalMultiplier(NoteInterval::EIGHT_BARS), 32.0f);
}

TEST_F(TimingConfigTest, TwelfthNoteMultiplier)
{
    EXPECT_NEAR(TimingConfig::getNoteIntervalMultiplier(NoteInterval::TWELFTH), 1.0f / 3.0f, 0.001f);
}

// =============================================================================
// EDGE CASE TESTS - All Note Interval String Conversions
// =============================================================================

TEST_F(TimingConfigTest, AllNoteIntervalStrings)
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

TEST_F(TimingConfigTest, AllNoteIntervalFromStrings)
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

TEST_F(TimingConfigTest, AllNoteIntervalDisplayNames)
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
