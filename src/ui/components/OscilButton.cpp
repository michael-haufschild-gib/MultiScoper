/*
    Oscil - Button Component Implementation
    Migrated to JUCE Animator for VBlank-synced animations
*/

#include "ui/components/OscilButton.h"
#include "plugin/PluginEditor.h"
#include <cmath>

namespace oscil
{

OscilButton::OscilButton(IThemeService& themeService, const juce::String& text)
    : ThemedComponent(themeService)
    , label_(text)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    currentScale_ = 1.0f;
    currentBrightness_ = 0.0f;
}

OscilButton::OscilButton(IThemeService& themeService, const juce::String& text, const juce::String& testId)
    : OscilButton(themeService, text)
{
    setTestId(testId);
}

OscilButton::OscilButton(IThemeService& themeService, const juce::Image& icon)
    : OscilButton(themeService, juce::String{})
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
    scaleAnimator_.reset();
    brightnessAnimator_.reset();
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
    // Handle edge case: empty or whitespace-only tooltip
    juce::String sanitized = tooltip.trim();
    if (sanitized.isEmpty())
    {
        tooltipText_ = {};
        setHelpText({});
        return;
    }

    // Truncate very long tooltips to prevent rendering issues
    static constexpr int MAX_TOOLTIP_LENGTH = 500;
    if (sanitized.length() > MAX_TOOLTIP_LENGTH)
    {
        sanitized = sanitized.substring(0, MAX_TOOLTIP_LENGTH - 3) + "...";
    }

    tooltipText_ = sanitized;
    setHelpText(sanitized);
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

        if (notify && onToggleStateChanged)
            onToggleStateChanged(isToggled_);
    }
}

void OscilButton::setBorder(juce::Colour color, float thickness)
{
    constexpr float epsilon = 1e-6f;
    if (borderColor_ != color || std::abs(borderWidth_ - thickness) > epsilon)
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
        return ComponentLayout::BUTTON_ICON_SIZE;

    auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
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
    // Guard against zero-size painting
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

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

void OscilButton::updatePathCache(const juce::Rectangle<float>& bounds)
{
    if (bounds == cachedPathBounds_ && segmentPosition_ == cachedSegmentPosition_)
        return;

    cachedPathBounds_ = bounds;
    cachedSegmentPosition_ = segmentPosition_;

    cachedButtonPath_.clear();
    float cornerRadius = variant_ == ButtonVariant::Icon
        ? ComponentLayout::RADIUS_MD
        : ComponentLayout::RADIUS_LG;

    if (segmentPosition_ == SegmentPosition::None || segmentPosition_ == SegmentPosition::Only)
    {
        cachedButtonPath_.addRoundedRectangle(bounds, cornerRadius);
    }
    else if (segmentPosition_ == SegmentPosition::First)
    {
        cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        true, false, true, false);
    }
    else if (segmentPosition_ == SegmentPosition::Last)
    {
        cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        false, true, false, true);
    }
    else
    {
        cachedButtonPath_.addRectangle(bounds);
    }
}

void OscilButton::paintButton(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    updatePathCache(bounds);

    auto bgColour = getBackgroundColour();
    if (std::abs(currentBrightness_) > 1e-6f)
        bgColour = bgColour.brighter(currentBrightness_);

    if (!enabled_)
        bgColour = bgColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    if (variant_ != ButtonVariant::Ghost && variant_ != ButtonVariant::Tertiary && variant_ != ButtonVariant::Icon)
    {
        g.setColour(bgColour);
        g.fillPath(cachedButtonPath_);
    }
    else if ((isHovered_ || isPressed_ || isToggled_) && (variant_ == ButtonVariant::Ghost || variant_ == ButtonVariant::Tertiary || variant_ == ButtonVariant::Icon))
    {
        g.setColour(bgColour);
        g.fillPath(cachedButtonPath_);
    }

    if (segmentPosition_ != SegmentPosition::None || variant_ == ButtonVariant::Secondary)
    {
        g.setColour(getBorderColour());
        g.strokePath(cachedButtonPath_, juce::PathStrokeType(1.0f));
    }

    if (borderWidth_ > 0.0f && borderColor_.getAlpha() > 0)
    {
        g.setColour(borderColor_);
        g.strokePath(cachedButtonPath_, juce::PathStrokeType(borderWidth_));
    }

    int horizontalPadding;
    if (segmentPosition_ != SegmentPosition::None)
    {
        horizontalPadding = ComponentLayout::BUTTON_SEGMENT_PADDING;
    }
    else if (bounds.getWidth() < TEXT_PADDING * 2.5f)
    {
        horizontalPadding = std::max(8, static_cast<int>(bounds.getWidth() * ComponentLayout::BUTTON_NARROW_PADDING_RATIO));
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

    if (!iconPath_.isEmpty())
    {
        auto pathBounds = iconPath_.getBounds();
        float padding = static_cast<float>(ComponentLayout::SPACING_XS);
        float availableSize = std::max(1.0f, std::min(bounds.getWidth(), bounds.getHeight()) - padding * 2);

        float pathDim = std::max(0.001f, std::max(pathBounds.getWidth(), pathBounds.getHeight()));
        float scale = availableSize / pathDim;

        float offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        auto transform = juce::AffineTransform::scale(scale).translated(offsetX, offsetY);

        juce::Path scaledPath = iconPath_;
        scaledPath.applyTransform(transform);

        g.fillPath(scaledPath);
        return;
    }

    if (variant_ == ButtonVariant::Icon)
    {
        if (icon_.isValid())
        {
            float reduction = std::max(0.0f, (bounds.getWidth() - ICON_SIZE) / 2);
            auto iconBounds = bounds.reduced(reduction);
            g.drawImage(icon_, iconBounds, juce::RectanglePlacement::centred);
            return;
        }
    }

    auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
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

    g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        cornerRadius,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

juce::Colour OscilButton::getBackgroundColour() const
{
    const auto& theme = getTheme();

    if (toggleable_ && isToggled_)
        return theme.btnPrimaryBgActive;

    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgDisabled;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgDisabled;
            case ButtonVariant::Tertiary:  return theme.btnTertiaryBgDisabled;
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgDisabled;
            case ButtonVariant::Danger:    return theme.statusError.withAlpha(0.5f);
            case ButtonVariant::Icon:      return theme.backgroundSecondary.withAlpha(0.5f);
            default:                       return theme.btnPrimaryBgDisabled;
        }
    }

    if (isPressed_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgActive;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgActive;
            case ButtonVariant::Tertiary:  return theme.btnTertiaryBgActive;
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgActive;
            case ButtonVariant::Danger:    return theme.statusError.darker(0.2f);
            case ButtonVariant::Icon:      return theme.controlHighlight;
            default:                       return theme.btnPrimaryBgActive;
        }
    }

    if (isHovered_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgHover;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgHover;
            case ButtonVariant::Tertiary:  return theme.btnTertiaryBgHover;
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgHover;
            case ButtonVariant::Danger:    return theme.statusError.brighter(0.1f);
            case ButtonVariant::Icon:      return theme.controlHighlight;
            default:                       return theme.btnPrimaryBgHover;
        }
    }

    switch (variant_)
    {
        case ButtonVariant::Primary:   return theme.btnPrimaryBg;
        case ButtonVariant::Secondary: return theme.btnSecondaryBg;
        case ButtonVariant::Tertiary:  return theme.btnTertiaryBg;
        case ButtonVariant::Ghost:     return theme.btnTertiaryBg;
        case ButtonVariant::Danger:    return theme.statusError;
        case ButtonVariant::Icon:      return theme.backgroundSecondary;
        default:                       return theme.btnPrimaryBg;
    }
}

juce::Colour OscilButton::getTextColour() const
{
    if (toggleable_ && isToggled_)
        return getTheme().btnPrimaryTextActive;

    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return getTheme().btnPrimaryTextDisabled;
            case ButtonVariant::Secondary: return getTheme().btnSecondaryTextDisabled;
            case ButtonVariant::Tertiary:  return getTheme().btnTertiaryTextDisabled;
            case ButtonVariant::Ghost:     return getTheme().btnTertiaryTextDisabled;
            case ButtonVariant::Danger:    return getTheme().textSecondary;
            case ButtonVariant::Icon:      return getTheme().textSecondary;
            default:                       return getTheme().textSecondary;
        }
    }

    if (isPressed_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return getTheme().btnPrimaryTextActive;
            case ButtonVariant::Secondary: return getTheme().btnSecondaryTextActive;
            case ButtonVariant::Tertiary:  return getTheme().btnTertiaryTextActive;
            case ButtonVariant::Ghost:     return getTheme().btnTertiaryTextActive;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:      return getTheme().textHighlight;
            default:                       return getTheme().textHighlight;
        }
    }

    if (isHovered_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:   return getTheme().btnPrimaryTextHover;
            case ButtonVariant::Secondary: return getTheme().btnSecondaryTextHover;
            case ButtonVariant::Tertiary:  return getTheme().btnTertiaryTextHover;
            case ButtonVariant::Ghost:     return getTheme().btnTertiaryTextHover;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:      return getTheme().textHighlight;
            default:                       return getTheme().textHighlight;
        }
    }

    switch (variant_)
    {
        case ButtonVariant::Primary:   return getTheme().btnPrimaryText;
        case ButtonVariant::Secondary: return getTheme().btnSecondaryText;
        case ButtonVariant::Tertiary:  return getTheme().btnTertiaryText;
        case ButtonVariant::Ghost:     return getTheme().btnTertiaryText;
        case ButtonVariant::Danger:    return juce::Colours::white;
        case ButtonVariant::Icon:      return getTheme().textPrimary;
        default:                       return getTheme().btnPrimaryText;
    }
}

juce::Colour OscilButton::getBorderColour() const
{
    return getTheme().controlBorder;
}

void OscilButton::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
    {
        juce::Logger::writeToLog("OscilButton::resized() - Invalid dimensions: "
            + juce::String(getWidth()) + "x" + juce::String(getHeight()));
        return;
    }
}

void OscilButton::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

    isHovered_ = true;

    // Get animation service if needed
    if (animService_ == nullptr)
    {
        if (auto* editor = dynamic_cast<OscilPluginEditor*>(getTopLevelComponent()))
            animService_ = &editor->getAnimationService();
    }

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startScale = currentScale_;
        float startBrightness = currentBrightness_;
        float targetScale = ComponentLayout::HOVER_SCALE;
        float targetBrightness = ComponentLayout::HOVER_BRIGHTNESS_OFFSET;
        
        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [this, startScale, startBrightness, targetScale, targetBrightness](float progress) {
                currentScale_ = startScale + (targetScale - startScale) * progress;
                currentBrightness_ = startBrightness + (targetBrightness - startBrightness) * progress;
                repaint();
            },
            juce::Easings::createEase());
        
        scaleAnimator_.set(std::move(animator));
        scaleAnimator_.start();
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

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startScale = currentScale_;
        float startBrightness = currentBrightness_;
        
        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [this, startScale, startBrightness](float progress) {
                currentScale_ = startScale + (1.0f - startScale) * progress;
                currentBrightness_ = startBrightness * (1.0f - progress);
                repaint();
            },
            juce::Easings::createEase());
        
        scaleAnimator_.set(std::move(animator));
        scaleAnimator_.start();
    }
    else
    {
        currentScale_ = 1.0f;
        currentBrightness_ = 0.0f;
        repaint();
    }
}

void OscilButton::mouseDown(const juce::MouseEvent&)
{
    if (!enabled_) return;

    isPressed_ = true;

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startScale = currentScale_;
        float startBrightness = currentBrightness_;
        float targetScale = ComponentLayout::PRESS_SCALE;
        float targetBrightness = ComponentLayout::PRESS_BRIGHTNESS_OFFSET;
        
        auto animator = animService_->createValueAnimation(
            AnimationPresets::SNAPPY_DURATION_MS,
            [this, startScale, startBrightness, targetScale, targetBrightness](float progress) {
                currentScale_ = startScale + (targetScale - startScale) * progress;
                currentBrightness_ = startBrightness + (targetBrightness - startBrightness) * progress;
                repaint();
            },
            juce::Easings::createEaseOut());
        
        scaleAnimator_.set(std::move(animator));
        scaleAnimator_.start();
    }
    else
    {
        currentScale_ = ComponentLayout::PRESS_SCALE;
        currentBrightness_ = ComponentLayout::PRESS_BRIGHTNESS_OFFSET;
        repaint();
    }
}

void OscilButton::mouseUp(const juce::MouseEvent& e)
{
    if (!enabled_) return;

    bool wasPressed = isPressed_;
    isPressed_ = false;

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startScale = currentScale_;
        float startBrightness = currentBrightness_;
        float targetScale = isHovered_ ? ComponentLayout::HOVER_SCALE : 1.0f;
        float targetBrightness = isHovered_ ? ComponentLayout::HOVER_BRIGHTNESS_OFFSET : 0.0f;
        
        auto animator = animService_->createValueAnimation(
            AnimationPresets::SNAPPY_DURATION_MS,
            [this, startScale, startBrightness, targetScale, targetBrightness](float progress) {
                currentScale_ = startScale + (targetScale - startScale) * progress;
                currentBrightness_ = startBrightness + (targetBrightness - startBrightness) * progress;
                repaint();
            },
            juce::Easings::createEaseOut());
        
        scaleAnimator_.set(std::move(animator));
        scaleAnimator_.start();
    }
    else
    {
        currentScale_ = isHovered_ ? ComponentLayout::HOVER_SCALE : 1.0f;
        currentBrightness_ = isHovered_ ? ComponentLayout::HOVER_BRIGHTNESS_OFFSET : 0.0f;
        repaint();
    }

    if (wasPressed && contains(e.getPosition()))
    {
        if (e.mods.isPopupMenu())
        {
            if (onRightClick)
                onRightClick();
        }
        else
        {
            triggerClick();
        }
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
        setToggled(!isToggled_);

    if (onClick)
        onClick();
}

// Accessibility handler
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
        // Don't show tooltip/help text for disabled buttons
        if (!button_.isEnabled())
            return {};

        // Return custom tooltip if set, otherwise default help text
        juce::String tooltip = button_.getTooltip();
        if (tooltip.isNotEmpty())
            return tooltip;

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
