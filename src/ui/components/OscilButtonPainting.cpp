/*
    Oscil - Button Component Painting
    Rendering, colour resolution, and path caching for OscilButton
*/

#include "ui/components/OscilButton.h"

namespace oscil
{

void OscilButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

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

void OscilButton::paintButtonBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds, juce::Colour bgColour)
{
    juce::ignoreUnused(bounds);

    if (variant_ != ButtonVariant::Ghost && variant_ != ButtonVariant::Tertiary && variant_ != ButtonVariant::Icon)
    {
        g.setColour(bgColour);
        g.fillPath(cachedButtonPath_);
    }
    else if (isHovered_ || isPressed_ || isToggled_)
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
}

void OscilButton::paintButtonContent(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     const juce::Rectangle<float>& contentBounds, juce::Colour textColour)
{
    g.setColour(textColour);

    // Path-based icon (SVG)
    if (!iconPath_.isEmpty())
    {
        auto pathBounds = iconPath_.getBounds();
        float padding = static_cast<float>(ComponentLayout::SPACING_XS);
        float availableSize = std::max(1.0f, std::min(bounds.getWidth(), bounds.getHeight()) - padding * 2);
        float pathDim = std::max(0.001f, std::max(pathBounds.getWidth(), pathBounds.getHeight()));
        float scale = availableSize / pathDim;
        float offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        juce::Path scaledPath = iconPath_;
        scaledPath.applyTransform(juce::AffineTransform::scale(scale).translated(offsetX, offsetY));
        g.fillPath(scaledPath);
        return;
    }

    // Icon-only variant
    if (variant_ == ButtonVariant::Icon && icon_.isValid())
    {
        float reduction = std::max(0.0f, (bounds.getWidth() - ICON_SIZE) / 2);
        g.drawImage(icon_, bounds.reduced(reduction), juce::RectanglePlacement::centred);
        return;
    }

    auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
    g.setFont(font);

    // Text + icon
    if (icon_.isValid())
    {
        float iconY = (bounds.getHeight() - ICON_SIZE) / 2;
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        float textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();
        float startX = (bounds.getWidth() - ICON_SIZE - ICON_PADDING - textWidth) / 2;

        if (iconOnLeft_)
        {
            g.drawImage(icon_, juce::Rectangle<float>(startX, iconY, ICON_SIZE, ICON_SIZE), juce::RectanglePlacement::centred);
            g.drawText(label_, juce::Rectangle<float>(startX + ICON_SIZE + ICON_PADDING, 0, textWidth, bounds.getHeight()), juce::Justification::centredLeft);
        }
        else
        {
            g.drawText(label_, juce::Rectangle<float>(startX, 0, textWidth, bounds.getHeight()), juce::Justification::centredLeft);
            g.drawImage(icon_, juce::Rectangle<float>(startX + textWidth + ICON_PADDING, iconY, ICON_SIZE, ICON_SIZE), juce::RectanglePlacement::centred);
        }
    }
    else
    {
        g.drawText(label_, contentBounds, juce::Justification::centred);
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

    paintButtonBackground(g, bounds, bgColour);

    int horizontalPadding;
    if (segmentPosition_ != SegmentPosition::None)
        horizontalPadding = ComponentLayout::BUTTON_SEGMENT_PADDING;
    else if (bounds.getWidth() < TEXT_PADDING * 2.5f)
        horizontalPadding = std::max(8, static_cast<int>(bounds.getWidth() * ComponentLayout::BUTTON_NARROW_PADDING_RATIO));
    else
        horizontalPadding = TEXT_PADDING;

    auto contentBounds = bounds.reduced(static_cast<float>(horizontalPadding), 0);
    auto textColour = getTextColour();
    if (!enabled_)
        textColour = textColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    paintButtonContent(g, bounds, contentBounds, textColour);
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
    }
    jassertfalse;
    return theme.btnPrimaryBg;
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
    }
    jassertfalse;
    return getTheme().btnPrimaryText;
}

juce::Colour OscilButton::getBorderColour() const
{
    return getTheme().controlBorder;
}

} // namespace oscil
