/*
    Oscil - Visual Configuration Validation Tests
    Tests for shader type validation, enum conversions, and default values
*/

#include <gtest/gtest.h>
#include "rendering/VisualConfiguration.h"

using namespace oscil;

class ShaderTypeTest : public ::testing::Test
{
};

TEST_F(ShaderTypeTest, ShaderTypeToId)
{
    EXPECT_EQ(shaderTypeToId(ShaderType::Basic2D), "basic");
    EXPECT_EQ(shaderTypeToId(ShaderType::NeonGlow), "neon_glow");
    EXPECT_EQ(shaderTypeToId(ShaderType::GradientFill), "gradient_fill");
    EXPECT_EQ(shaderTypeToId(ShaderType::DualOutline), "dual_outline");
}

TEST_F(ShaderTypeTest, IdToShaderType)
{
    EXPECT_EQ(idToShaderType("basic"), ShaderType::Basic2D);
    EXPECT_EQ(idToShaderType("neon_glow"), ShaderType::NeonGlow);
    EXPECT_EQ(idToShaderType("gradient_fill"), ShaderType::GradientFill);
    EXPECT_EQ(idToShaderType("dual_outline"), ShaderType::DualOutline);

    // Unknown ID defaults to Basic2D
    EXPECT_EQ(idToShaderType("unknown"), ShaderType::Basic2D);
    EXPECT_EQ(idToShaderType(""), ShaderType::Basic2D);
}

TEST_F(ShaderTypeTest, RoundTripConversion)
{
    std::vector<ShaderType> allTypes = {
        ShaderType::Basic2D,
        ShaderType::NeonGlow,
        ShaderType::GradientFill,
        ShaderType::DualOutline
    };

    for (auto type : allTypes)
    {
        juce::String id = shaderTypeToId(type);
        ShaderType restored = idToShaderType(id);
        EXPECT_EQ(restored, type);
    }
}

class BloomSettingsTest : public ::testing::Test
{
};

TEST_F(BloomSettingsTest, DefaultValues)
{
    BloomSettings bloom;

    EXPECT_FALSE(bloom.enabled);
    EXPECT_FLOAT_EQ(bloom.intensity, 1.0f);
    EXPECT_FLOAT_EQ(bloom.threshold, 0.8f);
    EXPECT_EQ(bloom.iterations, 4);
    EXPECT_FLOAT_EQ(bloom.spread, 1.0f);
}

class TrailSettingsTest : public ::testing::Test
{
};

TEST_F(TrailSettingsTest, DefaultValues)
{
    TrailSettings trails;

    EXPECT_FALSE(trails.enabled);
    EXPECT_FLOAT_EQ(trails.decay, 0.1f);
    EXPECT_FLOAT_EQ(trails.opacity, 0.8f);
}

class TiltShiftSettingsTest : public ::testing::Test
{
};

TEST_F(TiltShiftSettingsTest, DefaultValues)
{
    TiltShiftSettings tilt;

    EXPECT_FALSE(tilt.enabled);
    EXPECT_FLOAT_EQ(tilt.position, 0.5f);
    EXPECT_FLOAT_EQ(tilt.range, 0.3f);
    EXPECT_FLOAT_EQ(tilt.blurRadius, 2.0f);
    EXPECT_EQ(tilt.iterations, 3);
}

// ============================================================================
// Validate() clamping
// ============================================================================

class VisualConfigValidationTest : public ::testing::Test
{
};

TEST_F(VisualConfigValidationTest, ValidateClampsOutOfRangeBloom)
{
    VisualConfiguration config;
    config.bloom.intensity = 10.0f;
    config.bloom.threshold = -1.0f;
    config.bloom.iterations = 100;
    config.bloom.softKnee = 5.0f;

    config.validate();

    EXPECT_FLOAT_EQ(config.bloom.intensity, 2.0f);
    EXPECT_FLOAT_EQ(config.bloom.threshold, 0.0f);
    EXPECT_EQ(config.bloom.iterations, 8);
    EXPECT_FLOAT_EQ(config.bloom.softKnee, 1.0f);
}

TEST_F(VisualConfigValidationTest, ValidateClampsOutOfRangeColorGrade)
{
    VisualConfiguration config;
    config.colorGrade.brightness = 5.0f;
    config.colorGrade.contrast = 0.0f;
    config.colorGrade.saturation = -1.0f;
    config.colorGrade.temperature = 99.0f;

    config.validate();

    EXPECT_FLOAT_EQ(config.colorGrade.brightness, 1.0f);
    EXPECT_FLOAT_EQ(config.colorGrade.contrast, 0.5f);
    EXPECT_FLOAT_EQ(config.colorGrade.saturation, 0.0f);
    EXPECT_FLOAT_EQ(config.colorGrade.temperature, 1.0f);
}

TEST_F(VisualConfigValidationTest, ValidatePreservesInRangeValues)
{
    VisualConfiguration config;
    config.bloom.intensity = 1.5f;
    config.trails.decay = 0.2f;
    config.compositeOpacity = 0.75f;

    config.validate();

    EXPECT_FLOAT_EQ(config.bloom.intensity, 1.5f);
    EXPECT_FLOAT_EQ(config.trails.decay, 0.2f);
    EXPECT_FLOAT_EQ(config.compositeOpacity, 0.75f);
}

TEST_F(VisualConfigValidationTest, DeserializationClampsInvalidValues)
{
    // Build a ValueTree with out-of-range values
    juce::ValueTree tree("VisualConfiguration");
    tree.setProperty("shaderType", "basic", nullptr);
    tree.setProperty("compositeOpacity", 5.0f, nullptr);

    juce::ValueTree bloomTree("Bloom");
    bloomTree.setProperty("enabled", true, nullptr);
    bloomTree.setProperty("intensity", 99.0f, nullptr);
    tree.addChild(bloomTree, -1, nullptr);

    auto config = VisualConfiguration::fromValueTree(tree);

    EXPECT_FLOAT_EQ(config.compositeOpacity, 1.0f);
    EXPECT_FLOAT_EQ(config.bloom.intensity, 2.0f);
    EXPECT_TRUE(config.bloom.enabled);
}
