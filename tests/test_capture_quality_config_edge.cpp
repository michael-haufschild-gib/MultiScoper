/*
    Oscil - Capture Quality Configuration Tests (Edge Cases)
    Memory budget, buffer duration, effective quality, serialization, quality override
*/

#include "core/dsp/CaptureQualityConfig.h"

#include <gtest/gtest.h>

using namespace oscil;

//==============================================================================
// Memory Budget Tests
//==============================================================================

class MemoryBudgetConfigEdgeTest : public ::testing::Test
{
protected:
    MemoryBudget budget;
};

TEST_F(MemoryBudgetConfigEdgeTest, DefaultBudgetValues)
{
    EXPECT_EQ(budget.totalBudgetBytes, 50u * 1024 * 1024); // 50 MB
    EXPECT_EQ(budget.perTrackMinBytes, 100u * 1024);       // 100 KB
    EXPECT_EQ(budget.perTrackMaxBytes, 2u * 1024 * 1024);  // 2 MB
}

TEST_F(MemoryBudgetConfigEdgeTest, PerTrackBudgetWithFewTracks)
{
    // With 4 tracks: 50MB / 4 = 12.5MB, clamped to 2MB max
    EXPECT_EQ(budget.calculatePerTrackBudget(4), budget.perTrackMaxBytes);
}

TEST_F(MemoryBudgetConfigEdgeTest, PerTrackBudgetWithManyTracks)
{
    // With 100 tracks: 50MB / 100 = 500KB, within limits
    size_t expected = budget.totalBudgetBytes / 100;
    EXPECT_EQ(budget.calculatePerTrackBudget(100), expected);
}

TEST_F(MemoryBudgetConfigEdgeTest, PerTrackBudgetClampsToMinimum)
{
    // With 1000 tracks: 50MB / 1000 = 50KB, clamped to 100KB min
    EXPECT_EQ(budget.calculatePerTrackBudget(1000), budget.perTrackMinBytes);
}

TEST_F(MemoryBudgetConfigEdgeTest, PerTrackBudgetWithZeroTracksReturnsMax)
{
    EXPECT_EQ(budget.calculatePerTrackBudget(0), budget.perTrackMaxBytes);
}

TEST_F(MemoryBudgetConfigEdgeTest, RecommendedPresetForFewTracks)
{
    // With few tracks and high budget per track, should recommend High
    QualityPreset recommended = budget.calculateRecommendedPreset(4, 4.0f);
    EXPECT_EQ(recommended, QualityPreset::High);
}

TEST_F(MemoryBudgetConfigEdgeTest, RecommendedPresetForManyTracks)
{
    // With 100 tracks: 500KB per track
    // High requires: 44100 * 4 * 2 * 4 = 1,411,200 bytes ~ 1.35MB - too much
    // Standard requires: 22050 * 4 * 2 * 4 = 705,600 bytes ~ 689KB - too much
    // Eco requires: 11025 * 4 * 2 * 4 = 352,800 bytes ~ 345KB - fits!
    QualityPreset recommended = budget.calculateRecommendedPreset(100, 4.0f);
    EXPECT_EQ(recommended, QualityPreset::Eco);
}

//==============================================================================
// Buffer Duration Tests
//==============================================================================

class BufferDurationEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(BufferDurationEdgeTest, DurationToSecondsConversion)
{
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Short), 1.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Medium), 4.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::Long), 10.0f);
    EXPECT_FLOAT_EQ(bufferDurationToSeconds(BufferDuration::VeryLong), 30.0f);
}

TEST_F(BufferDurationEdgeTest, StringConversionRoundTrip)
{
    for (auto duration :
         {BufferDuration::Short, BufferDuration::Medium, BufferDuration::Long, BufferDuration::VeryLong})
    {
        juce::String str = bufferDurationToString(duration);
        EXPECT_EQ(stringToBufferDuration(str), duration);
    }
}

TEST_F(BufferDurationEdgeTest, InvalidStringDefaultsToMedium)
{
    EXPECT_EQ(stringToBufferDuration("invalid"), BufferDuration::Medium);
    EXPECT_EQ(stringToBufferDuration(""), BufferDuration::Medium);
}

//==============================================================================
// Effective Quality with Auto-Adjust Tests
//==============================================================================

class EffectiveQualityEdgeTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(EffectiveQualityEdgeTest, WithAutoAdjustDisabledReturnsConfiguredPreset)
{
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = false;

    // Even with 1000 tracks, should return High
    EXPECT_EQ(config.getEffectiveQuality(1000, 44100), QualityPreset::High);
}

TEST_F(EffectiveQualityEdgeTest, WithAutoAdjustEnabledReducesQualityForManyTracks)
{
    config.qualityPreset = QualityPreset::High;
    config.autoAdjustQuality = true;
    config.bufferDuration = BufferDuration::Medium;

    // With 100 tracks, budget recommends Eco, but configured is High
    // Should return Eco (the lower of the two)
    QualityPreset effective = config.getEffectiveQuality(100, 44100);
    EXPECT_EQ(effective, QualityPreset::Eco);
}

TEST_F(EffectiveQualityEdgeTest, WithAutoAdjustEnabledKeepsQualityForFewTracks)
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

class CaptureQualitySerializationEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(CaptureQualitySerializationEdgeTest, RoundTripPreservesValues)
{
    CaptureQualityConfig original;
    original.qualityPreset = QualityPreset::High;
    original.bufferDuration = BufferDuration::Long;
    original.memoryBudget.totalBudgetBytes = 100 * 1024 * 1024; // 100 MB
    original.autoAdjustQuality = false;

    juce::ValueTree tree = original.toValueTree();
    CaptureQualityConfig restored = CaptureQualityConfig::fromValueTree(tree);

    EXPECT_EQ(restored.qualityPreset, original.qualityPreset);
    EXPECT_EQ(restored.bufferDuration, original.bufferDuration);
    EXPECT_EQ(restored.memoryBudget.totalBudgetBytes, original.memoryBudget.totalBudgetBytes);
    EXPECT_EQ(restored.autoAdjustQuality, original.autoAdjustQuality);
}

TEST_F(CaptureQualitySerializationEdgeTest, InvalidTreeReturnsDefaults)
{
    juce::ValueTree invalidTree;
    CaptureQualityConfig config = CaptureQualityConfig::fromValueTree(invalidTree);

    EXPECT_EQ(config.qualityPreset, QualityPreset::Standard);
    EXPECT_EQ(config.bufferDuration, BufferDuration::Medium);
    EXPECT_TRUE(config.autoAdjustQuality);
}

TEST_F(CaptureQualitySerializationEdgeTest, NegativeTotalBudgetUsesDefault)
{
    juce::ValueTree tree("CaptureQuality");
    tree.setProperty("totalBudgetMB", -1, nullptr);

    CaptureQualityConfig config = CaptureQualityConfig::fromValueTree(tree);

    EXPECT_EQ(config.memoryBudget.totalBudgetBytes, 50u * 1024 * 1024);
}

TEST_F(CaptureQualitySerializationEdgeTest, EqualityOperatorWorks)
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

class QualityOverrideEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(QualityOverrideEdgeTest, UseGlobalReturnsGlobalPreset)
{
    EXPECT_EQ(resolveQualityOverride(QualityOverride::UseGlobal, QualityPreset::Eco), QualityPreset::Eco);
    EXPECT_EQ(resolveQualityOverride(QualityOverride::UseGlobal, QualityPreset::High), QualityPreset::High);
}

TEST_F(QualityOverrideEdgeTest, ExplicitOverrideIgnoresGlobal)
{
    EXPECT_EQ(resolveQualityOverride(QualityOverride::Eco, QualityPreset::High), QualityPreset::Eco);
    EXPECT_EQ(resolveQualityOverride(QualityOverride::Ultra, QualityPreset::Eco), QualityPreset::Ultra);
}

TEST_F(QualityOverrideEdgeTest, StringConversionRoundTrip)
{
    for (auto override : {QualityOverride::UseGlobal, QualityOverride::Eco, QualityOverride::Standard,
                          QualityOverride::High, QualityOverride::Ultra})
    {
        juce::String str = qualityOverrideToString(override);
        EXPECT_EQ(stringToQualityOverride(str), override);
    }
}
