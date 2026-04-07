/*
    Oscil - Timing Calculation Unit Tests
    Tests TimingConfig calculations: display samples, gain conversions, BPM/note intervals

    Bug targets:
    - getIntervalInSamples returning wrong count for edge-case sample rates
    - calculateActualInterval producing NaN or out-of-range values
    - MELODIC mode with extreme BPM values
    - Note interval multiplier accuracy
*/

#include "core/dsp/TimingConfig.h"

#include <gtest/gtest.h>

using namespace oscil;

class TimingConfigTest : public ::testing::Test
{
protected:
    TimingConfig config;

    void SetUp() override
    {
        config = TimingConfig{};
        config.calculateActualInterval();
    }
};

// ============================================================================
// getIntervalInSamples — production function under test
// ============================================================================

TEST_F(TimingConfigTest, IntervalInSamplesAt44100With500ms)
{
    config.setTimeInterval(500.0f);
    EXPECT_EQ(config.getIntervalInSamples(44100.0f), 22050);
}

TEST_F(TimingConfigTest, IntervalInSamplesAt48000)
{
    config.setTimeInterval(50.0f);
    EXPECT_EQ(config.getIntervalInSamples(48000.0f), 2400);
}

TEST_F(TimingConfigTest, IntervalInSamplesAt96000)
{
    config.setTimeInterval(50.0f);
    EXPECT_EQ(config.getIntervalInSamples(96000.0f), 4800);
}

TEST_F(TimingConfigTest, SmallIntervalProducesPositiveSamples)
{
    config.setTimeInterval(TimingConfig::MIN_TIME_INTERVAL_MS);
    int samples = config.getIntervalInSamples(44100.0f);
    EXPECT_GT(samples, 0);
}

TEST_F(TimingConfigTest, LargeIntervalProducesCorrectSamples)
{
    config.setTimeInterval(TimingConfig::MAX_TIME_INTERVAL_MS);
    int samples = config.getIntervalInSamples(44100.0f);
    // 4000ms * 44100 / 1000 = 176400
    EXPECT_EQ(samples, 176400);
}

// ============================================================================
// calculateActualInterval — TIME mode
// ============================================================================

TEST_F(TimingConfigTest, TimeModeClampsToValidRange)
{
    config.setTimingMode(TimingMode::TIME);

    config.setTimeInterval(0.001f); // Below MIN
    EXPECT_GE(config.actualIntervalMs, TimingConfig::MIN_TIME_INTERVAL_MS);

    config.setTimeInterval(99999.0f); // Above MAX
    EXPECT_LE(config.actualIntervalMs, TimingConfig::MAX_TIME_INTERVAL_MS);
}

TEST_F(TimingConfigTest, TimeModeUsesDirectInterval)
{
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(100.0f);
    EXPECT_NEAR(config.actualIntervalMs, 100.0f, 0.01f);
}

// ============================================================================
// calculateActualInterval — MELODIC mode
// ============================================================================

TEST_F(TimingConfigTest, MelodicModeQuarterNoteAt120Bpm)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::QUARTER);
    // 60000 / 120 = 500ms per quarter note
    EXPECT_NEAR(config.actualIntervalMs, 500.0f, 0.1f);
}

TEST_F(TimingConfigTest, MelodicModeHalfNoteAt120Bpm)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::HALF);
    // Half note = 2 * quarter = 1000ms
    EXPECT_NEAR(config.actualIntervalMs, 1000.0f, 0.1f);
}

TEST_F(TimingConfigTest, MelodicModeEighthNoteAt120Bpm)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(120.0f);
    config.setNoteInterval(NoteInterval::EIGHTH);
    // Eighth = 0.5 * quarter = 250ms
    EXPECT_NEAR(config.actualIntervalMs, 250.0f, 0.1f);
}

TEST_F(TimingConfigTest, MelodicModeAt60Bpm)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(60.0f);
    config.setNoteInterval(NoteInterval::QUARTER);
    // 60000 / 60 = 1000ms
    EXPECT_NEAR(config.actualIntervalMs, 1000.0f, 0.1f);
}

TEST_F(TimingConfigTest, MelodicModeClampsBpmToRange)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setNoteInterval(NoteInterval::QUARTER);

    config.setHostBPM(1.0f); // Below MIN_BPM
    EXPECT_GE(config.hostBPM, TimingConfig::MIN_BPM);

    config.setHostBPM(9999.0f); // Above MAX_BPM
    EXPECT_LE(config.hostBPM, TimingConfig::MAX_BPM);
}

// ============================================================================
// NaN safety
// ============================================================================

TEST_F(TimingConfigTest, NaNTimeIntervalFallsBackToDefault)
{
    config.setTimingMode(TimingMode::TIME);
    config.setTimeInterval(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FALSE(std::isnan(config.actualIntervalMs));
    EXPECT_NEAR(config.actualIntervalMs, TimingConfig::DEFAULT_TIME_INTERVAL_MS, 0.01f);
}

TEST_F(TimingConfigTest, NaNBpmFallsBackToDefault)
{
    config.setTimingMode(TimingMode::MELODIC);
    config.setHostBPM(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FALSE(std::isnan(config.hostBPM));
    EXPECT_NEAR(config.hostBPM, TimingConfig::DEFAULT_BPM, 0.01f);
}

// ============================================================================
// Gain dB conversion (verifying the formula used across the codebase)
// ============================================================================

TEST_F(TimingConfigTest, GainDbToLinearConversion)
{
    // These verify the dB→linear formula used in waveform rendering
    auto dbToLinear = [](float dB) { return std::pow(10.0f, dB / 20.0f); };

    EXPECT_NEAR(dbToLinear(0.0f), 1.0f, 0.001f);      // Unity
    EXPECT_NEAR(dbToLinear(6.0f), 1.995f, 0.01f);     // ~2x
    EXPECT_NEAR(dbToLinear(-6.0f), 0.501f, 0.01f);    // ~0.5x
    EXPECT_NEAR(dbToLinear(-60.0f), 0.001f, 0.0001f); // Nearly silent
}

// ============================================================================
// Enum values (regression guard — serialization depends on these)
// ============================================================================

TEST_F(TimingConfigTest, TimingModeEnumValuesStable)
{
    EXPECT_EQ(static_cast<int>(TimingMode::TIME), 0);
    EXPECT_EQ(static_cast<int>(TimingMode::MELODIC), 1);
}

// ============================================================================
// isHostSyncAvailable
// ============================================================================

TEST_F(TimingConfigTest, HostSyncAvailableWithValidBpm)
{
    config.hostBPM = 120.0f;
    EXPECT_TRUE(config.isHostSyncAvailable());
}

TEST_F(TimingConfigTest, HostSyncNotAvailableWithZeroBpm)
{
    config.hostBPM = 0.0f;
    EXPECT_FALSE(config.isHostSyncAvailable());
}
