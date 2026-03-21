/*
    Oscil - TimingConfig Entity Tests
*/

#include <gtest/gtest.h>
#include "core/dsp/TimingConfig.h"
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

TEST_F(TimingConfigTest, DeserializeOutOfRangeNumericFieldsAreClamped)
{
    juce::ValueTree tree("TimingConfig");
    tree.setProperty("timingMode", "MELODIC", nullptr);
    tree.setProperty("timeIntervalMs", -100.0f, nullptr);
    tree.setProperty("hostBPM", -10.0f, nullptr);
    tree.setProperty("midiTriggerNote", 200, nullptr);
    tree.setProperty("midiTriggerChannel", 42, nullptr);

    TimingConfig config;
    config.fromValueTree(tree);

    EXPECT_FLOAT_EQ(config.timeIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);
    EXPECT_FLOAT_EQ(config.hostBPM, TimingConfig::MIN_BPM);
    EXPECT_EQ(config.midiTriggerNote, 127);
    EXPECT_EQ(config.midiTriggerChannel, 16);
    EXPECT_TRUE(config.isHostSyncAvailable());

    juce::ValueTree lowerBoundTree("TimingConfig");
    lowerBoundTree.setProperty("midiTriggerNote", -99, nullptr);
    lowerBoundTree.setProperty("midiTriggerChannel", -5, nullptr);
    config.fromValueTree(lowerBoundTree);

    EXPECT_EQ(config.midiTriggerNote, -1);
    EXPECT_EQ(config.midiTriggerChannel, 0);
}

