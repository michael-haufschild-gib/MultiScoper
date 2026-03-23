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
        auto scaledBounds =
            bounds.withSizeKeepingCentre(bounds.getWidth() * currentScale_, bounds.getHeight() * currentScale_);
        bounds = scaledBounds;
    }

    paintButton(g, bounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, getLocalBounds().toFloat());
}

void OscilButton::updatePathCache(const juce::Rectangle<float>& bounds)
{
    cachedButtonPath_.clear();
    float cornerRadius = variant_ == ButtonVariant::Icon ? ComponentLayout::RADIUS_MD : ComponentLayout::RADIUS_LG;

    if (segmentPosition_ == SegmentPosition::None || segmentPosition_ == SegmentPosition::Only)
    {
        cachedButtonPath_.addRoundedRectangle(bounds, cornerRadius);
    }
    else if (segmentPosition_ == SegmentPosition::First)
    {
        cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                              cornerRadius, cornerRadius, true, false, true, false);
    }
    else if (segmentPosition_ == SegmentPosition::Last)
    {
        cachedButtonPath_.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                              cornerRadius, cornerRadius, false, true, false, true);
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
            g.drawImage(icon_, juce::Rectangle<float>(startX, iconY, ICON_SIZE, ICON_SIZE),
                        juce::RectanglePlacement::centred);
            g.drawText(label_,
                       juce::Rectangle<float>(startX + ICON_SIZE + ICON_PADDING, 0, textWidth, bounds.getHeight()),
                       juce::Justification::centredLeft);
        }
        else
        {
            g.drawText(label_, juce::Rectangle<float>(startX, 0, textWidth, bounds.getHeight()),
                       juce::Justification::centredLeft);
            g.drawImage(icon_, juce::Rectangle<float>(startX + textWidth + ICON_PADDING, iconY, ICON_SIZE, ICON_SIZE),
                        juce::RectanglePlacement::centred);
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
        horizontalPadding =
            std::max(8, static_cast<int>(bounds.getWidth() * ComponentLayout::BUTTON_NARROW_PADDING_RATIO));
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
    g.drawRoundedRectangle(bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET), cornerRadius,
                           ComponentLayout::FOCUS_RING_WIDTH);
}

namespace
{

struct VariantColours
{
    juce::Colour primary, secondary, tertiary, ghost, danger, icon;
};

juce::Colour resolveVariant(ButtonVariant v, const VariantColours& c)
{
    switch (v)
    {
        case ButtonVariant::Primary:
            return c.primary;
        case ButtonVariant::Secondary:
            return c.secondary;
        case ButtonVariant::Tertiary:
            return c.tertiary;
        case ButtonVariant::Ghost:
            return c.ghost;
        case ButtonVariant::Danger:
            return c.danger;
        case ButtonVariant::Icon:
            return c.icon;
    }
    jassertfalse;
    return c.primary;
}

} // namespace

juce::Colour OscilButton::getBackgroundColour() const
{
    const auto& t = getTheme();

    if (toggleable_ && isToggled_)
        return t.btnPrimaryBgActive;

    if (!enabled_)
        return resolveVariant(variant_, {t.btnPrimaryBgDisabled, t.btnSecondaryBgDisabled, t.btnTertiaryBgDisabled,
                                         t.btnTertiaryBgDisabled, t.statusError.withAlpha(0.5f),
                                         t.backgroundSecondary.withAlpha(0.5f)});

    if (isPressed_)
        return resolveVariant(variant_, {t.btnPrimaryBgActive, t.btnSecondaryBgActive, t.btnTertiaryBgActive,
                                         t.btnTertiaryBgActive, t.statusError.darker(0.2f), t.controlHighlight});

    if (isHovered_)
        return resolveVariant(variant_, {t.btnPrimaryBgHover, t.btnSecondaryBgHover, t.btnTertiaryBgHover,
                                         t.btnTertiaryBgHover, t.statusError.brighter(0.1f), t.controlHighlight});

    return resolveVariant(variant_, {t.btnPrimaryBg, t.btnSecondaryBg, t.btnTertiaryBg, t.btnTertiaryBg, t.statusError,
                                     t.backgroundSecondary});
}

juce::Colour OscilButton::getTextColour() const
{
    const auto& t = getTheme();

    if (toggleable_ && isToggled_)
        return t.btnPrimaryTextActive;

    if (!enabled_)
        return resolveVariant(variant_,
                              {t.btnPrimaryTextDisabled, t.btnSecondaryTextDisabled, t.btnTertiaryTextDisabled,
                               t.btnTertiaryTextDisabled, t.textSecondary, t.textSecondary});

    if (isPressed_)
        return resolveVariant(variant_, {t.btnPrimaryTextActive, t.btnSecondaryTextActive, t.btnTertiaryTextActive,
                                         t.btnTertiaryTextActive, juce::Colours::white, t.textHighlight});

    if (isHovered_)
        return resolveVariant(variant_, {t.btnPrimaryTextHover, t.btnSecondaryTextHover, t.btnTertiaryTextHover,
                                         t.btnTertiaryTextHover, juce::Colours::white, t.textHighlight});

    return resolveVariant(variant_, {t.btnPrimaryText, t.btnSecondaryText, t.btnTertiaryText, t.btnTertiaryText,
                                     juce::Colours::white, t.textPrimary});
}

juce::Colour OscilButton::getBorderColour() const { return getTheme().controlBorder; }

} // namespace oscil
