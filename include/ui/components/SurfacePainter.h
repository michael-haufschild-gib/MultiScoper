/*
    Oscil - Surface Painter
    Reusable flat-surface painting utilities for UI components. Historical
    name: "GlassPainter" (glassmorphism aesthetic, removed in 2026). The
    `SurfacePainter` namespace is the sole entry point for panel, input,
    shadow, focus-ring, and ripple painting used across all Oscil widgets.
*/

#pragma once

#include "ui/components/SurfaceStyle.h"

#include <juce_graphics/juce_graphics.h>

#include <vector>

namespace oscil
{

/// Border intensity levels for glass painting
enum class BorderLevel
{
    None,
    Subtle,
    Default,
    Strong,
    Accent
};

/// State of a single expanding ripple from a click point
struct RippleState
{
    float originX = 0.0f;
    float originY = 0.0f;
    double birthTime = 0.0; // juce::Time::getMillisecondCounterHiRes() / 1000.0
    float maxRadius = 100.0f;

    static constexpr float DURATION = 0.6f;

    /// Returns normalized progress [0, 1] based on elapsed time since birth
    float getProgress(double currentTime) const;
    /// Returns current opacity (fades from 10% to 0% over lifetime)
    float getAlpha(double currentTime) const;
    /// Returns current expansion radius in pixels
    float getRadius(double currentTime) const;
    /// Returns true when the ripple animation has completed
    bool isExpired(double currentTime) const;
};

/// Manages active ripples for a component
class RippleManager
{
public:
    /// Creates a new ripple at (x, y) with maxRadius derived from component dimensions
    void spawn(float x, float y, float componentWidth, float componentHeight);
    /// Removes all ripples whose animations have completed (uses wall clock)
    void removeExpired();
    /// Removes all ripples expired relative to the given time (for testability)
    void removeExpired(double currentTime);
    [[nodiscard]] const std::vector<RippleState>& getRipples() const { return ripples_; }
    [[nodiscard]] bool hasActiveRipples() const;
    /// Returns true if any ripple is still animating at the given time (for testability)
    [[nodiscard]] bool hasActiveRipples(double currentTime) const;
    [[nodiscard]] bool empty() const { return ripples_.empty(); }

private:
    std::vector<RippleState> ripples_;
    static double getTime();
};

/// Reusable painting functions for glassmorphism UI
namespace SurfacePainter
{
/// Full glass panel: translucent fill + inset light edge + shadow + border
void paintPanel(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass, float cornerRadius,
                BorderLevel border = BorderLevel::Subtle);

/// Just the glass background fill (no border or shadow)
void paintPanelBackground(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass,
                          float cornerRadius);

/// Glass input field (text field / dropdown trigger)
/// focused: accent border + glow. hovered: brighter border + bg. error: danger border.
void paintInput(juce::Graphics& g, juce::Rectangle<float> bounds, const SurfaceStyle& glass, float cornerRadius,
                bool focused, bool hovered, bool error = false, juce::Colour errorColour = juce::Colour(0xFFEE4444));

/// Multi-layer shadow behind a rounded rect
void paintShadow(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, float intensity, float spread);

/// Paint expanding ripple effect(s) from click points
void paintRipples(juce::Graphics& g, juce::Rectangle<float> bounds, const std::vector<RippleState>& ripples,
                  juce::Colour colour);

/// WCAG-compliant focus ring
void paintFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius, juce::Colour accentColour,
                    float width = 2.0f, float offset = 3.0f);

/// Checkerboard pattern for transparent color visualization.
/// Draws alternating white/lightgrey squares within the given bounds.
void paintCheckerboard(juce::Graphics& g, juce::Rectangle<int> bounds, int checkerSize = 6);
} // namespace SurfacePainter

} // namespace oscil
