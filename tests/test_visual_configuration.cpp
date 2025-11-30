/*
    Oscil - Visual Configuration Tests
    Tests for VisualConfiguration and related structures
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
    EXPECT_FALSE(is3DShader(ShaderType::DigitalGlitch));

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
        ShaderType::DigitalGlitch,
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
// ParticleSettings Tests
// =============================================================================

class ParticleSettingsTest : public ::testing::Test
{
};

TEST_F(ParticleSettingsTest, DefaultValues)
{
    ParticleSettings particles;

    EXPECT_FALSE(particles.enabled);
    EXPECT_EQ(particles.emissionMode, ParticleEmissionMode::AlongWaveform);
    EXPECT_FLOAT_EQ(particles.emissionRate, 100.0f);
    EXPECT_FLOAT_EQ(particles.particleLife, 2.0f);
    EXPECT_FLOAT_EQ(particles.particleSize, 4.0f);
    EXPECT_EQ(particles.blendMode, ParticleBlendMode::Additive);
    EXPECT_FLOAT_EQ(particles.gravity, 0.0f);
    EXPECT_FLOAT_EQ(particles.drag, 0.1f);
    EXPECT_FLOAT_EQ(particles.randomness, 0.5f);
    EXPECT_FLOAT_EQ(particles.velocityScale, 1.0f);
    EXPECT_TRUE(particles.audioReactive);
    EXPECT_FLOAT_EQ(particles.audioEmissionBoost, 2.0f);
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
// VisualConfiguration Tests
// =============================================================================

class VisualConfigurationTest : public ::testing::Test
{
};

TEST_F(VisualConfigurationTest, DefaultValues)
{
    VisualConfiguration config;

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_EQ(config.compositeBlendMode, BlendMode::Alpha);
    EXPECT_FLOAT_EQ(config.compositeOpacity, 1.0f);
    EXPECT_EQ(config.presetId, "default");
}

TEST_F(VisualConfigurationTest, Requires3DWithEnabled)
{
    VisualConfiguration config;

    EXPECT_FALSE(config.requires3D());

    config.settings3D.enabled = true;
    EXPECT_TRUE(config.requires3D());
}

TEST_F(VisualConfigurationTest, Requires3DWith3DShader)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::VolumetricRibbon;

    EXPECT_TRUE(config.requires3D());
}

TEST_F(VisualConfigurationTest, HasPostProcessing)
{
    VisualConfiguration config;

    EXPECT_FALSE(config.hasPostProcessing());

    config.bloom.enabled = true;
    EXPECT_TRUE(config.hasPostProcessing());

    config.bloom.enabled = false;
    config.trails.enabled = true;
    EXPECT_TRUE(config.hasPostProcessing());

    config.trails.enabled = false;
    config.vignette.enabled = true;
    EXPECT_TRUE(config.hasPostProcessing());
}

TEST_F(VisualConfigurationTest, HasParticles)
{
    VisualConfiguration config;

    EXPECT_FALSE(config.hasParticles());

    config.particles.enabled = true;
    EXPECT_TRUE(config.hasParticles());
}

TEST_F(VisualConfigurationTest, GetDefault)
{
    auto config = VisualConfiguration::getDefault();

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_FALSE(config.bloom.enabled);
}

TEST_F(VisualConfigurationTest, GetPresetDefault)
{
    auto config = VisualConfiguration::getPreset("default");

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_EQ(config.presetId, "default");
}

TEST_F(VisualConfigurationTest, GetPresetUnknown)
{
    auto config = VisualConfiguration::getPreset("nonexistent_preset");

    // Unknown preset should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

// =============================================================================
// VisualConfiguration Serialization Tests
// =============================================================================

TEST_F(VisualConfigurationTest, SerializationRoundTrip)
{
    VisualConfiguration original;
    original.shaderType = ShaderType::NeonGlow;
    original.compositeOpacity = 0.75f;

    // Enable some effects
    original.bloom.enabled = true;
    original.bloom.intensity = 2.0f;
    original.bloom.threshold = 0.6f;

    original.trails.enabled = true;
    original.trails.decay = 0.05f;

    original.particles.enabled = true;
    original.particles.emissionRate = 200.0f;
    original.particles.particleColor = juce::Colours::cyan;

    original.settings3D.enabled = true;
    original.settings3D.cameraDistance = 8.0f;
    original.settings3D.autoRotate = true;

    original.material.enabled = true;
    original.material.reflectivity = 0.8f;

    // Serialize
    auto tree = original.toValueTree();

    // Deserialize
    auto restored = VisualConfiguration::fromValueTree(tree);

    // Verify all values
    EXPECT_EQ(restored.shaderType, original.shaderType);
    EXPECT_FLOAT_EQ(restored.compositeOpacity, original.compositeOpacity);

    EXPECT_EQ(restored.bloom.enabled, original.bloom.enabled);
    EXPECT_FLOAT_EQ(restored.bloom.intensity, original.bloom.intensity);
    EXPECT_FLOAT_EQ(restored.bloom.threshold, original.bloom.threshold);

    EXPECT_EQ(restored.trails.enabled, original.trails.enabled);
    EXPECT_FLOAT_EQ(restored.trails.decay, original.trails.decay);

    EXPECT_EQ(restored.particles.enabled, original.particles.enabled);
    EXPECT_FLOAT_EQ(restored.particles.emissionRate, original.particles.emissionRate);
    EXPECT_EQ(restored.particles.particleColor.getARGB(), original.particles.particleColor.getARGB());

    EXPECT_EQ(restored.settings3D.enabled, original.settings3D.enabled);
    EXPECT_FLOAT_EQ(restored.settings3D.cameraDistance, original.settings3D.cameraDistance);
    EXPECT_EQ(restored.settings3D.autoRotate, original.settings3D.autoRotate);

    EXPECT_EQ(restored.material.enabled, original.material.enabled);
    EXPECT_FLOAT_EQ(restored.material.reflectivity, original.material.reflectivity);
}

TEST_F(VisualConfigurationTest, DeserializeInvalidTree)
{
    juce::ValueTree invalidTree("WrongType");

    auto config = VisualConfiguration::fromValueTree(invalidTree);

    // Should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigurationTest, DeserializeEmptyTree)
{
    juce::ValueTree emptyTree;

    auto config = VisualConfiguration::fromValueTree(emptyTree);

    // Should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigurationTest, SerializationPreservesAllPostProcessing)
{
    VisualConfiguration original;

    // Enable all post-processing effects
    original.bloom.enabled = true;
    original.trails.enabled = true;
    original.colorGrade.enabled = true;
    original.vignette.enabled = true;
    original.filmGrain.enabled = true;
    original.chromaticAberration.enabled = true;
    original.scanlines.enabled = true;
    original.distortion.enabled = true;

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_TRUE(restored.bloom.enabled);
    EXPECT_TRUE(restored.trails.enabled);
    EXPECT_TRUE(restored.colorGrade.enabled);
    EXPECT_TRUE(restored.vignette.enabled);
    EXPECT_TRUE(restored.filmGrain.enabled);
    EXPECT_TRUE(restored.chromaticAberration.enabled);
    EXPECT_TRUE(restored.scanlines.enabled);
    EXPECT_TRUE(restored.distortion.enabled);
}

TEST_F(VisualConfigurationTest, SerializationPreservesColorGradeDetails)
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

TEST_F(VisualConfigurationTest, SerializationPreservesParticleDetails)
{
    VisualConfiguration original;
    original.particles.enabled = true;
    original.particles.emissionMode = ParticleEmissionMode::AtPeaks;
    original.particles.blendMode = ParticleBlendMode::Screen;
    original.particles.gravity = -50.0f;
    original.particles.drag = 0.2f;
    original.particles.randomness = 0.8f;
    original.particles.audioReactive = false;
    original.particles.audioEmissionBoost = 3.0f;

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_EQ(restored.particles.emissionMode, original.particles.emissionMode);
    EXPECT_EQ(restored.particles.blendMode, original.particles.blendMode);
    EXPECT_FLOAT_EQ(restored.particles.gravity, original.particles.gravity);
    EXPECT_FLOAT_EQ(restored.particles.drag, original.particles.drag);
    EXPECT_FLOAT_EQ(restored.particles.randomness, original.particles.randomness);
    EXPECT_EQ(restored.particles.audioReactive, original.particles.audioReactive);
    EXPECT_FLOAT_EQ(restored.particles.audioEmissionBoost, original.particles.audioEmissionBoost);
}

TEST_F(VisualConfigurationTest, SerializationPreserves3DDetails)
{
    VisualConfiguration original;
    original.settings3D.enabled = true;
    original.settings3D.cameraAngleX = 45.0f;
    original.settings3D.cameraAngleY = 30.0f;
    original.settings3D.meshResolutionX = 256;
    original.settings3D.meshResolutionZ = 64;
    original.settings3D.meshScale = 1.5f;

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_FLOAT_EQ(restored.settings3D.cameraAngleX, original.settings3D.cameraAngleX);
    EXPECT_FLOAT_EQ(restored.settings3D.cameraAngleY, original.settings3D.cameraAngleY);
    EXPECT_EQ(restored.settings3D.meshResolutionX, original.settings3D.meshResolutionX);
    EXPECT_EQ(restored.settings3D.meshResolutionZ, original.settings3D.meshResolutionZ);
    EXPECT_FLOAT_EQ(restored.settings3D.meshScale, original.settings3D.meshScale);
}

TEST_F(VisualConfigurationTest, SerializationPreservesMaterialDetails)
{
    VisualConfiguration original;
    original.material.enabled = true;
    original.material.reflectivity = 0.9f;
    original.material.refractiveIndex = 1.33f;  // Water
    original.material.fresnelPower = 5.0f;
    original.material.tintColor = juce::Colours::lightblue;
    original.material.roughness = 0.05f;
    original.material.useEnvironmentMap = false;
    original.material.environmentMapId = "custom_env";

    auto tree = original.toValueTree();
    auto restored = VisualConfiguration::fromValueTree(tree);

    EXPECT_FLOAT_EQ(restored.material.reflectivity, original.material.reflectivity);
    EXPECT_FLOAT_EQ(restored.material.refractiveIndex, original.material.refractiveIndex);
    EXPECT_FLOAT_EQ(restored.material.fresnelPower, original.material.fresnelPower);
    EXPECT_EQ(restored.material.tintColor.getARGB(), original.material.tintColor.getARGB());
    EXPECT_FLOAT_EQ(restored.material.roughness, original.material.roughness);
    EXPECT_EQ(restored.material.useEnvironmentMap, original.material.useEnvironmentMap);
    EXPECT_EQ(restored.material.environmentMapId, original.material.environmentMapId);
}

// =============================================================================
// Preset Tests
// =============================================================================

TEST_F(VisualConfigurationTest, TestCyberpunkPreset)
{
    auto config = VisualConfiguration::getPreset("cyberpunk");

    EXPECT_EQ(config.shaderType, ShaderType::NeonGlow);
    EXPECT_TRUE(config.bloom.enabled);
    EXPECT_FLOAT_EQ(config.bloom.intensity, 1.8f);
    
    // Verify fix for particle emission mode
    EXPECT_TRUE(config.particles.enabled);
    EXPECT_EQ(config.particles.emissionMode, ParticleEmissionMode::AlongWaveform);
}

TEST_F(VisualConfigurationTest, TestLiquidGoldPreset)
{
    auto config = VisualConfiguration::getPreset("liquid_gold");

    EXPECT_EQ(config.shaderType, ShaderType::LiquidChrome);
    EXPECT_TRUE(config.material.enabled);
    EXPECT_TRUE(config.bloom.enabled);
}

TEST_F(VisualConfigurationTest, TestDeepOceanPreset)
{
    auto config = VisualConfiguration::getPreset("deep_ocean");

    EXPECT_EQ(config.shaderType, ShaderType::VolumetricRibbon);
    EXPECT_TRUE(config.distortion.enabled);
    EXPECT_FALSE(config.particles.enabled);
}

TEST_F(VisualConfigurationTest, TestSynthwavePreset)
{
    auto config = VisualConfiguration::getPreset("synthwave");

    EXPECT_EQ(config.shaderType, ShaderType::WireframeMesh);
    EXPECT_TRUE(config.scanlines.enabled);
    EXPECT_TRUE(config.vignette.enabled);
}

TEST_F(VisualConfigurationTest, TestMatrixPreset)
{
    auto config = VisualConfiguration::getPreset("matrix");

    EXPECT_EQ(config.shaderType, ShaderType::DigitalGlitch);
    EXPECT_TRUE(config.particles.enabled);
    EXPECT_EQ(config.particles.emissionMode, ParticleEmissionMode::AlongWaveform);
}

TEST_F(VisualConfigurationTest, GetAvailablePresets)
{
    auto presets = VisualConfiguration::getAvailablePresets();

    EXPECT_FALSE(presets.empty());

    // Check that standard presets exist
    bool foundDefault = false;
    bool foundCyberpunk = false;
    bool foundLiquidGold = false;

    for (const auto& preset : presets)
    {
        if (preset.first == "default") foundDefault = true;
        if (preset.first == "cyberpunk") foundCyberpunk = true;
        if (preset.first == "liquid_gold") foundLiquidGold = true;
    }

    EXPECT_TRUE(foundDefault);
    EXPECT_TRUE(foundCyberpunk);
    EXPECT_TRUE(foundLiquidGold);
}
