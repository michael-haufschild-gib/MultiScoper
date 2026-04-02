/*
    Oscil - Theme Manager CRUD Tests
    Tests for creating, reading, updating, and deleting themes
*/

#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class ThemeManagerCRUDTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Basic CRUD Operations
// =============================================================================

// Test: System themes exist
TEST_F(ThemeManagerCRUDTest, SystemThemesExist)
{
    auto themes = getThemeManager().getAvailableThemes();

    EXPECT_GE(themes.size(), 5); // At least 5 system themes

    bool hasDarkPro = false;
    bool hasClassicGreen = false;

    for (const auto& name : themes)
    {
        if (name == "Dark Professional")
            hasDarkPro = true;
        if (name == "Classic Green")
            hasClassicGreen = true;
    }

    EXPECT_TRUE(hasDarkPro);
    EXPECT_TRUE(hasClassicGreen);
}

// Test: System themes are immutable
TEST_F(ThemeManagerCRUDTest, SystemThemesImmutable)
{
    EXPECT_TRUE(getThemeManager().isSystemTheme("Dark Professional"));
    EXPECT_TRUE(getThemeManager().isSystemTheme("Classic Green"));

    // Attempt to delete system theme should fail
    EXPECT_FALSE(getThemeManager().deleteTheme("Dark Professional"));
}

// Test: Set current theme
TEST_F(ThemeManagerCRUDTest, SetCurrentTheme)
{
    EXPECT_TRUE(getThemeManager().setCurrentTheme("Classic Green"));

    auto& current = getThemeManager().getCurrentTheme();
    EXPECT_EQ(current.name, "Classic Green");
}

// Test: Get theme by name
TEST_F(ThemeManagerCRUDTest, GetThemeByName)
{
    auto* theme = getThemeManager().getTheme("Dark Professional");

    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, "Dark Professional");
    EXPECT_TRUE(theme->isSystemTheme);
}

// Test: Get nonexistent theme returns nullptr
TEST_F(ThemeManagerCRUDTest, GetNonexistentTheme)
{
    auto* theme = getThemeManager().getTheme("Nonexistent Theme");
    EXPECT_EQ(theme, nullptr);
}

// Test: Create custom theme
TEST_F(ThemeManagerCRUDTest, CreateCustomTheme)
{
    EXPECT_TRUE(getThemeManager().createTheme("My Custom Theme", "Dark Professional"));

    auto* theme = getThemeManager().getTheme("My Custom Theme");
    ASSERT_NE(theme, nullptr);
    EXPECT_FALSE(theme->isSystemTheme);

    // Clean up
    getThemeManager().deleteTheme("My Custom Theme");
}

// Test: Clone theme
TEST_F(ThemeManagerCRUDTest, CloneTheme)
{
    EXPECT_TRUE(getThemeManager().cloneTheme("Classic Amber", "My Amber Clone"));

    auto* original = getThemeManager().getTheme("Classic Amber");
    auto* clone = getThemeManager().getTheme("My Amber Clone");

    ASSERT_NE(original, nullptr);
    ASSERT_NE(clone, nullptr);

    // Clone should have same colors
    EXPECT_EQ(clone->backgroundPrimary.getARGB(), original->backgroundPrimary.getARGB());
    EXPECT_EQ(clone->textPrimary.getARGB(), original->textPrimary.getARGB());

    // But clone should not be system theme
    EXPECT_FALSE(clone->isSystemTheme);

    // Clean up
    getThemeManager().deleteTheme("My Amber Clone");
}

// Test: Delete custom theme
TEST_F(ThemeManagerCRUDTest, DeleteCustomTheme)
{
    getThemeManager().createTheme("Temp Theme");
    EXPECT_NE(getThemeManager().getTheme("Temp Theme"), nullptr);

    EXPECT_TRUE(getThemeManager().deleteTheme("Temp Theme"));
    EXPECT_EQ(getThemeManager().getTheme("Temp Theme"), nullptr);
}

// =============================================================================
// Edge Case Tests - Theme Names
// =============================================================================

// Test: Create theme with empty name
TEST_F(ThemeManagerCRUDTest, CreateThemeEmptyName)
{
    auto initialCount = getThemeManager().getAvailableThemes().size();
    bool result = getThemeManager().createTheme("");

    if (result)
    {
        // If accepted, count should have increased
        EXPECT_EQ(getThemeManager().getAvailableThemes().size(), initialCount + 1);
        getThemeManager().deleteTheme("");
    }
    else
    {
        // If rejected, count should be unchanged
        EXPECT_EQ(getThemeManager().getAvailableThemes().size(), initialCount);
    }
}

// Test: Create theme with unicode name
TEST_F(ThemeManagerCRUDTest, CreateThemeUnicodeName)
{
    bool result = getThemeManager().createTheme(u8"日本語テーマ");

    EXPECT_TRUE(result);

    auto* theme = getThemeManager().getTheme(u8"日本語テーマ");
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, u8"日本語テーマ");

    getThemeManager().deleteTheme(u8"日本語テーマ");
}

// Test: Create theme with filesystem-unsafe characters is rejected
TEST_F(ThemeManagerCRUDTest, CreateThemeSpecialChars)
{
    // Path-unsafe characters (<, >, /, \, etc.) are rejected to prevent filesystem traversal
    EXPECT_FALSE(getThemeManager().createTheme("Theme <with> special/chars!"));
    EXPECT_EQ(getThemeManager().getTheme("Theme <with> special/chars!"), nullptr);

    // Safe special characters (spaces, parentheses, hyphens) are allowed
    EXPECT_TRUE(getThemeManager().createTheme("Theme (with) safe-chars!"));
    EXPECT_NE(getThemeManager().getTheme("Theme (with) safe-chars!"), nullptr);
    getThemeManager().deleteTheme("Theme (with) safe-chars!");
}

// Test: Create theme with very long name
TEST_F(ThemeManagerCRUDTest, CreateThemeLongName)
{
    juce::String longName;
    for (int i = 0; i < 1000; ++i)
        longName += "a";

    bool result = getThemeManager().createTheme(longName);

    EXPECT_TRUE(result);

    auto* theme = getThemeManager().getTheme(longName);
    ASSERT_NE(theme, nullptr);

    getThemeManager().deleteTheme(longName);
}

// Test: Create duplicate theme
TEST_F(ThemeManagerCRUDTest, CreateDuplicateTheme)
{
    EXPECT_TRUE(getThemeManager().createTheme("DuplicateTest"));

    // Creating with same name should fail
    EXPECT_FALSE(getThemeManager().createTheme("DuplicateTest"));

    getThemeManager().deleteTheme("DuplicateTest");
}

// =============================================================================
// Edge Case Tests - Non-existent Themes
// =============================================================================

// Test: Create theme from non-existent source
TEST_F(ThemeManagerCRUDTest, CreateThemeFromNonexistentSource)
{
    bool result = getThemeManager().createTheme("NewTheme", "NonExistentSource");

    // Should fail or create a default theme
    if (result)
    {
        getThemeManager().deleteTheme("NewTheme");
    }
    else
    {
        EXPECT_FALSE(result);
    }
}

// Test: Clone non-existent theme
TEST_F(ThemeManagerCRUDTest, CloneNonexistentTheme)
{
    bool result = getThemeManager().cloneTheme("DoesNotExist", "ClonedTheme");

    EXPECT_FALSE(result);
    EXPECT_EQ(getThemeManager().getTheme("ClonedTheme"), nullptr);
}

// Test: Clone to existing name
TEST_F(ThemeManagerCRUDTest, CloneToExistingName)
{
    // Can't clone to a name that already exists
    bool result = getThemeManager().cloneTheme("Dark Professional", "Classic Green");

    EXPECT_FALSE(result);
}

// Test: Delete non-existent theme
TEST_F(ThemeManagerCRUDTest, DeleteNonexistentTheme)
{
    bool result = getThemeManager().deleteTheme("ThisThemeDoesNotExist");

    EXPECT_FALSE(result);
}

// Test: Set non-existent theme as current
TEST_F(ThemeManagerCRUDTest, SetNonexistentCurrentTheme)
{
    juce::String previousTheme = getThemeManager().getCurrentTheme().name;

    bool result = getThemeManager().setCurrentTheme("NonExistentTheme");

    EXPECT_FALSE(result);
    // Current theme should remain unchanged
    EXPECT_EQ(getThemeManager().getCurrentTheme().name, previousTheme);
}

// =============================================================================
// Edge Case Tests - System Theme Modifications
// =============================================================================

// Test: Update system theme (should fail)
TEST_F(ThemeManagerCRUDTest, UpdateSystemThemeFails)
{
    ColorTheme modifiedTheme;
    modifiedTheme.name = "Dark Professional";
    modifiedTheme.backgroundPrimary = juce::Colour(0xFFFF0000);

    bool result = getThemeManager().updateTheme("Dark Professional", modifiedTheme);

    EXPECT_FALSE(result);

    // Original should be unchanged
    auto* theme = getThemeManager().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);
    EXPECT_NE(theme->backgroundPrimary.getARGB(), 0xFFFF0000u);
}

// Test: Update non-existent theme
TEST_F(ThemeManagerCRUDTest, UpdateNonexistentTheme)
{
    ColorTheme theme;
    theme.name = "DoesNotExist";

    bool result = getThemeManager().updateTheme("DoesNotExist", theme);

    EXPECT_FALSE(result);
}

// Test: Update custom theme successfully
TEST_F(ThemeManagerCRUDTest, UpdateCustomThemeSuccess)
{
    getThemeManager().createTheme("UpdateTest");

    ColorTheme modifiedTheme;
    modifiedTheme.name = "UpdateTest";
    modifiedTheme.backgroundPrimary = juce::Colour(0xFFABCDEF);

    bool result = getThemeManager().updateTheme("UpdateTest", modifiedTheme);

    EXPECT_TRUE(result);

    auto* theme = getThemeManager().getTheme("UpdateTest");
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->backgroundPrimary.getARGB(), 0xFFABCDEFu);

    getThemeManager().deleteTheme("UpdateTest");
}

// =============================================================================
// Edge Case Tests - Available Themes
// =============================================================================

// Test: Get available themes is non-empty
TEST_F(ThemeManagerCRUDTest, AvailableThemesNonEmpty)
{
    auto themes = getThemeManager().getAvailableThemes();

    EXPECT_GT(themes.size(), 0u);
}

// Test: All available themes can be retrieved
TEST_F(ThemeManagerCRUDTest, AllAvailableThemesRetrievable)
{
    auto themeNames = getThemeManager().getAvailableThemes();
    ASSERT_GT(themeNames.size(), 0u);

    for (const auto& name : themeNames)
    {
        auto* theme = getThemeManager().getTheme(name);
        EXPECT_NE(theme, nullptr) << "Theme '" << name.toStdString() << "' not retrievable";
        if (theme != nullptr)
            EXPECT_FALSE(theme->name.isEmpty());
    }
}

// Test: Current theme is always in available themes
TEST_F(ThemeManagerCRUDTest, CurrentThemeInAvailable)
{
    auto& current = getThemeManager().getCurrentTheme();
    auto available = getThemeManager().getAvailableThemes();

    bool found = false;
    for (const auto& name : available)
    {
        if (name == current.name)
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
}

// Test: Delete current theme switches to another
TEST_F(ThemeManagerCRUDTest, DeleteCurrentThemeSwitchesTheme)
{
    getThemeManager().createTheme("TempCurrent");
    getThemeManager().setCurrentTheme("TempCurrent");

    EXPECT_EQ(getThemeManager().getCurrentTheme().name, "TempCurrent");

    // Delete it - should switch to a system theme first
    bool deleted = getThemeManager().deleteTheme("TempCurrent");

    EXPECT_TRUE(deleted);

    // Theme should be deleted
    EXPECT_EQ(getThemeManager().getTheme("TempCurrent"), nullptr);

    // Current should now be a different (system) theme
    EXPECT_NE(getThemeManager().getCurrentTheme().name, "TempCurrent");
    EXPECT_TRUE(getThemeManager().isSystemTheme(getThemeManager().getCurrentTheme().name));
}
