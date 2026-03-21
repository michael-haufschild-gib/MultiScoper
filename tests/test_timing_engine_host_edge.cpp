/*
    Oscil - Timing Engine Tests (Host Info Edge Cases)
    Play transitions with missing time, negative samples, jitter, sync toggle
*/

#include "test_timing_engine_fixture.h"

// =============================================================================
// Host Info Edge Cases - Missing/Invalid Timestamps
// =============================================================================

TEST_F(TimingEngineTest, HostInfoUpdate_PlayTransitionWithNegativeSampleClampsToZeroTimestamp)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo playWithNegativeTime;
    playWithNegativeTime.setIsPlaying(true);
    playWithNegativeTime.setTimeInSamples(-2);
    engine.updateHostInfo(playWithNegativeTime);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 0.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_NegativePlayingSampleIgnoredButSubsequentBackwardJumpTriggers)
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

    juce::AudioPlayHead::PositionInfo negativeSample;
    negativeSample.setIsPlaying(true);
    negativeSample.setTimeInSamples(-7);
    engine.updateHostInfo(negativeSample);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 12000);
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 10000.0);

    juce::AudioPlayHead::PositionInfo backwardJump;
    backwardJump.setIsPlaying(true);
    backwardJump.setTimeInSamples(3000);
    engine.updateHostInfo(backwardJump);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 3000.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 3000);
}

// =============================================================================
// Host Sync Toggle Edge Cases
// =============================================================================

TEST_F(TimingEngineTest, HostInfoUpdate_HostSyncPlayTransitionWithoutTimeUsesLastKnownTimestamp)
{
    engine.setHostSyncEnabled(true);
    engine.setSyncToPlayhead(false);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(4096);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithoutTime;
    playWithoutTime.setIsPlaying(true);
    playWithoutTime.setBpm(132.0);
    playWithoutTime.setPpqPosition(12.5);
    engine.updateHostInfo(playWithoutTime);

    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 4096.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 4096);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().ppqPosition, 12.5);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().bpm, 132.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_HostSyncPlayTransitionWithNegativeTimeUsesFallbackTimestamp)
{
    engine.setHostSyncEnabled(true);
    engine.setSyncToPlayhead(false);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(8192);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithNegativeTime;
    playWithNegativeTime.setIsPlaying(true);
    playWithNegativeTime.setTimeInSamples(-11);
    playWithNegativeTime.setBpm(140.0);
    playWithNegativeTime.setPpqPosition(24.0);
    engine.updateHostInfo(playWithNegativeTime);

    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8192.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 8192);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().ppqPosition, 24.0);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().bpm, 140.0);
}

TEST_F(TimingEngineTest, HostSyncToggleWhilePlayingWithMalformedTimelinePreservesSyncState)
{
    engine.setHostSyncEnabled(true);
    engine.setSyncToPlayhead(false);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(8000);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithTime;
    playWithTime.setIsPlaying(true);
    playWithTime.setTimeInSamples(9000);
    engine.updateHostInfo(playWithTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 9000.0);

    // Disable host sync while still playing.
    engine.setHostSyncEnabled(false);
    juce::AudioPlayHead::PositionInfo missingWhilePlaying;
    missingWhilePlaying.setIsPlaying(true);
    missingWhilePlaying.setBpm(128.0);
    missingWhilePlaying.setPpqPosition(16.0);
    engine.updateHostInfo(missingWhilePlaying);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 9000.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 9000);

    // Re-enable host sync while still playing and feed malformed negative sample.
    engine.setHostSyncEnabled(true);
    juce::AudioPlayHead::PositionInfo negativeWhilePlaying;
    negativeWhilePlaying.setIsPlaying(true);
    negativeWhilePlaying.setTimeInSamples(-5);
    negativeWhilePlaying.setBpm(129.0);
    negativeWhilePlaying.setPpqPosition(16.5);
    engine.updateHostInfo(negativeWhilePlaying);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 9000.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 9000);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().ppqPosition, 16.5);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().bpm, 129.0);

    // Next transition with missing sample should fallback to last known sample.
    juce::AudioPlayHead::PositionInfo stoppedWithTime;
    stoppedWithTime.setIsPlaying(false);
    stoppedWithTime.setTimeInSamples(9500);
    engine.updateHostInfo(stoppedWithTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 9500.0);

    juce::AudioPlayHead::PositionInfo playWithoutTime;
    playWithoutTime.setIsPlaying(true);
    playWithoutTime.setPpqPosition(17.0);
    engine.updateHostInfo(playWithoutTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 9500.0);
}

TEST_F(TimingEngineTest, HostSyncToggleWithSyncToPlayheadActiveIgnoresMalformedPlayingSamples)
{
    engine.setSyncToPlayhead(true);
    engine.setHostSyncEnabled(false);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(5000);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithTime;
    playWithTime.setIsPlaying(true);
    playWithTime.setTimeInSamples(6000);
    engine.updateHostInfo(playWithTime);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 6000.0);

    // Toggle host sync on while already playing.
    engine.setHostSyncEnabled(true);

    juce::AudioPlayHead::PositionInfo missingWhilePlaying;
    missingWhilePlaying.setIsPlaying(true);
    missingWhilePlaying.setBpm(131.0);
    missingWhilePlaying.setPpqPosition(20.0);
    engine.updateHostInfo(missingWhilePlaying);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 6000.0);

    juce::AudioPlayHead::PositionInfo negativeWhilePlaying;
    negativeWhilePlaying.setIsPlaying(true);
    negativeWhilePlaying.setTimeInSamples(-4);
    negativeWhilePlaying.setBpm(132.0);
    negativeWhilePlaying.setPpqPosition(20.5);
    engine.updateHostInfo(negativeWhilePlaying);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 6000.0);
    EXPECT_EQ(engine.getHostInfo().timeInSamples, 6000);
}

// =============================================================================
// Play Transition Reuse & Missing Samples
// =============================================================================

TEST_F(TimingEngineTest, HostInfoUpdate_PlayTransitionsReuseLatestKnownTimestampAcrossMissingSamples)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo stoppedWithTime;
    stoppedWithTime.setIsPlaying(false);
    stoppedWithTime.setTimeInSamples(4096);
    engine.updateHostInfo(stoppedWithTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithoutTime;
    playWithoutTime.setIsPlaying(true);
    engine.updateHostInfo(playWithoutTime);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 4096.0);

    juce::AudioPlayHead::PositionInfo stopWithNewTime;
    stopWithNewTime.setIsPlaying(false);
    stopWithNewTime.setTimeInSamples(8192);
    engine.updateHostInfo(stopWithNewTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8192.0);

    juce::AudioPlayHead::PositionInfo playWithoutTimeAgain;
    playWithoutTimeAgain.setIsPlaying(true);
    engine.updateHostInfo(playWithoutTimeAgain);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 8192.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_MissingSamplesWhilePlayingDoesNotRetriggerWithoutTransition)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo stopped;
    stopped.setIsPlaying(false);
    stopped.setTimeInSamples(12000);
    engine.updateHostInfo(stopped);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo playWithoutTime;
    playWithoutTime.setIsPlaying(true);
    engine.updateHostInfo(playWithoutTime);
    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo stillPlayingWithoutTime;
    stillPlayingWithoutTime.setIsPlaying(true);
    engine.updateHostInfo(stillPlayingWithoutTime);
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 12000.0);
}

TEST_F(TimingEngineTest, HostInfoUpdate_BackwardJumpAfterMissingSamplesStillTriggers)
{
    engine.setSyncToPlayhead(true);

    juce::AudioPlayHead::PositionInfo startPlaying;
    startPlaying.setIsPlaying(true);
    startPlaying.setTimeInSamples(14000);
    engine.updateHostInfo(startPlaying);
    EXPECT_TRUE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo missingOne;
    missingOne.setIsPlaying(true);
    engine.updateHostInfo(missingOne);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo missingTwo;
    missingTwo.setIsPlaying(true);
    engine.updateHostInfo(missingTwo);
    EXPECT_FALSE(engine.checkAndClearTrigger());

    juce::AudioPlayHead::PositionInfo backwardJump;
    backwardJump.setIsPlaying(true);
    backwardJump.setTimeInSamples(3000);
    engine.updateHostInfo(backwardJump);

    EXPECT_TRUE(engine.checkAndClearTrigger());
    EXPECT_FALSE(engine.checkAndClearTrigger());
    EXPECT_DOUBLE_EQ(engine.getConfig().lastSyncTimestamp, 3000.0);
}
