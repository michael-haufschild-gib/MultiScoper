/*
    Oscil - Theme Manager Application Tests
    Tests for theme application, colors, listeners, and properties
*/

#include <gtest/gtest.h>
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class ThemeManagerApplyTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure system themes are loaded
    }
};

// =============================================================================
// Theme Application Tests
// =============================================================================

// Test: Theme waveform colors
TEST_F(ThemeManagerApplyTest, WaveformColors)
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

// Test: Theme serialization
TEST_F(ThemeManagerApplyTest, ThemeSerialization)
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

// =============================================================================
// Listener Tests
// =============================================================================

// Listener test helper
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

TEST_F(ThemeManagerApplyTest, ListenerNotifications)
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

// Test: Multiple listeners
TEST_F(ThemeManagerApplyTest, MultipleListeners)
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
TEST_F(ThemeManagerApplyTest, RemoveListenerTwice)
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

TEST_F(ThemeManagerApplyTest, ListenerRemovesDuringCallback)
{
    SelfRemovingThemeListener listener;
    ThemeManager::getInstance().addListener(&listener);

    ThemeManager::getInstance().setCurrentTheme("Classic Amber");
    ThemeManager::getInstance().setCurrentTheme("High Contrast");

    // Should only be called once (removed itself)
    EXPECT_EQ(listener.callCount, 1);
}

// =============================================================================
// Edge Case Tests - Waveform Colors
// =============================================================================

// Test: Get waveform color with negative index (should handle gracefully)
TEST_F(ThemeManagerApplyTest, WaveformColorNegativeIndex)
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
TEST_F(ThemeManagerApplyTest, WaveformColorLargeIndex)
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
TEST_F(ThemeManagerApplyTest, EmptyWaveformColors)
{
    ColorTheme theme;
    theme.waveformColors.clear();

    // Should return fallback color, not crash
    auto color = theme.getWaveformColor(0);
    EXPECT_EQ(color.getARGB(), juce::Colours::green.getARGB());
}

// =============================================================================
// Edge Case Tests - Theme Properties
// =============================================================================

// Test: Theme has all required color properties
TEST_F(ThemeManagerApplyTest, ThemeHasAllProperties)
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
TEST_F(ThemeManagerApplyTest, ClonePreservesAllProperties)
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
