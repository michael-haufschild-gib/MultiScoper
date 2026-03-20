/*
    Oscil - Timing Engine Tests
    Comprehensive tests for DSP timing and synchronization
*/

#include <gtest/gtest.h>
#include "core/dsp/TimingEngine.h"
#include <thread>
#include <atomic>
#include <cmath>

using namespace oscil;

class TimingEngineTest : public ::testing::Test
{
protected:
    TimingEngine engine;

    void SetUp() override
    {
        // Reset to default state
        EngineTimingConfig defaultConfig;
        engine.setConfig(defaultConfig);
    }

    // Helper to generate audio buffer with specific waveform
    juce::AudioBuffer<float> generateSineWave(int numSamples, float frequency, float amplitude, float sampleRate)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float value = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate);
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, value);
        }
        return buffer;
    }

    // Helper to generate ramp signal (for trigger testing)
    juce::AudioBuffer<float> generateRamp(int numSamples, float startValue, float endValue)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(numSamples - 1);
            float value = startValue + t * (endValue - startValue);
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, value);
        }
        return buffer;
    }

    // Helper to generate step signal
    juce::AudioBuffer<float> generateStep(int numSamples, float lowValue, float highValue, int stepPosition)
    {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            float value = (i < stepPosition) ? lowValue : highValue;
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, value);
        }
        return buffer;
    }
};

// =============================================================================
// Default Configuration Tests
// =============================================================================

TEST_F(TimingEngineTest, DefaultConfiguration)
{
    const auto& config = engine.getConfig();

    EXPECT_EQ(config.timingMode, TimingMode::TIME);
    EXPECT_FLOAT_EQ(config.timeIntervalMs, 500.0f);  // New default: 500ms
    EXPECT_EQ(config.noteInterval, EngineNoteInterval::NOTE_1_4TH);
    EXPECT_FALSE(config.hostSyncEnabled);
    EXPECT_FLOAT_EQ(config.hostBPM, 120.0f);
    EXPECT_EQ(config.triggerMode, WaveformTriggerMode::None);
    EXPECT_FLOAT_EQ(config.triggerThreshold, 0.1f);
}

TEST_F(TimingEngineTest, DefaultHostInfo)
{
    const auto& hostInfo = engine.getHostInfo();

    EXPECT_DOUBLE_EQ(hostInfo.bpm, 120.0);
    EXPECT_DOUBLE_EQ(hostInfo.ppqPosition, 0.0);
    EXPECT_FALSE(hostInfo.isPlaying);
    EXPECT_DOUBLE_EQ(hostInfo.sampleRate, 44100.0);
}

// =============================================================================
// TIME Mode Tests
// =============================================================================

TEST_F(TimingEngineTest, TimeModeIntervalCalculation)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(500);

    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);
    EXPECT_DOUBLE_EQ(engine.getWindowSizeSeconds(), 0.5);
}

TEST_F(TimingEngineTest, TimeModeDisplaySampleCount)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(100); // 100ms

    // At 44100 Hz, 100ms = 4410 samples
    EXPECT_EQ(engine.getDisplaySampleCount(44100.0), 4410);

    // At 48000 Hz, 100ms = 4800 samples
    EXPECT_EQ(engine.getDisplaySampleCount(48000.0), 4800);

    // At 96000 Hz, 100ms = 9600 samples
    EXPECT_EQ(engine.getDisplaySampleCount(96000.0), 9600);
}

TEST_F(TimingEngineTest, TimeIntervalClamping_Minimum)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(0.05f); // Below minimum (0.1ms)

    EXPECT_GE(engine.getActualIntervalMs(), EngineTimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingEngineTest, TimeIntervalClamping_Maximum)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(10000.0f); // Above maximum (4000ms)

    EXPECT_LE(engine.getActualIntervalMs(), EngineTimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingEngineTest, TimeIntervalClamping_Negative)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(-100.0f);

    EXPECT_GE(engine.getActualIntervalMs(), EngineTimingConfig::MIN_TIME_INTERVAL_MS);
}

TEST_F(TimingEngineTest, TimeIntervalClamping_Zero)
{
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(0.0f);

    EXPECT_GE(engine.getActualIntervalMs(), EngineTimingConfig::MIN_TIME_INTERVAL_MS);
}

// =============================================================================
// MELODIC Mode Tests
// =============================================================================

TEST_F(TimingEngineTest, MelodicModeQuarterNote_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_4TH);

    // Simulate host BPM update
    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, quarter note = 500ms
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);
}

TEST_F(TimingEngineTest, MelodicModeWholeNote_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_1);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, whole note = 2000ms
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 2000.0f);
}

TEST_F(TimingEngineTest, MelodicModeEighthNote_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_8TH);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, eighth note = 250ms
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 250.0f);
}

TEST_F(TimingEngineTest, MelodicModeSixteenthNote_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_16TH);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, sixteenth note = 125ms
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 125.0f);
}

TEST_F(TimingEngineTest, MelodicModeDottedQuarter_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_DOTTED_1_4TH);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, dotted quarter = 750ms (1.5 * 500)
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 750.0f);
}

TEST_F(TimingEngineTest, MelodicModeTripletQuarter_120BPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_TRIPLET_1_4TH);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    engine.setConfig(config);

    // At 120 BPM, quarter triplet = 333.33ms (2/3 * 500)
    EXPECT_NEAR(engine.getActualIntervalMs(), 333.33f, 0.5f);
}

TEST_F(TimingEngineTest, MelodicModeBPMChange)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_4TH);
    engine.setHostSyncEnabled(true);  // Enable host sync so hostBPM is used

    // 120 BPM -> 500ms
    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    config.hostSyncEnabled = true;  // Ensure sync is enabled in config too
    engine.setConfig(config);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);

    // 60 BPM -> 1000ms
    config.hostBPM = 60.0f;
    engine.setConfig(config);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 1000.0f);

    // 240 BPM -> 250ms
    config.hostBPM = 240.0f;
    engine.setConfig(config);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 250.0f);
}

TEST_F(TimingEngineTest, MelodicModeVerySlowBPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_8_1); // 8 bars

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 20.0f; // Minimum BPM
    engine.setConfig(config);

    // 8 bars at 20 BPM = 32 beats * (60000/20) = 96000ms
    // But clamped to MAX_TIME_INTERVAL_MS (4000)
    EXPECT_LE(engine.getActualIntervalMs(), EngineTimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingEngineTest, MelodicModeVeryFastBPM)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_32ND);

    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 300.0f; // Maximum BPM
    engine.setConfig(config);

    // 1/32 at 300 BPM = 0.125 beats * (60000/300) = 25ms
    // Should be above MIN_TIME_INTERVAL_MS (0.1)
    EXPECT_GE(engine.getActualIntervalMs(), EngineTimingConfig::MIN_TIME_INTERVAL_MS);
}

// =============================================================================
// Trigger Detection Tests
// =============================================================================

TEST_F(TimingEngineTest, TriggerModeNone_AlwaysReturnsTrue)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::None);

    auto buffer = generateSineWave(512, 440.0f, 1.0f, 44100.0f);
    EXPECT_TRUE(engine.processBlock(buffer));
}

TEST_F(TimingEngineTest, TriggerRisingEdge_DetectsRisingCross)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.5f);

    // First process a low sample to set previous state
    auto lowBuffer = generateStep(64, 0.0f, 0.0f, 64);
    (void)engine.processBlock(lowBuffer);

    // Now process buffer with rising edge crossing 0.5
    auto risingBuffer = generateStep(64, 0.2f, 0.8f, 32);
    EXPECT_TRUE(engine.processBlock(risingBuffer));
}

TEST_F(TimingEngineTest, TriggerRisingEdge_NoTriggerWhenAlreadyHigh)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.5f);

    // Start with high sample
    auto highBuffer = generateStep(64, 0.8f, 0.8f, 64);
    (void)engine.processBlock(highBuffer);

    // Stay high - should not trigger
    auto stillHighBuffer = generateStep(64, 0.9f, 1.0f, 32);
    EXPECT_FALSE(engine.processBlock(stillHighBuffer));
}

TEST_F(TimingEngineTest, TriggerFallingEdge_DetectsFallingCross)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::FallingEdge);
    engine.setTriggerThreshold(0.5f);

    // First process a high sample
    auto highBuffer = generateStep(64, 1.0f, 1.0f, 64);
    (void)engine.processBlock(highBuffer);

    // Now process buffer with falling edge crossing 0.5
    auto fallingBuffer = generateStep(64, 0.8f, 0.2f, 32);
    EXPECT_TRUE(engine.processBlock(fallingBuffer));
}

TEST_F(TimingEngineTest, TriggerFallingEdge_NoTriggerWhenAlreadyLow)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::FallingEdge);
    engine.setTriggerThreshold(0.5f);

    // Start with low sample
    auto lowBuffer = generateStep(64, 0.2f, 0.2f, 64);
    (void)engine.processBlock(lowBuffer);

    // Stay low - should not trigger
    auto stillLowBuffer = generateStep(64, 0.1f, 0.0f, 32);
    EXPECT_FALSE(engine.processBlock(stillLowBuffer));
}

TEST_F(TimingEngineTest, TriggerLevel_DetectsLevelExceedance)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
    engine.setTriggerThreshold(0.5f);

    // Start below threshold
    auto lowBuffer = generateStep(64, 0.2f, 0.2f, 64);
    (void)engine.processBlock(lowBuffer);

    // Jump above threshold - should trigger
    auto highBuffer = generateStep(64, 0.2f, 0.8f, 32);
    EXPECT_TRUE(engine.processBlock(highBuffer));
}

TEST_F(TimingEngineTest, TriggerLevel_NoRetriggerWhileHigh)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
    engine.setTriggerThreshold(0.5f);

    // First trigger
    auto lowBuffer = generateStep(64, 0.2f, 0.2f, 64);
    (void)engine.processBlock(lowBuffer);

    auto highBuffer = generateStep(64, 0.2f, 0.8f, 32);
    EXPECT_TRUE(engine.processBlock(highBuffer));

    // Stay high - should NOT retrigger
    auto stillHighBuffer = generateStep(64, 0.9f, 0.9f, 64);
    EXPECT_FALSE(engine.processBlock(stillHighBuffer));
}

TEST_F(TimingEngineTest, TriggerBothEdges_DetectsRisingCross)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::BothEdges);
    engine.setTriggerThreshold(0.5f);

    // Start with low sample to set previous state
    auto lowBuffer = generateStep(64, 0.0f, 0.0f, 64);
    (void)engine.processBlock(lowBuffer);

    // Rising edge crossing 0.5 should trigger
    auto risingBuffer = generateStep(64, 0.2f, 0.8f, 32);
    EXPECT_TRUE(engine.processBlock(risingBuffer));
}

TEST_F(TimingEngineTest, TriggerBothEdges_DetectsFallingCross)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::BothEdges);
    engine.setTriggerThreshold(0.5f);

    // Start with high sample
    auto highBuffer = generateStep(64, 1.0f, 1.0f, 64);
    (void)engine.processBlock(highBuffer);

    // Falling edge crossing 0.5 should trigger
    auto fallingBuffer = generateStep(64, 0.8f, 0.2f, 32);
    EXPECT_TRUE(engine.processBlock(fallingBuffer));
}

TEST_F(TimingEngineTest, TriggerBothEdges_NoTriggerWhenStable)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::BothEdges);
    engine.setTriggerThreshold(0.5f);

    // Start below threshold
    auto lowBuffer = generateStep(64, 0.2f, 0.2f, 64);
    (void)engine.processBlock(lowBuffer);

    // Stay below threshold - should not trigger
    auto stillLowBuffer = generateStep(64, 0.1f, 0.3f, 32);
    EXPECT_FALSE(engine.processBlock(stillLowBuffer));
}

TEST_F(TimingEngineTest, ApplyEntityConfig_TriggerEdgeBoth_MapsToBothEdges)
{
    TimingConfig entityConfig;
    entityConfig.triggerMode = TriggerMode::TRIGGERED;
    entityConfig.triggerEdge = TriggerEdge::Both;

    engine.applyEntityConfig(entityConfig);

    EXPECT_EQ(engine.getConfig().triggerMode, WaveformTriggerMode::BothEdges);
}

TEST_F(TimingEngineTest, TriggerThresholdBoundary_Zero)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
    engine.setTriggerThreshold(0.0f);

    // With threshold at 0, any positive signal should trigger
    auto buffer = generateStep(64, -0.1f, 0.1f, 32);
    (void)engine.processBlock(buffer);

    // Check threshold is valid
    EXPECT_GE(engine.getConfig().triggerThreshold, 0.0f);
}

TEST_F(TimingEngineTest, TriggerThresholdBoundary_One)
{
    engine.setTriggerThreshold(1.0f);
    EXPECT_FLOAT_EQ(engine.getConfig().triggerThreshold, 1.0f);
}

TEST_F(TimingEngineTest, TriggerThresholdClamping_Above)
{
    engine.setTriggerThreshold(1.5f);
    EXPECT_LE(engine.getConfig().triggerThreshold, 1.0f);
}

TEST_F(TimingEngineTest, TriggerThresholdClamping_Below)
{
    engine.setTriggerThreshold(-0.5f);
    EXPECT_GE(engine.getConfig().triggerThreshold, 0.0f);
}

// =============================================================================
// Manual Trigger Tests
// =============================================================================

TEST_F(TimingEngineTest, ManualTrigger_RequestAndClear)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    // Initially no trigger
    EXPECT_FALSE(engine.checkAndClearManualTrigger());

    // Request trigger
    engine.requestManualTrigger();
    EXPECT_TRUE(engine.checkAndClearManualTrigger());

    // Should be cleared after check
    EXPECT_FALSE(engine.checkAndClearManualTrigger());
}

TEST_F(TimingEngineTest, ManualTrigger_ProcessBlockReturnsTriggerState)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    auto buffer = generateSineWave(512, 440.0f, 1.0f, 44100.0f);

    // No trigger requested
    EXPECT_FALSE(engine.processBlock(buffer));

    // Request trigger
    engine.requestManualTrigger();
    EXPECT_TRUE(engine.processBlock(buffer));

    // Manual trigger persists until explicitly cleared (e.g. by visualizer)
    // This prevents the audio thread from consuming a trigger meant for the UI/Render thread
    EXPECT_TRUE(engine.processBlock(buffer));

    // Clear it explicitly
    (void)engine.checkAndClearManualTrigger();
    EXPECT_FALSE(engine.processBlock(buffer));
}

TEST_F(TimingEngineTest, ManualTrigger_MultipleRequests)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    // Multiple requests should still only give one trigger
    engine.requestManualTrigger();
    engine.requestManualTrigger();
    engine.requestManualTrigger();

    EXPECT_TRUE(engine.checkAndClearManualTrigger());
    EXPECT_FALSE(engine.checkAndClearManualTrigger());
}

// =============================================================================
// Host Info Update Tests
// =============================================================================

TEST_F(TimingEngineTest, HostInfoUpdate_BPMChange)
{
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setNoteInterval(EngineNoteInterval::NOTE_1_4TH);
    engine.setHostSyncEnabled(true);  // Enable host sync so hostBPM affects interval

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
    (void)engine.processBlock(lowBuffer);

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
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{ 7, 8 });
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
    (void)engine.processBlock(lowBuffer);
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
    (void)engine.processBlock(highBuffer);
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

TEST_F(TimingEngineTest, DeserializationFromInvalidTree)
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

TEST_F(TimingEngineTest, FromValueTree_InvalidTypeResetsRuntimeStateAndPreservesConfig)
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
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{ 7, 8 });
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

TEST_F(TimingEngineTest, DeserializationWithMissingProperties)
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

TEST_F(TimingEngineTest, DeserializationClampsInvalidEnumsAndRanges)
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

TEST_F(TimingEngineTest, ApplyEntityConfig_TimingMode)
{
    TimingConfig entityConfig;
    entityConfig.timingMode = TimingMode::MELODIC;
    entityConfig.noteInterval = NoteInterval::EIGHTH;
    entityConfig.hostBPM = 120.0f;

    engine.applyEntityConfig(entityConfig);

    EXPECT_EQ(engine.getConfig().timingMode, TimingMode::MELODIC);
    EXPECT_EQ(engine.getConfig().noteInterval, EngineNoteInterval::NOTE_1_8TH);
}

TEST_F(TimingEngineTest, ApplyEntityConfig_TriggerModeConversion)
{
    TimingConfig entityConfig;
    entityConfig.triggerMode = TriggerMode::TRIGGERED;
    entityConfig.triggerEdge = TriggerEdge::Rising;

    engine.applyEntityConfig(entityConfig);

    EXPECT_EQ(engine.getConfig().triggerMode, WaveformTriggerMode::RisingEdge);
}

TEST_F(TimingEngineTest, ApplyEntityConfig_TriggerThresholdConversion)
{
    TimingConfig entityConfig;
    entityConfig.triggerMode = TriggerMode::TRIGGERED;
    entityConfig.triggerEdge = TriggerEdge::Rising;
    entityConfig.triggerThreshold = -6.0f; // -6 dBFS

    engine.applyEntityConfig(entityConfig);

    // -6 dBFS = 10^(-6/20) = ~0.501
    EXPECT_NEAR(engine.getConfig().triggerThreshold, 0.501f, 0.01f);
}

TEST_F(TimingEngineTest, SetConfigAfterFromValueTree_DoesNotResurrectRuntimeResetState)
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

TEST_F(TimingEngineTest, ApplyEntityConfigAfterFromValueTree_DoesNotResurrectRuntimeResetState)
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

TEST_F(TimingEngineTest, ToEntityConfig_RoundTrip)
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

TEST_F(TimingEngineTest, EngineNoteIntervalToBeats_AllValues)
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

TEST_F(TimingEngineTest, EngineNoteIntervalToString_AllValues)
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

TEST_F(TimingEngineTest, EntityToEngineNoteIntervalConversion)
{
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::QUARTER), EngineNoteInterval::NOTE_1_4TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::EIGHTH), EngineNoteInterval::NOTE_1_8TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::SIXTEENTH), EngineNoteInterval::NOTE_1_16TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::DOTTED_QUARTER), EngineNoteInterval::NOTE_DOTTED_1_4TH);
    EXPECT_EQ(entityToEngineNoteInterval(NoteInterval::TRIPLET_QUARTER), EngineNoteInterval::NOTE_TRIPLET_1_4TH);
}

TEST_F(TimingEngineTest, EngineToEntityNoteIntervalConversion)
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

TEST_F(TimingEngineTest, ProcessBlock_EmptyBuffer)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);

    juce::AudioBuffer<float> emptyBuffer(2, 0);
    // Should not crash and should return false (no trigger)
    EXPECT_FALSE(engine.processBlock(emptyBuffer));
}

TEST_F(TimingEngineTest, ProcessBlock_SingleSampleBuffer)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.5f);

    juce::AudioBuffer<float> singleSample(2, 1);
    singleSample.setSample(0, 0, 0.8f);
    singleSample.setSample(1, 0, 0.8f);

    // Should not crash
    (void)engine.processBlock(singleSample);
}

TEST_F(TimingEngineTest, ProcessBlock_MonoBuffer)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
    engine.setTriggerThreshold(0.5f);

    juce::AudioBuffer<float> monoBuffer(1, 64);
    monoBuffer.clear();
    for (int i = 32; i < 64; ++i)
        monoBuffer.setSample(0, i, 0.8f);

    // Should not crash with mono buffer
    (void)engine.processBlock(monoBuffer);
}

TEST_F(TimingEngineTest, SetSampleRate)
{
    engine.setSampleRate(96000.0);
    EXPECT_DOUBLE_EQ(engine.getHostInfo().sampleRate, 96000.0);
}

TEST_F(TimingEngineTest, ModeSwitchRecalculates)
{
    // Start in TIME mode
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(500);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);

    // Set up MELODIC parameters
    EngineTimingConfig config = engine.getConfig();
    config.hostBPM = 120.0f;
    config.noteInterval = EngineNoteInterval::NOTE_1_4TH;
    engine.setConfig(config);

    // Still in TIME mode, should use time interval
    engine.setTimingMode(TimingMode::TIME);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);

    // Switch to MELODIC - should recalculate to 500ms (quarter at 120 BPM)
    engine.setTimingMode(TimingMode::MELODIC);
    EXPECT_FLOAT_EQ(engine.getActualIntervalMs(), 500.0f);
}

// =============================================================================
// Listener Tests
// =============================================================================

class TestTimingListener : public TimingEngine::Listener
{
public:
    int modeChangedCount = 0;
    int intervalChangedCount = 0;
    int bpmChangedCount = 0;
    int syncStateChangedCount = 0;
    int timeSignatureChangedCount = 0;

    TimingMode lastMode = TimingMode::TIME;
    float lastInterval = 0.0f;
    float lastBPM = 0.0f;
    bool lastSyncState = false;
    int lastNumerator = 4;
    int lastDenominator = 4;

    void timingModeChanged(TimingMode mode) override
    {
        modeChangedCount++;
        lastMode = mode;
    }

    void intervalChanged(float actualIntervalMs) override
    {
        intervalChangedCount++;
        lastInterval = actualIntervalMs;
    }

    void hostBPMChanged(float bpm) override
    {
        bpmChangedCount++;
        lastBPM = bpm;
    }

    void hostSyncStateChanged(bool enabled) override
    {
        syncStateChangedCount++;
        lastSyncState = enabled;
    }

    void timeSignatureChanged(int numerator, int denominator) override
    {
        timeSignatureChangedCount++;
        lastNumerator = numerator;
        lastDenominator = denominator;
    }
};

TEST_F(TimingEngineTest, Listener_DeterministicNotification)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // 1. Change mode
    engine.setTimingMode(TimingMode::MELODIC);
    
    // Verify listener NOT called yet (it's pending)
    EXPECT_EQ(listener.modeChangedCount, 0);
    
    // Dispatch updates
    engine.dispatchPendingUpdates();
    
    // Verify listener CALLED
    EXPECT_EQ(listener.modeChangedCount, 1);
    EXPECT_EQ(listener.lastMode, TimingMode::MELODIC);

    // 2. Change Interval
    // Switch back to TIME mode first so timeIntervalMs affects actualIntervalMs
    engine.setTimingMode(TimingMode::TIME);
    engine.dispatchPendingUpdates(); // Dispatch mode change and potential interval change (if switching to TIME changed it)
    
    // Reset counts for clarity or just check increment
    int prevIntervalCount = listener.intervalChangedCount;
    
    engine.setTimeIntervalMs(123.4f);
    // Should be pending
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount);
    
    engine.dispatchPendingUpdates();
    // Should have incremented
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount + 1);
    
    engine.setTimeIntervalMs(250.0f);
    engine.dispatchPendingUpdates();
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount + 2);
    EXPECT_FLOAT_EQ(listener.lastInterval, 250.0f);

    engine.removeListener(&listener);
}

TEST_F(TimingEngineTest, Listener_AddAndRemove)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // Change mode
    engine.setTimingMode(TimingMode::MELODIC);
    engine.dispatchPendingUpdates();
    EXPECT_EQ(listener.modeChangedCount, 1);

    engine.removeListener(&listener);

    // Change mode again
    engine.setTimingMode(TimingMode::TIME);
    engine.dispatchPendingUpdates();
    
    // Should NOT have incremented
    EXPECT_EQ(listener.modeChangedCount, 1);
}

TEST_F(TimingEngineTest, FromValueTree_ClearsPendingNotificationFlags)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // Queue all pending notification flags while returning to persisted defaults.
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(700.0f);
    engine.setTimeIntervalMs(500.0f);
    engine.setHostSyncEnabled(true);
    engine.setHostSyncEnabled(false);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(147.0);
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{ 7, 8 });
    engine.updateHostInfo(posInfo);

    EXPECT_EQ(listener.modeChangedCount, 0);
    EXPECT_EQ(listener.intervalChangedCount, 0);
    EXPECT_EQ(listener.bpmChangedCount, 0);
    EXPECT_EQ(listener.syncStateChangedCount, 0);
    EXPECT_EQ(listener.timeSignatureChangedCount, 0);

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);
    engine.dispatchPendingUpdates();

    EXPECT_EQ(listener.modeChangedCount, 0);
    EXPECT_EQ(listener.intervalChangedCount, 0);
    EXPECT_EQ(listener.bpmChangedCount, 0);
    EXPECT_EQ(listener.syncStateChangedCount, 0);
    EXPECT_EQ(listener.timeSignatureChangedCount, 0);

    engine.removeListener(&listener);
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_F(TimingEngineTest, ThreadSafety_ConcurrentConfigChanges)
{
    std::atomic<bool> running{true};
    std::atomic<int> iterations{0};

    // Thread that changes timing mode
    std::thread modeChanger([this, &running, &iterations]()
    {
        while (running)
        {
            engine.setTimingMode(TimingMode::TIME);
            engine.setTimingMode(TimingMode::MELODIC);
            iterations++;
        }
    });

    // Thread that reads config
    std::thread configReader([this, &running]()
    {
        while (running)
        {
            auto config = engine.getConfig();
            auto interval = engine.getActualIntervalMs();
            (void)config;
            (void)interval;
        }
    });

    // Thread that changes interval
    std::thread intervalChanger([this, &running]()
    {
        int i = 0;
        while (running)
        {
            engine.setTimeIntervalMs(100 + (i % 1000));
            i++;
        }
    });

    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    modeChanger.join();
    configReader.join();
    intervalChanger.join();

    // Should have completed many iterations without crashing
    EXPECT_GT(iterations.load(), 10);
}

TEST_F(TimingEngineTest, ThreadSafety_ManualTriggerAtomic)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    std::atomic<int> triggerCount{0};
    std::atomic<bool> running{true};

    // Thread that requests triggers
    std::thread requester([this, &running]()
    {
        while (running)
        {
            engine.requestManualTrigger();
        }
    });

    // Thread that checks/clears triggers
    std::thread checker([this, &running, &triggerCount]()
    {
        while (running)
        {
            if (engine.checkAndClearManualTrigger())
            {
                triggerCount++;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    requester.join();
    checker.join();

    // Should have detected some triggers
    EXPECT_GT(triggerCount.load(), 0);
}
