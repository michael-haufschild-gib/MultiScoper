/*
    Oscil - Crosshair Overlay
    Mouse-following crosshair with floating tooltip showing time and amplitude
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * Crosshair overlay that follows mouse position.
 *
 * Features:
 * - Vertical and horizontal lines crossing at mouse position
 * - Floating tooltip showing time and amplitude values
 * - Tooltip flips position when near edges
 * - Click-through (doesn't block mouse events)
 *
 * Unlike PaneOverlay, this component:
 * - Covers the full body area
 * - Follows mouse position
 * - Has no fade animation (instant show/hide based on mouse hover)
 */
class CrosshairOverlay : public juce::Component,
                          public ThemeManagerListener,
                          public TestIdSupport
{
public:
    explicit CrosshairOverlay(IThemeService& themeService);
    CrosshairOverlay(IThemeService& themeService, const juce::String& testId);
    ~CrosshairOverlay() override;

    // Mouse position (relative to this component)
    void setMousePosition(juce::Point<int> pos);
    juce::Point<int> getMousePosition() const { return mousePos_; }

    // Data values to display in tooltip
    void setTimeValue(float timeMs);
    void setAmplitudeValue(float ampDb);

    // Getters for testing
    float getTimeValue() const { return timeMs_; }
    float getAmplitudeValue() const { return ampDb_; }

    // Enable/disable the crosshair display
    void setCrosshairVisible(bool visible);
    bool isCrosshairVisible() const { return crosshairVisible_; }

    // Component overrides
    void paint(juce::Graphics& g) override;
    bool hitTest(int x, int y) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void paintCrosshairLines(juce::Graphics& g);
    void paintTooltip(juce::Graphics& g);
    juce::Rectangle<int> calculateTooltipBounds() const;

    IThemeService& themeService_;

    // Mouse position
    juce::Point<int> mousePos_{ -100, -100 };  // Start off-screen
    bool crosshairVisible_ = false;

    // Data values
    float timeMs_ = 0.0f;
    float ampDb_ = 0.0f;

    // Tooltip layout
    static constexpr int TOOLTIP_OFFSET_X = 12;
    static constexpr int TOOLTIP_OFFSET_Y = 12;
    static constexpr int TOOLTIP_PADDING = ComponentLayout::SPACING_SM;
    static constexpr int TOOLTIP_ROW_HEIGHT = 16;
    static constexpr int TOOLTIP_LABEL_WIDTH = 45;
    static constexpr int TOOLTIP_VALUE_WIDTH = 65;
    static constexpr float TOOLTIP_CORNER_RADIUS = ComponentLayout::RADIUS_SM;
    static constexpr float TOOLTIP_BACKGROUND_OPACITY = 0.9f;

    // Crosshair styling
    static constexpr float LINE_OPACITY = 0.6f;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrosshairOverlay)
};

} // namespace oscil
