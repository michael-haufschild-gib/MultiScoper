/*
    Oscil - Radio Button Component Painting
*/

#include "ui/components/GlassPainter.h"
#include "ui/components/OscilRadioButton.h"

namespace oscil
{

void OscilRadioButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    juce::Rectangle<float> circleBounds;

    if (label_.isEmpty())
    {
        circleBounds = bounds.toFloat().withSizeKeepingCentre(RADIO_SIZE, RADIO_SIZE);
    }
    else if (labelOnRight_)
    {
        circleBounds = juce::Rectangle<float>(0, static_cast<float>(bounds.getHeight() - RADIO_SIZE) / 2.0f, RADIO_SIZE,
                                              RADIO_SIZE);

        auto labelBounds = bounds.toFloat().withLeft(RADIO_SIZE + ComponentLayout::SPACING_SM);

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(ComponentLayout::defaultFont());
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = ComponentLayout::defaultFont();
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int const labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());

        auto labelBounds =
            juce::Rectangle<float>(0, 0, static_cast<float>(labelWidth), static_cast<float>(bounds.getHeight()));

        g.setColour(getTheme().textPrimary.withAlpha(opacity));
        g.setFont(font);
        g.drawText(label_, labelBounds, juce::Justification::centredRight);

        circleBounds =
            juce::Rectangle<float>(static_cast<float>(labelWidth + ComponentLayout::SPACING_SM),
                                   static_cast<float>(bounds.getHeight() - RADIO_SIZE) / 2.0f, RADIO_SIZE, RADIO_SIZE);
    }

    paintCircle(g, circleBounds);

    if (selected_ || selectionSpring_.position > 0.01f)
        paintDot(g, circleBounds);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, circleBounds);
}

void OscilRadioButton::paintCircle(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const hoverAmount = hoverSpring_.position;

    // Background: bgGlass fill in the circle area
    g.setColour(getGlass().bgGlass.withAlpha(opacity));
    g.fillEllipse(bounds);

    // Border: borderDefault default, borderStrong on hover
    auto borderColour = getGlass().borderDefault;
    if (hoverAmount > 0.01f)
        borderColour = borderColour.interpolatedWith(getGlass().borderStrong, hoverAmount);

    g.setColour(borderColour.withAlpha(opacity));
    g.drawEllipse(bounds.reduced(0.5f), ComponentLayout::BORDER_THIN);
}

void OscilRadioButton::paintDot(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float const progress = selectionSpring_.position;

    if (progress < 0.01f)
        return;

    // Apply scale pulse from scaleSpring_ (1.0 -> 1.15 -> 1.0)
    float const pulseScale = scaleSpring_.position;
    float const dotScale = progress * pulseScale;
    float const dotRadius = (DOT_SIZE / 2.0f) * dotScale;

    // Selected: accent filled inner dot
    g.setColour(getGlass().accent.withAlpha(opacity * progress));
    g.fillEllipse(bounds.getCentreX() - dotRadius, bounds.getCentreY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void OscilRadioButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Use GlassPainter::paintFocusRing with accent color, adapted for ellipse
    auto ringBounds = bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET);
    g.setColour(getGlass().accent.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawEllipse(ringBounds, ComponentLayout::FOCUS_RING_WIDTH);
}

} // namespace oscil
