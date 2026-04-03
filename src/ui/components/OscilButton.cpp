/*
    Oscil - Button Component Implementation
    (Painting, path cache, and colour resolution are in OscilButtonPainting.cpp)
*/

#include "ui/components/OscilButton.h"

#include <utility>

namespace oscil
{

OscilButton::OscilButton(IThemeService& themeService, juce::String text)
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

OscilButton::OscilButton(IThemeService& themeService, juce::String text, const juce::String& testId)
    : OscilButton(themeService, std::move(text))
{
    setTestId(testId);
}

OscilButton::OscilButton(IThemeService& themeService, const juce::Image& icon)
    : OscilButton(themeService, juce::String())
{
    variant_ = ButtonVariant::Icon;
    icon_ = icon;
}

void OscilButton::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscilButton::~OscilButton() { stopTimer(); }

void OscilButton::setText(const juce::String& text)
{
    if (label_ != text)
    {
        label_ = text;
        repaint();
    }
}

void OscilButton::setVariant(ButtonVariant variant)
{
    if (variant_ != variant)
    {
        variant_ = variant;
        repaint();
    }
}

void OscilButton::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilButton::setIcon(const juce::Image& icon, bool iconOnLeft)
{
    icon_ = icon;
    iconOnLeft_ = iconOnLeft;
    repaint();
}

void OscilButton::clearIcon()
{
    icon_ = {};
    repaint();
}

void OscilButton::setIconPath(const juce::Path& path)
{
    iconPath_ = path;
    repaint();
}

void OscilButton::clearIconPath()
{
    iconPath_.clear();
    repaint();
}

void OscilButton::setShortcut(const juce::KeyPress& key) { shortcutKey_ = key; }

void OscilButton::setTooltip(const juce::String& tooltip)
{
    tooltipText_ = tooltip;
    setHelpText(tooltip);
}

void OscilButton::setToggleable(bool toggleable) { toggleable_ = toggleable; }

void OscilButton::setToggled(bool toggled, bool notify)
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

void OscilButton::setBorder(juce::Colour color, float thickness)
{
    if (borderColor_ != color || std::abs(borderWidth_ - thickness) > 1e-6f)
    {
        borderColor_ = color;
        borderWidth_ = thickness;
        repaint();
    }
}

void OscilButton::setSegmentPosition(SegmentPosition position)
{
    if (segmentPosition_ != position)
    {
        segmentPosition_ = position;
        repaint();
    }
}

int OscilButton::getPreferredWidth() const
{
    if (variant_ == ButtonVariant::Icon)
        return ComponentLayout::BUTTON_HEIGHT;

    auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, label_, 0, 0);
    float textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

    return static_cast<int>(textWidth) + TEXT_PADDING * 2;
}

int OscilButton::getPreferredHeight() const
{
    if (variant_ == ButtonVariant::Icon)
        return ComponentLayout::BUTTON_HEIGHT;

    return ComponentLayout::BUTTON_HEIGHT;
}

void OscilButton::resized() {}

void OscilButton::mouseEnter(const juce::MouseEvent&)
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

void OscilButton::mouseExit(const juce::MouseEvent&)
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

void OscilButton::mouseDown(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    isPressed_ = true;

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

void OscilButton::mouseUp(const juce::MouseEvent& e)
{
    if (!enabled_)
        return;

    bool wasPressed = isPressed_;
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

bool OscilButton::keyPressed(const juce::KeyPress& key)
{
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        triggerClick();
        return true;
    }

    if (enabled_ && shortcutKey_.isValid() && key == shortcutKey_)
    {
        triggerClick();
        return true;
    }

    return false;
}

void OscilButton::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilButton::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

void OscilButton::triggerClick()
{
    if (toggleable_)
    {
        setToggled(!isToggled_);
    }

    if (onClick)
        onClick();
}

void OscilButton::timerCallback()
{
    updateAnimations();

    if (scaleSpring_.isSettled() && brightnessSpring_.isSettled())
        stopTimer();

    repaint();
}

void OscilButton::updateAnimations()
{
    float dt = AnimationTiming::FRAME_DURATION_60FPS;

    scaleSpring_.update(dt);
    brightnessSpring_.update(dt);

    currentScale_ = scaleSpring_.position;
    currentBrightness_ = brightnessSpring_.position;
}

// Accessibility handler
class OscilButtonAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilButtonAccessibilityHandler(OscilButton& button)
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
    OscilButton& button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilButtonAccessibilityHandler)
};

std::unique_ptr<juce::AccessibilityHandler> OscilButton::createAccessibilityHandler()
{
    return std::make_unique<OscilButtonAccessibilityHandler>(*this);
}

} // namespace oscil
