/*
    Oscil - Visual Configuration Validation Tests
    Tests for shader type validation, enum conversions, and default values
*/

#include <gtest/gtest.h>
#include "rendering/VisualConfiguration.h"

using namespace oscil;

// =============================================================================
// ShaderType Utility Tests
// =============================================================================

class ShaderTypeTest : public ::testing::Test
{
};

TEST_F(ShaderTypeTest, Is3DShader)
{
    // 2D shaders
    EXPECT_FALSE(is3DShader(ShaderType::Basic2D));
    EXPECT_FALSE(is3DShader(ShaderType::NeonGlow));
    EXPECT_FALSE(is3DShader(ShaderType::GradientFill));
    EXPECT_FALSE(is3DShader(ShaderType::DualOutline));
    EXPECT_FALSE(is3DShader(ShaderType::PlasmaSine));

    // 3D shaders
    EXPECT_TRUE(is3DShader(ShaderType::VolumetricRibbon));
    EXPECT_TRUE(is3DShader(ShaderType::WireframeMesh));
    EXPECT_TRUE(is3DShader(ShaderType::VectorFlow));
    EXPECT_TRUE(is3DShader(ShaderType::StringTheory));

    // Material shaders (also 3D)
    EXPECT_TRUE(is3DShader(ShaderType::GlassRefraction));
    EXPECT_TRUE(is3DShader(ShaderType::LiquidChrome));
    EXPECT_TRUE(is3DShader(ShaderType::Crystalline));
}

TEST_F(ShaderTypeTest, IsMaterialShader)
{
    EXPECT_FALSE(isMaterialShader(ShaderType::Basic2D));
    EXPECT_FALSE(isMaterialShader(ShaderType::VolumetricRibbon));

    EXPECT_TRUE(isMaterialShader(ShaderType::GlassRefraction));
    EXPECT_TRUE(isMaterialShader(ShaderType::LiquidChrome));
    EXPECT_TRUE(isMaterialShader(ShaderType::Crystalline));
}

TEST_F(ShaderTypeTest, ShaderTypeToId)
{
    EXPECT_EQ(shaderTypeToId(ShaderType::Basic2D), "basic");
    EXPECT_EQ(shaderTypeToId(ShaderType::NeonGlow), "neon_glow");
    EXPECT_EQ(shaderTypeToId(ShaderType::GradientFill), "gradient_fill");
    EXPECT_EQ(shaderTypeToId(ShaderType::VolumetricRibbon), "volumetric_ribbon");
    EXPECT_EQ(shaderTypeToId(ShaderType::GlassRefraction), "glass_refraction");
}

TEST_F(ShaderTypeTest, IdToShaderType)
{
    EXPECT_EQ(idToShaderType("basic"), ShaderType::Basic2D);
    EXPECT_EQ(idToShaderType("neon_glow"), ShaderType::NeonGlow);
    EXPECT_EQ(idToShaderType("volumetric_ribbon"), ShaderType::VolumetricRibbon);
    EXPECT_EQ(idToShaderType("glass_refraction"), ShaderType::GlassRefraction);

    // Unknown ID should default to Basic2D
    EXPECT_EQ(idToShaderType("unknown"), ShaderType::Basic2D);
    EXPECT_EQ(idToShaderType(""), ShaderType::Basic2D);
}

TEST_F(ShaderTypeTest, RoundTripConversion)
{
    // Test all shader types round-trip correctly
    std::vector<ShaderType> allTypes = {
        ShaderType::Basic2D,
        ShaderType::NeonGlow,
        ShaderType::GradientFill,
        ShaderType::DualOutline,
        ShaderType::PlasmaSine,
        ShaderType::VolumetricRibbon,
        ShaderType::WireframeMesh,
        ShaderType::VectorFlow,
        ShaderType::StringTheory,
        ShaderType::GlassRefraction,
        ShaderType::LiquidChrome,
        ShaderType::Crystalline
    };

    for (auto type : allTypes)
    {
        juce::String id = shaderTypeToId(type);
        ShaderType restored = idToShaderType(id);
        EXPECT_EQ(restored, type);
    }
}

// =============================================================================
// BloomSettings Tests
// =============================================================================

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

// =============================================================================
// TrailSettings Tests
// =============================================================================

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

// =============================================================================
// Settings3D Tests
// =============================================================================

class Settings3DTest : public ::testing::Test
{
};

TEST_F(Settings3DTest, DefaultValues)
{
    Settings3D settings;

    EXPECT_FALSE(settings.enabled);
    EXPECT_FLOAT_EQ(settings.cameraDistance, 5.0f);
    EXPECT_FLOAT_EQ(settings.cameraAngleX, 15.0f);
    EXPECT_FLOAT_EQ(settings.cameraAngleY, 0.0f);
    EXPECT_FALSE(settings.autoRotate);
    EXPECT_FLOAT_EQ(settings.rotateSpeed, 10.0f);
    EXPECT_EQ(settings.meshResolutionX, 128);
    EXPECT_EQ(settings.meshResolutionZ, 32);
    EXPECT_FLOAT_EQ(settings.meshScale, 1.0f);
}

// =============================================================================
// MaterialSettings Tests
// =============================================================================

class MaterialSettingsTest : public ::testing::Test
{
};

TEST_F(MaterialSettingsTest, DefaultValues)
{
    MaterialSettings material;

    EXPECT_FALSE(material.enabled);
    EXPECT_FLOAT_EQ(material.reflectivity, 0.5f);
    EXPECT_FLOAT_EQ(material.refractiveIndex, 1.5f);
    EXPECT_FLOAT_EQ(material.fresnelPower, 2.0f);
    EXPECT_FLOAT_EQ(material.roughness, 0.1f);
    EXPECT_TRUE(material.useEnvironmentMap);
    EXPECT_EQ(material.environmentMapId, "default_studio");
}

// =============================================================================
// TiltShiftSettings Tests
// =============================================================================

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
