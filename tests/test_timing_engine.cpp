/*
    Oscil - Timing Engine Tests (Core)
    Default config, TIME mode, MELODIC mode, trigger detection, manual trigger
*/

#include "test_timing_engine_fixture.h"

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
