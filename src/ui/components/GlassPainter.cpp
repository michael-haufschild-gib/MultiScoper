/*
    Oscil - Glass Painter
    Reusable glassmorphism painting utilities for UI components
*/

#include "ui/components/GlassPainter.h"

#include "ui/components/ComponentConstants.h"

#include <algorithm>
#include <cmath>

namespace oscil
{

// ============================================================================
// RippleState
// ============================================================================

float RippleState::getProgress(double currentTime) const
{
    auto elapsed = static_cast<float>(currentTime - birthTime);
    return std::clamp(elapsed / DURATION, 0.0f, 1.0f);
}

float RippleState::getAlpha(double currentTime) const
{
    float const progress = getProgress(currentTime);
    return (1.0f - progress) * 0.1f;
}

float RippleState::getRadius(double currentTime) const { return getProgress(currentTime) * maxRadius; }

bool RippleState::isExpired(double currentTime) const { return getProgress(currentTime) >= 1.0f; }

// ============================================================================
// RippleManager
// ============================================================================

double RippleManager::getTime() { return juce::Time::getMillisecondCounterHiRes() / 1000.0; }

void RippleManager::spawn(float x, float y, float componentWidth, float componentHeight)
{
    RippleState ripple;
    ripple.originX = x;
    ripple.originY = y;
    ripple.birthTime = getTime();
    ripple.maxRadius = std::sqrt(componentWidth * componentWidth + componentHeight * componentHeight);
    ripples_.push_back(ripple);
}

void RippleManager::removeExpired() { removeExpired(getTime()); }

void RippleManager::removeExpired(double currentTime)
{
    std::erase_if(ripples_, [currentTime](const RippleState& r) { return r.isExpired(currentTime); });
}

bool RippleManager::hasActiveRipples() const { return hasActiveRipples(getTime()); }

bool RippleManager::hasActiveRipples(double currentTime) const
{
    return std::ranges::any_of(ripples_, [currentTime](const RippleState& r) { return !r.isExpired(currentTime); });
}

// ============================================================================
// GlassPainter
// ============================================================================

namespace GlassPainter
{

void paintShadow(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, float intensity, float spread)
{
    if (intensity <= 0.0f)
        return;

    auto black = juce::Colours::black;

    // Layer 1: tight shadow, 1px down
    {
        float const expand = 2.0f;
        auto shadowBounds = bounds.expanded(expand).translated(0.0f, 1.0f);
        g.setColour(black.withAlpha(intensity * 0.3f));
        g.fillRoundedRectangle(shadowBounds, cornerRadius + expand);
    }

    // Layer 2: medium shadow, 4px down
    {
        float const expand = spread * 0.5f;
        auto shadowBounds = bounds.expanded(expand).translated(0.0f, 4.0f);
        g.setColour(black.withAlpha(intensity * 0.5f));
        g.fillRoundedRectangle(shadowBounds, cornerRadius + expand);
    }

    // Layer 3: wide shadow, 8px down
    {
        float const expand = spread;
        auto shadowBounds = bounds.expanded(expand).translated(0.0f, 8.0f);
        g.setColour(black.withAlpha(intensity * 0.3f));
        g.fillRoundedRectangle(shadowBounds, cornerRadius + expand);
    }
}

void paintInsetLightEdge(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, float alpha)
{
    if (alpha <= 0.0f)
        return;

    auto white = juce::Colours::white;

    // 1px horizontal highlight at the top, inset by cornerRadius
    float const lineY = bounds.getY() + 1.0f;
    float const lineLeft = bounds.getX() + cornerRadius;
    float const lineRight = bounds.getRight() - cornerRadius;

    if (lineRight > lineLeft)
    {
        g.setColour(white.withAlpha(alpha));
        g.fillRect(lineLeft, lineY, lineRight - lineLeft, 1.0f);
    }

    // Subtle perimeter glow: inner stroke at white 2% alpha
    g.setColour(white.withAlpha(0.02f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

void paintGlassBackground(juce::Graphics& g, juce::Rectangle<float> bounds, const GlassStyle& glass, float cornerRadius)
{
    // Fill with glass background
    g.setColour(glass.bgGlass);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Inset light edge
    paintInsetLightEdge(g, bounds, cornerRadius, glass.insetLightEdge.getFloatAlpha());
}

void paintGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds, const GlassStyle& glass, float cornerRadius,
                     BorderLevel border)
{
    // 1. Shadow
    paintShadow(g, bounds, cornerRadius, glass.shadowIntensity, glass.shadowSpread);

    // 2. Glass background fill + inset light edge
    paintGlassBackground(g, bounds, glass, cornerRadius);

    // 3. Border based on level
    juce::Colour borderColour;
    bool drawBorder = true;

    switch (border)
    {
        case BorderLevel::None:
            drawBorder = false;
            break;
        case BorderLevel::Subtle:
            borderColour = glass.borderSubtle;
            break;
        case BorderLevel::Default:
            borderColour = glass.borderDefault;
            break;
        case BorderLevel::Strong:
            borderColour = glass.borderStrong;
            break;
        case BorderLevel::Accent:
            borderColour = glass.accent;
            break;
    }

    if (drawBorder)
    {
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }

    // 4. Dark outline: black at 15% alpha, 0.5px stroke
    g.setColour(juce::Colours::black.withAlpha(0.15f));
    g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);
}

void paintGlassInput(juce::Graphics& g, juce::Rectangle<float> bounds, const GlassStyle& glass, float cornerRadius,
                     bool focused, bool hovered, bool error, juce::Colour errorColour)
{
    juce::Colour bgColour = glass.bgGlass;
    juce::Colour borderColour = glass.borderSubtle;

    if (error)
    {
        // Error state: danger border, no glow
        borderColour = errorColour;
    }
    else if (focused)
    {
        // Focused: accent border + glow + bg tint
        borderColour = glass.accent;
        bgColour = glass.bgGlass.interpolatedWith(glass.accentSubtle, 0.3f);

        // Accent glow ring
        paintAccentGlow(g, bounds, glass.accent, glass.accentGlowRadius, glass.accentGlowAlpha);
    }
    else if (hovered)
    {
        // Hovered: brighter border + slightly lighter bg
        borderColour = glass.borderDefault;
        bgColour = glass.bgGlass.interpolatedWith(glass.bgHover, 0.5f);
    }

    // Fill background
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Inset shadow at top: 1px inner shadow (black 5% alpha)
    {
        float const lineY = bounds.getY() + 0.5f;
        float const lineLeft = bounds.getX() + cornerRadius;
        float const lineRight = bounds.getRight() - cornerRadius;
        if (lineRight > lineLeft)
        {
            g.setColour(juce::Colours::black.withAlpha(0.05f));
            g.fillRect(lineLeft, lineY, lineRight - lineLeft, 1.0f);
        }
    }

    // Border
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void paintAccentGlow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accentColour, float radius,
                     float alpha)
{
    if (alpha <= 0.0f || radius <= 0.0f)
        return;

    // Multiple layers at decreasing alpha for soft falloff
    constexpr int numLayers = 3;
    for (int i = numLayers; i >= 1; --i)
    {
        float const layerExpand = radius * (static_cast<float>(i) / static_cast<float>(numLayers));
        float const layerAlpha = alpha * (1.0f / static_cast<float>(i + 1));

        auto glowBounds = bounds.expanded(layerExpand);
        g.setColour(accentColour.withAlpha(layerAlpha));
        g.fillRoundedRectangle(glowBounds, layerExpand * 0.5f + 4.0f);
    }
}

void paintRipples(juce::Graphics& g, juce::Rectangle<float> bounds, const std::vector<RippleState>& ripples,
                  juce::Colour colour)
{
    if (ripples.empty())
        return;

    double const now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    g.saveState();
    g.reduceClipRegion(bounds.toNearestIntEdges());

    for (const auto& ripple : ripples)
    {
        if (ripple.isExpired(now))
            continue;

        float const currentRadius = ripple.getRadius(now);
        float const currentAlpha = ripple.getAlpha(now);

        if (currentAlpha > 0.0f && currentRadius > 0.0f)
        {
            g.setColour(colour.withAlpha(currentAlpha));
            g.fillEllipse(ripple.originX - currentRadius, ripple.originY - currentRadius, currentRadius * 2.0f,
                          currentRadius * 2.0f);
        }
    }

    g.restoreState();
}

void paintFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, juce::Colour accentColour,
                    float width, float offset)
{
    auto ringBounds = bounds.expanded(offset);
    g.setColour(accentColour.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawRoundedRectangle(ringBounds, cornerRadius + offset, width);
}

void paintCheckerboard(juce::Graphics& g, juce::Rectangle<int> bounds, int checkerSize)
{
    if (checkerSize <= 0)
        return;

    for (int y = bounds.getY(); y < bounds.getBottom(); y += checkerSize)
    {
        for (int x = bounds.getX(); x < bounds.getRight(); x += checkerSize)
        {
            bool const isWhite = (((x - bounds.getX()) / checkerSize) + ((y - bounds.getY()) / checkerSize)) % 2 == 0;
            g.setColour(isWhite ? juce::Colours::white : juce::Colours::lightgrey);
            g.fillRect(x, y, std::min(checkerSize, bounds.getRight() - x),
                       std::min(checkerSize, bounds.getBottom() - y));
        }
    }
}

} // namespace GlassPainter

} // namespace oscil
