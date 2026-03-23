/*
    Oscil - Timing Engine Tests (Host Info)
    Host info updates, transport, BPM, time signature
*/

#include "test_timing_engine_fixture.h"

// Host Info Update Tests
// =============================================================================

TEST_F(TimingEngineTest, HostInfoUpdate_BPMChange)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_4TH);
    engine.setHostSyncEnabled(true); // Enable host sync so hostBPM affects interval

    // Create position info with new BPM
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(140.0);

    engine.updateHostInfo(posInfo);

    EXPECT_NEAR(engine.getHostInfo().bpm, 140.0, 0.1);

    // Quarter note at 140 BPM = 60000/140 = ~428.57ms
    EXPECT_NEAR(engine.getActualIntervalMs(), 428.57f, 1.0f);
}

TEST_F(TimingEngineTest, HostInfoUpdate_BPMClamping_Low)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(5.0); // Below minimum

    engine.updateHostInfo(posInfo);

    EXPECT_GE(engine.getHostInfo().bpm, EngineTimingConfig::MIN_BPM);
}

TEST_F(TimingEngineTest, HostInfoUpdate_BPMClamping_High)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(500.0); // Above maximum

    engine.updateHostInfo(posInfo);

    EXPECT_LE(engine.getHostInfo().bpm, EngineTimingConfig::MAX_BPM);
}

TEST_F(TimingEngineTest, HostInfoUpdate_PlayingState)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setIsPlaying(true);

    engine.updateHostInfo(posInfo);

    EXPECT_TRUE(engine.getHostInfo().isPlaying);
}

TEST_F(TimingEngineTest, HostInfoUpdate_PPQPosition)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setPpqPosition(16.5);

    engine.updateHostInfo(posInfo);

    EXPECT_DOUBLE_EQ(engine.getHostInfo().ppqPosition, 16.5);
}

TEST_F(TimingEngineTest, HostInfoUpdate_TimeSignature)
{
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{6, 8});

    engine.updateHostInfo(posInfo);

    EXPECT_EQ(engine.getHostInfo().timeSigNumerator, 6);
    EXPECT_EQ(engine.getHostInfo().timeSigDenominator, 8);
}

TEST_F(TimingEngineTest, HostInfoUpdate_SyncToPlayheadTriggersOnPlayTransition)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(1024);
    engine.updateHostInfo(stopped);

    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playing;
    playing.setIsPlaying(true);
    playing.setTimeInSamples(2048);
    engine.updateHostInfo(playing);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
}

TEST_F(TimingEngineTest, HostInfoUpdate_SyncToPlayheadTriggersOnBackwardTimelineJump)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(10000);
    engine.updateHostInfo(startPlaying);

    // Clear initial start trigger from STOP->PLAY transition.
    EXPECT_TRUE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo forward;
    forward.setIsPlaying(true);
    forward.setTimeInSamples(12000);
    engine.updateHostInfo(forward);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo backwardJump;
    backwardJump.setIsPlaying(true);
    backwardJump.setTimeInSamples(8000);
    engine.updateHostInfo(backwardJump);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8000.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_SyncToPlayheadDoesNotTriggerOnForwardTimelineJump)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(10000);
    engine.updateHostInfo(startPlaying);

    // Clear initial start trigger from STOP->PLAY transition.
    EXPECT_TRUE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo forwardJump;
    forwardJump.setIsPlaying(true);
    forwardJump.setTimeInSamples(50000);
    engine.updateHostInfo(forwardJump);

    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 10000.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_RepeatedIdenticalPlayingPacketsDoNotRetrigger)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(11000);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playing;
    playing.setIsPlaying(true);
    playing.setTimeInSamples(12000);
    engine.updateHostInfo(playing);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo repeatedPlayingOne;
    repeatedPlayingOne.setIsPlaying(true);
    repeatedPlayingOne.setTimeInSamples(12000);
    engine.updateHostInfo(repeatedPlayingOne);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo repeatedPlayingTwo;
    repeatedPlayingTwo.setIsPlaying(true);
    repeatedPlayingTwo.setTimeInSamples(12000);
    engine.updateHostInfo(repeatedPlayingTwo);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 12000.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_MicroBackwardJitterDoesNotRetrigger)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(10000);
    engine.updateHostInfo(startPlaying);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo stableForward;
    stableForward.setIsPlaying(true);
    stableForward.setTimeInSamples(12000);
    engine.updateHostInfo(stableForward);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    // Host sample-position jitter (1-2 samples backward) should not retrigger.
    juce::AudioPlayHead::PositionInfo jitterMinusOne;
    jitterMinusOne.setIsPlaying(true);
    jitterMinusOne.setTimeInSamples(11999);
    engine.updateHostInfo(jitterMinusOne);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo jitterMinusTwo;
    jitterMinusTwo.setIsPlaying(true);
    jitterMinusTwo.setTimeInSamples(11998);
    engine.updateHostInfo(jitterMinusTwo);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo realBackwardJump;
    realBackwardJump.setIsPlaying(true);
    realBackwardJump.setTimeInSamples(11800);
    engine.updateHostInfo(realBackwardJump);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 11800.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_BackwardDeltaTwoSamplesDoesNotRetrigger)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(10000);
    engine.updateHostInfo(startPlaying);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo stableForward;
    stableForward.setIsPlaying(true);
    stableForward.setTimeInSamples(12000);
    engine.updateHostInfo(stableForward);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo backwardByTwo;
    backwardByTwo.setIsPlaying(true);
    backwardByTwo.setTimeInSamples(11998);
    engine.updateHostInfo(backwardByTwo);

    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 10000.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_BackwardDeltaThreeSamplesRetriggers)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(10000);
    engine.updateHostInfo(startPlaying);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo stableForward;
    stableForward.setIsPlaying(true);
    stableForward.setTimeInSamples(12000);
    engine.updateHostInfo(stableForward);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo backwardByThree;
    backwardByThree.setIsPlaying(true);
    backwardByThree.setTimeInSamples(11997);
    engine.updateHostInfo(backwardByThree);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 11997.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_PlayTransitionWithoutTimeUsesLastKnownTimestamp)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(4096);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playNoTime;
    playNoTime.setIsPlaying(true);
    engine.updateHostInfo(playNoTime);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 4096.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_PlayTransitionWithoutAnyKnownSampleUsesZeroTimestamp)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo playNoTime;
    playNoTime.setIsPlaying(true);
    engine.updateHostInfo(playNoTime);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
}
