/*
    Oscil - Button Component Painting
    Flat-surface rendering, colour resolution, and path caching for OscilButton.
    (Historical: this module was "glassmorphism rendering" prior to the 2026-Q2
    flat-surface uplift; field names such as `bgGlass` remain for ColorTheme
    serialization compatibility — see ui/components/SurfaceStyle.cpp.)
*/

#include "ui/components/OscilButton.h"
#include "ui/theme/ColorTheme.h"

#include <algorithm>

namespace oscil
{

void OscilButton::paint(juce::Graphics& g)
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

void OscilButton::updatePathCache(const juce::Rectangle<float>& bounds)
{
    cachedButtonPath_.clear();
    float const cornerRadius =
        variant_ == ButtonVariant::Icon ? ComponentLayout::RADIUS_MD : ComponentLayout::RADIUS_LG;

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

void OscilButton::paintButtonContent(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     const juce::Rectangle<float>& contentBounds, juce::Colour textColour)
{
    g.setColour(textColour);

    // Path-based icon (SVG)
    if (!iconPath_.isEmpty())
    {
        auto pathBounds = iconPath_.getBounds();
        auto const padding = static_cast<float>(ComponentLayout::SPACING_XS);
        float const availableSize = std::max(1.0f, std::min(bounds.getWidth(), bounds.getHeight()) - (padding * 2));
        float const pathDim = std::max({0.001f, pathBounds.getWidth(), pathBounds.getHeight()});
        float const scale = availableSize / pathDim;
        float const offsetX = bounds.getCentreX() - (pathBounds.getCentreX() * scale);
        float const offsetY = bounds.getCentreY() - (pathBounds.getCentreY() * scale);

        juce::Path scaledPath = iconPath_;
        scaledPath.applyTransform(juce::AffineTransform::scale(scale).translated(offsetX, offsetY));
        g.fillPath(scaledPath);
        return;
    }

    // Icon-only variant
    if (variant_ == ButtonVariant::Icon && icon_.isValid())
    {
        float const reduction = std::max(0.0f, (bounds.getWidth() - ICON_SIZE) / 2);
        g.drawImage(icon_, bounds.reduced(reduction), juce::RectanglePlacement::centred);
        return;
    }

    auto font = ComponentLayout::defaultFont();
    g.setFont(font);

    if (icon_.isValid())
        paintIconWithText(g, bounds, font);
    else
        g.drawText(label_, contentBounds, juce::Justification::centred);
}

void OscilButton::paintIconWithText(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::Font& font)
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

void OscilButton::paintButton(juce::Graphics& g, const juce::Rectangle<float>& bounds)
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

void OscilButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const cornerRadius =
        variant_ == ButtonVariant::Icon ? ComponentLayout::RADIUS_MD : ComponentLayout::RADIUS_LG;

    SurfacePainter::paintFocusRing(g, bounds, cornerRadius, getSurface().accent);
}

// ---------------------------------------------------------------------------
// Colour resolution — glass-based
// ---------------------------------------------------------------------------

juce::Colour OscilButton::getBackgroundColour() const
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

namespace
{
// Composite a semi-transparent tint over an opaque panel background and
// return the contrast-safe text colour for the resulting surface. Primary /
// Danger buttons paint accent-tinted or error-tinted backgrounds; using the
// same accent/error hue for text collapses hue contrast ("blue on blue") and
// can fail WCAG AA even when luminance contrast appears ok in isolation.
juce::Colour pickContrastingTextOver(juce::Colour tint, juce::Colour panelBg, const ColorTheme& theme)
{
    auto effective = ColorTheme::compositeOnBackground(tint, panelBg);
    // Threshold 0.4 — below this we treat surface as "dark" and use the
    // brightest token (textHighlight, usually white); above, use a dark
    // token. Exact fallbacks below keep contrast predictable across themes.
    if (ColorTheme::calculateLuminance(effective) < 0.4f)
        return theme.textHighlight;
    return juce::Colour(0xFF1A1A1A);
}
} // namespace

juce::Colour OscilButton::getTextColour() const
{
    const auto& glass = getSurface();
    const auto& theme = getTheme();

    // Primary / Danger / toggled buttons paint solid-accent / solid-error
    // backgrounds. Text must contrast with the bright fill (no alpha
    // compositing — the fill is opaque).
    auto primaryTextColour = [&]() { return pickContrastingTextOver(glass.accent, theme.backgroundPane, theme); };
    auto dangerTextColour = [&]() { return pickContrastingTextOver(theme.statusError, theme.backgroundPane, theme); };

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

juce::Colour OscilButton::getBorderColour() const
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

} // namespace oscil
