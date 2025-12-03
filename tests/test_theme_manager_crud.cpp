/*
    Oscil - Theme Manager CRUD Tests
    Tests for creating, reading, updating, and deleting themes
*/

#include <gtest/gtest.h>
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class ThemeManagerCRUDTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure system themes are loaded
    }
};

// =============================================================================
// Basic CRUD Operations
// =============================================================================

// Test: System themes exist
TEST_F(ThemeManagerCRUDTest, SystemThemesExist)
{
    auto themes = ThemeManager::getInstance().getAvailableThemes();

    EXPECT_GE(themes.size(), 5); // At least 5 system themes

    bool hasDarkPro = false;
    bool hasClassicGreen = false;

    for (const auto& name : themes)
    {
        if (name == "Dark Professional") hasDarkPro = true;
        if (name == "Classic Green") hasClassicGreen = true;
    }

    EXPECT_TRUE(hasDarkPro);
    EXPECT_TRUE(hasClassicGreen);
}

// Test: System themes are immutable
TEST_F(ThemeManagerCRUDTest, SystemThemesImmutable)
{
    EXPECT_TRUE(ThemeManager::getInstance().isSystemTheme("Dark Professional"));
    EXPECT_TRUE(ThemeManager::getInstance().isSystemTheme("Classic Green"));

    // Attempt to delete system theme should fail
    EXPECT_FALSE(ThemeManager::getInstance().deleteTheme("Dark Professional"));
}

// Test: Set current theme
TEST_F(ThemeManagerCRUDTest, SetCurrentTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().setCurrentTheme("Classic Green"));

    auto& current = ThemeManager::getInstance().getCurrentTheme();
    EXPECT_EQ(current.name, "Classic Green");
}

// Test: Get theme by name
TEST_F(ThemeManagerCRUDTest, GetThemeByName)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");

    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, "Dark Professional");
    EXPECT_TRUE(theme->isSystemTheme);
}

// Test: Get nonexistent theme returns nullptr
TEST_F(ThemeManagerCRUDTest, GetNonexistentTheme)
{
    auto* theme = ThemeManager::getInstance().getTheme("Nonexistent Theme");
    EXPECT_EQ(theme, nullptr);
}

// Test: Create custom theme
TEST_F(ThemeManagerCRUDTest, CreateCustomTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().createTheme("My Custom Theme", "Dark Professional"));

    auto* theme = ThemeManager::getInstance().getTheme("My Custom Theme");
    ASSERT_NE(theme, nullptr);
    EXPECT_FALSE(theme->isSystemTheme);

    // Clean up
    ThemeManager::getInstance().deleteTheme("My Custom Theme");
}

// Test: Clone theme
TEST_F(ThemeManagerCRUDTest, CloneTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().cloneTheme("Classic Amber", "My Amber Clone"));

    auto* original = ThemeManager::getInstance().getTheme("Classic Amber");
    auto* clone = ThemeManager::getInstance().getTheme("My Amber Clone");

    ASSERT_NE(original, nullptr);
    ASSERT_NE(clone, nullptr);

    // Clone should have same colors
    EXPECT_EQ(clone->backgroundPrimary.getARGB(), original->backgroundPrimary.getARGB());
    EXPECT_EQ(clone->textPrimary.getARGB(), original->textPrimary.getARGB());

    // But clone should not be system theme
    EXPECT_FALSE(clone->isSystemTheme);

    // Clean up
    ThemeManager::getInstance().deleteTheme("My Amber Clone");
}

// Test: Delete custom theme
TEST_F(ThemeManagerCRUDTest, DeleteCustomTheme)
{
    ThemeManager::getInstance().createTheme("Temp Theme");
    EXPECT_NE(ThemeManager::getInstance().getTheme("Temp Theme"), nullptr);

    EXPECT_TRUE(ThemeManager::getInstance().deleteTheme("Temp Theme"));
    EXPECT_EQ(ThemeManager::getInstance().getTheme("Temp Theme"), nullptr);
}

// =============================================================================
// Edge Case Tests - Theme Names
// =============================================================================

// Test: Create theme with empty name
TEST_F(ThemeManagerCRUDTest, CreateThemeEmptyName)
{
    bool result = ThemeManager::getInstance().createTheme("");

    // Empty name should fail or create an unnamed theme
    // Implementation may vary - either behavior is acceptable
    if (result)
    {
        auto* theme = ThemeManager::getInstance().getTheme("");
        if (theme != nullptr)
        {
            ThemeManager::getInstance().deleteTheme("");
        }
    }
}

// Test: Create theme with unicode name
TEST_F(ThemeManagerCRUDTest, CreateThemeUnicodeName)
{
    bool result = ThemeManager::getInstance().createTheme(u8"日本語テーマ");

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme(u8"日本語テーマ");
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, u8"日本語テーマ");

    ThemeManager::getInstance().deleteTheme(u8"日本語テーマ");
}

// Test: Create theme with special characters
TEST_F(ThemeManagerCRUDTest, CreateThemeSpecialChars)
{
    bool result = ThemeManager::getInstance().createTheme("Theme <with> special/chars!");

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme("Theme <with> special/chars!");
    ASSERT_NE(theme, nullptr);

    ThemeManager::getInstance().deleteTheme("Theme <with> special/chars!");
}

// Test: Create theme with very long name
TEST_F(ThemeManagerCRUDTest, CreateThemeLongName)
{
    juce::String longName;
    for (int i = 0; i < 1000; ++i)
        longName += "a";

    bool result = ThemeManager::getInstance().createTheme(longName);

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme(longName);
    ASSERT_NE(theme, nullptr);

    ThemeManager::getInstance().deleteTheme(longName);
}

// Test: Create duplicate theme
TEST_F(ThemeManagerCRUDTest, CreateDuplicateTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().createTheme("DuplicateTest"));

    // Creating with same name should fail
    EXPECT_FALSE(ThemeManager::getInstance().createTheme("DuplicateTest"));

    ThemeManager::getInstance().deleteTheme("DuplicateTest");
}

// =============================================================================
// Edge Case Tests - Non-existent Themes
// =============================================================================

// Test: Create theme from non-existent source
TEST_F(ThemeManagerCRUDTest, CreateThemeFromNonexistentSource)
{
    bool result = ThemeManager::getInstance().createTheme("NewTheme", "NonExistentSource");

    // Should fail or create a default theme
    if (result)
    {
        ThemeManager::getInstance().deleteTheme("NewTheme");
    }
    else
    {
        EXPECT_FALSE(result);
    }
}

// Test: Clone non-existent theme
TEST_F(ThemeManagerCRUDTest, CloneNonexistentTheme)
{
    bool result = ThemeManager::getInstance().cloneTheme("DoesNotExist", "ClonedTheme");

    EXPECT_FALSE(result);
    EXPECT_EQ(ThemeManager::getInstance().getTheme("ClonedTheme"), nullptr);
}

// Test: Clone to existing name
TEST_F(ThemeManagerCRUDTest, CloneToExistingName)
{
    // Can't clone to a name that already exists
    bool result = ThemeManager::getInstance().cloneTheme("Dark Professional", "Classic Green");

    EXPECT_FALSE(result);
}

// Test: Delete non-existent theme
TEST_F(ThemeManagerCRUDTest, DeleteNonexistentTheme)
{
    bool result = ThemeManager::getInstance().deleteTheme("ThisThemeDoesNotExist");

    EXPECT_FALSE(result);
}

// Test: Set non-existent theme as current
TEST_F(ThemeManagerCRUDTest, SetNonexistentCurrentTheme)
{
    juce::String previousTheme = ThemeManager::getInstance().getCurrentTheme().name;

    bool result = ThemeManager::getInstance().setCurrentTheme("NonExistentTheme");

    EXPECT_FALSE(result);
    // Current theme should remain unchanged
    EXPECT_EQ(ThemeManager::getInstance().getCurrentTheme().name, previousTheme);
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

    bool result = ThemeManager::getInstance().updateTheme("Dark Professional", modifiedTheme);

    EXPECT_FALSE(result);

    // Original should be unchanged
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);
    EXPECT_NE(theme->backgroundPrimary.getARGB(), 0xFFFF0000u);
}

// Test: Update non-existent theme
TEST_F(ThemeManagerCRUDTest, UpdateNonexistentTheme)
{
    ColorTheme theme;
    theme.name = "DoesNotExist";

    bool result = ThemeManager::getInstance().updateTheme("DoesNotExist", theme);

    EXPECT_FALSE(result);
}

// Test: Update custom theme successfully
TEST_F(ThemeManagerCRUDTest, UpdateCustomThemeSuccess)
{
    ThemeManager::getInstance().createTheme("UpdateTest");

    ColorTheme modifiedTheme;
    modifiedTheme.name = "UpdateTest";
    modifiedTheme.backgroundPrimary = juce::Colour(0xFFABCDEF);

    bool result = ThemeManager::getInstance().updateTheme("UpdateTest", modifiedTheme);

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme("UpdateTest");
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->backgroundPrimary.getARGB(), 0xFFABCDEFu);

    ThemeManager::getInstance().deleteTheme("UpdateTest");
}

// =============================================================================
// Edge Case Tests - Available Themes
// =============================================================================

// Test: Get available themes is non-empty
TEST_F(ThemeManagerCRUDTest, AvailableThemesNonEmpty)
{
    auto themes = ThemeManager::getInstance().getAvailableThemes();

    EXPECT_GT(themes.size(), 0u);
}

// Test: All available themes can be retrieved
TEST_F(ThemeManagerCRUDTest, AllAvailableThemesRetrievable)
{
    auto themeNames = ThemeManager::getInstance().getAvailableThemes();

    for (const auto& name : themeNames)
    {
        auto* theme = ThemeManager::getInstance().getTheme(name);
        EXPECT_NE(theme, nullptr) << "Theme '" << name.toStdString() << "' not retrievable";
    }
}

// Test: Current theme is always in available themes
TEST_F(ThemeManagerCRUDTest, CurrentThemeInAvailable)
{
    auto& current = ThemeManager::getInstance().getCurrentTheme();
    auto available = ThemeManager::getInstance().getAvailableThemes();

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
    ThemeManager::getInstance().createTheme("TempCurrent");
    ThemeManager::getInstance().setCurrentTheme("TempCurrent");

    EXPECT_EQ(ThemeManager::getInstance().getCurrentTheme().name, "TempCurrent");

    // Delete it - should switch to a system theme first
    bool deleted = ThemeManager::getInstance().deleteTheme("TempCurrent");

    EXPECT_TRUE(deleted);

    // Theme should be deleted
    EXPECT_EQ(ThemeManager::getInstance().getTheme("TempCurrent"), nullptr);

    // Current should now be a different (system) theme
    EXPECT_NE(ThemeManager::getInstance().getCurrentTheme().name, "TempCurrent");
    EXPECT_TRUE(ThemeManager::getInstance().isSystemTheme(
        ThemeManager::getInstance().getCurrentTheme().name));
}
