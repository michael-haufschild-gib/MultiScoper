/*
    MultiScoper - Button Component Implementation
    (Painting, path cache, and colour resolution are in MultiScoperButtonPainting.cpp)
*/

#include "ui/components/MultiScoperButton.h"

#include <utility>

namespace multiscoper
{

MultiScoperButton::MultiScoperButton(IThemeService& themeService, juce::String text)
    : ThemedComponent(themeService)
    , label_(std::move(text))
    , scaleSpring_(SpringPresets::medium())
    , brightnessSpring_(SpringPresets::fast())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    scaleSpring_.position = 1.0f;
    scaleSpring_.target = 1.0f;
    brightnessSpring_.position = 0.0f;
    brightnessSpring_.target = 0.0f;
}

MultiScoperButton::MultiScoperButton(IThemeService& themeService, juce::String text, const juce::String& testId)
    : MultiScoperButton(themeService, std::move(text))
{
    setTestId(testId);
}

MultiScoperButton::MultiScoperButton(IThemeService& themeService, const juce::Image& icon)
    : MultiScoperButton(themeService, juce::String())
{
    variant_ = ButtonVariant::Icon;
    icon_ = icon;
}

void MultiScoperButton::registerTestId() { MULTISCOPER_REGISTER_TEST_ID(testId_); }

MultiScoperButton::~MultiScoperButton() { stopTimer(); }

void MultiScoperButton::setText(const juce::String& text)
{
    if (label_ != text)
    {
        label_ = text;
        repaint();
    }
}

void MultiScoperButton::setVariant(ButtonVariant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void MultiScoperButton::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        juce::Component::setEnabled(enabled);
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void MultiScoperButton::setIcon(const juce::Image& icon, bool iconOnLeft)
{
    icon_ = icon;
    iconOnLeft_ = iconOnLeft;
    repaint();
}

void MultiScoperButton::clearIcon()
{
    icon_ = {};
    repaint();
}

void MultiScoperButton::setIconPath(const juce::Path& path)
{
    iconPath_ = path;
    repaint();
}

void MultiScoperButton::clearIconPath()
{
    iconPath_.clear();
    repaint();
}

void MultiScoperButton::setIconPadding(float padding)
{
    iconPadding_ = std::max(0.0f, padding);
    repaint();
}

void MultiScoperButton::setShortcut(const juce::KeyPress& key) { shortcutKey_ = key; }

void MultiScoperButton::setTooltip(const juce::String& tooltip)
{
    tooltipText_ = tooltip;
    setHelpText(tooltip);
}

void MultiScoperButton::setToggleable(bool toggleable) { toggleable_ = toggleable; }

void MultiScoperButton::setToggled(bool toggled, bool notify)
{
    if (isToggled_ != toggled)
    {
        isToggled_ = toggled;
        repaint();

        if (notify && onToggle)
            onToggle(isToggled_);

        if (notify && onToggleStateChanged)
            onToggleStateChanged(isToggled_);
    }
}

void MultiScoperButton::setBorder(juce::Colour color, float thickness)
{
    if (borderColor_ != color || std::abs(borderWidth_ - thickness) > 1e-6f)
    {
        borderColor_ = color;
        borderWidth_ = thickness;
        repaint();
    }
}

void MultiScoperButton::setSegmentPosition(SegmentPosition position)
{
    if (segmentPosition_ != position)
    {
        segmentPosition_ = position;
        repaint();
    }
}

int MultiScoperButton::getPreferredWidth() const
{
    if (variant_ == ButtonVariant::Icon)
        return ComponentLayout::BUTTON_HEIGHT;

    auto font = ComponentLayout::defaultFont();
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, label_, 0, 0);
    float const textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

    return static_cast<int>(textWidth) + (TEXT_PADDING * 2);
}

int MultiScoperButton::getPreferredHeight() const
{
    if (variant_ == ButtonVariant::Icon)
        return ComponentLayout::BUTTON_HEIGHT;

    return ComponentLayout::BUTTON_HEIGHT;
}

void MultiScoperButton::resized() {}

void MultiScoperButton::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        scaleSpring_.setTarget(ComponentLayout::HOVER_SCALE);
        brightnessSpring_.setTarget(ComponentLayout::HOVER_BRIGHTNESS_OFFSET);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentScale_ = ComponentLayout::HOVER_SCALE;
        currentBrightness_ = ComponentLayout::HOVER_BRIGHTNESS_OFFSET;
        repaint();
    }
}

void MultiScoperButton::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;
    isPressed_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        scaleSpring_.setTarget(1.0f);
        brightnessSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentScale_ = 1.0f;
        currentBrightness_ = 0.0f;
        repaint();
    }
}

void MultiScoperButton::mouseDown(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    isPressed_ = true;

    // Spawn ripple at click position
    rippleManager_.spawn(static_cast<float>(e.x), static_cast<float>(e.y), static_cast<float>(getWidth()),
                         static_cast<float>(getHeight()));

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        scaleSpring_.setTarget(ComponentLayout::PRESS_SCALE);
        brightnessSpring_.setTarget(ComponentLayout::PRESS_BRIGHTNESS_OFFSET);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentScale_ = ComponentLayout::PRESS_SCALE;
        currentBrightness_ = ComponentLayout::PRESS_BRIGHTNESS_OFFSET;
        repaint();
    }

    if (e.mods.isPopupMenu() && onRightClick)
    {
        onRightClick();
    }
}

void MultiScoperButton::mouseUp(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    bool const wasPressed = isPressed_;
    isPressed_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        scaleSpring_.setTarget(isHovered_ ? ComponentLayout::HOVER_SCALE : 1.0f);
        brightnessSpring_.setTarget(isHovered_ ? ComponentLayout::HOVER_BRIGHTNESS_OFFSET : 0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        currentScale_ = isHovered_ ? ComponentLayout::HOVER_SCALE : 1.0f;
        currentBrightness_ = isHovered_ ? ComponentLayout::HOVER_BRIGHTNESS_OFFSET : 0.0f;
        repaint();
    }

    if (wasPressed && contains(e.getPosition()) && !e.mods.isPopupMenu())
    {
        triggerClick();
    }
}

bool MultiScoperButton::keyPressed(const juce::KeyPress& key)
{
    // Do NOT handle space/return here. Plugin hosts (Ableton, Bitwig, etc.)
    // use spacebar for transport start/stop; consuming those keys here would
    // swallow them before they reach the DAW. Activation is mouse-only,
    // except for explicit setShortcut() bindings below.
    if (enabled_ && shortcutKey_.isValid() && key == shortcutKey_)
    {
        triggerClick();
        return true;
    }

    return false;
}

void MultiScoperButton::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperButton::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperButton::triggerClick()
{
    // Disabled buttons must not fire their click or flip their toggle state,
    // even when triggered programmatically. All in-tree callers (mouseUp,
    // keyPressed, the a11y handler) already gate on isEnabled() — this guard
    // makes the API contract explicit for third-party callers, tests, and
    // automation tooling so nothing bypasses the visual "disabled" state.
    if (!enabled_)
        return;

    if (toggleable_)
    {
        setToggled(!isToggled_);
    }

    if (onClick)
        onClick();
}

void MultiScoperButton::timerCallback()
{
    updateAnimations();
    rippleManager_.removeExpired();

    if (scaleSpring_.isSettled() && brightnessSpring_.isSettled() && !rippleManager_.hasActiveRipples())
        stopTimer();

    repaint();
}

void MultiScoperButton::updateAnimations()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;

    scaleSpring_.update(dt);
    brightnessSpring_.update(dt);

    currentScale_ = scaleSpring_.position;
    currentBrightness_ = brightnessSpring_.position;
}

// Accessibility handler
class MultiScoperButtonAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit MultiScoperButtonAccessibilityHandler(MultiScoperButton& button)
        : juce::AccessibilityHandler(
              button, button.isToggleable() ? juce::AccessibilityRole::toggleButton : juce::AccessibilityRole::button,
              juce::AccessibilityActions().addAction(juce::AccessibilityActionType::press,
                                                     [&button] {
                                                         if (button.isEnabled())
                                                             button.triggerClick();
                                                     }))
        , button_(button)
    {
    }

    juce::String getTitle() const override { return button_.getText().isNotEmpty() ? button_.getText() : "Button"; }

    juce::String getDescription() const override
    {
        juce::String desc;

        if (!button_.isEnabled())
            desc = "Disabled";
        else if (button_.isToggleable())
            desc = button_.isToggled() ? "Selected" : "Not selected";

        if (button_.getShortcut().isValid())
        {
            if (desc.isNotEmpty())
                desc += ". ";
            desc += "Shortcut: " + button_.getShortcut().getTextDescription();
        }

        return desc;
    }

    juce::String getHelp() const override { return "Press Enter or Space to activate."; }

    juce::AccessibleState getCurrentState() const override
    {
        auto state = AccessibilityHandler::getCurrentState();
        if (button_.isToggleable() && button_.isToggled())
            state = state.withChecked();
        return state;
    }

private:
    MultiScoperButton& button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScoperButtonAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> MultiScoperButton::createAccessibilityHandler()
{
    return std::make_unique<MultiScoperButtonAccessibilityHandler>(*this);
}

} // namespace multiscoper
