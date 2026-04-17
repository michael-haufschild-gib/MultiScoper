/*
    Oscil - Theme Manager Application Tests
    Tests for theme application, colors, listeners, and properties
*/

#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class ThemeManagerApplyTest : public ::testing::Test
{
protected:
    std::unique_ptr<ThemeManager> themeManager_;

    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }
};

// =============================================================================
// Theme Application Tests
// =============================================================================

// Test: Theme waveform colors
TEST_F(ThemeManagerApplyTest, WaveformColors)
{
    auto* theme = getThemeManager().getTheme("Dark Professional");
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
    getThemeManager().addListener(&listener);

    getThemeManager().setCurrentTheme("High Contrast");

    EXPECT_EQ(listener.changeCount, 1);
    EXPECT_EQ(listener.lastThemeName, "High Contrast");

    getThemeManager().setCurrentTheme("Light Mode");

    EXPECT_EQ(listener.changeCount, 2);
    EXPECT_EQ(listener.lastThemeName, "Light Mode");

    getThemeManager().removeListener(&listener);

    // After removal, listener should not be notified
    getThemeManager().setCurrentTheme("Dark Professional");
    EXPECT_EQ(listener.changeCount, 2);
}

// Test: Multiple listeners
TEST_F(ThemeManagerApplyTest, MultipleListeners)
{
    TestThemeListener listener1;
    TestThemeListener listener2;
    TestThemeListener listener3;

    getThemeManager().addListener(&listener1);
    getThemeManager().addListener(&listener2);
    getThemeManager().addListener(&listener3);

    getThemeManager().setCurrentTheme("Classic Amber");

    EXPECT_EQ(listener1.changeCount, 1);
    EXPECT_EQ(listener2.changeCount, 1);
    EXPECT_EQ(listener3.changeCount, 1);

    getThemeManager().removeListener(&listener1);
    getThemeManager().removeListener(&listener2);
    getThemeManager().removeListener(&listener3);
}

// Test: Remove listener twice
TEST_F(ThemeManagerApplyTest, RemoveListenerTwice)
{
    TestThemeListener listener;

    getThemeManager().addListener(&listener);
    getThemeManager().removeListener(&listener);
    getThemeManager().removeListener(&listener); // Second removal

    // Should not crash
    getThemeManager().setCurrentTheme("Classic Green");

    EXPECT_EQ(listener.changeCount, 0);
}

// Test: Listener that removes itself during callback
class SelfRemovingThemeListener : public ThemeManagerListener
{
public:
    int callCount = 0;
    ThemeManager* themeManager = nullptr;

    void themeChanged(const ColorTheme&) override
    {
        callCount++;
        if (themeManager)
            themeManager->removeListener(this);
    }
};

TEST_F(ThemeManagerApplyTest, ListenerRemovesDuringCallback)
{
    SelfRemovingThemeListener listener;
    listener.themeManager = &getThemeManager();
    getThemeManager().addListener(&listener);

    getThemeManager().setCurrentTheme("Classic Amber");
    getThemeManager().setCurrentTheme("High Contrast");

    // Should only be called once (removed itself)
    EXPECT_EQ(listener.callCount, 1);
}

// =============================================================================
// Edge Case Tests - Waveform Colors
// =============================================================================

// Test: Get waveform color with negative index (should handle gracefully)
TEST_F(ThemeManagerApplyTest, WaveformColorNegativeIndex)
{
    auto* theme = getThemeManager().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Negative index should still return a valid (non-transparent) color.
    auto color = theme->getWaveformColor(-1);
    EXPECT_FALSE(color.isTransparent());
}

// Test: Get waveform color with very large index
TEST_F(ThemeManagerApplyTest, WaveformColorLargeIndex)
{
    auto* theme = getThemeManager().getTheme("Dark Professional");
    ASSERT_NE(theme, nullptr);

    // Should cycle properly
    auto color0 = theme->getWaveformColor(0);
    auto colorLarge = theme->getWaveformColor(1000000);

    // With 64 colors, index 1000000 % 64 should equal a valid cycling pattern
    EXPECT_EQ(color0.getARGB(), theme->getWaveformColor(64).getARGB());
    EXPECT_EQ(colorLarge.getARGB(), theme->getWaveformColor(1000000 % 64).getARGB());
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
    auto* theme = getThemeManager().getTheme("Dark Professional");
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
    getThemeManager().cloneTheme("Dark Professional", "PropertyTest");

    auto* original = getThemeManager().getTheme("Dark Professional");
    auto* cloned = getThemeManager().getTheme("PropertyTest");

    ASSERT_NE(original, nullptr);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->backgroundPrimary.getARGB(), original->backgroundPrimary.getARGB());
    EXPECT_EQ(cloned->backgroundSecondary.getARGB(), original->backgroundSecondary.getARGB());
    EXPECT_EQ(cloned->textPrimary.getARGB(), original->textPrimary.getARGB());
    EXPECT_EQ(cloned->gridMajor.getARGB(), original->gridMajor.getARGB());
    EXPECT_EQ(cloned->statusActive.getARGB(), original->statusActive.getARGB());
    EXPECT_EQ(cloned->waveformColors.size(), original->waveformColors.size());

    getThemeManager().deleteTheme("PropertyTest");
}

// =============================================================================
// Glass Theme Application Tests
// =============================================================================

// Test: Setting a glass theme updates getCurrentTheme with correct glass fields
TEST_F(ThemeManagerApplyTest, SetGlassThemeUpdatesCurrentTheme)
{
    EXPECT_TRUE(getThemeManager().setCurrentTheme("Glass Dark Blue"));

    const auto& current = getThemeManager().getCurrentTheme();
    EXPECT_EQ(current.name, "Glass Dark Blue");
    EXPECT_NEAR(current.accentHue, 220.0f, 0.01f);
    EXPECT_NEAR(current.accentSaturation, 0.7f, 0.001f);
    // Accent brightness was bumped when the glass themes were flattened so
    // accent-tinted UI (knob rings, focus outlines) stays readable on a
    // near-black surface. The hue is the theme's identity; the brightness
    // is tuned, not load-bearing.
    EXPECT_NEAR(current.accentLightness, 0.95f, 0.001f);
}

// Test: Glass fields are accessible after switching between glass themes
TEST_F(ThemeManagerApplyTest, SwitchBetweenGlassThemesUpdatesFields)
{
    getThemeManager().setCurrentTheme("Glass Dark Blue");
    float blueHue = getThemeManager().getCurrentTheme().accentHue;

    getThemeManager().setCurrentTheme("Glass Dark Purple");
    float purpleHue = getThemeManager().getCurrentTheme().accentHue;

    getThemeManager().setCurrentTheme("Glass Dark Brown");
    float brownHue = getThemeManager().getCurrentTheme().accentHue;

    // Each glass theme has a distinct accent hue
    EXPECT_NE(blueHue, purpleHue);
    EXPECT_NE(purpleHue, brownHue);
    EXPECT_NE(blueHue, brownHue);
}

// Test: High Contrast theme has full opacity glass (accessibility)
TEST_F(ThemeManagerApplyTest, HighContrastThemeHasFullOpacityGlass)
{
    getThemeManager().setCurrentTheme("High Contrast");
    const auto& theme = getThemeManager().getCurrentTheme();

    EXPECT_NEAR(theme.glassAlpha, 1.0f, 0.001f);
    EXPECT_NEAR(theme.panelAlpha, 1.0f, 0.001f);
}

// Test: Glass theme clone preserves glass fields
TEST_F(ThemeManagerApplyTest, ClonePreservesGlassFields)
{
    getThemeManager().cloneTheme("Glass Dark Purple", "GlassCloneTest");

    auto* original = getThemeManager().getTheme("Glass Dark Purple");
    auto* cloned = getThemeManager().getTheme("GlassCloneTest");

    ASSERT_NE(original, nullptr);
    ASSERT_NE(cloned, nullptr);

    EXPECT_NEAR(cloned->accentHue, original->accentHue, 0.01f);
    EXPECT_NEAR(cloned->accentSaturation, original->accentSaturation, 0.001f);
    EXPECT_NEAR(cloned->glassAlpha, original->glassAlpha, 0.001f);
    EXPECT_NEAR(cloned->borderSubtleAlpha, original->borderSubtleAlpha, 0.001f);
    EXPECT_NEAR(cloned->shadowIntensity, original->shadowIntensity, 0.001f);

    getThemeManager().deleteTheme("GlassCloneTest");
}

// Test: Listener is notified with correct accent fields on theme change.
// Glass-era `glassAlpha < 1.0` was dropped when the aesthetic flattened —
// the field is kept for serialization back-compat but now always 1.0 on
// system themes. The hue is the identity and is still load-bearing.
TEST_F(ThemeManagerApplyTest, ListenerReceivesGlassFieldsOnChange)
{
    struct CapturingListener : ThemeManagerListener
    {
        float capturedHue = -1.0f;
        float capturedGlassAlpha = -1.0f;

        void themeChanged(const ColorTheme& newTheme) override
        {
            capturedHue = newTheme.accentHue;
            capturedGlassAlpha = newTheme.glassAlpha;
        }
    };

    CapturingListener listener;
    getThemeManager().addListener(&listener);

    getThemeManager().setCurrentTheme("Glass Dark Brown");

    EXPECT_NEAR(listener.capturedHue, 40.0f, 0.01f);
    EXPECT_NEAR(listener.capturedGlassAlpha, 1.0f, 0.001f);

    getThemeManager().removeListener(&listener);
}
