/*
    Oscil - Theme Manager Persistence Tests
    Tests for theme import/export, JSON, and ValueTree serialization
*/

#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

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
// XML Import/Export Tests (format is XML despite historical "json" naming)
// =============================================================================

// Test: Import invalid XML
TEST_F(ThemeManagerPersistenceTest, ImportInvalidXml)
{
    bool result = getThemeManager().importTheme("not valid xml");
    EXPECT_FALSE(result);
}

// Test: Import empty string
TEST_F(ThemeManagerPersistenceTest, ImportEmptyString)
{
    bool result = getThemeManager().importTheme("");
    EXPECT_FALSE(result);
}

// Test: Import non-XML content is rejected
TEST_F(ThemeManagerPersistenceTest, ImportNonXmlContent)
{
    bool result = getThemeManager().importTheme("{}");
    EXPECT_FALSE(result);
}

// Test: Import cannot overwrite a protected system theme
TEST_F(ThemeManagerPersistenceTest, ImportSystemThemeNameRejected)
{
    ColorTheme t;
    t.name = "Dark Professional"; // built-in system theme
    t.backgroundPrimary = juce::Colour(0xFF123456);
    auto xml = t.toXmlString();

    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Import trims whitespace before checking system theme names
TEST_F(ThemeManagerPersistenceTest, ImportTrimmedSystemThemeNameRejected)
{
    ColorTheme t;
    t.name = "  Dark Professional  "; // whitespace-padded system theme name
    t.backgroundPrimary = juce::Colour(0xFF123456);
    auto xml = t.toXmlString();

    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Import rejects unsafe filenames
TEST_F(ThemeManagerPersistenceTest, ImportUnsafeNameRejected)
{
    ColorTheme t;
    t.name = "../../../etc/evil";
    auto xml = t.toXmlString();
    EXPECT_FALSE(getThemeManager().importTheme(xml));

    t.name = "theme:with*bad|chars";
    xml = t.toXmlString();
    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Export non-existent theme
TEST_F(ThemeManagerPersistenceTest, ExportNonexistentTheme)
{
    juce::String exported = getThemeManager().exportTheme("NonExistentTheme");
    EXPECT_TRUE(exported.isEmpty());
}

// Test: Export then import roundtrip verifies data survives serialization
TEST_F(ThemeManagerPersistenceTest, ExportImportRoundtrip)
{
    EXPECT_TRUE(getThemeManager().createTheme("RoundtripTest"));
    juce::String exported = getThemeManager().exportTheme("RoundtripTest");
    EXPECT_FALSE(exported.isEmpty());

    // Import the exported theme — should succeed and theme should exist
    bool imported = getThemeManager().importTheme(exported);
    EXPECT_TRUE(imported);

    // Verify the theme is accessible after import
    juce::String exportedAgain = getThemeManager().exportTheme("RoundtripTest");
    EXPECT_FALSE(exportedAgain.isEmpty());
    EXPECT_TRUE(exportedAgain.contains("RoundtripTest"));

    getThemeManager().deleteTheme("RoundtripTest");
}

// Test: ColorTheme XML export/import with valid data
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromXmlStringValid)
{
    ColorTheme original;
    original.name = "XmlTest";
    original.backgroundPrimary = juce::Colour(0xFF123456);
    original.textPrimary = juce::Colour(0xFFABCDEF);

    juce::String xmlStr = original.toXmlString();

    ColorTheme restored;
    bool result = restored.fromXmlString(xmlStr);

    EXPECT_TRUE(result);
    EXPECT_EQ(restored.name, "XmlTest");
    EXPECT_EQ(restored.backgroundPrimary.getARGB(), 0xFF123456u);
}

// Test: ColorTheme XML import with invalid data
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromXmlStringInvalid)
{
    ColorTheme theme;
    bool result = theme.fromXmlString("invalid xml data");

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

    // After loading empty tree, theme should retain valid defaults.
    EXPECT_FALSE(theme.backgroundPrimary.isTransparent());
    EXPECT_FALSE(theme.textPrimary.isTransparent());
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
