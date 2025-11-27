/*
    Oscil - Color Swatches Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilColorSwatches.h"

using namespace oscil;

class OscilColorSwatchesTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilColorSwatchesTest, DefaultConstruction)
{
    OscilColorSwatches swatches;

    EXPECT_EQ(swatches.getNumColors(), 0);
    EXPECT_EQ(swatches.getSelectedIndex(), -1);
}

// Test: Set colors
TEST_F(OscilColorSwatchesTest, SetColors)
{
    OscilColorSwatches swatches;

    std::vector<juce::Colour> colors = {
        juce::Colours::red,
        juce::Colours::green,
        juce::Colours::blue
    };

    swatches.setColors(colors);
    EXPECT_EQ(swatches.getNumColors(), 3);
}

// Test: Add color
TEST_F(OscilColorSwatchesTest, AddColor)
{
    OscilColorSwatches swatches;

    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    EXPECT_EQ(swatches.getNumColors(), 2);
}

// Test: Get color at index
TEST_F(OscilColorSwatchesTest, GetColorAtIndex)
{
    OscilColorSwatches swatches;

    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::blue);

    EXPECT_EQ(swatches.getColorAt(0).getARGB(), juce::Colours::red.getARGB());
    EXPECT_EQ(swatches.getColorAt(1).getARGB(), juce::Colours::blue.getARGB());
}

// Test: Clear colors
TEST_F(OscilColorSwatchesTest, ClearColors)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    swatches.clearColors();
    EXPECT_EQ(swatches.getNumColors(), 0);
}

// Test: Select by index
TEST_F(OscilColorSwatchesTest, SelectByIndex)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    swatches.setSelectedIndex(1);
    EXPECT_EQ(swatches.getSelectedIndex(), 1);
    EXPECT_EQ(swatches.getSelectedColor().getARGB(), juce::Colours::green.getARGB());
}

// Test: Select by color
TEST_F(OscilColorSwatchesTest, SelectByColor)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    swatches.setSelectedColor(juce::Colours::blue);
    EXPECT_EQ(swatches.getSelectedIndex(), 2);
}

// Test: Callback on selection
TEST_F(OscilColorSwatchesTest, CallbackOnSelection)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);

    int changeCount = 0;
    juce::Colour lastColor;

    swatches.onColorSelected = [&changeCount, &lastColor](juce::Colour color) {
        changeCount++;
        lastColor = color;
    };

    swatches.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastColor.getARGB(), juce::Colours::green.getARGB());
}

// Test: Hover callback
TEST_F(OscilColorSwatchesTest, HoverCallback)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);

    bool hoverCalled = false;
    swatches.onColorHovered = [&hoverCalled](juce::Colour) {
        hoverCalled = true;
    };

    // Hover would be triggered by mouse events
    // Just verify callback can be set
    EXPECT_FALSE(hoverCalled);
}

// Test: Swatch size
TEST_F(OscilColorSwatchesTest, SwatchSize)
{
    OscilColorSwatches swatches;

    swatches.setSwatchSize(24);
    EXPECT_EQ(swatches.getSwatchSize(), 24);
}

// Test: Spacing between swatches
TEST_F(OscilColorSwatchesTest, Spacing)
{
    OscilColorSwatches swatches;

    swatches.setSpacing(8);
    EXPECT_EQ(swatches.getSpacing(), 8);
}

// Test: Columns
TEST_F(OscilColorSwatchesTest, Columns)
{
    OscilColorSwatches swatches;

    swatches.setColumns(8);
    EXPECT_EQ(swatches.getColumns(), 8);
}

// Test: Show selection indicator
TEST_F(OscilColorSwatchesTest, ShowSelectionIndicator)
{
    OscilColorSwatches swatches;

    swatches.setShowSelectionIndicator(true);
    EXPECT_TRUE(swatches.getShowSelectionIndicator());

    swatches.setShowSelectionIndicator(false);
    EXPECT_FALSE(swatches.getShowSelectionIndicator());
}

// Test: Show tooltips
TEST_F(OscilColorSwatchesTest, ShowTooltips)
{
    OscilColorSwatches swatches;

    swatches.setShowTooltips(true);
    EXPECT_TRUE(swatches.getShowTooltips());

    swatches.setShowTooltips(false);
    EXPECT_FALSE(swatches.getShowTooltips());
}

// Test: Preferred size
TEST_F(OscilColorSwatchesTest, PreferredSize)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.addColor(juce::Colours::green);
    swatches.addColor(juce::Colours::blue);

    int width = swatches.getPreferredWidth();
    int height = swatches.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Invalid index returns transparent
TEST_F(OscilColorSwatchesTest, InvalidIndexReturnsTransparent)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);

    auto color = swatches.getColorAt(999);
    EXPECT_EQ(color.getAlpha(), 0);
}

// Test: Theme changes
TEST_F(OscilColorSwatchesTest, ThemeChanges)
{
    OscilColorSwatches swatches;
    swatches.addColor(juce::Colours::red);
    swatches.setSelectedIndex(0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    swatches.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(swatches.getSelectedIndex(), 0);
}

// Test: Accessibility
TEST_F(OscilColorSwatchesTest, Accessibility)
{
    OscilColorSwatches swatches;

    auto* handler = swatches.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}

// Test: Focus handling
TEST_F(OscilColorSwatchesTest, FocusHandling)
{
    OscilColorSwatches swatches;
    EXPECT_TRUE(swatches.getWantsKeyboardFocus());
}

// Test: Set color labels
TEST_F(OscilColorSwatchesTest, ColorLabels)
{
    OscilColorSwatches swatches;

    swatches.addColor(juce::Colours::red, "Red");
    swatches.addColor(juce::Colours::blue, "Blue");

    EXPECT_EQ(swatches.getColorLabel(0), juce::String("Red"));
    EXPECT_EQ(swatches.getColorLabel(1), juce::String("Blue"));
}
