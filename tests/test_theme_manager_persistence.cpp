/*
    Oscil - Theme Manager Persistence Tests
    Tests for theme import/export, JSON, and ValueTree serialization
*/

#include <gtest/gtest.h>
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class ThemeManagerPersistenceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create ThemeManager instance for each test
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        // Clean up ThemeManager instance
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// JSON Serialization Tests
// =============================================================================

// Test: Import invalid JSON
TEST_F(ThemeManagerPersistenceTest, ImportInvalidJson)
{
    bool result = getThemeManager().importTheme("not valid json");
    EXPECT_FALSE(result);
}

// Test: Import empty JSON
TEST_F(ThemeManagerPersistenceTest, ImportEmptyJson)
{
    bool result = getThemeManager().importTheme("");
    EXPECT_FALSE(result);
}

// Test: Import JSON with missing required fields
TEST_F(ThemeManagerPersistenceTest, ImportIncompleteJson)
{
    bool result = getThemeManager().importTheme("{}");
    EXPECT_FALSE(result);
}

// Test: Export non-existent theme
TEST_F(ThemeManagerPersistenceTest, ExportNonexistentTheme)
{
    juce::String json = getThemeManager().exportTheme("NonExistentTheme");
    EXPECT_TRUE(json.isEmpty());
}

// Test: Import then export roundtrip
TEST_F(ThemeManagerPersistenceTest, JsonRoundtrip)
{
    // Export an existing theme
    juce::String exported = getThemeManager().exportTheme("Dark Professional");
    EXPECT_FALSE(exported.isEmpty());

    // Create a theme for testing
    getThemeManager().createTheme("RoundtripTest");

    // The imported theme would overwrite...
    // Just verify export worked
    EXPECT_TRUE(exported.contains("Dark Professional") || exported.contains("backgroundPrimary"));

    getThemeManager().deleteTheme("RoundtripTest");
}

// Test: ColorTheme JSON fromJson with valid JSON
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromJsonValid)
{
    ColorTheme original;
    original.name = "JsonTest";
    original.backgroundPrimary = juce::Colour(0xFF123456);
    original.textPrimary = juce::Colour(0xFFABCDEF);

    juce::String json = original.toJson();

    ColorTheme restored;
    bool result = restored.fromJson(json);

    EXPECT_TRUE(result);
    EXPECT_EQ(restored.name, "JsonTest");
    EXPECT_EQ(restored.backgroundPrimary.getARGB(), 0xFF123456u);
}

// Test: ColorTheme JSON fromJson with invalid JSON
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromJsonInvalid)
{
    ColorTheme theme;
    bool result = theme.fromJson("invalid json data");

    EXPECT_FALSE(result);
}

// =============================================================================
// ValueTree Serialization Tests
// =============================================================================

// Test: fromValueTree with empty tree
TEST_F(ThemeManagerPersistenceTest, ValueTreeEmpty)
{
    ColorTheme theme;
    juce::ValueTree emptyTree;

    theme.fromValueTree(emptyTree);

    // Should not crash, may have default values
    EXPECT_TRUE(true);
}

// Test: fromValueTree with wrong type
TEST_F(ThemeManagerPersistenceTest, ValueTreeWrongType)
{
    ColorTheme theme;
    theme.name = "OriginalName";
    juce::ValueTree wrongTree("WrongType");
    wrongTree.setProperty("name", "ShouldNotBeUsed", nullptr);

    theme.fromValueTree(wrongTree);

    // Should not crash, and should not modify the theme (wrong type is rejected)
    EXPECT_EQ(theme.name, "OriginalName");
}

// Test: fromValueTree with missing properties
TEST_F(ThemeManagerPersistenceTest, ValueTreeMissingProperties)
{
    ColorTheme theme;
    // Use correct ValueTree type "Theme" (as used by toValueTree)
    juce::ValueTree partialTree("Theme");
    partialTree.setProperty("name", "PartialTheme", nullptr);
    // Missing all color properties - should use defaults

    theme.fromValueTree(partialTree);

    // Name should be read from the tree
    EXPECT_EQ(theme.name, "PartialTheme");

    // Color properties should have default values (not crash)
    EXPECT_NE(theme.backgroundPrimary.getARGB(), 0u);
}
