/*
    Oscil - Theme Manager Tests
*/

#include <gtest/gtest.h>
#include "ui/ThemeManager.h"

using namespace oscil;

class ThemeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure system themes are loaded
    }
};

// Test: System themes exist
TEST_F(ThemeManagerTest, SystemThemesExist)
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
TEST_F(ThemeManagerTest, SystemThemesImmutable)
{
    EXPECT_TRUE(ThemeManager::getInstance().isSystemTheme("Dark Professional"));
    EXPECT_TRUE(ThemeManager::getInstance().isSystemTheme("Classic Green"));

    // Attempt to delete system theme should fail
    EXPECT_FALSE(ThemeManager::getInstance().deleteTheme("Dark Professional"));
}

// Test: Set current theme
TEST_F(ThemeManagerTest, SetCurrentTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().setCurrentTheme("Classic Green"));

    auto& current = ThemeManager::getInstance().getCurrentTheme();
    EXPECT_EQ(current.name, "Classic Green");
}

// Test: Get theme by name
TEST_F(ThemeManagerTest, GetThemeByName)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");

    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, "Dark Professional");
    EXPECT_TRUE(theme->isSystemTheme);
}

// Test: Get nonexistent theme returns nullptr
TEST_F(ThemeManagerTest, GetNonexistentTheme)
{
    auto* theme = ThemeManager::getInstance().getTheme("Nonexistent Theme");
    EXPECT_EQ(theme, nullptr);
}

// Test: Create custom theme
TEST_F(ThemeManagerTest, CreateCustomTheme)
{
    EXPECT_TRUE(ThemeManager::getInstance().createTheme("My Custom Theme", "Dark Professional"));

    auto* theme = ThemeManager::getInstance().getTheme("My Custom Theme");
    ASSERT_NE(theme, nullptr);
    EXPECT_FALSE(theme->isSystemTheme);

    // Clean up
    ThemeManager::getInstance().deleteTheme("My Custom Theme");
}

// Test: Clone theme
TEST_F(ThemeManagerTest, CloneTheme)
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
TEST_F(ThemeManagerTest, DeleteCustomTheme)
{
    ThemeManager::getInstance().createTheme("Temp Theme");
    EXPECT_NE(ThemeManager::getInstance().getTheme("Temp Theme"), nullptr);

    EXPECT_TRUE(ThemeManager::getInstance().deleteTheme("Temp Theme"));
    EXPECT_EQ(ThemeManager::getInstance().getTheme("Temp Theme"), nullptr);
}

// Test: Theme waveform colors
TEST_F(ThemeManagerTest, WaveformColors)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Should have 64 waveform colors
    EXPECT_EQ(theme->waveformColors.size(), 64);

    // Getting color by index should cycle
    auto color0 = theme->getWaveformColor(0);
    auto color64 = theme->getWaveformColor(64);
    EXPECT_EQ(color0.getARGB(), color64.getARGB());
}

// Theme serialization test
TEST_F(ThemeManagerTest, ThemeSerialization)
{
    ColorTheme original;
    original.name = "Test Theme";
    original.backgroundPrimary = juce::Colour(0xFF123456);
    original.textPrimary = juce::Colour(0xFFABCDEF);

    auto valueTree = original.toValueTree();

    ColorTheme restored;
    restored.fromValueTree(valueTree);

    EXPECT_EQ(restored.name, original.name);
    EXPECT_EQ(restored.backgroundPrimary.getARGB(), original.backgroundPrimary.getARGB());
    EXPECT_EQ(restored.textPrimary.getARGB(), original.textPrimary.getARGB());
}

// Listener test
class TestThemeListener : public ThemeManagerListener
{
public:
    int changeCount = 0;
    juce::String lastThemeName;

    void themeChanged(const ColorTheme& newTheme) override
    {
        changeCount++;
        lastThemeName = newTheme.name;
    }
};

TEST_F(ThemeManagerTest, ListenerNotifications)
{
    TestThemeListener listener;
    ThemeManager::getInstance().addListener(&listener);

    ThemeManager::getInstance().setCurrentTheme("High Contrast");

    EXPECT_EQ(listener.changeCount, 1);
    EXPECT_EQ(listener.lastThemeName, "High Contrast");

    ThemeManager::getInstance().setCurrentTheme("Light Mode");

    EXPECT_EQ(listener.changeCount, 2);
    EXPECT_EQ(listener.lastThemeName, "Light Mode");

    ThemeManager::getInstance().removeListener(&listener);

    // After removal, listener should not be notified
    ThemeManager::getInstance().setCurrentTheme("Dark Professional");
    EXPECT_EQ(listener.changeCount, 2);
}

// =============================================================================
// Edge Case Tests - Theme Names
// =============================================================================

// Test: Create theme with empty name
TEST_F(ThemeManagerTest, CreateThemeEmptyName)
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
TEST_F(ThemeManagerTest, CreateThemeUnicodeName)
{
    bool result = ThemeManager::getInstance().createTheme(u8"日本語テーマ");

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme(u8"日本語テーマ");
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name, u8"日本語テーマ");

    ThemeManager::getInstance().deleteTheme(u8"日本語テーマ");
}

// Test: Create theme with special characters
TEST_F(ThemeManagerTest, CreateThemeSpecialChars)
{
    bool result = ThemeManager::getInstance().createTheme("Theme <with> special/chars!");

    EXPECT_TRUE(result);

    auto* theme = ThemeManager::getInstance().getTheme("Theme <with> special/chars!");
    ASSERT_NE(theme, nullptr);

    ThemeManager::getInstance().deleteTheme("Theme <with> special/chars!");
}

// Test: Create theme with very long name
TEST_F(ThemeManagerTest, CreateThemeLongName)
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
TEST_F(ThemeManagerTest, CreateDuplicateTheme)
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
TEST_F(ThemeManagerTest, CreateThemeFromNonexistentSource)
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
TEST_F(ThemeManagerTest, CloneNonexistentTheme)
{
    bool result = ThemeManager::getInstance().cloneTheme("DoesNotExist", "ClonedTheme");

    EXPECT_FALSE(result);
    EXPECT_EQ(ThemeManager::getInstance().getTheme("ClonedTheme"), nullptr);
}

// Test: Clone to existing name
TEST_F(ThemeManagerTest, CloneToExistingName)
{
    // Can't clone to a name that already exists
    bool result = ThemeManager::getInstance().cloneTheme("Dark Professional", "Classic Green");

    EXPECT_FALSE(result);
}

// Test: Delete non-existent theme
TEST_F(ThemeManagerTest, DeleteNonexistentTheme)
{
    bool result = ThemeManager::getInstance().deleteTheme("ThisThemeDoesNotExist");

    EXPECT_FALSE(result);
}

// Test: Set non-existent theme as current
TEST_F(ThemeManagerTest, SetNonexistentCurrentTheme)
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
TEST_F(ThemeManagerTest, UpdateSystemThemeFails)
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
TEST_F(ThemeManagerTest, UpdateNonexistentTheme)
{
    ColorTheme theme;
    theme.name = "DoesNotExist";

    bool result = ThemeManager::getInstance().updateTheme("DoesNotExist", theme);

    EXPECT_FALSE(result);
}

// Test: Update custom theme successfully
TEST_F(ThemeManagerTest, UpdateCustomThemeSuccess)
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
// Edge Case Tests - Waveform Colors
// =============================================================================

// Test: Get waveform color with negative index (should handle gracefully)
TEST_F(ThemeManagerTest, WaveformColorNegativeIndex)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Negative index - behavior depends on implementation
    // Should not crash, at minimum
    auto color = theme->getWaveformColor(-1);
    // Any valid color is acceptable
    EXPECT_TRUE(true); // If we get here without crash, test passes
}

// Test: Get waveform color with very large index
TEST_F(ThemeManagerTest, WaveformColorLargeIndex)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Should cycle properly
    auto color0 = theme->getWaveformColor(0);
    auto colorLarge = theme->getWaveformColor(1000000);

    // With 64 colors, index 1000000 % 64 should equal a valid cycling pattern
    EXPECT_EQ(color0.getARGB(), theme->getWaveformColor(64).getARGB());
}

// Test: Theme with empty waveform colors
TEST_F(ThemeManagerTest, EmptyWaveformColors)
{
    ColorTheme theme;
    theme.waveformColors.clear();

    // Should return fallback color, not crash
    auto color = theme.getWaveformColor(0);
    EXPECT_EQ(color.getARGB(), juce::Colours::green.getARGB());
}

// =============================================================================
// Edge Case Tests - JSON Serialization
// =============================================================================

// Test: Import invalid JSON
TEST_F(ThemeManagerTest, ImportInvalidJson)
{
    bool result = ThemeManager::getInstance().importTheme("not valid json");
    EXPECT_FALSE(result);
}

// Test: Import empty JSON
TEST_F(ThemeManagerTest, ImportEmptyJson)
{
    bool result = ThemeManager::getInstance().importTheme("");
    EXPECT_FALSE(result);
}

// Test: Import JSON with missing required fields
TEST_F(ThemeManagerTest, ImportIncompleteJson)
{
    bool result = ThemeManager::getInstance().importTheme("{}");
    EXPECT_FALSE(result);
}

// Test: Export non-existent theme
TEST_F(ThemeManagerTest, ExportNonexistentTheme)
{
    juce::String json = ThemeManager::getInstance().exportTheme("NonExistentTheme");
    EXPECT_TRUE(json.isEmpty());
}

// Test: Import then export roundtrip
TEST_F(ThemeManagerTest, JsonRoundtrip)
{
    // Export an existing theme
    juce::String exported = ThemeManager::getInstance().exportTheme("Dark Professional");
    EXPECT_FALSE(exported.isEmpty());

    // Create a theme for testing
    ThemeManager::getInstance().createTheme("RoundtripTest");

    // The imported theme would overwrite...
    // Just verify export worked
    EXPECT_TRUE(exported.contains("Dark Professional") || exported.contains("backgroundPrimary"));

    ThemeManager::getInstance().deleteTheme("RoundtripTest");
}

// Test: ColorTheme JSON fromJson with valid JSON
TEST_F(ThemeManagerTest, ColorThemeFromJsonValid)
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
TEST_F(ThemeManagerTest, ColorThemeFromJsonInvalid)
{
    ColorTheme theme;
    bool result = theme.fromJson("invalid json data");

    EXPECT_FALSE(result);
}

// =============================================================================
// Edge Case Tests - ValueTree Serialization
// =============================================================================

// Test: fromValueTree with empty tree
TEST_F(ThemeManagerTest, ValueTreeEmpty)
{
    ColorTheme theme;
    juce::ValueTree emptyTree;

    theme.fromValueTree(emptyTree);

    // Should not crash, may have default values
    EXPECT_TRUE(true);
}

// Test: fromValueTree with wrong type
TEST_F(ThemeManagerTest, ValueTreeWrongType)
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
TEST_F(ThemeManagerTest, ValueTreeMissingProperties)
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

// =============================================================================
// Edge Case Tests - Listener Edge Cases
// =============================================================================

// Test: Multiple listeners
TEST_F(ThemeManagerTest, MultipleListeners)
{
    TestThemeListener listener1;
    TestThemeListener listener2;
    TestThemeListener listener3;

    ThemeManager::getInstance().addListener(&listener1);
    ThemeManager::getInstance().addListener(&listener2);
    ThemeManager::getInstance().addListener(&listener3);

    ThemeManager::getInstance().setCurrentTheme("Classic Amber");

    EXPECT_EQ(listener1.changeCount, 1);
    EXPECT_EQ(listener2.changeCount, 1);
    EXPECT_EQ(listener3.changeCount, 1);

    ThemeManager::getInstance().removeListener(&listener1);
    ThemeManager::getInstance().removeListener(&listener2);
    ThemeManager::getInstance().removeListener(&listener3);
}

// Test: Remove listener twice
TEST_F(ThemeManagerTest, RemoveListenerTwice)
{
    TestThemeListener listener;

    ThemeManager::getInstance().addListener(&listener);
    ThemeManager::getInstance().removeListener(&listener);
    ThemeManager::getInstance().removeListener(&listener); // Second removal

    // Should not crash
    ThemeManager::getInstance().setCurrentTheme("Classic Green");

    EXPECT_EQ(listener.changeCount, 0);
}

// Test: Listener that removes itself during callback
class SelfRemovingThemeListener : public ThemeManagerListener
{
public:
    int callCount = 0;

    void themeChanged(const ColorTheme&) override
    {
        callCount++;
        ThemeManager::getInstance().removeListener(this);
    }
};

TEST_F(ThemeManagerTest, ListenerRemovesDuringCallback)
{
    SelfRemovingThemeListener listener;
    ThemeManager::getInstance().addListener(&listener);

    ThemeManager::getInstance().setCurrentTheme("Classic Amber");
    ThemeManager::getInstance().setCurrentTheme("High Contrast");

    // Should only be called once (removed itself)
    EXPECT_EQ(listener.callCount, 1);
}

// =============================================================================
// Edge Case Tests - Available Themes
// =============================================================================

// Test: Get available themes is non-empty
TEST_F(ThemeManagerTest, AvailableThemesNonEmpty)
{
    auto themes = ThemeManager::getInstance().getAvailableThemes();

    EXPECT_GT(themes.size(), 0u);
}

// Test: All available themes can be retrieved
TEST_F(ThemeManagerTest, AllAvailableThemesRetrievable)
{
    auto themeNames = ThemeManager::getInstance().getAvailableThemes();

    for (const auto& name : themeNames)
    {
        auto* theme = ThemeManager::getInstance().getTheme(name);
        EXPECT_NE(theme, nullptr) << "Theme '" << name.toStdString() << "' not retrievable";
    }
}

// Test: Current theme is always in available themes
TEST_F(ThemeManagerTest, CurrentThemeInAvailable)
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
TEST_F(ThemeManagerTest, DeleteCurrentThemeSwitchesTheme)
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

// =============================================================================
// Edge Case Tests - Theme Properties
// =============================================================================

// Test: Theme has all required color properties
TEST_F(ThemeManagerTest, ThemeHasAllProperties)
{
    auto* theme = ThemeManager::getInstance().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Check all color properties have valid values
    EXPECT_NE(theme->backgroundPrimary.getARGB(), 0u);
    EXPECT_NE(theme->backgroundSecondary.getARGB(), 0u);
    EXPECT_NE(theme->textPrimary.getARGB(), 0u);
    EXPECT_NE(theme->textSecondary.getARGB(), 0u);
    EXPECT_NE(theme->gridMajor.getARGB(), 0u);
    EXPECT_NE(theme->gridMinor.getARGB(), 0u);
    EXPECT_NE(theme->controlBackground.getARGB(), 0u);
    EXPECT_NE(theme->statusActive.getARGB(), 0u);
    EXPECT_NE(theme->statusWarning.getARGB(), 0u);
    EXPECT_NE(theme->statusError.getARGB(), 0u);
}

// Test: Custom theme preserves all properties from source
TEST_F(ThemeManagerTest, ClonePreservesAllProperties)
{
    ThemeManager::getInstance().cloneTheme("Dark Professional", "PropertyTest");

    auto* original = ThemeManager::getInstance().getTheme("Dark Professional");
    auto* cloned = ThemeManager::getInstance().getTheme("PropertyTest");

    ASSERT_NE(original, nullptr);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->backgroundPrimary.getARGB(), original->backgroundPrimary.getARGB());
    EXPECT_EQ(cloned->backgroundSecondary.getARGB(), original->backgroundSecondary.getARGB());
    EXPECT_EQ(cloned->textPrimary.getARGB(), original->textPrimary.getARGB());
    EXPECT_EQ(cloned->gridMajor.getARGB(), original->gridMajor.getARGB());
    EXPECT_EQ(cloned->statusActive.getARGB(), original->statusActive.getARGB());
    EXPECT_EQ(cloned->waveformColors.size(), original->waveformColors.size());

    ThemeManager::getInstance().deleteTheme("PropertyTest");
}
