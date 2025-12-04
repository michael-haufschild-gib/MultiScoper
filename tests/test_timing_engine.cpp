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

    TimingMode lastMode = TimingMode::TIME;
    float lastInterval = 0.0f;
    float lastBPM = 0.0f;
    bool lastSyncState = false;

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
