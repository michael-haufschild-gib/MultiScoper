/*
    Oscil - Timing Engine Tests (Serialization)
    ValueTree persistence, entity config, note interval, edge cases
*/

#include "test_timing_engine_fixture.h"

// =============================================================================
// Serialization Tests
// =============================================================================

TEST_F(TimingEngineTest, SerializationRoundTrip)
{
    // Configure engine with non-default values
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setTimeIntervalMs(750);
    engine.setNoteInterval(EngineNoteInterval::NOTE_DOTTED_1_8TH);
    engine.setHostSyncEnabled(true);
    engine.setSyncToPlayhead(true);
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.35f);

    // Serialize
    auto valueTree = engine.toValueTree();

    // Create new engine and deserialize
    TimingEngine restoredEngine;
    restoredEngine.fromValueTree(valueTree);

    const auto& config = restoredEngine.getConfig();

    EXPECT_EQ(config.timingMode, TimingMode::MELODIC);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, 750.0f);
    EXPECT_EQ(config.noteInterval, EngineNoteInterval::NOTE_DOTTED_1_8TH);
    EXPECT_TRUE(config.hostSyncEnabled);
    EXPECT_TRUE(config.syncToPlayhead);
    EXPECT_EQ(config.triggerMode, WaveformTriggerMode::RisingEdge);
    EXPECT_FLOAT_EQ(config.triggerThreshold, 0.35f);
}

TEST_F(TimingEngineTest, SetConfig_RoundTripsMidiTriggerFields)
{
    EngineTimingConfig config = engine.getConfig();
    config.triggerMode = WaveformTriggerMode::Midi;
    config.midiTriggerNote = 60;
    config.midiTriggerChannel = 2;

    engine.setConfig(config);

    const auto updated = engine.getConfig();
    EXPECT_EQ(updated.triggerMode, WaveformTriggerMode::Midi);
    EXPECT_EQ(updated.midiTriggerNote, 60);
    EXPECT_EQ(updated.midiTriggerChannel, 2);
}

TEST_F(TimingEngineTest, SetConfig_MidiFiltersAreAppliedInProcessMidi)
{
    EngineTimingConfig config = engine.getConfig();
    config.triggerMode = WaveformTriggerMode::Midi;
    config.midiTriggerNote = 60;
    config.midiTriggerChannel = 2;
    engine.setConfig(config);

    juce::MidiBuffer wrongNote;
    wrongNote.addEvent(juce::MidiMessage::noteOn(2, 61, static_cast<juce::uint8>(100)), 0);
    EXPECT_FALSE(engine.processMidi(wrongNote));

    juce::MidiBuffer wrongChannel;
    wrongChannel.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    EXPECT_FALSE(engine.processMidi(wrongChannel));

    juce::MidiBuffer matching;
    matching.addEvent(juce::MidiMessage::noteOn(2, 60, static_cast<juce::uint8>(100)), 0);
    EXPECT_TRUE(engine.processMidi(matching));
}

TEST_F(TimingEngineTest, ProcessMidi_UpdatesLastSyncTimestampFromHostSamplePosition)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Midi);
    engine.setMidiTriggerNote(60);
    engine.setMidiTriggerChannel(1);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setIsPlaying(true);
    posInfo.setTimeInSamples(4096);
    engine.updateHostInfo(posInfo);

    juce::MidiBuffer matching;
    matching.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    EXPECT_TRUE(engine.processMidi(matching));
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 4096.0);
}

TEST_F(TimingEngineTest, AudioTrigger_UpdatesLastSyncTimestampFromHostSamplePosition)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.2f);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setTimeInSamples(2048);
    engine.updateHostInfo(posInfo);

    auto lowBuffer = generateStep(64, -0.5f, -0.5f, 64);
    (void) engine.processBlock(lowBuffer);

    auto risingBuffer = generateStep(64, -0.5f, 0.5f, 32);
    EXPECT_TRUE(engine.processBlock(risingBuffer));
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 2048.0);
}

TEST_F(TimingEngineTest, ManualTrigger_UpdatesLastSyncTimestampFromHostSamplePosition)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setTimeInSamples(8192);
    engine.updateHostInfo(posInfo);

    engine.requestManualTrigger();
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8192.0);
}

TEST_F(TimingEngineTest, FromValueTree_ResetsRuntimeLastSyncTimestamp)
{
    EngineTimingConfig config = engine.getConfig();
    config.syncToPlayhead = true;
    config.lastSyncTimestamp = 8192.0;
    engine.setConfig(config);
    ASSERT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8192.0);

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    // Runtime-only sync anchors should not survive persistence reloads.
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
}

TEST_F(TimingEngineTest, FromValueTree_IgnoresInjectedLastSyncTimestampProperty)
{
    auto persisted = engine.toValueTree();
    persisted.setProperty("lastSyncTimestamp", -1234.0, nullptr);
    persisted.setProperty("syncToPlayhead", true, nullptr);

    engine.fromValueTree(persisted);

    EXPECT_TRUE(engine.getConfig().syncToPlayhead);
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
}

TEST_F(TimingEngineTest, FromValueTree_ResetsRuntimeTriggerFlags)
{
    engine.requestManualTrigger();

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    // Runtime-only trigger latches must not survive persistence reloads.
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());
}

TEST_F(TimingEngineTest, FromValueTree_IgnoresInjectedRuntimeTriggerProperties)
{
    auto persisted = engine.toValueTree();
    persisted.setProperty("manualTrigger", true, nullptr);
    persisted.setProperty("triggered", true, nullptr);

    engine.fromValueTree(persisted);

    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());
}

TEST_F(TimingEngineTest, FromValueTree_ResetsRuntimeHostTransportSnapshot)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(147.0);
    posInfo.setPpqPosition(32.5);
    posInfo.setIsPlaying(true);
    posInfo.setTimeInSamples(16384);
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{7, 8});
    engine.updateHostInfo(posInfo);

    const auto hostBefore = engine.getHostInfo();
    ASSERT_TRUE(hostBefore.isPlaying);
    ASSERT_EQ(hostBefore.timeInSamples, 16384);
    ASSERT_EQ(hostBefore.transportState, HostTimingInfo::TransportState::PLAYING);
    ASSERT_EQ(hostBefore.timeSigNumerator, 7);
    ASSERT_EQ(hostBefore.timeSigDenominator, 8);
    ASSERT_DOUBLE_EQ(hostBefore.ppqPosition, 32.5);
    ASSERT_DOUBLE_EQ(hostBefore.bpm, 147.0);

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    const auto hostAfter = engine.getHostInfo();
    EXPECT_FALSE(hostAfter.isPlaying);
    EXPECT_EQ(hostAfter.timeInSamples, 0);
    EXPECT_EQ(hostAfter.transportState, HostTimingInfo::TransportState::STOPPED);
    EXPECT_EQ(hostAfter.timeSigNumerator, 4);
    EXPECT_EQ(hostAfter.timeSigDenominator, 4);
    EXPECT_DOUBLE_EQ(hostAfter.ppqPosition, 0.0);
    EXPECT_DOUBLE_EQ(hostAfter.bpm, 120.0);
}

TEST_F(TimingEngineTest, FromValueTree_IgnoresInjectedRuntimeHostTransportProperties)
{
    auto persisted = engine.toValueTree();
    persisted.setProperty("isPlaying", true, nullptr);
    persisted.setProperty("timeInSamples", 9999, nullptr);
    persisted.setProperty("ppqPosition", 42.0, nullptr);
    persisted.setProperty("transportState", 2, nullptr);
    persisted.setProperty("timeSigNumerator", 11, nullptr);
    persisted.setProperty("timeSigDenominator", 16, nullptr);
    persisted.setProperty("hostBpm", 201.0, nullptr);

    engine.fromValueTree(persisted);

    const auto hostInfo = engine.getHostInfo();
    EXPECT_FALSE(hostInfo.isPlaying);
    EXPECT_EQ(hostInfo.timeInSamples, 0);
    EXPECT_EQ(hostInfo.transportState, HostTimingInfo::TransportState::STOPPED);
    EXPECT_EQ(hostInfo.timeSigNumerator, 4);
    EXPECT_EQ(hostInfo.timeSigDenominator, 4);
    EXPECT_DOUBLE_EQ(hostInfo.ppqPosition, 0.0);
    EXPECT_DOUBLE_EQ(hostInfo.bpm, 120.0);
}

TEST_F(TimingEngineTest, FromValueTree_ResetsLevelTriggerHistoryForNextBuffer)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
    engine.setTriggerThreshold(0.5f);

    auto lowBuffer = generateStep(64, 0.2f, 0.2f, 64);
    auto highBuffer = generateStep(64, 0.9f, 0.9f, 64);
    (void) engine.processBlock(lowBuffer);
    EXPECT_TRUE(engine.processBlock(highBuffer));
    EXPECT_FALSE(engine.processBlock(highBuffer));

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    // Post-load evaluation should not reuse stale "already above threshold" history.
    EXPECT_TRUE(engine.processBlock(highBuffer));
}

TEST_F(TimingEngineTest, FromValueTree_ResetsRisingEdgeHistoryForNextBuffer)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.5f);

    auto highBuffer = generateStep(64, 0.8f, 0.8f, 64);
    (void) engine.processBlock(highBuffer);
    EXPECT_FALSE(engine.processBlock(highBuffer));

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    // First post-load high sample should be evaluated from neutral baseline.
    EXPECT_TRUE(engine.processBlock(highBuffer));
}

TEST_F(TimingEngineTest, FromValueTree_DeferredHistoryResetHasNoSideEffectsInTriggerModeNone)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::None);

    auto buffer = generateSineWave(128, 440.0f, 0.7f, 44100.0f);
    EXPECT_TRUE(engine.processBlock(buffer));

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    EXPECT_TRUE(engine.processBlock(buffer));
    EXPECT_FALSE(engine.checkAndClearTrigger());
}

TEST_F(TimingEngineTest, FromValueTree_DeferredHistoryResetHasNoSideEffectsInManualMode)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);

    auto buffer = generateSineWave(128, 440.0f, 0.7f, 44100.0f);
    EXPECT_FALSE(engine.processBlock(buffer));

    engine.requestManualTrigger();
    EXPECT_TRUE(engine.processBlock(buffer));
}
