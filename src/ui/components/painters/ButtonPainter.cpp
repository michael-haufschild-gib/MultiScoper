/*
    Oscil - Button Painter Implementation
*/

#include "ui/components/painters/ButtonPainter.h"
#include "ui/components/OscilButton.h"
#include <cmath>

namespace oscil
{

// Constants replicated or accessed
// ComponentLayout is available via OscilButton includes usually?
// OscilButton includes ComponentConstants.h

void ButtonPainter::paint(juce::Graphics& g, OscilButton& button)
{
    auto bounds = button.getLocalBounds().toFloat();

    // Apply scale animation
    if (std::abs(button.currentScale_ - 1.0f) > 0.001f)
    {
        auto scaledBounds = bounds.withSizeKeepingCentre(
            bounds.getWidth() * button.currentScale_,
            bounds.getHeight() * button.currentScale_
        );
        bounds = scaledBounds;
    }

    paintButton(g, button, bounds);

    if (button.hasFocus_ && button.enabled_)
        paintFocusRing(g, button, button.getLocalBounds().toFloat());
}

void ButtonPainter::updatePathCache(OscilButton& button, const juce::Rectangle<float>& bounds)
{
    button.cachedButtonPath_.clear();
    float cornerRadius = button.variant_ == ButtonVariant::Icon
        ? ComponentLayout::RADIUS_MD
        : ComponentLayout::RADIUS_LG;

    if (button.segmentPosition_ == SegmentPosition::None || button.segmentPosition_ == SegmentPosition::Only)
    {
        // Standard rounded rectangle
        button.cachedButtonPath_.addRoundedRectangle(bounds, cornerRadius);
    }
    else if (button.segmentPosition_ == SegmentPosition::First)
    {
        // Round left corners only
        button.cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        true, false, true, false);
    }
    else if (button.segmentPosition_ == SegmentPosition::Last)
    {
        // Round right corners only
        button.cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        false, true, false, true);
    }
    else // Middle
    {
        // No rounded corners
        button.cachedButtonPath_.addRectangle(bounds);
    }
}

void ButtonPainter::paintButton(juce::Graphics& g, OscilButton& button, const juce::Rectangle<float>& bounds)
{
    updatePathCache(button, bounds);

    // Background
    auto bgColour = getBackgroundColour(button);
    if (std::abs(button.currentBrightness_) > 1e-6f)
        bgColour = bgColour.brighter(button.currentBrightness_);

    if (!button.enabled_)
        bgColour = bgColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    // Helper for variants
    bool isGhostOrTertiaryOrIcon = (button.variant_ == ButtonVariant::Ghost || 
                                    button.variant_ == ButtonVariant::Tertiary || 
                                    button.variant_ == ButtonVariant::Icon);

    if (!isGhostOrTertiaryOrIcon)
    {
        g.setColour(bgColour);
        g.fillPath(button.cachedButtonPath_);
    }
    else if ((button.isHovered_ || button.isPressed_ || button.isToggled_) && isGhostOrTertiaryOrIcon)
    {
        g.setColour(bgColour);
        g.fillPath(button.cachedButtonPath_);
    }

    // Border
    if (button.segmentPosition_ != SegmentPosition::None || button.variant_ == ButtonVariant::Secondary)
    {
        g.setColour(getBorderColour(button));
        g.strokePath(button.cachedButtonPath_, juce::PathStrokeType(1.0f));
    }

    // Custom border
    if (button.borderWidth_ > 0.0f && button.borderColor_.getAlpha() > 0)
    {
        g.setColour(button.borderColor_);
        g.strokePath(button.cachedButtonPath_, juce::PathStrokeType(button.borderWidth_));
    }

    // Content
    int horizontalPadding;
    if (button.segmentPosition_ != SegmentPosition::None)
    {
        horizontalPadding = ComponentLayout::BUTTON_SEGMENT_PADDING;
    }
    else if (bounds.getWidth() < OscilButton::TEXT_PADDING * 2.5f)
    {
        horizontalPadding = std::max(8, static_cast<int>(bounds.getWidth() * ComponentLayout::BUTTON_NARROW_PADDING_RATIO));
    }
    else
    {
        horizontalPadding = OscilButton::TEXT_PADDING;
    }
    auto contentBounds = bounds.reduced(static_cast<float>(horizontalPadding), 0);
    auto textColour = getTextColour(button);
    if (!button.enabled_)
        textColour = textColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    g.setColour(textColour);

    // Path icon
    if (!button.iconPath_.isEmpty())
    {
        auto pathBounds = button.iconPath_.getBounds();
        float padding = static_cast<float>(ComponentLayout::SPACING_XS);
        float availableSize = std::max(1.0f, std::min(bounds.getWidth(), bounds.getHeight()) - padding * 2);

        float pathDim = std::max(0.001f, std::max(pathBounds.getWidth(), pathBounds.getHeight()));
        float scale = availableSize / pathDim;

        float offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        auto transform = juce::AffineTransform::scale(scale).translated(offsetX, offsetY);

        juce::Path scaledPath = button.iconPath_;
        scaledPath.applyTransform(transform);

        g.fillPath(scaledPath);
        return;
    }

    // Icon-only
    if (button.variant_ == ButtonVariant::Icon)
    {
        if (button.icon_.isValid())
        {
            float reduction = std::max(0.0f, (bounds.getWidth() - OscilButton::ICON_SIZE) / 2);
            auto iconBounds = bounds.reduced(reduction);
            g.drawImage(button.icon_, iconBounds, juce::RectanglePlacement::centred);
            return;
        }
    }

    // Text with optional icon
    // Fix deprecated constructor
    auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
    g.setFont(font);

    if (button.icon_.isValid())
    {
        float iconY = (bounds.getHeight() - OscilButton::ICON_SIZE) / 2;
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, button.label_, 0, 0);
        float textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

        if (button.iconOnLeft_)
        {
            float startX = (bounds.getWidth() - OscilButton::ICON_SIZE - OscilButton::ICON_PADDING - textWidth) / 2;
            g.drawImage(button.icon_,
                juce::Rectangle<float>(startX, iconY, OscilButton::ICON_SIZE, OscilButton::ICON_SIZE),
                juce::RectanglePlacement::centred);
            g.drawText(button.label_,
                juce::Rectangle<float>(startX + OscilButton::ICON_SIZE + OscilButton::ICON_PADDING, 0,
                                       textWidth, bounds.getHeight()),
                juce::Justification::centredLeft);
        }
        else
        {
            float startX = (bounds.getWidth() - OscilButton::ICON_SIZE - OscilButton::ICON_PADDING - textWidth) / 2;
            g.drawText(button.label_,
                juce::Rectangle<float>(startX, 0, textWidth, bounds.getHeight()),
                juce::Justification::centredLeft);
            g.drawImage(button.icon_,
                juce::Rectangle<float>(startX + textWidth + OscilButton::ICON_PADDING, iconY,
                                       OscilButton::ICON_SIZE, OscilButton::ICON_SIZE),
                juce::RectanglePlacement::centred);
        }
    }
    else
    {
        g.drawText(button.label_, contentBounds, juce::Justification::centred);
    }
}

void ButtonPainter::paintFocusRing(juce::Graphics& g, OscilButton& button, const juce::Rectangle<float>& bounds)
{
    float cornerRadius = button.variant_ == ButtonVariant::Icon
        ? ComponentLayout::RADIUS_MD + ComponentLayout::FOCUS_RING_OFFSET
        : ComponentLayout::RADIUS_LG + ComponentLayout::FOCUS_RING_OFFSET;

    g.setColour(button.getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(
        bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
        cornerRadius,
        ComponentLayout::FOCUS_RING_WIDTH
    );
}

juce::Colour ButtonPainter::getBackgroundColour(const OscilButton& button)
{
    const auto& theme = button.getTheme();

    if (button.toggleable_ && button.isToggled_)
    {
        return theme.btnPrimaryBgActive;
    }

    if (!button.enabled_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgDisabled;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgDisabled;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgDisabled;
            case ButtonVariant::Danger:    return theme.statusError.withAlpha(0.5f);
            case ButtonVariant::Icon:      return theme.backgroundSecondary.withAlpha(0.5f);
            default:                                    return theme.btnPrimaryBgDisabled;
        }
    }

    if (button.isPressed_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgActive;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgActive;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgActive;
            case ButtonVariant::Danger:    return theme.statusError.darker(0.2f);
            case ButtonVariant::Icon:      return theme.controlHighlight;
            default:                                    return theme.btnPrimaryBgActive;
        }
    }

    if (button.isHovered_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryBgHover;
            case ButtonVariant::Secondary: return theme.btnSecondaryBgHover;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryBgHover;
            case ButtonVariant::Danger:    return theme.statusError.brighter(0.1f);
            case ButtonVariant::Icon:      return theme.controlHighlight;
            default:                                    return theme.btnPrimaryBgHover;
        }
    }

    switch (button.variant_)
    {
        case ButtonVariant::Primary:   return theme.btnPrimaryBg;
        case ButtonVariant::Secondary: return theme.btnSecondaryBg;
        case ButtonVariant::Tertiary:
        case ButtonVariant::Ghost:     return theme.btnTertiaryBg;
        case ButtonVariant::Danger:    return theme.statusError;
        case ButtonVariant::Icon:      return theme.backgroundSecondary;
        default:                                    return theme.btnPrimaryBg;
    }
}

juce::Colour ButtonPainter::getTextColour(const OscilButton& button)
{
    const auto& theme = button.getTheme();

    if (button.toggleable_ && button.isToggled_)
    {
        return theme.btnPrimaryTextActive;
    }

    if (!button.enabled_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryTextDisabled;
            case ButtonVariant::Secondary: return theme.btnSecondaryTextDisabled;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryTextDisabled;
            case ButtonVariant::Danger:
            case ButtonVariant::Icon:
            default:                       return theme.textSecondary;
        }
    }

    if (button.isPressed_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryTextActive;
            case ButtonVariant::Secondary: return theme.btnSecondaryTextActive;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryTextActive;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:
            default:                       return theme.textHighlight;
        }
    }

    if (button.isHovered_)
    {
        switch (button.variant_)
        {
            case ButtonVariant::Primary:   return theme.btnPrimaryTextHover;
            case ButtonVariant::Secondary: return theme.btnSecondaryTextHover;
            case ButtonVariant::Tertiary:
            case ButtonVariant::Ghost:     return theme.btnTertiaryTextHover;
            case ButtonVariant::Danger:    return juce::Colours::white;
            case ButtonVariant::Icon:
            default:                       return theme.textHighlight;
        }
    }

    switch (button.variant_)
    {
        case ButtonVariant::Primary:   return theme.btnPrimaryText;
        case ButtonVariant::Secondary: return theme.btnSecondaryText;
        case ButtonVariant::Tertiary:
        case ButtonVariant::Ghost:     return theme.btnTertiaryText;
        case ButtonVariant::Danger:    return juce::Colours::white;
        case ButtonVariant::Icon:      return theme.textPrimary;
        default:                                    return theme.btnPrimaryText;
    }
}

juce::Colour ButtonPainter::getBorderColour(const OscilButton& button)
{
    return button.getTheme().controlBorder;
}

} // namespace oscil
