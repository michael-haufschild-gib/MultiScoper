/*
    Oscil - Visual Configuration Preset Tests
    Tests for preset management and serialization
*/

#include <gtest/gtest.h>
#include "rendering/VisualConfiguration.h"

using namespace oscil;

// =============================================================================
// VisualConfiguration Serialization Tests
// =============================================================================

class VisualConfigPresetTest : public ::testing::Test
{
};

TEST_F(VisualConfigPresetTest, SerializationRoundTrip)
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

TEST_F(VisualConfigPresetTest, DeserializeInvalidTree)
{
    juce::ValueTree invalidTree("WrongType");

    auto config = VisualConfiguration::fromValueTree(invalidTree);

    // Should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigPresetTest, DeserializeEmptyTree)
{
    juce::ValueTree emptyTree;

    auto config = VisualConfiguration::fromValueTree(emptyTree);

    // Should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}

TEST_F(VisualConfigPresetTest, SerializationPreservesAllPostProcessing)
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
    original.tiltShift.enabled = true;

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
    EXPECT_TRUE(restored.tiltShift.enabled);
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

TEST_F(VisualConfigPresetTest, SerializationPreservesParticleDetails)
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

TEST_F(VisualConfigPresetTest, SerializationPreserves3DDetails)
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

TEST_F(VisualConfigPresetTest, SerializationPreservesMaterialDetails)
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

TEST_F(VisualConfigPresetTest, GetAvailablePresets)
{
    auto presets = VisualConfiguration::getAvailablePresets();

    EXPECT_FALSE(presets.empty());

    // Check that standard presets exist
    bool foundDefault = false;
    bool foundCyberpunk = false;
    bool foundLiquidGold = false;
    bool foundCinematic = false;
    bool foundSolarStorm = false;
    bool foundDeepOcean = false;
    bool foundSynthwave = false;
    bool foundMatrix = false;

    for (const auto& preset : presets)
    {
        if (preset.first == "default") foundDefault = true;
        if (preset.first == "cyberpunk") foundCyberpunk = true;
        if (preset.first == "liquid_gold") foundLiquidGold = true;
        if (preset.first == "cinematic") foundCinematic = true;
        if (preset.first == "solar_storm") foundSolarStorm = true;
        if (preset.first == "deep_ocean") foundDeepOcean = true;
        if (preset.first == "synthwave") foundSynthwave = true;
        if (preset.first == "matrix") foundMatrix = true;
    }

    EXPECT_TRUE(foundDefault);
    EXPECT_FALSE(foundCyberpunk); // Cyberpunk was removed
    EXPECT_FALSE(foundLiquidGold);
    EXPECT_FALSE(foundCinematic);
    EXPECT_FALSE(foundSolarStorm);
    EXPECT_FALSE(foundDeepOcean);
    EXPECT_FALSE(foundSynthwave);
    EXPECT_FALSE(foundMatrix);
}
