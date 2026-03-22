/*
    Oscil - Visual Configuration Application Tests
    Tests for applying configurations and checking requirements
*/

#include <gtest/gtest.h>
#include "rendering/VisualConfiguration.h"

using namespace oscil;

class VisualConfigApplicationTest : public ::testing::Test
{
};

TEST_F(VisualConfigApplicationTest, DefaultValues)
{
    VisualConfiguration config;

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_EQ(config.compositeBlendMode, BlendMode::Alpha);
    EXPECT_FLOAT_EQ(config.compositeOpacity, 1.0f);
    EXPECT_EQ(config.presetId, "default");
}

TEST_F(VisualConfigApplicationTest, HasPostProcessingDefaultIsFalse)
{
    VisualConfiguration config;
    EXPECT_FALSE(config.hasPostProcessing());
}

TEST_F(VisualConfigApplicationTest, HasPostProcessingDetectsEveryEffect)
{
    // Test each effect individually to ensure none are missed
    auto testEffect = [](auto enableFn) {
        VisualConfiguration config;
        enableFn(config);
        EXPECT_TRUE(config.hasPostProcessing());
    };

    testEffect([](VisualConfiguration& c) { c.bloom.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.radialBlur.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.tiltShift.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.trails.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.colorGrade.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.vignette.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.filmGrain.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.chromaticAberration.enabled = true; });
    testEffect([](VisualConfiguration& c) { c.scanlines.enabled = true; });
}

TEST_F(VisualConfigApplicationTest, HasPostProcessingMultipleEffectsStillTrue)
{
    VisualConfiguration config;
    config.bloom.enabled = true;
    config.scanlines.enabled = true;
    EXPECT_TRUE(config.hasPostProcessing());

    config.bloom.enabled = false;
    EXPECT_TRUE(config.hasPostProcessing());

    config.scanlines.enabled = false;
    EXPECT_FALSE(config.hasPostProcessing());
}

TEST_F(VisualConfigApplicationTest, GetDefault)
{
    auto config = VisualConfiguration::getDefault();

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_FALSE(config.bloom.enabled);
}

TEST_F(VisualConfigApplicationTest, GetPresetDefault)
{
    auto config = VisualConfiguration::getPreset("default");

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
    EXPECT_EQ(config.presetId, "default");
}

TEST_F(VisualConfigApplicationTest, GetPresetUnknown)
{
    auto config = VisualConfiguration::getPreset("nonexistent_preset");

    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}
