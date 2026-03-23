/*
    Oscil - Visual Configuration Preset Tests
    Tests for preset management and serialization
*/

#include "rendering/VisualConfiguration.h"

#include <gtest/gtest.h>

using namespace oscil;

class VisualConfigPresetTest : public ::testing::Test
{
};

TEST_F(VisualConfigPresetTest, SerializationRoundTrip)
{
    VisualConfiguration original;
    original.shaderType = ShaderType::NeonGlow;
    original.compositeOpacity = 0.75f;

    original.bloom.enabled = true;
    original.bloom.intensity = 2.0f;
    original.bloom.threshold = 0.6f;

    original.trails.enabled = true;
    original.trails.decay = 0.05f;

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_EQ(restored.shaderType, original.shaderType);
    EXPECT_FLOAT_EQ(restored.compositeOpacity, original.compositeOpacity);

    EXPECT_EQ(restored.bloom.enabled, original.bloom.enabled);
    EXPECT_FLOAT_EQ(restored.bloom.intensity, original.bloom.intensity);
    EXPECT_FLOAT_EQ(restored.bloom.threshold, original.bloom.threshold);

    EXPECT_EQ(restored.trails.enabled, original.trails.enabled);
    EXPECT_FLOAT_EQ(restored.trails.decay, original.trails.decay);
}

TEST_F(VisualConfigPresetTest, DeserializeInvalidTree)
{
    juce::ValueTree invalidTree("WrongType");
    auto config = VisualConfiguration::fromValueTree(invalidTree);
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigPresetTest, DeserializeEmptyTree)
{
    juce::ValueTree emptyTree;
    auto config = VisualConfiguration::fromValueTree(emptyTree);
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigPresetTest, SerializationPreservesAllPostProcessing)
{
    VisualConfiguration original;
    original.bloom.enabled = true;
    original.radialBlur.enabled = true;
    original.trails.enabled = true;
    original.colorGrade.enabled = true;
    original.vignette.enabled = true;
    original.filmGrain.enabled = true;
    original.chromaticAberration.enabled = true;
    original.scanlines.enabled = true;
    original.tiltShift.enabled = true;

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_TRUE(restored.bloom.enabled);
    EXPECT_TRUE(restored.radialBlur.enabled);
    EXPECT_TRUE(restored.trails.enabled);
    EXPECT_TRUE(restored.colorGrade.enabled);
    EXPECT_TRUE(restored.vignette.enabled);
    EXPECT_TRUE(restored.filmGrain.enabled);
    EXPECT_TRUE(restored.chromaticAberration.enabled);
    EXPECT_TRUE(restored.scanlines.enabled);
    EXPECT_TRUE(restored.tiltShift.enabled);
}

TEST_F(VisualConfigPresetTest, SerializationPreservesPresetId)
{
    VisualConfiguration original;
    original.presetId = "custom_preset_42";

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_EQ(restored.presetId, original.presetId);
}

TEST_F(VisualConfigPresetTest, SerializationPreservesColorGradeDetails)
{
    VisualConfiguration original;
    original.colorGrade.enabled = true;
    original.colorGrade.brightness = 0.2f;
    original.colorGrade.contrast = 1.5f;
    original.colorGrade.saturation = 0.8f;
    original.colorGrade.temperature = 0.3f;
    original.colorGrade.tint = -0.1f;
    original.colorGrade.shadows = juce::Colour(0xFF112233);
    original.colorGrade.highlights = juce::Colour(0xFFAABBCC);

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_FLOAT_EQ(restored.colorGrade.brightness, original.colorGrade.brightness);
    EXPECT_FLOAT_EQ(restored.colorGrade.contrast, original.colorGrade.contrast);
    EXPECT_FLOAT_EQ(restored.colorGrade.saturation, original.colorGrade.saturation);
    EXPECT_FLOAT_EQ(restored.colorGrade.temperature, original.colorGrade.temperature);
    EXPECT_FLOAT_EQ(restored.colorGrade.tint, original.colorGrade.tint);
    EXPECT_EQ(restored.colorGrade.shadows.getARGB(), original.colorGrade.shadows.getARGB());
    EXPECT_EQ(restored.colorGrade.highlights.getARGB(), original.colorGrade.highlights.getARGB());
}

TEST_F(VisualConfigPresetTest, DeserializeWithNaNEffectParameters)
{
    juce::ValueTree tree("VisualConfiguration");
    tree.setProperty("shaderType", "basic", nullptr);
    tree.setProperty("compositeOpacity", std::numeric_limits<float>::quiet_NaN(), nullptr);

    juce::ValueTree bloomTree("Bloom");
    bloomTree.setProperty("enabled", true, nullptr);
    bloomTree.setProperty("intensity", std::numeric_limits<float>::quiet_NaN(), nullptr);
    bloomTree.setProperty("threshold", std::numeric_limits<float>::infinity(), nullptr);
    tree.addChild(bloomTree, -1, nullptr);

    auto config = VisualConfiguration::fromValueTree(tree);

    // All values should be finite after validation
    EXPECT_TRUE(std::isfinite(config.compositeOpacity));
    EXPECT_TRUE(std::isfinite(config.bloom.intensity));
    EXPECT_TRUE(std::isfinite(config.bloom.threshold));
}

TEST_F(VisualConfigPresetTest, DeserializeWithMissingChildNodes)
{
    // Bug caught: deserialization crashing when child nodes are absent
    juce::ValueTree tree("VisualConfiguration");
    tree.setProperty("shaderType", "neon_glow", nullptr);
    tree.setProperty("compositeOpacity", 0.5f, nullptr);
    // No child nodes for Bloom, Trails, etc.

    auto config = VisualConfiguration::fromValueTree(tree);

    EXPECT_EQ(config.shaderType, ShaderType::NeonGlow);
    EXPECT_FLOAT_EQ(config.compositeOpacity, 0.5f);
    // Effects should be at defaults (disabled)
    EXPECT_FALSE(config.bloom.enabled);
    EXPECT_FALSE(config.trails.enabled);
}

TEST_F(VisualConfigPresetTest, SerializationIdempotencyAcrossMultipleRoundTrips)
{
    // Bug caught: float precision drift across round-trips
    VisualConfiguration config;
    config.shaderType = ShaderType::GradientFill;
    config.bloom.enabled = true;
    config.bloom.intensity = 1.23456f;
    config.colorGrade.enabled = true;
    config.colorGrade.temperature = 0.31415f;

    // First round-trip
    auto tree1 = config.toValueTree();
    auto config2 = VisualConfiguration::fromValueTree(tree1);

    // Second round-trip
    auto tree2 = config2.toValueTree();
    auto config3 = VisualConfiguration::fromValueTree(tree2);

    EXPECT_FLOAT_EQ(config.bloom.intensity, config3.bloom.intensity);
    EXPECT_FLOAT_EQ(config.colorGrade.temperature, config3.colorGrade.temperature);
}

TEST_F(VisualConfigPresetTest, GetAvailablePresets)
{
    auto presets = VisualConfiguration::getAvailablePresets();

    EXPECT_FALSE(presets.empty());

    bool foundDefault = false;
    bool foundVectorScope = false;

    for (const auto& preset : presets)
    {
        if (preset.first == "default")
            foundDefault = true;
        if (preset.first == "vector_scope")
            foundVectorScope = true;
    }

    EXPECT_TRUE(foundDefault);
    EXPECT_TRUE(foundVectorScope);
}
