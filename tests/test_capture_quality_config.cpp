/*
    Oscil - Capture Quality Configuration Unit Tests
    Tests for quality presets, memory budget calculations, and buffer sizing
*/

#include <gtest/gtest.h>
#include "core/dsp/CaptureQualityConfig.h"

using namespace oscil;

//==============================================================================
// Quality Preset Tests
//==============================================================================

class CaptureQualityPresetTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(CaptureQualityPresetTest, PresetToStringConversion)
{
    EXPECT_EQ(qualityPresetToString(QualityPreset::Eco), "Eco");
    EXPECT_EQ(qualityPresetToString(QualityPreset::Standard), "Standard");
    EXPECT_EQ(qualityPresetToString(QualityPreset::High), "High");
    EXPECT_EQ(qualityPresetToString(QualityPreset::Ultra), "Ultra");
}

TEST_F(CaptureQualityPresetTest, StringToPresetConversion)
{
    EXPECT_EQ(stringToQualityPreset("Eco"), QualityPreset::Eco);
    EXPECT_EQ(stringToQualityPreset("Standard"), QualityPreset::Standard);
    EXPECT_EQ(stringToQualityPreset("High"), QualityPreset::High);
    EXPECT_EQ(stringToQualityPreset("Ultra"), QualityPreset::Ultra);
    // Default fallback
    EXPECT_EQ(stringToQualityPreset("InvalidPreset"), QualityPreset::Standard);
    EXPECT_EQ(stringToQualityPreset(""), QualityPreset::Standard);
}

TEST_F(CaptureQualityPresetTest, DisplayNamesAreDescriptive)
{
    EXPECT_TRUE(qualityPresetToDisplayName(QualityPreset::Eco).contains("11"));
    EXPECT_TRUE(qualityPresetToDisplayName(QualityPreset::Standard).contains("22"));
    EXPECT_TRUE(qualityPresetToDisplayName(QualityPreset::High).contains("44"));
    EXPECT_TRUE(qualityPresetToDisplayName(QualityPreset::Ultra).contains("Source"));
}

//==============================================================================
// Capture Rate Calculation Tests
//==============================================================================

class CaptureRateCalculationTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(CaptureRateCalculationTest, EcoPresetReturns11kHz)
{
    config.qualityPreset = QualityPreset::Eco;
    EXPECT_EQ(config.getCaptureRate(44100), CaptureRate::ECO);
    EXPECT_EQ(config.getCaptureRate(48000), CaptureRate::ECO);
    EXPECT_EQ(config.getCaptureRate(96000), CaptureRate::ECO);
    EXPECT_EQ(config.getCaptureRate(192000), CaptureRate::ECO);
}

TEST_F(CaptureRateCalculationTest, StandardPresetReturns22kHz)
{
    config.qualityPreset = QualityPreset::Standard;
    EXPECT_EQ(config.getCaptureRate(44100), CaptureRate::STANDARD);
    EXPECT_EQ(config.getCaptureRate(192000), CaptureRate::STANDARD);
}

TEST_F(CaptureRateCalculationTest, HighPresetReturns44kHz)
{
    config.qualityPreset = QualityPreset::High;
    EXPECT_EQ(config.getCaptureRate(44100), CaptureRate::HIGH);
    EXPECT_EQ(config.getCaptureRate(192000), CaptureRate::HIGH);
}

TEST_F(CaptureRateCalculationTest, UltraPresetReturnsSourceRate)
{
    config.qualityPreset = QualityPreset::Ultra;
    EXPECT_EQ(config.getCaptureRate(44100), 44100);
    EXPECT_EQ(config.getCaptureRate(48000), 48000);
    EXPECT_EQ(config.getCaptureRate(96000), 96000);
    EXPECT_EQ(config.getCaptureRate(192000), 192000);
}

TEST_F(CaptureRateCalculationTest, UltraPresetClampsToMaxSupportedRate)
{
    config.qualityPreset = QualityPreset::Ultra;

    EXPECT_EQ(config.getCaptureRate(384000), CaptureRate::MAX_SOURCE_RATE);
    EXPECT_EQ(CaptureQualityConfig::calculateCaptureRateForPreset(QualityPreset::Ultra, 768000),
              CaptureRate::MAX_SOURCE_RATE);
}

TEST_F(CaptureRateCalculationTest, UltraPresetFallsBackToHighIfSourceRateInvalid)
{
    config.qualityPreset = QualityPreset::Ultra;
    EXPECT_EQ(config.getCaptureRate(0), CaptureRate::HIGH);
    EXPECT_EQ(config.getCaptureRate(-1), CaptureRate::HIGH);
}

TEST_F(CaptureRateCalculationTest, StaticCalculationMatchesInstanceMethod)
{
    for (auto preset : {QualityPreset::Eco, QualityPreset::Standard, QualityPreset::High, QualityPreset::Ultra})
    {
        config.qualityPreset = preset;
        int sourceRate = 48000;
        EXPECT_EQ(config.getCaptureRate(sourceRate),
                  CaptureQualityConfig::calculateCaptureRateForPreset(preset, sourceRate));
    }
}

//==============================================================================
// Decimation Ratio Tests
//==============================================================================

class DecimationRatioTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(DecimationRatioTest, EcoPresetAt192kHzGives17xDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    // 192000 / 11025 ≈ 17.4, truncated to 17
    EXPECT_EQ(config.getDecimationRatio(192000), 17);
}

TEST_F(DecimationRatioTest, EcoPresetAt44kHzGives4xDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    // 44100 / 11025 = 4
    EXPECT_EQ(config.getDecimationRatio(44100), 4);
}

TEST_F(DecimationRatioTest, StandardPresetAt192kHzGives8xDecimation)
{
    config.qualityPreset = QualityPreset::Standard;
    // 192000 / 22050 ≈ 8.7, truncated to 8
    EXPECT_EQ(config.getDecimationRatio(192000), 8);
}

TEST_F(DecimationRatioTest, HighPresetAt192kHzGives4xDecimation)
{
    config.qualityPreset = QualityPreset::High;
    // 192000 / 44100 ≈ 4.35, truncated to 4
    EXPECT_EQ(config.getDecimationRatio(192000), 4);
}

TEST_F(DecimationRatioTest, UltraPresetGivesNoDecimation)
{
    config.qualityPreset = QualityPreset::Ultra;
    EXPECT_EQ(config.getDecimationRatio(44100), 1);
    EXPECT_EQ(config.getDecimationRatio(192000), 1);
}

TEST_F(DecimationRatioTest, DecimationRatioNeverLessThanOne)
{
    config.qualityPreset = QualityPreset::High;
    // Even if source rate equals capture rate
    EXPECT_GE(config.getDecimationRatio(44100), 1);
    // Edge cases
    EXPECT_GE(config.getDecimationRatio(0), 1);
    EXPECT_GE(config.getDecimationRatio(-1), 1);
}

//==============================================================================
// Buffer Size Calculation Tests
//==============================================================================

class BufferSizeCalculationTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(BufferSizeCalculationTest, BufferSizeForStandard4Seconds)
{
    config.qualityPreset = QualityPreset::Standard;
    config.bufferDuration = BufferDuration::Medium;  // 4 seconds

    // 22050 Hz * 4 seconds = 88200 samples
    EXPECT_EQ(config.calculateBufferSizeSamples(CaptureRate::STANDARD), 88200u);
}

TEST_F(BufferSizeCalculationTest, BufferSizeForEco1Second)
{
    config.qualityPreset = QualityPreset::Eco;
    config.bufferDuration = BufferDuration::Short;  // 1 second

    // 11025 Hz * 1 second = 11025 samples
    EXPECT_EQ(config.calculateBufferSizeSamples(CaptureRate::ECO), 11025u);
}

TEST_F(BufferSizeCalculationTest, BufferSizeForHigh30Seconds)
{
    config.qualityPreset = QualityPreset::High;
    config.bufferDuration = BufferDuration::VeryLong;  // 30 seconds

    // 44100 Hz * 30 seconds = 1,323,000 samples
    EXPECT_EQ(config.calculateBufferSizeSamples(CaptureRate::HIGH), 1323000u);
}

TEST_F(BufferSizeCalculationTest, StaticBufferSizeForDuration)
{
    // 22050 Hz * 2000ms = 44100 samples
    EXPECT_EQ(CaptureQualityConfig::calculateBufferSizeForDuration(22050, 2000.0f), 44100u);

    // 44100 Hz * 500ms = 22050 samples
    EXPECT_EQ(CaptureQualityConfig::calculateBufferSizeForDuration(44100, 500.0f), 22050u);
}

//==============================================================================
// Memory Usage Calculation Tests
//==============================================================================

class MemoryUsageCalculationTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(MemoryUsageCalculationTest, StandardPreset4SecondsMemoryUsage)
{
    config.qualityPreset = QualityPreset::Standard;
    config.bufferDuration = BufferDuration::Medium;

    // 88200 samples * 2 channels * 4 bytes = 705,600 bytes ≈ 689 KB
    size_t expected = 88200 * 2 * sizeof(float);
    EXPECT_EQ(config.calculateMemoryUsageBytes(44100), expected);
}

TEST_F(MemoryUsageCalculationTest, EcoPresetUsesLessMemoryThanStandard)
{
    config.bufferDuration = BufferDuration::Medium;

    config.qualityPreset = QualityPreset::Eco;
    size_t ecoMemory = config.calculateMemoryUsageBytes(44100);

    config.qualityPreset = QualityPreset::Standard;
    size_t standardMemory = config.calculateMemoryUsageBytes(44100);

    EXPECT_LT(ecoMemory, standardMemory);
    // Eco should be roughly half of Standard (11kHz vs 22kHz)
    EXPECT_NEAR(static_cast<double>(ecoMemory) / standardMemory, 0.5, 0.01);
}

TEST_F(MemoryUsageCalculationTest, UltraPresetAtHighSampleRateUsesMoreMemory)
{
    config.bufferDuration = BufferDuration::Medium;
    config.qualityPreset = QualityPreset::Ultra;

    size_t memoryAt44k = config.calculateMemoryUsageBytes(44100);
    size_t memoryAt192k = config.calculateMemoryUsageBytes(192000);

    // 192kHz should use ~4.35x more memory than 44.1kHz
    double ratio = static_cast<double>(memoryAt192k) / memoryAt44k;
    EXPECT_NEAR(ratio, 192000.0 / 44100.0, 0.01);
}

TEST_F(MemoryUsageCalculationTest, UltraPresetMemoryIsCappedAtMaxSupportedRate)
{
    config.bufferDuration = BufferDuration::Medium;
    config.qualityPreset = QualityPreset::Ultra;

    size_t memoryAt192k = config.calculateMemoryUsageBytes(192000);
    size_t memoryAt384k = config.calculateMemoryUsageBytes(384000);

    EXPECT_EQ(memoryAt384k, memoryAt192k);
}

//==============================================================================
// Memory Budget Tests
//==============================================================================

class MemoryBudgetTest : public ::testing::Test
{
protected:
    MemoryBudget budget;
};

TEST_F(MemoryBudgetTest, DefaultBudgetValues)
{
    EXPECT_EQ(budget.totalBudgetBytes, 50u * 1024 * 1024);  // 50 MB
    EXPECT_EQ(budget.perTrackMinBytes, 100u * 1024);         // 100 KB
    EXPECT_EQ(budget.perTrackMaxBytes, 2u * 1024 * 1024);    // 2 MB
}

TEST_F(MemoryBudgetTest, PerTrackBudgetWithFewTracks)
{
    // With 4 tracks: 50MB / 4 = 12.5MB, clamped to 2MB max
    EXPECT_EQ(budget.calculatePerTrackBudget(4), budget.perTrackMaxBytes);
}

TEST_F(MemoryBudgetTest, PerTrackBudgetWithManyTracks)
{
    // With 100 tracks: 50MB / 100 = 500KB, within limits
    size_t expected = budget.totalBudgetBytes / 100;
    EXPECT_EQ(budget.calculatePerTrackBudget(100), expected);
}

TEST_F(MemoryBudgetTest, PerTrackBudgetClampsToMinimum)
{
    // With 1000 tracks: 50MB / 1000 = 50KB, clamped to 100KB min
    EXPECT_EQ(budget.calculatePerTrackBudget(1000), budget.perTrackMinBytes);
}

TEST_F(MemoryBudgetTest, PerTrackBudgetWithZeroTracksReturnsMax)
{
    EXPECT_EQ(budget.calculatePerTrackBudget(0), budget.perTrackMaxBytes);
}

TEST_F(MemoryBudgetTest, RecommendedPresetForFewTracks)
{
    // With few tracks and high budget per track, should recommend High
    QualityPreset recommended = budget.calculateRecommendedPreset(4, 4.0f);
    EXPECT_EQ(recommended, QualityPreset::High);
}

TEST_F(MemoryBudgetTest, RecommendedPresetForManyTracks)
{
    // With 100 tracks: 500KB per track
    // High requires: 44100 * 4 * 2 * 4 = 1,411,200 bytes ≈ 1.35MB - too much
    // Standard requires: 22050 * 4 * 2 * 4 = 705,600 bytes ≈ 689KB - too much
    // Eco requires: 11025 * 4 * 2 * 4 = 352,800 bytes ≈ 345KB - fits!
    QualityPreset recommended = budget.calculateRecommendedPreset(100, 4.0f);
    EXPECT_EQ(recommended, QualityPreset::Eco);
}

//==============================================================================
// Buffer Duration Tests
//==============================================================================

class BufferDurationTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(BufferDurationTest, DurationToSecondsConversion)
{
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Short), 1.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Medium), 4.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Long), 10.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::VeryLong), 30.0f);
}

TEST_F(BufferDurationTest, StringConversionRoundTrip)
{
    for (auto duration : {BufferDuration::Short, BufferDuration::Medium,
                          BufferDuration::Long, BufferDuration::VeryLong})
    {
        juce::String str = bufferDurationToString(duration);
        EXPECT_EQ(stringToBufferDuration(str), duration);
    }
}

TEST_F(BufferDurationTest, InvalidStringDefaultsToMedium)
{
    EXPECT_EQ(stringToBufferDuration("invalid"), BufferDuration::Medium);
    EXPECT_EQ(stringToBufferDuration(""), BufferDuration::Medium);
}

//==============================================================================
// Effective Quality with Auto-Adjust Tests
//==============================================================================

class EffectiveQualityTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(EffectiveQualityTest, WithAutoAdjustDisabledReturnsConfiguredPreset)
{
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = false;

    // Even with 1000 tracks, should return High
    EXPECT_EQ(config.getEffectiveQuality(1000, 44100), QualityPreset::High);
}

TEST_F(EffectiveQualityTest, WithAutoAdjustEnabledReducesQualityForManyTracks)
{
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = true;
    config.bufferDuration = BufferDuration::Medium;

    // With 100 tracks, budget recommends Eco, but configured is High
    // Should return Eco (the lower of the two)
    QualityPreset effective = config.getEffectiveQuality(100, 44100);
    EXPECT_EQ(effective, QualityPreset::Eco);
}

TEST_F(EffectiveQualityTest, WithAutoAdjustEnabledKeepsQualityForFewTracks)
{
    config.qualityPreset = QualityPreset::Standard;
    config.autoAdjustQuality = true;
    config.bufferDuration = BufferDuration::Medium;

    // With 4 tracks, budget allows High, but configured is Standard
    // Should return Standard (the lower of configured and recommended)
    QualityPreset effective = config.getEffectiveQuality(4, 44100);
    EXPECT_EQ(effective, QualityPreset::Standard);
}

//==============================================================================
// Serialization Tests
//==============================================================================

class CaptureQualitySerializationTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(CaptureQualitySerializationTest, RoundTripPreservesValues)
{
    CaptureQualityConfig original;
    original.qualityPreset = QualityPreset::High;
    original.bufferDuration = BufferDuration::Long;
    original.memoryBudget.totalBudgetBytes = 100 * 1024 * 1024;  // 100 MB
    original.autoAdjustQuality = false;

    juce::ValueTree tree = original.toValueTree();
    CaptureQualityConfig restored = CaptureQualityConfig::fromValueTree(tree);

    EXPECT_EQ(restored.qualityPreset, original.qualityPreset);
    EXPECT_EQ(restored.bufferDuration, original.bufferDuration);
    EXPECT_EQ(restored.memoryBudget.totalBudgetBytes, original.memoryBudget.totalBudgetBytes);
    EXPECT_EQ(restored.autoAdjustQuality, original.autoAdjustQuality);
}

TEST_F(CaptureQualitySerializationTest, InvalidTreeReturnsDefaults)
{
    juce::ValueTree invalidTree;
    CaptureQualityConfig config = CaptureQualityConfig::fromValueTree(invalidTree);

    EXPECT_EQ(config.qualityPreset, QualityPreset::Standard);
    EXPECT_EQ(config.bufferDuration, BufferDuration::Medium);
    EXPECT_TRUE(config.autoAdjustQuality);
}

TEST_F(CaptureQualitySerializationTest, NegativeTotalBudgetUsesDefault)
{
    juce::ValueTree tree("CaptureQuality");
    tree.setProperty("totalBudgetMB", -1, nullptr);

    CaptureQualityConfig config = CaptureQualityConfig::fromValueTree(tree);

    EXPECT_EQ(config.memoryBudget.totalBudgetBytes, 50u * 1024 * 1024);
}

TEST_F(CaptureQualitySerializationTest, EqualityOperatorWorks)
{
    CaptureQualityConfig a, b;
    EXPECT_EQ(a, b);

    a.qualityPreset = QualityPreset::High;
    EXPECT_NE(a, b);

    b.qualityPreset = QualityPreset::High;
    EXPECT_EQ(a, b);
}

//==============================================================================
// Quality Override Tests
//==============================================================================

class QualityOverrideTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(QualityOverrideTest, UseGlobalReturnsGlobalPreset)
{
    EXPECT_EQ(resolveQualityOverride(QualityOverride::UseGlobal, QualityPreset::Eco), QualityPreset::Eco);
    EXPECT_EQ(resolveQualityOverride(QualityOverride::UseGlobal, QualityPreset::High), QualityPreset::High);
}

TEST_F(QualityOverrideTest, ExplicitOverrideIgnoresGlobal)
{
    EXPECT_EQ(resolveQualityOverride(QualityOverride::Eco, QualityPreset::High), QualityPreset::Eco);
    EXPECT_EQ(resolveQualityOverride(QualityOverride::Ultra, QualityPreset::Eco), QualityPreset::Ultra);
}

TEST_F(QualityOverrideTest, StringConversionRoundTrip)
{
    for (auto override : {QualityOverride::UseGlobal, QualityOverride::Eco,
                          QualityOverride::Standard, QualityOverride::High, QualityOverride::Ultra})
    {
        juce::String str = qualityOverrideToString(override);
        EXPECT_EQ(stringToQualityOverride(str), override);
    }
}
