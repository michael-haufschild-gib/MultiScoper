/*
    Oscil - Visual Configuration Application Tests
    Tests for applying configurations and checking requirements
*/

#include <gtest/gtest.h>
#include "rendering/VisualConfiguration.h"

using namespace oscil;

// =============================================================================
// VisualConfiguration Application Tests
// =============================================================================

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

TEST_F(VisualConfigApplicationTest, Requires3DWithEnabled)
{
    VisualConfiguration config;

    EXPECT_FALSE(config.requires3D());

    config.settings3D.enabled = true;
    EXPECT_TRUE(config.requires3D());
}

TEST_F(VisualConfigApplicationTest, Requires3DWith3DShader)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::VolumetricRibbon;

    EXPECT_TRUE(config.requires3D());
}

TEST_F(VisualConfigApplicationTest, HasPostProcessing)
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

    // Unknown preset should return default configuration
    EXPECT_EQ(config.shaderType, ShaderType::Basic2D);
}
