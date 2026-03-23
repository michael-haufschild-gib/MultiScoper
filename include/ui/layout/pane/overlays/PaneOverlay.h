/*
    Oscil - Pane Overlay Base Class
    Base class for fixed-position overlays within pane body
*/

#pragma once

#include "ui/components/ComponentConstants.h"
#include "ui/components/TestId.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

/**
 * Base class for fixed-position overlays in the pane body.
 *
 * Features:
 * - Corner positioning (TopLeft, TopRight, BottomLeft, BottomRight)
 * - Themed semi-transparent background
 * - Fade in/out animation
 * - Click-through support (passes mouse events to components below)
 * - Auto-sizing to content
 */
class PaneOverlay
    : public juce::Component
    , public ThemeManagerListener
    , public TestIdSupport
{
public:
    /**
     * Position within the parent body
     */
    enum class Position
    {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    explicit PaneOverlay(IThemeService& themeService);
    PaneOverlay(IThemeService& themeService, const juce::String& testId);
    ~PaneOverlay() override;

    // Position
    void setPosition(Position pos);
    Position getPosition() const { return position_; }

    // Visibility with animation
    void setVisibleAnimated(bool shouldBeVisible);
    bool isAnimating() const { return isAnimating_; }

    // Click-through behavior
    void setClickThrough(bool enabled) { clickThrough_ = enabled; }
    bool isClickThrough() const { return clickThrough_; }

    // Margin from parent edges
    void setMargin(int margin) { margin_ = margin; }
    int getMargin() const { return margin_; }

    // Background opacity (0.0 - 1.0)
    void setBackgroundOpacity(float opacity) { backgroundOpacity_ = opacity; }
    float getBackgroundOpacity() const { return backgroundOpacity_; }

    // Component overrides
    void paint(juce::Graphics& g) override;
    bool hitTest(int x, int y) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Calculate preferred size based on content
    virtual juce::Rectangle<int> getPreferredBounds() const;

    /// Reposition the overlay within the given parent bounds using the anchor/margin rules.
    void updatePositionInParent(juce::Rectangle<int> parentBounds);

protected:
    /**
     * Paint the overlay background with theme colors
     * Subclasses should call this first in paint()
     */
    void paintOverlayBackground(juce::Graphics& g);

    /**
     * Get content bounds (inside padding)
     */
    juce::Rectangle<int> getContentBounds() const;

    /**
     * Get the preferred content size (subclasses override)
     */
    virtual juce::Rectangle<int> getPreferredContentSize() const { return {0, 0, 100, 60}; }

    /**
     * Get current animation opacity (0.0 - 1.0)
     * Subclasses can use this to apply opacity to their content
     */
    float getCurrentOpacity() const { return currentOpacity_; }

    IThemeService& themeService_;

private:
    void startFadeAnimation(bool fadeIn);
    void updateFadeAnimation();

    Position position_ = Position::TopRight;
    bool clickThrough_ = true;
    int margin_ = ComponentLayout::SPACING_SM;
    float backgroundOpacity_ = 0.85f;

    // Animation state
    bool isAnimating_ = false;
    bool fadeDirection_ = true; // true = fade in, false = fade out
    float currentOpacity_ = 0.0f;
    float targetOpacity_ = 0.0f;
    std::unique_ptr<juce::Timer> fadeTimer_;

    // Padding inside the overlay
    static constexpr int CONTENT_PADDING = ComponentLayout::SPACING_SM;
    static constexpr float CORNER_RADIUS = ComponentLayout::RADIUS_MD;

    // Animation timing
    static constexpr int FADE_DURATION_MS = ComponentLayout::ANIMATION_FAST_MS;
    static constexpr int FADE_TIMER_INTERVAL_MS = 16; // ~60fps

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneOverlay)
};

} // namespace oscil
