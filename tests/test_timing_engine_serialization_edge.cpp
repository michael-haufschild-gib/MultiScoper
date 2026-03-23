/*
    Oscil - Timing Engine Tests (Serialization Edge Cases)
    Invalid trees, entity config integration, note interval conversion, empty buffers
*/

#include "test_timing_engine_fixture.h"

// =============================================================================
// Deserialization Error Handling
// =============================================================================

TEST_F(TimingEngineTest, DeserializationFromInvalidTree_Edge)
{
    TimingEngine testEngine;

    // Store original config
    auto originalConfig = testEngine.getConfig();

    // Try to deserialize from invalid tree
    juce::ValueTree invalidTree("WrongType");
    testEngine.fromValueTree(invalidTree);

    // Config should remain unchanged
    auto config = testEngine.getConfig();
    EXPECT_EQ(config.timingMode, originalConfig.timingMode);
    EXPECT_EQ(config.timeIntervalMs, originalConfig.timeIntervalMs);
}

TEST_F(TimingEngineTest, FromValueTree_InvalidTypeResetsRuntimeStateAndPreservesConfig_Edge)
{
    EngineTimingConfig config = engine.getConfig();
    config.timingMode = TimingMode::MELODIC;
    config.noteInterval = EngineNoteInterval::NOTE_1_8TH;
    config.timeIntervalMs = 777.0f;
    config.syncToPlayhead = true;
    engine.setConfig(config);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setIsPlaying(true);
    posInfo.setTimeInSamples(4096);
    posInfo.setBpm(147.0);
    posInfo.setPpqPosition(8.0);
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{7, 8});
    engine.updateHostInfo(posInfo);
    engine.requestManualTrigger();

    ASSERT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 4096.0);
    ASSERT_TRUE(engine.checkAndClearTrigger());
    engine.requestManualTrigger();
    ASSERT_TRUE(engine.checkAndClearManualTrigger());
    engine.requestManualTrigger();

    juce::ValueTree invalidTree("WrongType");
    engine.fromValueTree(invalidTree);

    // Config fields should remain unchanged on invalid input.
    const auto configAfter = engine.getConfig();
    EXPECT_EQ(configAfter.timingMode, TimingMode::MELODIC);
    EXPECT_EQ(configAfter.noteInterval, EngineNoteInterval::NOTE_1_8TH);
    EXPECT_FLOAT_EQ(configAfter.timeIntervalMs, 777.0f);
    EXPECT_TRUE(configAfter.syncToPlayhead);

    // Runtime-only state must still be reset.
    EXPECT_DOUBLE_EQ(configAfter.lastSyncTimestamp, 0.0);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());

    const auto hostInfo = engine.getHostInfo();
    EXPECT_FALSE(hostInfo.isPlaying);
    EXPECT_EQ(hostInfo.timeInSamples, 0);
    EXPECT_EQ(hostInfo.transportState, HostTimingInfo::TransportState::STOPPED);
}

TEST_F(TimingEngineTest, DeserializationWithMissingProperties_Edge)
{
    // Create tree with only some properties
    juce::ValueTree partialTree("Timing");
    partialTree.setProperty("timingMode", 1, nullptr); // MELODIC

    TimingEngine testEngine;
    testEngine.fromValueTree(partialTree);

    // Should have the set property
    EXPECT_EQ(testEngine.getConfig().timingMode, TimingMode::MELODIC);

    // Other properties should have valid defaults (float comparison)
    EXPECT_GE(testEngine.getConfig().timeIntervalMs, EngineTimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingEngineTest, DeserializationClampsInvalidEnumsAndRanges_Edge)
{
    juce::ValueTree invalidTree("Timing");
    invalidTree.setProperty("timingMode", 999, nullptr);
    invalidTree.setProperty("timeIntervalMs", -42.0f, nullptr);
    invalidTree.setProperty("noteInterval", 999, nullptr);
    invalidTree.setProperty("triggerMode", 999, nullptr);
    invalidTree.setProperty("triggerThreshold", 4.0f, nullptr);
    invalidTree.setProperty("midiTriggerNote", 200, nullptr);
    invalidTree.setProperty("midiTriggerChannel", 99, nullptr);

    TimingEngine testEngine;
    testEngine.fromValueTree(invalidTree);

    const auto config = testEngine.getConfig();
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, EngineTimingConfig::MIN_TIME_INTERVAL_MS);
    EXPECT_EQ(config.noteInterval, EngineNoteInterval::NOTE_1_4TH);
    EXPECT_EQ(config.triggerMode, WaveformTriggerMode::None);
    EXPECT_FLOAT_EQ(config.triggerThreshold, 1.0f);
    EXPECT_EQ(config.midiTriggerNote, 127);
    EXPECT_EQ(config.midiTriggerChannel, 16);
}

// =============================================================================
// Entity Config Integration Tests
// =============================================================================

TEST_F(TimingEngineTest, ApplyEntityConfig_TimingMode_Edge)
{
    TimingConfig entityConfig;
    entityConfig.timingMode = TimingMode::MELODIC;
    entityConfig.noteInterval = NoteInterval::EIGHTH;
    entityConfig.hostBPM = 120.0f;

    engine.applyEntityConfig(entityConfig);

    EXPECT_EQ(engine.getConfig().timingMode, TimingMode::MELODIC);
    EXPECT_EQ(engine.getConfig().noteInterval, EngineNoteInterval::NOTE_1_8TH);
}

TEST_F(TimingEngineTest, ApplyEntityConfig_TriggerModeConversion_Edge)
{
    TimingConfig entityConfig;
    entityConfig.triggerMode = TriggerMode::TRIGGERED;
    entityConfig.triggerEdge = TriggerEdge::Rising;

    engine.applyEntityConfig(entityConfig);

    EXPECT_EQ(engine.getConfig().triggerMode, WaveformTriggerMode::RisingEdge);
}

TEST_F(TimingEngineTest, ApplyEntityConfig_TriggerThresholdConversion_Edge)
{
    TimingConfig entityConfig;
    entityConfig.triggerMode = TriggerMode::TRIGGERED;
    entityConfig.triggerEdge = TriggerEdge::Rising;
    entityConfig.triggerThreshold = -6.0f; // -6 dBFS

    engine.applyEntityConfig(entityConfig);

    // -6 dBFS = 10^(-6/20) = ~0.501
    EXPECT_NEAR(engine.getConfig().triggerThreshold, 0.501f, 0.01f);
}

TEST_F(TimingEngineTest, SetConfigAfterFromValueTree_DoesNotResurrectRuntimeResetState_Edge)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(147.0);
    posInfo.setPpqPosition(12.5);
    posInfo.setIsPlaying(true);
    posInfo.setTimeInSamples(8192);
    engine.updateHostInfo(posInfo);
    engine.requestManualTrigger();

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    ASSERT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
    ASSERT_FALSE(engine.checkAndClearTrigger());
    ASSERT_FALSE(engine.checkAndClearManualTrigger());

    EngineTimingConfig config = engine.getConfig();
    engine.setConfig(config);

    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());
}

TEST_F(TimingEngineTest, ApplyEntityConfigAfterFromValueTree_DoesNotResurrectRuntimeResetState_Edge)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(140.0);
    posInfo.setPpqPosition(8.0);
    posInfo.setIsPlaying(true);
    posInfo.setTimeInSamples(4096);
    engine.updateHostInfo(posInfo);
    engine.requestManualTrigger();

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    TimingConfig entityConfig;
    entityConfig.timingMode = TimingMode::MELODIC;
    entityConfig.noteInterval = NoteInterval::EIGHTH;
    entityConfig.hostSyncEnabled = true;
    entityConfig.hostBPM = 132.0f;
    entityConfig.triggerMode = TriggerMode::FREE_RUNNING;
    engine.applyEntityConfig(entityConfig);

    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());

    const auto hostInfo = engine.getHostInfo();
    EXPECT_FALSE(hostInfo.isPlaying);
    EXPECT_EQ(hostInfo.timeInSamples, 0);
}

TEST_F(TimingEngineTest, ToEntityConfig_RoundTrip_Edge)
{
    // Set up engine state
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_DOTTED_1_4TH);
    engine.setHostSyncEnabled(true);
    engine.setWaveformTriggerMode(WaveformTriggerMode::FallingEdge);
    engine.setTriggerThreshold(0.5f);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 140.0f;
    engine.setConfig(config);

    // Convert to entity config
    TimingConfig entityConfig = engine.toEntityConfig();

    EXPECT_EQ(entityConfig.timingMode, TimingMode::MELODIC);
    EXPECT_EQ(entityConfig.noteInterval, NoteInterval::DOTTED_QUARTER);
    EXPECT_TRUE(entityConfig.hostSyncEnabled);
    EXPECT_FLOAT_EQ(entityConfig.hostBPM, 140.0f);
    EXPECT_EQ(entityConfig.triggerMode, TriggerMode::TRIGGERED);
    EXPECT_EQ(entityConfig.triggerEdge, TriggerEdge::Falling);
}

// =============================================================================
// Note Interval Conversion Tests
// =============================================================================

TEST_F(TimingEngineTest, EngineNoteIntervalToBeats_AllValues_Edge)
{
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_32ND), 0.125);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_16TH), 0.25);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_8TH), 0.5);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_4TH), 1.0);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_2), 2.0);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_1_1), 4.0);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_2_1), 8.0);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_4_1), 16.0);
    EXPECT_DOUBLE_EQ(engineNoteIntervalToBeats(EngineNoteInterval::NOTE_8_1), 32.0);
}

TEST_F(TimingEngineTest, EngineNoteIntervalToString_AllValues_Edge)
{
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_32ND), "1/32");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_16TH), "1/16");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_8TH), "1/8");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_4TH), "1/4");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_2), "1/2");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_1_1), "1 Bar");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_DOTTED_1_4TH), "1/4.");
    EXPECT_EQ(engineNoteIntervalToString(EngineNoteInterval::NOTE_TRIPLET_1_4TH), "1/4T");
}

TEST_F(TimingEngineTest, EntityToEngineNoteIntervalConversion_Edge)
{
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::QUARTER), EngineNoteInterval::NOTE_1_4TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::EIGHTH), EngineNoteInterval::NOTE_1_8TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::SIXTEENTH), EngineNoteInterval::NOTE_1_16TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::DOTTED_QUARTER), EngineNoteInterval::NOTE_DOTTED_1_4TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::TRIPLET_QUARTER), EngineNoteInterval::NOTE_TRIPLET_1_4TH);
}

TEST_F(TimingEngineTest, EngineToEntityNoteIntervalConversion_Edge)
{
    EXPECT_EQ(engineToEntityNoteInterval(EngineNoteInterval::NOTE_1_4TH), NoteInterval::QUARTER);
    EXPECT_EQ(engineToEntityNoteInterval(EngineNoteInterval::NOTE_1_8TH), NoteInterval::EIGHTH);
    EXPECT_EQ(engineToEntityNoteInterval(EngineNoteInterval::NOTE_1_16TH), NoteInterval::SIXTEENTH);
    EXPECT_EQ(engineToEntityNoteInterval(EngineNoteInterval::NOTE_DOTTED_1_4TH), NoteInterval::DOTTED_QUARTER);
    EXPECT_EQ(engineToEntityNoteInterval(EngineNoteInterval::NOTE_TRIPLET_1_4TH), NoteInterval::TRIPLET_QUARTER);
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(TimingEngineTest, ProcessBlock_EmptyBuffer_Edge)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);

    juce::AudioBuffer<float> emptyBuffer(2, 0);
    // Should not crash and should return false (no trigger)
    EXPECT_FALSE(engine.processBlock(emptyBuffer));
}
