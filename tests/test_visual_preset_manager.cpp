/*
    Oscil - Visual Preset Manager Tests
    Tests for VisualPresetManager and VisualPreset
*/

#include <gtest/gtest.h>
#include "core/VisualPresetManager.h"
#include "core/VisualPreset.h"
#include "rendering/VisualConfiguration.h"
#include <filesystem>

using namespace oscil;

// =============================================================================
// Test Fixtures
// =============================================================================

/**
 * Test fixture that creates a temporary directory for preset storage.
 * This ensures tests don't interfere with actual user presets.
 */
class VisualPresetManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a unique temporary directory for this test
        tempDir_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("oscil_test_presets_" + juce::Uuid().toString());
        tempDir_.createDirectory();

        // Create manager instance with temp directory
        manager_ = std::make_unique<VisualPresetManager>(tempDir_);

        // Generate a unique suffix for this test run
        testId_ = juce::Uuid().toString().substring(0, 8);
    }

    void TearDown() override
    {
        // Clean up any presets created during the test
        for (const auto& id : createdPresetIds_)
        {
            manager_->deletePreset(id);
        }

        // Shutdown manager before cleanup
        if (manager_)
        {
            manager_->shutdown();
            manager_.reset();
        }

        // Clean up temporary directory
        if (tempDir_.exists())
        {
            tempDir_.deleteRecursively();
        }
    }

    // Helper to create unique preset names
    juce::String uniqueName(const juce::String& baseName)
    {
        return baseName + " " + testId_;
    }

    // Helper to create a preset and track it for cleanup
    Result<VisualPreset> createAndTrack(const juce::String& baseName, const VisualConfiguration& config)
    {
        auto result = manager_->createPreset(uniqueName(baseName), config);
        if (result.success)
        {
            createdPresetIds_.push_back(result.value.id);
        }
        return result;
    }

    juce::File tempDir_;
    std::unique_ptr<VisualPresetManager> manager_;
    juce::String testId_;
    std::vector<juce::String> createdPresetIds_;
};

// =============================================================================
// VisualPreset Entity Tests
// =============================================================================

class VisualPresetTest : public ::testing::Test
{
};

TEST_F(VisualPresetTest, DefaultConstruction)
{
    VisualPreset preset;
    EXPECT_TRUE(preset.id.isEmpty());
    EXPECT_TRUE(preset.name.isEmpty());
    EXPECT_TRUE(preset.description.isEmpty());
    EXPECT_EQ(preset.category, PresetCategory::User);
    EXPECT_FALSE(preset.isFavorite);
    EXPECT_FALSE(preset.isReadOnly);
    EXPECT_EQ(preset.version, 1);
}

TEST_F(VisualPresetTest, IsValidRequiresIdAndName)
{
    VisualPreset preset;
    EXPECT_FALSE(preset.isValid());

    preset.id = "test-id";
    EXPECT_FALSE(preset.isValid());

    preset.name = "Test Name";
    EXPECT_TRUE(preset.isValid());
}

TEST_F(VisualPresetTest, GenerateIdCreatesUniqueIds)
{
    juce::String id1 = VisualPreset::generateId();
    juce::String id2 = VisualPreset::generateId();
    juce::String id3 = VisualPreset::generateId();

    EXPECT_FALSE(id1.isEmpty());
    EXPECT_FALSE(id2.isEmpty());
    EXPECT_FALSE(id3.isEmpty());
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(VisualPresetTest, NowReturnsCurrentTime)
{
    juce::Time before = juce::Time::getCurrentTime();
    juce::Time now = VisualPreset::now();
    juce::Time after = juce::Time::getCurrentTime();

    EXPECT_GE(now.toMilliseconds(), before.toMilliseconds());
    EXPECT_LE(now.toMilliseconds(), after.toMilliseconds());
}

TEST_F(VisualPresetTest, JsonRoundTrip)
{
    // Create a preset with all fields populated
    VisualPreset original;
    original.id = VisualPreset::generateId();
    original.name = "Test Preset";
    original.description = "A test description";
    original.category = PresetCategory::User;
    original.isFavorite = true;
    original.isReadOnly = false;
    original.createdAt = juce::Time::getCurrentTime();
    original.modifiedAt = juce::Time::getCurrentTime();
    original.version = 1;
    original.configuration = VisualConfiguration::getDefault();
    original.configuration.bloom.enabled = true;
    original.configuration.bloom.intensity = 1.5f;

    // Serialize to JSON
    juce::var json = original.toJson();

    // Deserialize back
    VisualPreset restored = VisualPreset::fromJson(json);

    // Verify all fields match
    EXPECT_EQ(restored.id, original.id);
    EXPECT_EQ(restored.name, original.name);
    EXPECT_EQ(restored.description, original.description);
    EXPECT_EQ(restored.category, original.category);
    EXPECT_EQ(restored.isFavorite, original.isFavorite);
    EXPECT_EQ(restored.isReadOnly, original.isReadOnly);
    EXPECT_EQ(restored.version, original.version);

    // Verify configuration preserved
    EXPECT_TRUE(restored.configuration.bloom.enabled);
    EXPECT_FLOAT_EQ(restored.configuration.bloom.intensity, 1.5f);
}

TEST_F(VisualPresetTest, JsonRoundTripWithSystemCategory)
{
    VisualPreset original;
    original.id = VisualPreset::generateId();
    original.name = "System Preset";
    original.category = PresetCategory::System;
    original.isReadOnly = true;
    original.configuration = VisualConfiguration::getDefault();

    juce::var json = original.toJson();
    VisualPreset restored = VisualPreset::fromJson(json);

    EXPECT_EQ(restored.category, PresetCategory::System);
    EXPECT_TRUE(restored.isReadOnly);
}

// =============================================================================
// PresetCategory Tests
// =============================================================================

class PresetCategoryTest : public ::testing::Test
{
};

TEST_F(PresetCategoryTest, CategoryToString)
{
    EXPECT_EQ(presetCategoryToString(PresetCategory::System), "system");
    EXPECT_EQ(presetCategoryToString(PresetCategory::User), "user");
    EXPECT_EQ(presetCategoryToString(PresetCategory::Factory), "factory");
}

TEST_F(PresetCategoryTest, StringToCategory)
{
    EXPECT_EQ(stringToPresetCategory("system"), PresetCategory::System);
    EXPECT_EQ(stringToPresetCategory("user"), PresetCategory::User);
    EXPECT_EQ(stringToPresetCategory("factory"), PresetCategory::Factory);

    // Unknown strings default to User
    EXPECT_EQ(stringToPresetCategory("unknown"), PresetCategory::User);
    EXPECT_EQ(stringToPresetCategory(""), PresetCategory::User);
}

TEST_F(PresetCategoryTest, RoundTripConversion)
{
    std::vector<PresetCategory> categories = {
        PresetCategory::System,
        PresetCategory::User,
        PresetCategory::Factory
    };

    for (auto category : categories)
    {
        juce::String str = presetCategoryToString(category);
        PresetCategory restored = stringToPresetCategory(str);
        EXPECT_EQ(restored, category);
    }
}

// =============================================================================
// Result Type Tests
// =============================================================================

class ResultTest : public ::testing::Test
{
};

TEST_F(ResultTest, ResultOkWithValue)
{
    auto result = Result<int>::ok(42);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value, 42);
    EXPECT_TRUE(result.error.isEmpty());
}

TEST_F(ResultTest, ResultOkWithMoveValue)
{
    juce::String value = "test";
    auto result = Result<juce::String>::ok(std::move(value));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value, "test");
}

TEST_F(ResultTest, ResultFail)
{
    auto result = Result<int>::fail("Something went wrong");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Something went wrong");
}

TEST_F(ResultTest, ResultVoidOk)
{
    auto result = Result<void>::ok();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.isEmpty());
}

TEST_F(ResultTest, ResultVoidFail)
{
    auto result = Result<void>::fail("Operation failed");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Operation failed");
}

// =============================================================================
// Preset Name Validation Tests
// =============================================================================

class PresetNameValidationTest : public ::testing::Test
{
};

TEST_F(PresetNameValidationTest, ValidNames)
{
    EXPECT_TRUE(validatePresetName("My Preset").success);
    EXPECT_TRUE(validatePresetName("preset-1").success);
    EXPECT_TRUE(validatePresetName("preset_2").success);
    EXPECT_TRUE(validatePresetName("A").success);
    EXPECT_TRUE(validatePresetName("Neon Glow 2024").success);
    EXPECT_TRUE(validatePresetName("Test-Preset_Name 123").success);
}

TEST_F(PresetNameValidationTest, EmptyNameFails)
{
    auto result = validatePresetName("");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.containsIgnoreCase("empty"));
}

TEST_F(PresetNameValidationTest, TooLongNameFails)
{
    juce::String longName;
    for (int i = 0; i < 65; ++i)
        longName += "a";

    auto result = validatePresetName(longName);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.containsIgnoreCase("64"));
}

TEST_F(PresetNameValidationTest, ExactlyMaxLengthSucceeds)
{
    juce::String maxName;
    for (int i = 0; i < 64; ++i)
        maxName += "a";

    EXPECT_TRUE(validatePresetName(maxName).success);
}

TEST_F(PresetNameValidationTest, InvalidCharactersFail)
{
    EXPECT_FALSE(validatePresetName("preset@name").success);
    EXPECT_FALSE(validatePresetName("preset#name").success);
    EXPECT_FALSE(validatePresetName("preset!name").success);
    EXPECT_FALSE(validatePresetName("preset.name").success);
    EXPECT_FALSE(validatePresetName("preset/name").success);
}

TEST_F(PresetNameValidationTest, ReservedNamesFail)
{
    EXPECT_FALSE(validatePresetName("default").success);
    EXPECT_FALSE(validatePresetName("Default").success);
    EXPECT_FALSE(validatePresetName("DEFAULT").success);
    EXPECT_FALSE(validatePresetName("none").success);
    EXPECT_FALSE(validatePresetName("None").success);
    EXPECT_FALSE(validatePresetName("new").success);
    EXPECT_FALSE(validatePresetName("New").success);
}

// =============================================================================
// VisualPresetManager CRUD Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, InitializeCreatesSystemPresets)
{
    manager_->initialize();

    auto presets = manager_->getAllPresets();
    EXPECT_GT(presets.size(), 0u);

    // Should have at least the default system preset
    auto defaultPreset = manager_->getPresetByName("Default");
    EXPECT_TRUE(defaultPreset.has_value());
    EXPECT_EQ(defaultPreset->category, PresetCategory::System);
    EXPECT_TRUE(defaultPreset->isReadOnly);
}

TEST_F(VisualPresetManagerTest, GetPresetsByCategory)
{
    manager_->initialize();

    auto systemPresets = manager_->getPresetsByCategory(PresetCategory::System);
    EXPECT_GT(systemPresets.size(), 0u);

    for (const auto& preset : systemPresets)
    {
        EXPECT_EQ(preset.category, PresetCategory::System);
        EXPECT_TRUE(preset.isReadOnly);
    }
}

TEST_F(VisualPresetManagerTest, CreateUserPreset)
{
    manager_->initialize();

    VisualConfiguration config = VisualConfiguration::getDefault();
    config.bloom.enabled = true;
    config.bloom.intensity = 1.8f;

    juce::String name = uniqueName("My Custom Preset");
    auto result = manager_->createPreset(name, config);
    EXPECT_TRUE(result.success) << "Failed: " << result.error.toStdString();
    if (result.success) createdPresetIds_.push_back(result.value.id);
    EXPECT_FALSE(result.value.id.isEmpty());
    EXPECT_EQ(result.value.name, name);
    EXPECT_EQ(result.value.category, PresetCategory::User);
    EXPECT_FALSE(result.value.isReadOnly);

    // Verify configuration was saved
    EXPECT_TRUE(result.value.configuration.bloom.enabled);
    EXPECT_FLOAT_EQ(result.value.configuration.bloom.intensity, 1.8f);

    // Should be retrievable
    auto retrieved = manager_->getPresetById(result.value.id);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, name);
}

TEST_F(VisualPresetManagerTest, CreatePresetWithDescription)
{
    manager_->initialize();

    juce::String name = uniqueName("Described Preset");
    auto result = manager_->createPreset(
        name,
        "This is a detailed description of my preset",
        VisualConfiguration::getDefault()
    );

    EXPECT_TRUE(result.success) << "Failed: " << result.error.toStdString();
    if (result.success) createdPresetIds_.push_back(result.value.id);
    EXPECT_EQ(result.value.description, "This is a detailed description of my preset");
}

TEST_F(VisualPresetManagerTest, CreatePresetWithInvalidNameFails)
{
    manager_->initialize();

    // Empty name
    auto result1 = manager_->createPreset("", VisualConfiguration::getDefault());
    EXPECT_FALSE(result1.success);

    // Reserved name
    auto result2 = manager_->createPreset("default", VisualConfiguration::getDefault());
    EXPECT_FALSE(result2.success);

    // Invalid characters
    auto result3 = manager_->createPreset("preset@invalid", VisualConfiguration::getDefault());
    EXPECT_FALSE(result3.success);
}

TEST_F(VisualPresetManagerTest, GetPresetById)
{
    manager_->initialize();

    // Create a preset
    juce::String name = uniqueName("Find Me");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Find by ID
    auto found = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->name, name);

    // Non-existent ID
    auto notFound = manager_->getPresetById("non-existent-id-12345");
    EXPECT_FALSE(notFound.has_value());
}

TEST_F(VisualPresetManagerTest, GetPresetByName)
{
    manager_->initialize();

    // Create a preset
    juce::String name = uniqueName("Named Preset");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Find by name
    auto found = manager_->getPresetByName(name);
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->id, createResult.value.id);

    // Non-existent name
    auto notFound = manager_->getPresetByName("Non Existent Name " + testId_);
    EXPECT_FALSE(notFound.has_value());
}

TEST_F(VisualPresetManagerTest, UpdatePreset)
{
    manager_->initialize();

    // Create a preset
    juce::String name = uniqueName("Update Me");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Update configuration
    VisualConfiguration newConfig = VisualConfiguration::getDefault();
    newConfig.shaderType = ShaderType::NeonGlow;
    newConfig.bloom.enabled = true;

    auto updateResult = manager_->updatePreset(createResult.value.id, newConfig);
    EXPECT_TRUE(updateResult.success);

    // Verify update
    auto updated = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(updated.has_value());
    EXPECT_EQ(updated->configuration.shaderType, ShaderType::NeonGlow);
    EXPECT_TRUE(updated->configuration.bloom.enabled);
}

TEST_F(VisualPresetManagerTest, UpdateNonExistentPresetFails)
{
    manager_->initialize();

    auto result = manager_->updatePreset("non-existent", VisualConfiguration::getDefault());
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.containsIgnoreCase("not found"));
}

TEST_F(VisualPresetManagerTest, CannotUpdateSystemPreset)
{
    manager_->initialize();

    // Get a system preset
    auto systemPreset = manager_->getPresetByName("Default");
    EXPECT_TRUE(systemPreset.has_value());

    // Try to update it
    auto result = manager_->updatePreset(systemPreset->id, VisualConfiguration::getDefault());
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.containsIgnoreCase("read-only") || result.error.containsIgnoreCase("system"));
}

TEST_F(VisualPresetManagerTest, RenamePreset)
{
    manager_->initialize();

    juce::String originalName = uniqueName("Original Name");
    juce::String newName = uniqueName("New Name");
    auto createResult = manager_->createPreset(originalName, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    auto renameResult = manager_->renamePreset(createResult.value.id, newName);
    EXPECT_TRUE(renameResult.success);

    auto renamed = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(renamed.has_value());
    EXPECT_EQ(renamed->name, newName);
}

TEST_F(VisualPresetManagerTest, RenameToInvalidNameFails)
{
    manager_->initialize();

    juce::String name = uniqueName("Valid Name");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Try to rename to invalid name
    auto result = manager_->renamePreset(createResult.value.id, "");
    EXPECT_FALSE(result.success);
}

TEST_F(VisualPresetManagerTest, DeletePreset)
{
    manager_->initialize();

    juce::String name = uniqueName("Delete Me");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    // Don't track for cleanup since we're deleting it

    int countBefore = manager_->getPresetCount();

    auto deleteResult = manager_->deletePreset(createResult.value.id);
    EXPECT_TRUE(deleteResult.success);

    EXPECT_EQ(manager_->getPresetCount(), countBefore - 1);

    auto notFound = manager_->getPresetById(createResult.value.id);
    EXPECT_FALSE(notFound.has_value());
}

TEST_F(VisualPresetManagerTest, CannotDeleteSystemPreset)
{
    manager_->initialize();

    auto systemPreset = manager_->getPresetByName("Default");
    EXPECT_TRUE(systemPreset.has_value());

    auto result = manager_->deletePreset(systemPreset->id);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.containsIgnoreCase("read-only") || result.error.containsIgnoreCase("system"));

    // Should still exist
    auto stillThere = manager_->getPresetByName("Default");
    EXPECT_TRUE(stillThere.has_value());
}

TEST_F(VisualPresetManagerTest, DuplicatePreset)
{
    manager_->initialize();

    // Create original
    VisualConfiguration config = VisualConfiguration::getDefault();
    config.bloom.enabled = true;
    config.bloom.intensity = 1.5f;

    juce::String originalName = uniqueName("Original");
    juce::String copyName = uniqueName("Original Copy");
    auto createResult = manager_->createPreset(originalName, config);
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Duplicate
    auto dupResult = manager_->duplicatePreset(createResult.value.id, copyName);
    EXPECT_TRUE(dupResult.success) << "Failed to duplicate: " << dupResult.error.toStdString();
    if (dupResult.success) createdPresetIds_.push_back(dupResult.value.id);
    EXPECT_NE(dupResult.value.id, createResult.value.id);
    EXPECT_EQ(dupResult.value.name, copyName);
    EXPECT_EQ(dupResult.value.category, PresetCategory::User);
    EXPECT_FALSE(dupResult.value.isReadOnly);

    // Configuration should be copied
    EXPECT_TRUE(dupResult.value.configuration.bloom.enabled);
    EXPECT_FLOAT_EQ(dupResult.value.configuration.bloom.intensity, 1.5f);
}

TEST_F(VisualPresetManagerTest, DuplicateSystemPreset)
{
    manager_->initialize();

    auto systemPreset = manager_->getPresetByName("Default");
    EXPECT_TRUE(systemPreset.has_value());

    // Can duplicate a system preset (creates a user copy)
    juce::String copyName = uniqueName("My Default Copy");
    auto dupResult = manager_->duplicatePreset(systemPreset->id, copyName);
    EXPECT_TRUE(dupResult.success) << "Failed to duplicate: " << dupResult.error.toStdString();
    if (dupResult.success) createdPresetIds_.push_back(dupResult.value.id);
    EXPECT_EQ(dupResult.value.category, PresetCategory::User);
    EXPECT_FALSE(dupResult.value.isReadOnly);
}

// =============================================================================
// Favorites Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, SetFavorite)
{
    manager_->initialize();

    juce::String name = uniqueName("Favorite Me");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);
    EXPECT_FALSE(createResult.value.isFavorite);

    // Mark as favorite
    auto favResult = manager_->setFavorite(createResult.value.id, true);
    EXPECT_TRUE(favResult.success);

    auto preset = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(preset.has_value());
    EXPECT_TRUE(preset->isFavorite);

    // Unmark as favorite
    auto unfavResult = manager_->setFavorite(createResult.value.id, false);
    EXPECT_TRUE(unfavResult.success);

    preset = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(preset.has_value());
    EXPECT_FALSE(preset->isFavorite);
}

TEST_F(VisualPresetManagerTest, GetFavoritePresets)
{
    manager_->initialize();

    // Create multiple presets
    auto p1 = manager_->createPreset(uniqueName("Preset 1"), VisualConfiguration::getDefault());
    auto p2 = manager_->createPreset(uniqueName("Preset 2"), VisualConfiguration::getDefault());
    auto p3 = manager_->createPreset(uniqueName("Preset 3"), VisualConfiguration::getDefault());

    EXPECT_TRUE(p1.success) << "Failed: " << p1.error.toStdString();
    EXPECT_TRUE(p2.success) << "Failed: " << p2.error.toStdString();
    EXPECT_TRUE(p3.success) << "Failed: " << p3.error.toStdString();

    if (p1.success) createdPresetIds_.push_back(p1.value.id);
    if (p2.success) createdPresetIds_.push_back(p2.value.id);
    if (p3.success) createdPresetIds_.push_back(p3.value.id);

    // Mark some as favorites
    manager_->setFavorite(p1.value.id, true);
    manager_->setFavorite(p3.value.id, true);

    auto favorites = manager_->getFavoritePresets();
    EXPECT_GE(favorites.size(), 2u);

    // All returned presets should be favorites
    for (const auto& fav : favorites)
    {
        EXPECT_TRUE(fav.isFavorite);
    }
}

// =============================================================================
// Name Uniqueness Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, IsNameUnique)
{
    manager_->initialize();

    // Create a preset with unique name
    juce::String name = uniqueName("Unique Name");
    auto createResult = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed: " << createResult.error.toStdString();
    if (createResult.success) createdPresetIds_.push_back(createResult.value.id);

    // Same name should not be unique within User category
    EXPECT_FALSE(manager_->isNameUnique(name, PresetCategory::User));

    // Excluding the preset itself should make it unique (for rename)
    EXPECT_TRUE(manager_->isNameUnique(name, PresetCategory::User, createResult.value.id));

    // Different name should be unique
    EXPECT_TRUE(manager_->isNameUnique(uniqueName("Different Name"), PresetCategory::User));
}

TEST_F(VisualPresetManagerTest, CreateDuplicateNameFails)
{
    manager_->initialize();

    juce::String name = uniqueName("Same Name");
    auto result1 = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_TRUE(result1.success) << "Failed: " << result1.error.toStdString();
    if (result1.success) createdPresetIds_.push_back(result1.value.id);

    // Try to create another with same name
    auto result2 = manager_->createPreset(name, VisualConfiguration::getDefault());
    EXPECT_FALSE(result2.success);
    EXPECT_TRUE(result2.error.containsIgnoreCase("exists") || result2.error.containsIgnoreCase("unique"));
}

// =============================================================================
// Configuration JSON Round-Trip Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, ConfigurationJsonRoundTrip)
{
    // Create a configuration with various settings enabled
    VisualConfiguration original;
    original.shaderType = ShaderType::NeonGlow;

    original.bloom.enabled = true;
    original.bloom.intensity = 1.5f;
    original.bloom.threshold = 0.7f;
    original.bloom.iterations = 5;

    original.trails.enabled = true;
    original.trails.decay = 0.2f;

    original.settings3D.enabled = true;
    original.settings3D.cameraDistance = 3.5f;
    original.settings3D.autoRotate = true;

    original.lighting.ambientR = 0.2f;
    original.lighting.ambientG = 0.2f;
    original.lighting.ambientB = 0.2f;
    original.lighting.specularIntensity = 0.3f;

    // Serialize and deserialize
    juce::var json = original.toJson();
    VisualConfiguration restored = VisualConfiguration::fromJson(json);

    // Verify all settings are preserved
    EXPECT_EQ(restored.shaderType, ShaderType::NeonGlow);

    EXPECT_TRUE(restored.bloom.enabled);
    EXPECT_FLOAT_EQ(restored.bloom.intensity, 1.5f);
    EXPECT_FLOAT_EQ(restored.bloom.threshold, 0.7f);
    EXPECT_EQ(restored.bloom.iterations, 5);

    EXPECT_TRUE(restored.trails.enabled);
    EXPECT_FLOAT_EQ(restored.trails.decay, 0.2f);

    EXPECT_TRUE(restored.settings3D.enabled);
    EXPECT_FLOAT_EQ(restored.settings3D.cameraDistance, 3.5f);
    EXPECT_TRUE(restored.settings3D.autoRotate);

    EXPECT_FLOAT_EQ(restored.lighting.ambientR, 0.2f);
    EXPECT_FLOAT_EQ(restored.lighting.ambientG, 0.2f);
    EXPECT_FLOAT_EQ(restored.lighting.ambientB, 0.2f);
    EXPECT_FLOAT_EQ(restored.lighting.specularIntensity, 0.3f);
}

TEST_F(VisualPresetManagerTest, AllPostProcessingEffectsRoundTrip)
{
    VisualConfiguration original;

    // Enable all post-processing effects with non-default values
    original.bloom.enabled = true;
    original.radialBlur.enabled = true;
    original.radialBlur.amount = 0.25f;
    original.trails.enabled = true;
    original.colorGrade.enabled = true;
    original.colorGrade.saturation = 1.5f;
    original.vignette.enabled = true;
    original.vignette.intensity = 0.7f;
    original.filmGrain.enabled = true;
    original.filmGrain.intensity = 0.2f;
    original.chromaticAberration.enabled = true;
    original.chromaticAberration.intensity = 0.01f;
    original.scanlines.enabled = true;
    original.scanlines.intensity = 0.4f;
    original.distortion.enabled = true;
    original.distortion.intensity = 0.1f;
    original.tiltShift.enabled = true;
    original.tiltShift.blurRadius = 3.0f;

    juce::var json = original.toJson();
    VisualConfiguration restored = VisualConfiguration::fromJson(json);

    EXPECT_TRUE(restored.bloom.enabled);
    EXPECT_TRUE(restored.radialBlur.enabled);
    EXPECT_FLOAT_EQ(restored.radialBlur.amount, 0.25f);
    EXPECT_TRUE(restored.trails.enabled);
    EXPECT_TRUE(restored.colorGrade.enabled);
    EXPECT_FLOAT_EQ(restored.colorGrade.saturation, 1.5f);
    EXPECT_TRUE(restored.vignette.enabled);
    EXPECT_FLOAT_EQ(restored.vignette.intensity, 0.7f);
    EXPECT_TRUE(restored.filmGrain.enabled);
    EXPECT_FLOAT_EQ(restored.filmGrain.intensity, 0.2f);
    EXPECT_TRUE(restored.chromaticAberration.enabled);
    EXPECT_FLOAT_EQ(restored.chromaticAberration.intensity, 0.01f);
    EXPECT_TRUE(restored.scanlines.enabled);
    EXPECT_FLOAT_EQ(restored.scanlines.intensity, 0.4f);
    EXPECT_TRUE(restored.distortion.enabled);
    EXPECT_FLOAT_EQ(restored.distortion.intensity, 0.1f);
    EXPECT_TRUE(restored.tiltShift.enabled);
    EXPECT_FLOAT_EQ(restored.tiltShift.blurRadius, 3.0f);
}

// =============================================================================
// Listener Tests
// =============================================================================

class MockPresetListener : public VisualPresetManager::Listener
{
public:
    int addedCount = 0;
    int updatedCount = 0;
    int removedCount = 0;
    int reloadedCount = 0;
    juce::String lastAddedId;
    juce::String lastRemovedId;

    void presetAdded(const VisualPreset& preset) override
    {
        ++addedCount;
        lastAddedId = preset.id;
    }

    void presetUpdated(const VisualPreset& /*preset*/) override
    {
        ++updatedCount;
    }

    void presetRemoved(const juce::String& presetId) override
    {
        ++removedCount;
        lastRemovedId = presetId;
    }

    void presetsReloaded() override
    {
        ++reloadedCount;
    }
};

TEST_F(VisualPresetManagerTest, ListenerNotifiedOnCreate)
{
    manager_->initialize();

    MockPresetListener listener;
    manager_->addListener(&listener);

    // Use unique name with UUID to avoid collisions with other tests
    juce::String uniqueName = "Listener Test " + juce::Uuid().toString().substring(0, 8);
    auto result = manager_->createPreset(uniqueName, VisualConfiguration::getDefault());
    EXPECT_TRUE(result.success) << "Failed to create preset: " << result.error.toStdString();

    EXPECT_EQ(listener.addedCount, 1);
    EXPECT_EQ(listener.lastAddedId, result.value.id);

    manager_->removeListener(&listener);
}

TEST_F(VisualPresetManagerTest, ListenerNotifiedOnUpdate)
{
    manager_->initialize();

    juce::String uniqueName = "Update Listener " + juce::Uuid().toString().substring(0, 8);
    auto createResult = manager_->createPreset(uniqueName, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed to create preset: " << createResult.error.toStdString();

    MockPresetListener listener;
    manager_->addListener(&listener);

    manager_->updatePreset(createResult.value.id, VisualConfiguration::getDefault());
    EXPECT_EQ(listener.updatedCount, 1);

    manager_->removeListener(&listener);
}

TEST_F(VisualPresetManagerTest, ListenerNotifiedOnDelete)
{
    manager_->initialize();

    juce::String uniqueName = "Delete Listener " + juce::Uuid().toString().substring(0, 8);
    auto createResult = manager_->createPreset(uniqueName, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed to create preset: " << createResult.error.toStdString();

    MockPresetListener listener;
    manager_->addListener(&listener);

    manager_->deletePreset(createResult.value.id);
    EXPECT_EQ(listener.removedCount, 1);
    EXPECT_EQ(listener.lastRemovedId, createResult.value.id);

    manager_->removeListener(&listener);
}

TEST_F(VisualPresetManagerTest, RemovedListenerNotNotified)
{
    manager_->initialize();

    MockPresetListener listener;
    manager_->addListener(&listener);
    manager_->removeListener(&listener);

    juce::String uniqueName = "After Remove " + juce::Uuid().toString().substring(0, 8);
    manager_->createPreset(uniqueName, VisualConfiguration::getDefault());
    EXPECT_EQ(listener.addedCount, 0);
}

// =============================================================================
// Preset Count Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, GetPresetCount)
{
    manager_->initialize();

    int initialCount = manager_->getPresetCount();
    EXPECT_GT(initialCount, 0); // System presets exist

    juce::String uuid = juce::Uuid().toString().substring(0, 8);
    auto result1 = manager_->createPreset("Count Test 1 " + uuid, VisualConfiguration::getDefault());
    EXPECT_TRUE(result1.success) << "Failed to create preset: " << result1.error.toStdString();
    EXPECT_EQ(manager_->getPresetCount(), initialCount + 1);

    auto result2 = manager_->createPreset("Count Test 2 " + uuid, VisualConfiguration::getDefault());
    EXPECT_TRUE(result2.success) << "Failed to create preset: " << result2.error.toStdString();
    EXPECT_EQ(manager_->getPresetCount(), initialCount + 2);

    manager_->deletePreset(result2.value.id);
    EXPECT_EQ(manager_->getPresetCount(), initialCount + 1);

    // Clean up
    manager_->deletePreset(result1.value.id);
}

// =============================================================================
// Set Description Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, SetDescription)
{
    manager_->initialize();

    juce::String uniqueName = "Describe Me " + juce::Uuid().toString().substring(0, 8);
    auto createResult = manager_->createPreset(uniqueName, VisualConfiguration::getDefault());
    EXPECT_TRUE(createResult.success) << "Failed to create preset: " << createResult.error.toStdString();
    EXPECT_TRUE(createResult.value.description.isEmpty());

    auto descResult = manager_->setDescription(createResult.value.id, "A new description");
    EXPECT_TRUE(descResult.success) << "Failed to set description: " << descResult.error.toStdString();

    auto preset = manager_->getPresetById(createResult.value.id);
    EXPECT_TRUE(preset.has_value());
    if (preset.has_value())
    {
        EXPECT_EQ(preset->description, "A new description");
    }

    // Clean up
    manager_->deletePreset(createResult.value.id);
}

// =============================================================================
// Directory Path Tests
// =============================================================================

TEST_F(VisualPresetManagerTest, GetPresetsDirectory)
{
    juce::File presetsDir = VisualPresetManager::getPresetsDirectory();
    EXPECT_FALSE(presetsDir.getFullPathName().isEmpty());

#if JUCE_MAC
    EXPECT_TRUE(presetsDir.getFullPathName().contains("MultiScoper"));
#elif JUCE_WINDOWS
    EXPECT_TRUE(presetsDir.getFullPathName().contains("MultiScoper"));
#elif JUCE_LINUX
    EXPECT_TRUE(presetsDir.getFullPathName().contains("MultiScoper") ||
                presetsDir.getFullPathName().contains("multiscoper"));
#endif
}

TEST_F(VisualPresetManagerTest, SystemAndUserDirectoriesDifferent)
{
    juce::File systemDir = VisualPresetManager::getSystemPresetsDirectory();
    juce::File userDir = VisualPresetManager::getUserPresetsDirectory();

    // Directories should have different paths (system vs user subdirs)
    EXPECT_NE(systemDir.getFullPathName(), userDir.getFullPathName());
}
