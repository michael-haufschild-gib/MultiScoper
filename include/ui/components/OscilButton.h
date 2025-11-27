/*
    Oscil - Button Component
    Versatile button with 5 variants, spring animations, and full accessibility support
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"

namespace oscil
{

/**
 * A themed button component with multiple variants and smooth animations
 *
 * Features:
 * - 5 variants: Primary, Secondary, Danger, Ghost, Icon
 * - Spring-based hover/press animations
 * - Full keyboard accessibility
 * - Shortcut key support
 * - Icon support (left, right, or center)
 */
class OscilButton : public juce::Component,
                    public ThemeManagerListener,
                    private juce::Timer
{
public:
    /**
     * Create a button with a text label
     */
    explicit OscilButton(const juce::String& text = {});

    /**
     * Create an icon-only button
     */
    explicit OscilButton(const juce::Image& icon);

    ~OscilButton() override;

    // Configuration
    void setText(const juce::String& text);
    juce::String getText() const { return label_; }

    void setVariant(ButtonVariant variant);
    ButtonVariant getVariant() const { return variant_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    void setIcon(const juce::Image& icon, bool iconOnLeft = true);
    void clearIcon();

    void setShortcut(const juce::KeyPress& key);
    juce::KeyPress getShortcut() const { return shortcutKey_; }

    void setTooltip(const juce::String& tooltip);
    juce::String getTooltip() const { return tooltipText_; }

    // Toggleable button support (for segmented controls)
    void setToggleable(bool toggleable);
    bool isToggleable() const { return toggleable_; }

    void setToggled(bool toggled, bool notify = true);
    bool isToggled() const { return isToggled_; }

    // Segment position for segmented button bars
    void setSegmentPosition(SegmentPosition position);
    SegmentPosition getSegmentPosition() const { return segmentPosition_; }

    // Button ID for identification in groups
    void setButtonId(int id) { buttonId_ = id; }
    int getButtonId() const { return buttonId_; }

    // Callbacks
    std::function<void()> onClick;
    std::function<void()> onRightClick;
    std::function<void(bool)> onToggle;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void updateAnimations();
    void triggerClick();

    // Rendering helpers
    void paintButton(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    juce::Colour getBackgroundColour() const;
    juce::Colour getTextColour() const;
    juce::Colour getBorderColour() const;

    // State
    ButtonVariant variant_ = ButtonVariant::Primary;
    juce::String label_;
    juce::Image icon_;
    bool iconOnLeft_ = true;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool isPressed_ = false;
    bool hasFocus_ = false;
    juce::KeyPress shortcutKey_;
    juce::String tooltipText_;

    // Toggleable state
    bool toggleable_ = false;
    bool isToggled_ = false;

    // Segment position for segmented controls
    SegmentPosition segmentPosition_ = SegmentPosition::None;
    int buttonId_ = -1;

    // Animation
    SpringAnimation scaleSpring_;
    SpringAnimation brightnessSpring_;
    float currentScale_ = 1.0f;
    float currentBrightness_ = 0.0f;

    // Theme cache
    ColorTheme theme_;

    // Constants
    static constexpr int ICON_PADDING = 8;
    static constexpr int TEXT_PADDING = 16;
    static constexpr float ICON_SIZE = 16.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilButton)
};

} // namespace oscil
