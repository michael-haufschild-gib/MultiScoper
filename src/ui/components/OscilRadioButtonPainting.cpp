/*
    Oscil - Radio Button Component Painting
*/

#include "ui/components/OscilRadioButton.h"

namespace oscil
{

void OscilRadioButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

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
        g.setFont(juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT)));
        g.drawText(label_, labelBounds, juce::Justification::centredLeft);
    }
    else
    {
        auto font = juce::Font(juce::FontOptions().withHeight(ComponentLayout::FONT_SIZE_DEFAULT));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, label_, 0, 0);
        int labelWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());

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
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverSpring_.position;

    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.1f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillEllipse(bounds);

    auto borderColour = selected_ ? getTheme().controlActive : getTheme().controlBorder;
    g.setColour(borderColour.withAlpha(opacity));
    g.drawEllipse(bounds.reduced(0.5f), ComponentLayout::BORDER_THIN);
}

void OscilRadioButton::paintDot(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float progress = selectionSpring_.position;

    if (progress < 0.01f)
        return;

    float dotScale = progress;
    float dotRadius = (DOT_SIZE / 2.0f) * dotScale;

    g.setColour(getTheme().controlActive.withAlpha(opacity * progress));
    g.fillEllipse(bounds.getCentreX() - dotRadius, bounds.getCentreY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void OscilRadioButton::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
    g.drawEllipse(bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET), ComponentLayout::FOCUS_RING_WIDTH);
}

} // namespace oscil
