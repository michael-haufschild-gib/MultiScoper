/*
    Oscil - Button Component Painting
    Glassmorphism rendering, colour resolution, and path caching for OscilButton
*/

#include "ui/components/OscilButton.h"

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
    const auto& glass = getGlass();

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
        GlassPainter::paintRipples(g, bounds, rippleManager_.getRipples(), rippleColour);
    }

    // Primary hover: accent glow behind the button
    if (variant_ == ButtonVariant::Primary && isHovered_ && enabled_)
    {
        GlassPainter::paintAccentGlow(g, bounds, glass.accentGlow, glass.accentGlowRadius,
                                      glass.accentGlowAlpha * 0.5f);
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

    GlassPainter::paintFocusRing(g, bounds, cornerRadius, getGlass().accent);
}

// ---------------------------------------------------------------------------
// Colour resolution — glass-based
// ---------------------------------------------------------------------------

juce::Colour OscilButton::getBackgroundColour() const
{
    const auto& glass = getGlass();
    const auto& theme = getTheme();

    // Toggled state (segmented button bars)
    if (toggleable_ && isToggled_)
        return glass.accentSubtle;

    // Disabled — same base color at reduced opacity (handled by caller)
    if (!enabled_)
    {
        if (variant_ == ButtonVariant::Primary)
            return glass.accentSubtle;
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.withAlpha(0.15f);
        return juce::Colours::transparentBlack;
    }

    // Active / pressed
    if (isPressed_)
    {
        if (variant_ == ButtonVariant::Primary)
            return glass.accent.withAlpha(0.25f);
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.withAlpha(0.20f);
        return glass.bgActive;
    }

    // Hovered
    if (isHovered_)
    {
        if (variant_ == ButtonVariant::Primary)
            return glass.accentMuted;
        if (variant_ == ButtonVariant::Danger)
            return theme.statusError.withAlpha(0.15f);
        return glass.bgHover;
    }

    // Default — only Primary has a visible background
    if (variant_ == ButtonVariant::Primary)
        return glass.accentSubtle;
    return juce::Colours::transparentBlack;
}

juce::Colour OscilButton::getTextColour() const
{
    const auto& glass = getGlass();
    const auto& theme = getTheme();

    // Toggled state
    if (toggleable_ && isToggled_)
        return glass.accent;

    // Disabled
    if (!enabled_)
    {
        switch (variant_)
        {
            case ButtonVariant::Primary:
                return glass.accent;
            case ButtonVariant::Danger:
                return theme.statusError;
            case ButtonVariant::Secondary:
            case ButtonVariant::Ghost:
            case ButtonVariant::Tertiary:
            case ButtonVariant::Icon:
                return theme.textSecondary;
        }
    }

    // Pressed / Hovered — same logic, just potentially different brightness
    // For Primary: accent text color throughout all states
    // For Secondary: textPrimary on hover/press, textSecondary default
    // For Ghost/Tertiary/Icon: same as Secondary
    // For Danger: statusError throughout

    switch (variant_)
    {
        case ButtonVariant::Primary:
            return glass.accent;
        case ButtonVariant::Danger:
            return theme.statusError;
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
    const auto& glass = getGlass();
    const auto& theme = getTheme();

    // Toggled state
    if (toggleable_ && isToggled_)
        return glass.accent;

    // Segmented buttons always need a visible border regardless of variant
    if (segmentPosition_ != SegmentPosition::None)
        return glass.borderDefault;

    switch (variant_)
    {
        case ButtonVariant::Primary:
            return glass.accent;
        case ButtonVariant::Secondary:
            return isHovered_ ? glass.borderStrong : glass.borderDefault;
        case ButtonVariant::Danger:
            return theme.statusError.withAlpha(0.3f);
        case ButtonVariant::Ghost:
        case ButtonVariant::Tertiary:
        case ButtonVariant::Icon:
            return juce::Colours::transparentBlack;
    }

    return juce::Colours::transparentBlack;
}

} // namespace oscil
