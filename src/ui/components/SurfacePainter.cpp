/*
    Oscil - Surface Painter (flat) — Implementation

    Historical name: "SurfacePainter". The 2026 uplift replaced the glassmorphism
    aesthetic with a flat surface system:

      * Single-layer drop shadow under raised surfaces (popups, modals).
        No three-layer stack, no blur radii masquerading as shadows.
      * Solid panel fills with a single 1px hairline border. No inset light
        edge, no translucent glass.
      * No accent glow. Focus is communicated by a 2px accent ring.
      * Ripples and focus-ring painters remain — they are interaction
        feedback, not glass ornamentation.

    Function names are kept for API compatibility with ~30 call sites; a
    follow-up commit will rename this module to `SurfacePainter`.
*/

#include "ui/components/SurfacePainter.h"

#include "ui/components/ComponentConstants.h"

#include <algorithm>
#include <cmath>

namespace oscil
{

// ============================================================================
// RippleState / RippleManager — unchanged from the pre-uplift implementation.
// Ripples are a modern interaction affordance, not a glass effect.
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
// SurfacePainter — flat surface painters
// ============================================================================

namespace SurfacePainter
{

void paintShadow(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, float intensity, float spread)
{
    if (intensity <= 0.0f || spread <= 0.0f)
        return;

    // Single drop shadow: 4px offset down, spread-sized blur. No halo
    // stack; modals and popups get one clean shadow to establish depth.
    float const expand = spread;
    auto shadowBounds = bounds.expanded(expand * 0.5f).translated(0.0f, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(intensity));
    g.fillRoundedRectangle(shadowBounds, cornerRadius + (expand * 0.5f));
}

void paintInsetLightEdge(juce::Graphics& /*g*/, juce::Rectangle<float> /*bounds*/, float /*cornerRadius*/,
                         float /*alpha*/)
{
    // Flat aesthetic — no inset highlight. Retained as a no-op for API
    // compatibility with legacy call sites.
}

void paintPanelBackground(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass, float cornerRadius)
{
    // Solid fill, no translucency.
    g.setColour(glass.bgPanel);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

void paintPanel(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass, float cornerRadius,
                     BorderLevel border)
{
    // 1. Single-layer drop shadow (skipped when shadowIntensity == 0).
    paintShadow(g, bounds, cornerRadius, glass.shadowIntensity, glass.shadowSpread);

    // 2. Flat solid fill.
    g.setColour(glass.bgPanel);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // 3. 1px hairline border at the chosen intensity.
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
}

void paintInput(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass, float cornerRadius,
                     bool focused, bool hovered, bool error, juce::Colour errorColour)
{
    // Flat input: solid dark fill, 1px border that shifts colour by state.
    juce::Colour const bgColour = glass.bgPanel;
    juce::Colour borderColour = glass.borderSubtle;

    if (error)
    {
        borderColour = errorColour;
    }
    else if (focused)
    {
        borderColour = glass.accent;
    }
    else if (hovered)
    {
        borderColour = glass.borderDefault;
    }

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, cornerRadius, focused ? 1.5f : 1.0f);
}

void paintAccentGlow(juce::Graphics& /*g*/, juce::Rectangle<float> /*bounds*/, juce::Colour /*colour*/,
                     float /*radius*/, float /*alpha*/)
{
    // Flat aesthetic — no glow. No-op retained for legacy call sites.
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
    // WCAG focus ring: solid accent stroke, offset outside the element.
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

} // namespace SurfacePainter

} // namespace oscil
