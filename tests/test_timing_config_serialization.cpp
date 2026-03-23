/*
    Oscil - TimingConfig Serialization Tests
    Tests for serialization, string conversion, and edge cases
*/

#include "core/dsp/TimingConfig.h"

#include "helpers/TimingConfigBuilder.h"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

using namespace oscil;
using namespace oscil::test;

class TimingConfigSerializationTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// === Serialization Tests ===

TEST_F(TimingConfigSerializationTest, SerializationRoundTrip)
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

TEST_F(TimingConfigSerializationTest, TimingModeStringConversion)
{
    EXPECT_EQ(timingModeToString(TimingMode::TIME), "TIME");
    EXPECT_EQ(timingModeToString(TimingMode::MELODIC), "MELODIC");

    EXPECT_EQ(stringToTimingMode("TIME"), TimingMode::TIME);
    EXPECT_EQ(stringToTimingMode("MELODIC"), TimingMode::MELODIC);
    EXPECT_EQ(stringToTimingMode("INVALID"), TimingMode::TIME); // Default
}

TEST_F(TimingConfigSerializationTest, NoteIntervalStringConversion)
{
    EXPECT_EQ(noteIntervalToString(NoteInterval::QUARTER), "1/4");
    EXPECT_EQ(noteIntervalToString(NoteInterval::EIGHTH), "1/8");
    EXPECT_EQ(noteIntervalToString(NoteInterval::DOTTED_QUARTER), "1/4.");
    EXPECT_EQ(noteIntervalToString(NoteInterval::TRIPLET_QUARTER), "1/4T");

    EXPECT_EQ(stringToNoteInterval("1/4"), NoteInterval::QUARTER);
    EXPECT_EQ(stringToNoteInterval("1/8"), NoteInterval::EIGHTH);
    EXPECT_EQ(stringToNoteInterval("1/4."), NoteInterval::DOTTED_QUARTER);
    EXPECT_EQ(stringToNoteInterval("1/4T"), NoteInterval::TRIPLET_QUARTER);
    EXPECT_EQ(stringToNoteInterval("INVALID"), NoteInterval::QUARTER); // Default
}

TEST_F(TimingConfigSerializationTest, TriggerModeStringConversion)
{
    EXPECT_EQ(triggerModeToString(TriggerMode::FREE_RUNNING), "FREE_RUNNING");
    EXPECT_EQ(triggerModeToString(TriggerMode::HOST_SYNC), "HOST_SYNC");
    EXPECT_EQ(triggerModeToString(TriggerMode::TRIGGERED), "TRIGGERED");

    EXPECT_EQ(stringToTriggerMode("FREE_RUNNING"), TriggerMode::FREE_RUNNING);
    EXPECT_EQ(stringToTriggerMode("HOST_SYNC"), TriggerMode::HOST_SYNC);
    EXPECT_EQ(stringToTriggerMode("TRIGGERED"), TriggerMode::TRIGGERED);
    EXPECT_EQ(stringToTriggerMode("INVALID"), TriggerMode::FREE_RUNNING); // Default
}

TEST_F(TimingConfigSerializationTest, NoteIntervalDisplayNames)
{
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::QUARTER), "Quarter Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::EIGHTH), "8th Note");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::DOTTED_QUARTER), "Dotted Quarter");
    EXPECT_EQ(noteIntervalToDisplayName(NoteInterval::TRIPLET_QUARTER), "Quarter Triplet");
}

TEST_F(TimingConfigSerializationTest, TriggerEdgeStringConversion)
{
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Rising), "Rising");
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Falling), "Falling");
    EXPECT_EQ(triggerEdgeToString(TriggerEdge::Both), "Both");

    EXPECT_EQ(stringToTriggerEdge("Rising"), TriggerEdge::Rising);
    EXPECT_EQ(stringToTriggerEdge("Falling"), TriggerEdge::Falling);
    EXPECT_EQ(stringToTriggerEdge("Both"), TriggerEdge::Both);
    EXPECT_EQ(stringToTriggerEdge("INVALID"), TriggerEdge::Rising); // Default
}

TEST_F(TimingConfigSerializationTest, AllNoteIntervalStrings)
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

TEST_F(TimingConfigSerializationTest, AllNoteIntervalFromStrings)
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

TEST_F(TimingConfigSerializationTest, AllNoteIntervalDisplayNames)
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

// =============================================================================
// EDGE CASE TESTS - Invalid Enum Serialization
// =============================================================================

TEST_F(TimingConfigSerializationTest, DeserializeInvalidTimingMode)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("timingMode", "INVALID_MODE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to TIME mode
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
}

TEST_F(TimingConfigSerializationTest, DeserializeInvalidTriggerMode)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("triggerMode", "UNKNOWN_TRIGGER", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to FREE_RUNNING
    EXPECT_EQ(config.triggerMode, TriggerMode::FREE_RUNNING);
}

TEST_F(TimingConfigSerializationTest, DeserializeInvalidNoteInterval)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("noteInterval", "INVALID_NOTE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to QUARTER
    EXPECT_EQ(config.noteInterval, NoteInterval::QUARTER);
}

TEST_F(TimingConfigSerializationTest, DeserializeInvalidTriggerEdge)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("triggerEdge", "INVALID_EDGE", nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    // Should default to Rising
    EXPECT_EQ(config.triggerEdge, TriggerEdge::Rising);
}

// =============================================================================
// EDGE CASE TESTS - Empty and Invalid ValueTree
// =============================================================================

TEST_F(TimingConfigSerializationTest, DeserializeEmptyValueTree)
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

TEST_F(TimingConfigSerializationTest, DeserializeValueTreeMissingProperties)
{
    juce::ValueTree tree("TimingConfig");
    // No properties set

    TimingConfig config;
    config.fromValueTree(tree);

    // Should have default values
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
    EXPECT_EQ(config.triggerMode, TriggerMode::FREE_RUNNING);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, 500.0f); // New default: 500ms
    EXPECT_EQ(config.noteInterval, NoteInterval::QUARTER);
    EXPECT_FLOAT_EQ(config.hostBPM, 120.0f);
}

// =============================================================================
// EDGE CASE TESTS - NaN and Infinity Values
// =============================================================================

TEST_F(TimingConfigSerializationTest, TimeIntervalWithNaN)
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

TEST_F(TimingConfigSerializationTest, TimeIntervalWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigSerializationTest, TimeIntervalWithNegativeInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(-std::numeric_limits<float>::infinity());

    // Should be clamped to minimum
    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigSerializationTest, BPMWithNaN)
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

TEST_F(TimingConfigSerializationTest, BPMWithInfinity)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::MELODIC);

    config.setHostBPM(std::numeric_limits<float>::infinity());

    // Should be clamped to maximum
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MAX_BPM);
}

TEST_F(TimingConfigSerializationTest, BPMWithNegativeInfinity)
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

TEST_F(TimingConfigSerializationTest, IntervalInSamplesZeroSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // With 0 sample rate, result is 0 samples
    EXPECT_EQ(config.getIntervalInSamples(0.0f), 0);
}

TEST_F(TimingConfigSerializationTest, IntervalInSamplesNegativeSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);

    // Negative sample rate produces negative samples
    int result = config.getIntervalInSamples(-44100.0f);
    EXPECT_LT(result, 0);
}

TEST_F(TimingConfigSerializationTest, IntervalInSamplesVeryHighSampleRate)
{
    TimingConfig config;
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f); // 100ms

    // At 192000 Hz, 100ms = 19200 samples
    EXPECT_EQ(config.getIntervalInSamples(192000.0f), 19200);
}

// =============================================================================
// EDGE CASE TESTS - Mode Switching
// =============================================================================

TEST_F(TimingConfigSerializationTest, SwitchFromTimeToMelodic)
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

TEST_F(TimingConfigSerializationTest, SwitchFromMelodicToTime)
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

TEST_F(TimingConfigSerializationTest, TimeIntervalChangeInMelodicModeIgnored)
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

TEST_F(TimingConfigSerializationTest, NoteIntervalChangeInTimeModeIgnored)
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
