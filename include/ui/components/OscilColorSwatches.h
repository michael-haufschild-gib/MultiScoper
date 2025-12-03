/*
    Oscil - Color Swatches Component
    Grid of selectable color swatches with hover preview
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * A grid of selectable color swatches
 *
 * Features:
 * - Configurable grid of colors
 * - Hover preview
 * - Selection indicator
 * - Keyboard navigation
 * - Accessibility support
 */
class OscilColorSwatches : public juce::Component,
                           public ThemeManagerListener,
                           public TestIdSupport,
                           private juce::Timer
{
public:
    OscilColorSwatches();
    explicit OscilColorSwatches(const juce::String& testId);
    ~OscilColorSwatches() override;

    // Colors
    void setColors(const std::vector<juce::Colour>& colors);
    const std::vector<juce::Colour>& getColors() const { return colors_; }

    void addColor(juce::Colour color);
    void clearColors();

    // Selection
    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const { return selectedIndex_; }

    void setSelectedColor(juce::Colour color, bool notify = true);
    juce::Colour getSelectedColor() const;

    // Configuration
    void setSwatchSize(int size);
    int getSwatchSize() const { return swatchSize_; }

    void setSpacing(int spacing);
    int getSpacing() const { return spacing_; }

    void setColumns(int cols);  // 0 = auto
    int getColumns() const { return columns_; }

    void setShowCheckmark(bool show);
    bool getShowCheckmark() const { return showCheckmark_; }

    // Callbacks
    std::function<void(int, juce::Colour)> onColorSelected;
    std::function<void(juce::Colour)> onColorHovered;  // For live preview

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;

    int getSwatchAtPosition(juce::Point<int> pos) const;
    juce::Rectangle<int> getSwatchBounds(int index) const;
    int getColumnCount() const;
    int getRowCount() const;

    void paintSwatch(juce::Graphics& g, int index, juce::Rectangle<int> bounds);
    void paintCheckmark(juce::Graphics& g, juce::Rectangle<int> bounds);

    std::vector<juce::Colour> colors_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;

    int swatchSize_ = 24;
    int spacing_ = 4;
    int columns_ = 0;  // 0 = auto
    bool showCheckmark_ = true;

    bool hasFocus_ = false;
    int focusedIndex_ = 0;

    SpringAnimation hoverSpring_;

    ColorTheme theme_;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilColorSwatches)
};

} // namespace oscil
