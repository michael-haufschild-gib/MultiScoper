/*
    MultiScoper - Button Component Painting
    Flat-surface rendering, colour resolution, and path caching for MultiScoperButton.
*/

#include "ui/components/MultiScoperButton.h"
#include "ui/theme/ColorTheme.h"

#include <algorithm>

namespace multiscoper
{

void MultiScoperButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Scale animation
    if (std::abs(currentScale_ - 1.0f) > 0.001f)
    {
        bounds = bounds.withSizeKeepingCentre(bounds.getWidth() * currentScale_, bounds.getHeight() * currentScale_);
    }

    // Hover: slight upward shift (0.5px)
    if (isHovered_ && !isPressed_ && enabled_)
        bounds = bounds.translated(0.0f, -0.5f);

    // Active: slight downward shift (1px)
    if (isPressed_ && enabled_)
        bounds = bounds.translated(0.0f, 1.0f);

    paintButton(g, bounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, bounds);
}

void MultiScoperButton::updatePathCache(const juce::Rectangle<float>& bounds)
{
    cachedButtonPath_.clear();
    // Flat aesthetic: button corners are square. The variant + segment
    // branches collapse to plain rectangles.
    cachedButtonPath_.addRectangle(bounds);
}

void MultiScoperButton::paintButtonBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                              juce::Colour bgColour)
{
    juce::ignoreUnused(bounds);

    bool const isGhostType =
        (variant_ == ButtonVariant::Ghost || variant_ == ButtonVariant::Tertiary || variant_ == ButtonVariant::Icon);

    // Fill background
    if (!isGhostType || isHovered_ || isPressed_ || isToggled_)
    {
        g.setColour(bgColour);
        g.fillPath(cachedButtonPath_);
    }

    // Ripples (after background, before border/content)
    if (!rippleManager_.empty())
    {
        auto rippleColour = getTheme().textPrimary.withAlpha(0.1f);
        SurfacePainter::paintRipples(g, bounds, rippleManager_.getRipples(), rippleColour);
    }

    // Border
    auto borderCol = getBorderColour();
    if (borderCol.getAlpha() > 0)
    {
        g.setColour(borderCol);
        g.strokePath(cachedButtonPath_, juce::PathStrokeType(1.0f));
    }

    // Custom border override
    if (borderWidth_ > 0.0f && borderColor_.getAlpha() > 0)
    {
        g.setColour(borderColor_);
        g.strokePath(cachedButtonPath_, juce::PathStrokeType(borderWidth_));
    }
}

void MultiScoperButton::paintButtonContent(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                           const juce::Rectangle<float>& contentBounds, juce::Colour textColour)
{
    g.setColour(textColour);

    // Path-based icon (SVG). Icons ship as Lucide outline geometry — the path
    // is the *stroke line* itself, so we stroke with a thin brush instead of
    // filling a silhouette. Stroke width is pinned in screen pixels so icons
    // stay crisp at any button size.
    if (!iconPath_.isEmpty())
    {
        auto pathBounds = iconPath_.getBounds();
        auto const padding = iconPadding_;
        float const availableSize = std::max(1.0f, std::min(bounds.getWidth(), bounds.getHeight()) - (padding * 2));
        float const pathDim = std::max({0.001f, pathBounds.getWidth(), pathBounds.getHeight()});
        float const scale = availableSize / pathDim;
        float const offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float const offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        juce::Path scaledPath = iconPath_;
        scaledPath.applyTransform(juce::AffineTransform::scale(scale).translated(offsetX, offsetY));

        constexpr float kIconStrokeWidth = 1.25f;
        g.strokePath(scaledPath, juce::PathStrokeType(kIconStrokeWidth, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        return;
    }

    // Icon-only variant
    if (variant_ == ButtonVariant::Icon && icon_.isValid())
    {
        float const reduction = std::max(0.0f, (bounds.getWidth() - ICON_SIZE) / 2);
        g.drawImage(icon_, bounds.reduced(reduction), juce::RectanglePlacement::centred);
        return;
    }

    auto font = Typography::headingRegular();
    g.setFont(font);

    if (icon_.isValid())
        paintIconWithText(g, bounds, font);
    else
        g.drawText(label_, contentBounds, juce::Justification::centred);
}

void MultiScoperButton::paintIconWithText(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                          const juce::Font& font)
{
    float const iconY = (bounds.getHeight() - ICON_SIZE) / 2;
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, label_, 0, 0);
    float const textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();
    float const startX = (bounds.getWidth() - ICON_SIZE - ICON_PADDING - textWidth) / 2;

    if (iconOnLeft_)
    {
        g.drawImage(icon_, juce::Rectangle<float>(startX, iconY, ICON_SIZE, ICON_SIZE),
                    juce::RectanglePlacement::centred);
        g.drawText(label_, juce::Rectangle<float>(startX + ICON_SIZE + ICON_PADDING, 0, textWidth, bounds.getHeight()),
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

void MultiScoperButton::paintButton(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    updatePathCache(bounds);

    auto bgColour = getBackgroundColour();
    if (std::abs(currentBrightness_) > 1e-6f)
        bgColour = bgColour.brighter(currentBrightness_);
    if (!enabled_)
        bgColour = bgColour.withAlpha(ComponentLayout::DISABLED_OPACITY);

    paintButtonBackground(g, bounds, bgColour);

    int horizontalPadding = 0;
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

void MultiScoperButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Flat buttons → square focus ring matches the button outline.
    SurfacePainter::paintFocusRing(g, bounds, 0.0f, getSurface().accent);
}

// ---------------------------------------------------------------------------
// Colour resolution — glass-based
// ---------------------------------------------------------------------------

juce::Colour MultiScoperButton::getBackgroundColour() const
{
    const auto& glass = getSurface();
    const auto& theme = getTheme();

    // Flat Primary: solid accent fill, shifted slightly on hover
    // (lighter) and pressed (darker). No glass tinting.
    // Secondary / Ghost / Tertiary / Icon: transparent at rest,
    // faint white overlay on hover / pressed (surface-style feedback).

    // Toggled state (segmented button bars): behaves like a Primary.
    if (toggleable_ && isToggled_)
        return glass.accent;

    // Disabled
    if (!enabled_)
    {
        if (variant_ == ButtonVariant::Primary)
            return theme.controlBackground;
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.withAlpha(0.15f);
        return juce::Colours::transparentBlack;
    }

    // Pressed
    if (isPressed_)
    {
        if (variant_ == ButtonVariant::Primary)
            return glass.accent.darker(0.18f);
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.darker(0.15f);
        return glass.bgActive;
    }

    // Hovered
    if (isHovered_)
    {
        if (variant_ == ButtonVariant::Primary)
            return glass.accent.brighter(0.10f);
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.brighter(0.08f);
        return glass.bgHover;
    }

    // Default
    if (variant_ == ButtonVariant::Primary)
        return glass.accent;
    if (variant_ == ButtonVariant::Danger)
        return theme.statusError;
    return juce::Colours::transparentBlack;
}

juce::Colour MultiScoperButton::getTextColour() const
{
    const auto& glass = getSurface();
    const auto& theme = getTheme();

    // Primary / Danger / toggled buttons paint solid-accent / solid-error
    // backgrounds. Text must contrast with the bright fill (no alpha
    // compositing — the fill is opaque). Light themes set textHighlight to
    // BLACK, so the picker uses literal white/near-black by WCAG ratio.
    auto primaryTextColour = [&]() {
        return ColorTheme::pickContrastingText(ColorTheme::compositeOnBackground(glass.accent, theme.backgroundPane));
    };
    auto dangerTextColour = [&]() {
        return ColorTheme::pickContrastingText(
            ColorTheme::compositeOnBackground(theme.statusError, theme.backgroundPane));
    };

    // Toggled state — toggled segmented buttons render on accent tint,
    // so we need a contrast-safe text colour rather than the accent hue.
    if (toggleable_ && isToggled_)
        return primaryTextColour();

    // Disabled
    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:
                return primaryTextColour();
            case ButtonVariant::Danger:
                return dangerTextColour();
            case ButtonVariant::Secondary:
            case ButtonVariant::Ghost:
            case ButtonVariant::Tertiary:
            case ButtonVariant::Icon:
                return theme.textSecondary;
        }
    }

    switch (variant_)
    {
        case ButtonVariant::Primary:
            return primaryTextColour();
        case ButtonVariant::Danger:
            return dangerTextColour();
        case ButtonVariant::Secondary:
        case ButtonVariant::Ghost:
        case ButtonVariant::Tertiary:
        case ButtonVariant::Icon:
            return (isHovered_ || isPressed_) ? theme.textPrimary : theme.textSecondary;
    }

    jassertfalse;
    return theme.textPrimary;
}

juce::Colour MultiScoperButton::getBorderColour() const
{
    const auto& glass = getSurface();

    // Flat borders: Primary has no border (solid fill speaks for
    // itself), Secondary uses a 1px accent-tinted hairline that brightens
    // on hover, Ghost/Tertiary/Icon have no border.
    if (toggleable_ && isToggled_)
        return juce::Colours::transparentBlack;

    if (segmentPosition_ != SegmentPosition::None)
        return glass.borderDefault;

    switch (variant_)
    {
        case ButtonVariant::Secondary:
            return isHovered_ || hasFocus_ ? glass.accent.withAlpha(0.6f) : glass.borderDefault;
        case ButtonVariant::Primary:
        case ButtonVariant::Danger:
        case ButtonVariant::Ghost:
        case ButtonVariant::Tertiary:
        case ButtonVariant::Icon:
            return juce::Colours::transparentBlack;
    }

    return juce::Colours::transparentBlack;
}

} // namespace multiscoper
