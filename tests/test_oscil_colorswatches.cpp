/*
    Oscil - Color Swatches Component Tests
    Tests for OscilColorSwatches UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilColorSwatches.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilColorSwatchesTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, DefaultConstruction)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    EXPECT_EQ(swatches.getColors().size(), 0u);
    EXPECT_EQ(swatches.getSelectedIndex(), -1);
}

TEST_F(OscilColorSwatchesTest, ConstructionWithTestId)
{
    OscilColorSwatches swatches(ThemeManager::getInstance(), "swatches-1");

    EXPECT_EQ(swatches.getColors().size(), 0u);
}

// =============================================================================
// Color Management Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, SetColors)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    std::vector<juce::Colour> colors = {
        juce::Colours::red,
        juce::Colours::green,
        juce::Colours::blue
    };

    swatches.setColors(colors);
    EXPECT_EQ(swatches.getColors().size(), 3u);
}

TEST_F(OscilColorSwatchesTest, AddColor)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    EXPECT_EQ(swatches.getColors().size(), 2u);
}

TEST_F(OscilColorSwatchesTest, GetColorAtIndex)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::blue);

    const auto& colors = swatches.getColors();
    EXPECT_EQ(colors[0].getARGB(), juce::Colours::red.getARGB());
    EXPECT_EQ(colors[1].getARGB(), juce::Colours::blue.getARGB());
}

TEST_F(OscilColorSwatchesTest, ClearColors)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    swatches.clearColors();
    EXPECT_EQ(swatches.getColors().size(), 0u);
}

// =============================================================================
// Selection Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, SelectByIndex)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    swatches.setSelectedIndex(1, false);
    EXPECT_EQ(swatches.getSelectedIndex(), 1);
    EXPECT_EQ(swatches.getSelectedColor().getARGB(), juce::Colours::green.getARGB());
}

TEST_F(OscilColorSwatchesTest, SelectByColor)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    swatches.setSelectedColor(juce::Colours::blue, false);
    EXPECT_EQ(swatches.getSelectedIndex(), 2);
}

TEST_F(OscilColorSwatchesTest, GetSelectedColorWhenNoSelection)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);

    // No selection yet
    auto color = swatches.getSelectedColor();
    // Implementation-dependent, but shouldn't crash
}

TEST_F(OscilColorSwatchesTest, SelectInvalidIndex)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.setSelectedIndex(0, false);

    swatches.setSelectedIndex(999, false);
    // Should handle gracefully
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, SetSwatchSize)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.setSwatchSize(32);
    EXPECT_EQ(swatches.getSwatchSize(), 32);
}

TEST_F(OscilColorSwatchesTest, SetSpacing)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.setSpacing(8);
    EXPECT_EQ(swatches.getSpacing(), 8);
}

TEST_F(OscilColorSwatchesTest, SetColumns)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.setColumns(8);
    EXPECT_EQ(swatches.getColumns(), 8);
}

TEST_F(OscilColorSwatchesTest, SetColumnsZeroForAuto)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.setColumns(0);  // Auto columns
    EXPECT_EQ(swatches.getColumns(), 0);
}

TEST_F(OscilColorSwatchesTest, SetShowCheckmark)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    swatches.setShowCheckmark(true);
    EXPECT_TRUE(swatches.getShowCheckmark());

    swatches.setShowCheckmark(false);
    EXPECT_FALSE(swatches.getShowCheckmark());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, OnColorSelectedCallback)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    int changeCount = 0;
    int lastIndex = -1;
    juce::Colour lastColor;

    swatches.onColorSelected = [&changeCount, &lastIndex, &lastColor](int index, juce::Colour color) {
        changeCount++;
        lastIndex = index;
        lastColor = color;
    };

    swatches.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
    EXPECT_EQ(lastColor.getARGB(), juce::Colours::green.getARGB());
}

TEST_F(OscilColorSwatchesTest, NoCallbackWhenNotifyFalse)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);

    int changeCount = 0;

    swatches.onColorSelected = [&changeCount](int, juce::Colour) {
        changeCount++;
    };

    swatches.setSelectedIndex(0, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(OscilColorSwatchesTest, OnColorHoveredCallback)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);

    bool hoverCalled = false;
    swatches.onColorHovered = [&hoverCalled](juce::Colour) {
        hoverCalled = true;
    };

    // Hover would be triggered by mouse events
    // Just verify callback can be set
    EXPECT_FALSE(hoverCalled);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, PreferredWidthPositive)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    int width = swatches.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilColorSwatchesTest, PreferredHeightPositive)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    int height = swatches.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilColorSwatchesTest, EmptySwatchesReturnSize)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    // Empty swatches may return 0 or negative values
    // Just verify the methods don't crash
    swatches.getPreferredWidth();
    swatches.getPreferredHeight();
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, ThemeChangeDoesNotThrow)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.setSelectedIndex(0, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    swatches.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(swatches.getSelectedIndex(), 0);
}

TEST_F(OscilColorSwatchesTest, ThemeChangePreservesColors)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::blue);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    swatches.themeChanged(newTheme);

    EXPECT_EQ(swatches.getColors().size(), 2u);
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilColorSwatchesTest, WantsKeyboardFocus)
{
    OscilColorSwatches swatches(ThemeManager::getInstance());

    EXPECT_TRUE(swatches.getWantsKeyboardFocus());
}
