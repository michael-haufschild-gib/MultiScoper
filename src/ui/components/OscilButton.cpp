/*
    Oscil - Button Component Implementation
*/

#include "ui/components/OscilButton.h"

namespace oscil
{

OscilButton::OscilButton(const juce::String& text)
    : label_(text)
    , scaleSpring_(SpringPresets::snappy())
    , brightnessSpring_(SpringPresets::stiff())
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    scaleSpring_.position = 1.0f;
    scaleSpring_.target = 1.0f;
    brightnessSpring_.position = 0.0f;
    brightnessSpring_.target = 0.0f;
}

OscilButton::OscilButton(const juce::String& text, const juce::String& testId)
    : OscilButton(text)
{
    setTestId(testId);
}

OscilButton::OscilButton(const juce::Image& icon)
    : OscilButton(juce::String{})
{
    icon_ = icon;
    variant_ = ButtonVariant::Icon;
}

void OscilButton::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilButton::~OscilButton()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

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
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
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

void OscilButton::setShortcut(const juce::KeyPress& key)
{
    shortcutKey_ = key;
}

void OscilButton::setTooltip(const juce::String& tooltip)
{
    tooltipText_ = tooltip;
    setHelpText(tooltip);
}

void OscilButton::setToggleable(bool toggleable)
{
    toggleable_ = toggleable;
}

void OscilButton::setToggled(bool toggled, bool notify)
{
    if (isToggled_ != toggled)
    {
        isToggled_ = toggled;
        repaint();

        if (notify && onToggle)
            onToggle(isToggled_);
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
        return ComponentLayout::BUTTON_ICON_SIZE;

    auto font = juce::Font(juce::FontOptions().withHeight(14.0f));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, label_, 0, 0);
    int textWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
    int iconWidth = icon_.isValid() ? static_cast<int>(ICON_SIZE + ICON_PADDING) : 0;

    return std::max(ComponentLayout::BUTTON_MIN_WIDTH,
                    textWidth + iconWidth + TEXT_PADDING * 2);
}

int OscilButton::getPreferredHeight() const
{
    if (variant_ == ButtonVariant::Icon)
        return ComponentLayout::BUTTON_ICON_SIZE;

    return ComponentLayout::BUTTON_HEIGHT;
}

void OscilButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Apply scale animation
    if (std::abs(currentScale_ - 1.0f) > 0.001f)
    {
        auto scaledBounds = bounds.withSizeKeepingCentre(
            bounds.getWidth() * currentScale_,
            bounds.getHeight() * currentScale_
        );
        bounds = scaledBounds;
    }

    paintButton(g, bounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, getLocalBounds().toFloat());
}

void OscilButton::paintButton(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float cornerRadius = variant_ == ButtonVariant::Icon
        ? ComponentLayout::RADIUS_MD
        : ComponentLayout::RADIUS_LG;

    // Background
    auto bgColour = getBackgroundColour();
    if (std::abs(currentBrightness_) > 1e-6f)
        bgColour = bgColour.brighter(currentBrightness_);

    if (!enabled_)
        bgColour = bgColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    // Create path based on segment position
    juce::Path buttonPath;

    if (segmentPosition_ == SegmentPosition::None || segmentPosition_ == SegmentPosition::Only)
    {
        // Standard rounded rectangle
        buttonPath.addRoundedRectangle(bounds, cornerRadius);
    }
    else if (segmentPosition_ == SegmentPosition::First)
    {
        // Round left corners only
        buttonPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        true, false, true, false);
    }
    else if (segmentPosition_ == SegmentPosition::Last)
    {
        // Round right corners only
        buttonPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        false, true, false, true);
    }
    else // Middle
    {
        // No rounded corners
        buttonPath.addRectangle(bounds);
    }

    if (variant_ != ButtonVariant::Ghost && variant_ != ButtonVariant::Tertiary && variant_ != ButtonVariant::Icon)
    {
        g.setColour(bgColour);
        g.fillPath(buttonPath);
    }
    else if ((isHovered_ || isPressed_ || isToggled_) && (variant_ == ButtonVariant::Ghost || variant_ == ButtonVariant::Tertiary || variant_ == ButtonVariant::Icon))
    {
        g.setColour(bgColour);
        g.fillPath(buttonPath);
    }

    // Border for segmented buttons and Secondary variant (for visual definition)
    if (segmentPosition_ != SegmentPosition::None || variant_ == ButtonVariant::Secondary)
    {
        g.setColour(getBorderColour());
        g.strokePath(buttonPath, juce::PathStrokeType(1.0f));
    }

    // Content - use reduced padding for segmented buttons and narrow buttons
    int horizontalPadding;
    if (segmentPosition_ != SegmentPosition::None)
    {
        horizontalPadding = 6;  // Slightly more padding for better readability
    }
    else if (bounds.getWidth() < TEXT_PADDING * 2.5f)
    {
        // For narrow buttons, use proportional padding (min 8px for better readability)
        horizontalPadding = std::max(8, static_cast<int>(bounds.getWidth() * 0.15f));
    }
    else
    {
        horizontalPadding = TEXT_PADDING;
    }
    auto contentBounds = bounds.reduced(static_cast<float>(horizontalPadding), 0);
    auto textColour = getTextColour();
    if (!enabled_)
        textColour = textColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    g.setColour(textColour);

    // Path icon rendering (takes precedence over image icon and text)
    if (!iconPath_.isEmpty())
    {
        auto pathBounds = iconPath_.getBounds();
        float padding = 4.0f;
        float availableSize = std::min(bounds.getWidth(), bounds.getHeight()) - padding * 2;

        // Scale path to fit within available space
        float scale = availableSize / std::max(pathBounds.getWidth(), pathBounds.getHeight());

        // Center the path within the bounds
        float offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        auto transform = juce::AffineTransform::scale(scale).translated(offsetX, offsetY);

        juce::Path scaledPath = iconPath_;
        scaledPath.applyTransform(transform);

        g.fillPath(scaledPath);
        return;
    }

    // Icon-only button (image-based)
    if (variant_ == ButtonVariant::Icon)
    {
        if (icon_.isValid())
        {
            auto iconBounds = bounds.reduced((bounds.getWidth() - ICON_SIZE) / 2);
            g.drawImage(icon_, iconBounds, juce::RectanglePlacement::centred);
            return;
        }
        // Fall through to render text label if no image icon is set
        // This allows Icon variant buttons to use text labels (e.g., Unicode symbols)
    }

    // Text with optional icon
    auto font = juce::Font(juce::FontOptions().withHeight(14.0f));
    g.setFont(font);

    if (icon_.isValid())
    {
        float iconY = (bounds.getHeight() - ICON_SIZE) / 2;
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        float textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

        if (iconOnLeft_)
        {
            float startX = (bounds.getWidth() - ICON_SIZE - ICON_PADDING - textWidth) / 2;
            g.drawImage(icon_,
                juce::Rectangle<float>(startX, iconY, ICON_SIZE, ICON_SIZE),
                juce::RectanglePlacement::centred);
            g.drawText(label_,
                juce::Rectangle<float>(startX + ICON_SIZE + ICON_PADDING, 0,
                                       textWidth, bounds.getHeight()),
                juce::Justification::centredLeft);
        }
        else
        {
            float startX = (bounds.getWidth() - ICON_SIZE - ICON_PADDING - textWidth) / 2;
            g.drawText(label_,
                juce::Rectangle<float>(startX, 0, textWidth, bounds.getHeight()),
                juce::Justification::centredLeft);
            g.drawImage(icon_,
                juce::Rectangle<float>(startX + textWidth + ICON_PADDING, iconY,
                                       ICON_SIZE, ICON_SIZE),
                juce::RectanglePlacement::centred);
        }
    }
    else
    {
        g.drawText(label_, contentBounds, juce::Justification::centred);
    }
}

void OscilButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float cornerRadius = variant_ == ButtonVariant::Icon
        ? ComponentLayout::RADIUS_MD + ComponentLayout::FOCUS_RING_OFFSET
        : ComponentLayout::RADIUS_LG + ComponentLayout::FOCUS_RING_OFFSET;

    g.setColour(theme_.controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        cornerRadius,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

juce::Colour OscilButton::getBackgroundColour() const
{
    // Handle toggled state for toggleable buttons
    if (toggleable_ && isToggled_)
    {
        return theme_.btnPrimaryBgActive; 
    }

    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryBgDisabled;
            case ButtonVariant::Secondary: return theme_.btnSecondaryBgDisabled;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryBgDisabled;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryBgDisabled;
            case ButtonVariant::Danger:    return theme_.statusError.withAlpha(0.5f); // Fallback
            case ButtonVariant::Icon:      return theme_.backgroundSecondary.withAlpha(0.5f);
            default:                       return theme_.btnPrimaryBgDisabled;
        }
    }

    if (isPressed_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryBgActive;
            case ButtonVariant::Secondary: return theme_.btnSecondaryBgActive;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryBgActive;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryBgActive;
            case ButtonVariant::Danger:    return theme_.statusError.darker(0.2f);
            case ButtonVariant::Icon:      return theme_.controlHighlight;
            default:                       return theme_.btnPrimaryBgActive;
        }
    }

    if (isHovered_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryBgHover;
            case ButtonVariant::Secondary: return theme_.btnSecondaryBgHover;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryBgHover;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryBgHover;
            case ButtonVariant::Danger:    return theme_.statusError.brighter(0.1f);
            case ButtonVariant::Icon:      return theme_.controlHighlight;
            default:                       return theme_.btnPrimaryBgHover;
        }
    }

    // Default state
    switch (variant_)
    {
        case ButtonVariant::Primary:   return theme_.btnPrimaryBg;
        case ButtonVariant::Secondary: return theme_.btnSecondaryBg;
        case ButtonVariant::Tertiary:  return theme_.btnTertiaryBg;
        case ButtonVariant::Ghost:     return theme_.btnTertiaryBg; // Ghost is alias for Tertiary style
        case ButtonVariant::Danger:    return theme_.statusError;
        case ButtonVariant::Icon:      return theme_.backgroundSecondary; // Default icon bg
        default:                       return theme_.btnPrimaryBg;
    }
}

juce::Colour OscilButton::getTextColour() const
{
    // Handle toggled state for toggleable buttons
    if (toggleable_ && isToggled_)
    {
        return theme_.btnPrimaryTextActive;
    }

    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryTextDisabled;
            case ButtonVariant::Secondary: return theme_.btnSecondaryTextDisabled;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryTextDisabled;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryTextDisabled;
            case ButtonVariant::Danger:    return theme_.textSecondary;
            case ButtonVariant::Icon:      return theme_.textSecondary;
            default:                       return theme_.textSecondary;
        }
    }

    if (isPressed_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryTextActive;
            case ButtonVariant::Secondary: return theme_.btnSecondaryTextActive;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryTextActive;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryTextActive;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:      return theme_.textHighlight;
            default:                       return theme_.textHighlight;
        }
    }

    if (isHovered_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme_.btnPrimaryTextHover;
            case ButtonVariant::Secondary: return theme_.btnSecondaryTextHover;
            case ButtonVariant::Tertiary:  return theme_.btnTertiaryTextHover;
            case ButtonVariant::Ghost:     return theme_.btnTertiaryTextHover;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:      return theme_.textHighlight;
            default:                       return theme_.textHighlight;
        }
    }

    // Default state
    switch (variant_)
    {
        case ButtonVariant::Primary:   return theme_.btnPrimaryText;
        case ButtonVariant::Secondary: return theme_.btnSecondaryText;
        case ButtonVariant::Tertiary:  return theme_.btnTertiaryText;
        case ButtonVariant::Ghost:     return theme_.btnTertiaryText;
        case ButtonVariant::Danger:    return juce::Colours::white;
        case ButtonVariant::Icon:      return theme_.textPrimary;
        default:                       return theme_.btnPrimaryText;
    }
}

juce::Colour OscilButton::getBorderColour() const
{
    return theme_.controlBorder;
}

void OscilButton::resized()
{
    // No child components to layout
}

void OscilButton::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

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
    if (!enabled_) return;

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

    // Handle right-click
    if (e.mods.isPopupMenu() && onRightClick)
    {
        onRightClick();
    }
}

void OscilButton::mouseUp(const juce::MouseEvent& e)
{
    if (!enabled_) return;

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

    // Trigger click if released inside button
    if (wasPressed && contains(e.getPosition()) && !e.mods.isPopupMenu())
    {
        triggerClick();
    }
}

bool OscilButton::keyPressed(const juce::KeyPress& key)
{
    // Handle Enter or Space to activate
    if (enabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey))
    {
        triggerClick();
        return true;
    }

    // Handle shortcut key
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

void OscilButton::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

// Custom accessibility handler with descriptive text for screen readers
class OscilButtonAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilButtonAccessibilityHandler(OscilButton& button)
        : juce::AccessibilityHandler(button,
            button.isToggleable() ? juce::AccessibilityRole::toggleButton : juce::AccessibilityRole::button,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press,
                    [&button] { if (button.isEnabled()) button.triggerClick(); })
          )
        , button_(button)
    {
    }

    juce::String getTitle() const override
    {
        return button_.getText().isNotEmpty() ? button_.getText() : "Button";
    }

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

    juce::String getHelp() const override
    {
        return "Press Enter or Space to activate.";
    }

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
