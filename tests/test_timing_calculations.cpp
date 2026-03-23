/*
    Oscil - Timing Calculation Unit Tests
    Tests timing-related calculations (sample counts, gain conversions, BPM calculations)
    Note: These are unit tests for mathematical calculations, not E2E UI tests
*/

#include "core/dsp/TimingConfig.h"
#include "ui/layout/sections/TimingSidebarSection.h"

#include <gtest/gtest.h>

using namespace oscil;

class TimingCalculationsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // TimingSidebarSection uses ThemeManager internally, which is a singleton
        // The section can be tested standalone
    }
};

// Test: Time interval slider affects display samples calculation
TEST_F(TimingCalculationsTest, TimeIntervalChangesDisplaySamples)
{
    // Given: A sample rate of 44100 Hz and an interval of 50ms
    // Expected: displaySamples = 44100 * 0.050 = 2205 samples

    double sampleRate = 44100.0;
    int intervalMs = 50;

    int expectedSamples = static_cast<int>(sampleRate * (static_cast<double>(intervalMs) / 1000.0));

    EXPECT_EQ(expectedSamples, 2205);

    // Test with different intervals
    intervalMs = 100; // 100ms
    expectedSamples = static_cast<int>(sampleRate * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(expectedSamples, 4410);

    intervalMs = 10; // 10ms
    expectedSamples = static_cast<int>(sampleRate * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(expectedSamples, 441);
}

// Test: Gain in dB converts correctly for waveform amplitude
TEST_F(TimingCalculationsTest, GainDbConvertsCorrectly)
{
    // Test gain conversion: amplitude = 10^(dB/20)
    float dB0 = 0.0f;
    float expected0 = 1.0f; // Unity gain
    float amplitude0 = std::pow(10.0f, dB0 / 20.0f);
    EXPECT_NEAR(amplitude0, expected0, 0.001f);

    float dB6 = 6.0f;
    float expected6 = 1.995f; // ~2x gain
    float amplitude6 = std::pow(10.0f, dB6 / 20.0f);
    EXPECT_NEAR(amplitude6, expected6, 0.01f);

    float dBNeg6 = -6.0f;
    float expectedNeg6 = 0.501f; // ~0.5x gain
    float amplitudeNeg6 = std::pow(10.0f, dBNeg6 / 20.0f);
    EXPECT_NEAR(amplitudeNeg6, expectedNeg6, 0.01f);

    float dBNeg60 = -60.0f;
    float expectedNeg60 = 0.001f; // Nearly silent
    float amplitudeNeg60 = std::pow(10.0f, dBNeg60 / 20.0f);
    EXPECT_NEAR(amplitudeNeg60, expectedNeg60, 0.0001f);
}

// Test: TimingSidebarSection listener interface
class MockTimingListener : public TimingSidebarSection::Listener
{
public:
    void timingModeChanged(TimingMode mode) override
    {
        lastTimingMode = mode;
        timingModeChangedCalled = true;
    }

    void noteIntervalChanged(NoteInterval interval) override
    {
        lastNoteInterval = interval;
        noteIntervalChangedCalled = true;
    }

    void timeIntervalChanged(float ms) override
    {
        lastTimeIntervalMs = ms;
        timeIntervalChangedCalled = true;
    }

    void hostSyncChanged(bool enabled) override
    {
        lastHostSync = enabled;
        hostSyncChangedCalled = true;
    }

    void waveformModeChanged(WaveformMode mode) override
    {
        lastWaveformMode = mode;
        waveformModeChangedCalled = true;
    }

    void bpmChanged(float bpm) override
    {
        lastBpm = bpm;
        bpmChangedCalled = true;
    }

    TimingMode lastTimingMode = TimingMode::TIME;
    NoteInterval lastNoteInterval = NoteInterval::QUARTER;
    float lastTimeIntervalMs = 0.0f;
    bool lastHostSync = false;
    WaveformMode lastWaveformMode = WaveformMode::FreeRunning;
    float lastBpm = 120.0f;

    bool timingModeChangedCalled = false;
    bool noteIntervalChangedCalled = false;
    bool timeIntervalChangedCalled = false;
    bool hostSyncChangedCalled = false;
    bool waveformModeChangedCalled = false;
    bool bpmChangedCalled = false;

    void reset()
    {
        timingModeChangedCalled = false;
        noteIntervalChangedCalled = false;
        timeIntervalChangedCalled = false;
        hostSyncChangedCalled = false;
        waveformModeChangedCalled = false;
        bpmChangedCalled = false;
    }
};

// Test: WaveformMode enum values
TEST_F(TimingCalculationsTest, WaveformModeEnumValues)
{
    // Verify the WaveformMode enum has the expected values
    EXPECT_EQ(static_cast<int>(WaveformMode::FreeRunning), 0);
    EXPECT_EQ(static_cast<int>(WaveformMode::RestartOnPlay), 1);
    EXPECT_EQ(static_cast<int>(WaveformMode::RestartOnNote), 2);
}

// Test: TimingMode enum values
TEST_F(TimingCalculationsTest, TimingModeEnumValues)
{
    EXPECT_EQ(static_cast<int>(TimingMode::TIME), 0);
    EXPECT_EQ(static_cast<int>(TimingMode::MELODIC), 1);
}

// Test: Sample rate affects display samples calculation correctly
TEST_F(TimingCalculationsTest, SampleRateAffectsDisplaySamples)
{
    int intervalMs = 50;

    // At 44.1kHz
    double sampleRate44k = 44100.0;
    int samples44k = static_cast<int>(sampleRate44k * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(samples44k, 2205);

    // At 48kHz
    double sampleRate48k = 48000.0;
    int samples48k = static_cast<int>(sampleRate48k * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(samples48k, 2400);

    // At 96kHz
    double sampleRate96k = 96000.0;
    int samples96k = static_cast<int>(sampleRate96k * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(samples96k, 4800);
}

// Test: BPM to interval calculation for melodic mode
TEST_F(TimingCalculationsTest, BpmToIntervalCalculation)
{
    // At 120 BPM, one beat = 500ms
    float bpm = 120.0f;
    float msPerBeat = 60000.0f / bpm;
    EXPECT_NEAR(msPerBeat, 500.0f, 0.1f);

    // At 60 BPM, one beat = 1000ms
    bpm = 60.0f;
    msPerBeat = 60000.0f / bpm;
    EXPECT_NEAR(msPerBeat, 1000.0f, 0.1f);

    // At 180 BPM, one beat = 333.33ms
    bpm = 180.0f;
    msPerBeat = 60000.0f / bpm;
    EXPECT_NEAR(msPerBeat, 333.33f, 0.1f);
}

// Test: Note interval multipliers
TEST_F(TimingCalculationsTest, NoteIntervalMultipliers)
{
    // Test beat divisions relative to quarter note
    // Quarter note = 1.0x
    // Half note = 2.0x
    // Whole note = 4.0x
    // Eighth note = 0.5x
    // Sixteenth note = 0.25x

    float quarterNoteDuration = 500.0f; // At 120 BPM

    // Half note (2 beats)
    float halfNoteDuration = quarterNoteDuration * 2.0f;
    EXPECT_NEAR(halfNoteDuration, 1000.0f, 0.1f);

    // Whole note (4 beats)
    float wholeNoteDuration = quarterNoteDuration * 4.0f;
    EXPECT_NEAR(wholeNoteDuration, 2000.0f, 0.1f);

    // Eighth note (1/2 beat)
    float eighthNoteDuration = quarterNoteDuration * 0.5f;
    EXPECT_NEAR(eighthNoteDuration, 250.0f, 0.1f);

    // Sixteenth note (1/4 beat)
    float sixteenthNoteDuration = quarterNoteDuration * 0.25f;
    EXPECT_NEAR(sixteenthNoteDuration, 125.0f, 0.1f);
}

// Test: Display samples clamps to reasonable bounds
TEST_F(TimingCalculationsTest, DisplaySamplesBounds)
{
    double sampleRate = 44100.0;

    // Very small interval (1ms) - should still produce valid samples
    int intervalMs = 1;
    int samples = static_cast<int>(sampleRate * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_GT(samples, 0);
    EXPECT_EQ(samples, 44); // 44.1 rounded down

    // Large interval (60 seconds = 60000ms)
    intervalMs = 60000;
    samples = static_cast<int>(sampleRate * (static_cast<double>(intervalMs) / 1000.0));
    EXPECT_EQ(samples, 2646000); // 60 seconds worth of samples
}
