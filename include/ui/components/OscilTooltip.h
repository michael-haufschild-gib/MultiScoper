/*
    Oscil - Tooltip Component
    Rich tooltips with contextual help, shortcuts, and color previews
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * A rich tooltip component with contextual information
 *
 * Features:
 * - Title, value, shortcut, and hint text
 * - Optional color preview swatch
 * - Automatic positioning (above, below, left, right)
 * - Fade-in animation
 * - Theme-aware styling
 */
class OscilTooltip : public juce::Component,
                     public ThemeManagerListener,
                     public TestIdSupport,
                     private juce::Timer
{
public:
    OscilTooltip();
    explicit OscilTooltip(const juce::String& testId);
    ~OscilTooltip() override;

    // Content configuration
    void setContent(const RichTooltipContent& content);
    void setContent(const juce::String& title, const juce::String& value = {});

    void setTitle(const juce::String& title);
    void setValue(const juce::String& value);
    void setShortcut(const juce::String& shortcut);
    void setHint(const juce::String& hint);
    void setColourPreview(juce::Colour colour);
    void clearColourPreview();

    // Display control
    void showAt(juce::Point<int> position, TooltipPosition preferredPosition = TooltipPosition::Auto);
    void showNear(juce::Component* target, TooltipPosition preferredPosition = TooltipPosition::Auto);
    void hide();

    bool isShowing() const { return isVisible(); }

    // Size calculation
    juce::Rectangle<int> calculateBounds(juce::Point<int> targetPoint,
                                          TooltipPosition position) const;

    // Component overrides
    void paint(juce::Graphics& g) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Static helper for simple tooltips
    static void showSimple(juce::Component* target, const juce::String& text);
    static void hideAll();

private:
    void timerCallback() override;
    int calculateContentHeight() const;
    int calculateContentWidth() const;
    TooltipPosition calculateBestPosition(juce::Point<int> target,
                                           juce::Rectangle<int> screenBounds) const;

    // Content
    RichTooltipContent content_;

    // Animation
    SpringAnimation opacitySpring_;
    float currentOpacity_ = 0.0f;
    bool hiding_ = false;

    // Theme
    ColorTheme theme_;

    // Layout constants
    static constexpr int PADDING = 8;
    static constexpr int LINE_SPACING = 4;
    static constexpr int ARROW_SIZE = 6;
    static constexpr int COLOUR_PREVIEW_SIZE = 16;

    // Global tooltip instance for simple usage
    static inline juce::Component::SafePointer<OscilTooltip> globalTooltip_;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilTooltip)
};

/**
 * Mixin class for components that want tooltip support
 * Handles hover delay and tooltip positioning automatically
 */
class TooltipClient : private juce::Timer
{
public:
    ~TooltipClient() override = default;

    void setRichTooltip(const RichTooltipContent& content);
    void setSimpleTooltip(const juce::String& text);

protected:
    void tooltipMouseEnter();
    void tooltipMouseExit();
    void tooltipMouseMove(const juce::Point<int>& position);

    virtual juce::Component* getTooltipTarget() = 0;

private:
    void timerCallback() override;

    RichTooltipContent tooltipContent_;
    juce::Point<int> lastMousePosition_;
    bool tooltipScheduled_ = false;
};

} // namespace oscil
