/*
    MultiScoper - Preset Manager Tests
*/

#include "rendering/PresetManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class PresetManagerTest : public ::testing::Test
{
protected:
    juce::File tempDir_;
    std::unique_ptr<PresetManager> manager_;

    void SetUp() override
    {
        tempDir_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("multiscoper_preset_test_" + juce::Uuid().toString());
        ASSERT_TRUE(tempDir_.createDirectory())
            << "Failed to create temp preset directory: " << tempDir_.getFullPathName();
        manager_ = std::make_unique<PresetManager>(tempDir_);
    }

    void TearDown() override
    {
        manager_.reset();
        tempDir_.deleteRecursively();
    }
};

TEST_F(PresetManagerTest, BuiltInPresetsAvailable)
{
    auto presets = manager_->getAvailablePresets();

    ASSERT_GE(presets.size(), 2u);

    bool foundDefault = false;
    bool foundVectorScope = false;
    for (const auto& p : presets)
    {
        if (p.id == "default")
            foundDefault = true;
        if (p.id == "vector_scope")
            foundVectorScope = true;
        EXPECT_TRUE(p.isBuiltIn);
    }
    EXPECT_TRUE(foundDefault);
    EXPECT_TRUE(foundVectorScope);
}

TEST_F(PresetManagerTest, LoadBuiltInPreset)
{
    auto config = manager_->loadPreset("vector_scope");
    EXPECT_TRUE(config.bloom.enabled);
    EXPECT_TRUE(config.trails.enabled);
    EXPECT_TRUE(config.filmGrain.enabled);
}

TEST_F(PresetManagerTest, SaveAndLoadUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::NeonGlow;
    config.bloom.enabled = true;
    config.bloom.intensity = 1.8f;
    auto id = manager_->saveUserPreset("My Custom Preset", config);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, "user_my_custom_preset");

    auto presets = manager_->getAvailablePresets();
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

    auto loaded = manager_->loadPreset(*id);
    EXPECT_EQ(loaded.shaderType, ShaderType::NeonGlow);
    EXPECT_TRUE(loaded.bloom.enabled);
    EXPECT_FLOAT_EQ(loaded.bloom.intensity, 1.8f);
}

TEST_F(PresetManagerTest, DeleteUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::GradientFill;
    auto id = manager_->saveUserPreset("ToDelete", config);
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(manager_->isUserPreset(*id));

    EXPECT_TRUE(manager_->deleteUserPreset(*id));
    EXPECT_FALSE(manager_->isUserPreset(*id));
}

TEST_F(PresetManagerTest, CannotDeleteBuiltInPreset)
{
    EXPECT_FALSE(manager_->deleteUserPreset("default"));
    EXPECT_FALSE(manager_->deleteUserPreset("vector_scope"));
}

TEST_F(PresetManagerTest, ExportAndImportPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::DualOutline;
    config.chromaticAberration.enabled = true;
    config.chromaticAberration.intensity = 0.015f;
    auto id = manager_->saveUserPreset("Exportable", config);
    ASSERT_TRUE(id.has_value());

    auto exportFile = tempDir_.getChildFile("test_export.oscpreset");
    ASSERT_TRUE(manager_->exportPreset(*id, exportFile));
    EXPECT_TRUE(exportFile.existsAsFile());

    manager_->deleteUserPreset(*id);
    EXPECT_FALSE(manager_->isUserPreset(*id));

    ASSERT_TRUE(manager_->importPreset(exportFile));
    EXPECT_TRUE(manager_->isUserPreset(*id));

    auto loaded = manager_->loadPreset(*id);
    EXPECT_EQ(loaded.shaderType, ShaderType::DualOutline);
    EXPECT_TRUE(loaded.chromaticAberration.enabled);
    EXPECT_FLOAT_EQ(loaded.chromaticAberration.intensity, 0.015f);

    exportFile.deleteFile();
}

TEST_F(PresetManagerTest, SaveEmptyNameFails)
{
    VisualConfiguration config;
    EXPECT_FALSE(manager_->saveUserPreset("", config).has_value());
}

TEST_F(PresetManagerTest, RenameUserPreset)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::GradientFill;
    auto id = manager_->saveUserPreset("OldName", config);
    ASSERT_TRUE(id.has_value());

    EXPECT_TRUE(manager_->renameUserPreset(*id, "NewName"));
    EXPECT_FALSE(manager_->isUserPreset(*id));
    EXPECT_TRUE(manager_->isUserPreset("user_newname"));

    auto loaded = manager_->loadPreset("user_newname");
    EXPECT_EQ(loaded.shaderType, ShaderType::GradientFill);
}

TEST_F(PresetManagerTest, RenameRefusesExistingTarget)
{
    VisualConfiguration a;
    VisualConfiguration b;
    auto idA = manager_->saveUserPreset("Alpha", a);
    auto idB = manager_->saveUserPreset("Beta", b);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    // Alpha -> "Beta" collides with existing Beta; must fail and leave both intact.
    EXPECT_FALSE(manager_->renameUserPreset(*idA, "Beta"));
    EXPECT_TRUE(manager_->isUserPreset(*idA));
    EXPECT_TRUE(manager_->isUserPreset(*idB));
}

TEST_F(PresetManagerTest, ImportCannotShadowBuiltInId)
{
    // A tampered export claims presetId="default" (a built-in) to try to
    // shadow the built-in on the next loadPreset call. The new import path
    // derives the ID from displayName and forces the "user_" prefix, so the
    // import lands as "user_default" (if its displayName was "default"),
    // never at "default.oscpreset".
    VisualConfiguration config;
    config.shaderType = ShaderType::NeonGlow;
    config.bloom.enabled = true;
    auto id = manager_->saveUserPreset("TamperSource", config);
    ASSERT_TRUE(id.has_value());

    auto exportFile = tempDir_.getChildFile("tampered.oscpreset");
    ASSERT_TRUE(manager_->exportPreset(*id, exportFile));

    auto xml = juce::XmlDocument::parse(exportFile);
    ASSERT_NE(xml, nullptr);
    auto tree = juce::ValueTree::fromXml(*xml);
    tree.setProperty("presetId", "default", nullptr);
    tree.setProperty("displayName", "default", nullptr);
    auto rewritten = tree.createXml();
    ASSERT_NE(rewritten, nullptr);
    ASSERT_TRUE(rewritten->writeTo(exportFile));

    EXPECT_TRUE(manager_->importPreset(exportFile));

    // The untouched built-in "default" must not be shadowed.
    EXPECT_FALSE(tempDir_.getChildFile("default.oscpreset").existsAsFile());
    auto builtIn = manager_->loadPreset("default");
    EXPECT_FALSE(builtIn.bloom.enabled);
    EXPECT_EQ(builtIn.shaderType, ShaderType::Basic2D);

    // The tampered file instead landed at user_default.
    EXPECT_TRUE(manager_->isUserPreset("user_default"));
}

TEST_F(PresetManagerTest, ImportRefusesExistingUserPresetCollision)
{
    VisualConfiguration config;
    config.shaderType = ShaderType::NeonGlow;
    auto id = manager_->saveUserPreset("Dup", config);
    ASSERT_TRUE(id.has_value());

    auto exportFile = tempDir_.getChildFile("dup.oscpreset");
    ASSERT_TRUE(manager_->exportPreset(*id, exportFile));

    // Existing user preset with the same derived ID must block the import.
    EXPECT_FALSE(manager_->importPreset(exportFile));

    // Deleting the original allows the import to land.
    EXPECT_TRUE(manager_->deleteUserPreset(*id));
    EXPECT_TRUE(manager_->importPreset(exportFile));
}

TEST_F(PresetManagerTest, PresetExistsReportsBuiltInsAndUser)
{
    EXPECT_TRUE(manager_->presetExists("default"));
    EXPECT_TRUE(manager_->presetExists("vector_scope"));
    EXPECT_FALSE(manager_->presetExists("not_a_real_id"));

    VisualConfiguration config;
    auto id = manager_->saveUserPreset("Existing", config);
    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(manager_->presetExists(*id));
}
