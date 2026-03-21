/*
    Oscil - Preset Manager Tests
*/

#include <gtest/gtest.h>
#include "rendering/PresetManager.h"

using namespace oscil;

class PresetManagerTest : public ::testing::Test
{
protected:
    PresetManager manager;

    void SetUp() override
    {
        // Clean up any user presets from previous test runs
        auto dir = manager.getPresetsDirectory();
        for (const auto& entry : juce::RangedDirectoryIterator(dir, false, "*.oscpreset"))
        {
            entry.getFile().deleteFile();
        }
    }

    void TearDown() override
    {
        // Clean up test presets
        auto dir = manager.getPresetsDirectory();
        for (const auto& entry : juce::RangedDirectoryIterator(dir, false, "*.oscpreset"))
        {
            entry.getFile().deleteFile();
        }
    }
};

TEST_F(PresetManagerTest, BuiltInPresetsAvailable)
{
    auto presets = manager.getAvailablePresets();

    // Should have at least the 4 built-in presets
    ASSERT_GE(presets.size(), 4u);

    bool foundDefault = false;
    bool foundVectorScope = false;
    for (const auto& p : presets)
    {
        if (p.id == "default") foundDefault = true;
        if (p.id == "vector_scope") foundVectorScope = true;
        EXPECT_TRUE(p.isBuiltIn);
    }
    EXPECT_TRUE(foundDefault);
    EXPECT_TRUE(foundVectorScope);
}

TEST_F(PresetManagerTest, LoadBuiltInPreset)
{
    auto config = manager.loadPreset("vector_scope");
    EXPECT_TRUE(config.scanlines.enabled);
    EXPECT_TRUE(config.bloom.enabled);
    EXPECT_TRUE(config.trails.enabled);
}

TEST_F(PresetManagerTest, SaveAndLoadUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::NeonGlow;
    config.bloom.enabled = true;
    config.bloom.intensity = 2.5f;
    config.particles.enabled = true;
    config.particles.emissionRate = 100.0f;

    ASSERT_TRUE(manager.saveUserPreset("My Custom Preset", config));

    // Verify it appears in available presets
    auto presets = manager.getAvailablePresets();
    bool found = false;
    for (const auto& p : presets)
    {
        if (p.displayName == "My Custom Preset")
        {
            found = true;
            EXPECT_FALSE(p.isBuiltIn);
        }
    }
    EXPECT_TRUE(found);

    // Load and verify the values roundtrip
    auto loaded = manager.loadPreset("user_my_custom_preset");
    EXPECT_EQ(loaded.shaderType, ShaderType::NeonGlow);
    EXPECT_TRUE(loaded.bloom.enabled);
    EXPECT_FLOAT_EQ(loaded.bloom.intensity, 2.5f);
    EXPECT_TRUE(loaded.particles.enabled);
    EXPECT_FLOAT_EQ(loaded.particles.emissionRate, 100.0f);
}

TEST_F(PresetManagerTest, DeleteUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::GradientFill;
    ASSERT_TRUE(manager.saveUserPreset("ToDelete", config));
    EXPECT_TRUE(manager.isUserPreset("user_todelete"));

    EXPECT_TRUE(manager.deleteUserPreset("user_todelete"));
    EXPECT_FALSE(manager.isUserPreset("user_todelete"));
}

TEST_F(PresetManagerTest, CannotDeleteBuiltInPreset)
{
    EXPECT_FALSE(manager.deleteUserPreset("default"));
    EXPECT_FALSE(manager.deleteUserPreset("vector_scope"));
}

TEST_F(PresetManagerTest, ExportAndImportPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::PlasmaSine;
    config.chromaticAberration.enabled = true;
    config.chromaticAberration.intensity = 0.8f;
    ASSERT_TRUE(manager.saveUserPreset("Exportable", config));

    auto exportFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("test_export.oscpreset");
    ASSERT_TRUE(manager.exportPreset("user_exportable", exportFile));
    EXPECT_TRUE(exportFile.existsAsFile());

    // Delete the original
    manager.deleteUserPreset("user_exportable");
    EXPECT_FALSE(manager.isUserPreset("user_exportable"));

    // Import it back
    ASSERT_TRUE(manager.importPreset(exportFile));
    EXPECT_TRUE(manager.isUserPreset("user_exportable"));

    // Verify data survived roundtrip
    auto loaded = manager.loadPreset("user_exportable");
    EXPECT_EQ(loaded.shaderType, ShaderType::PlasmaSine);
    EXPECT_TRUE(loaded.chromaticAberration.enabled);
    EXPECT_FLOAT_EQ(loaded.chromaticAberration.intensity, 0.8f);

    exportFile.deleteFile();
}

TEST_F(PresetManagerTest, SaveEmptyNameFails)
{
    VisualConfiguration config;
    EXPECT_FALSE(manager.saveUserPreset("", config));
}

TEST_F(PresetManagerTest, RenameUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::DigitalGlitch;
    ASSERT_TRUE(manager.saveUserPreset("OldName", config));

    EXPECT_TRUE(manager.renameUserPreset("user_oldname", "NewName"));
    EXPECT_FALSE(manager.isUserPreset("user_oldname"));
    EXPECT_TRUE(manager.isUserPreset("user_newname"));

    auto loaded = manager.loadPreset("user_newname");
    EXPECT_EQ(loaded.shaderType, ShaderType::DigitalGlitch);
}
