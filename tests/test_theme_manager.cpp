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
