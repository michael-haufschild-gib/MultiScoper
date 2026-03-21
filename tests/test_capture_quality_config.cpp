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

