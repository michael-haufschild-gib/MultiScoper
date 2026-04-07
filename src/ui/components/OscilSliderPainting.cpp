/*
    Oscil - Slider Component Painting
    Glassmorphism rendering for OscilSlider (track, thumb, tooltip, focus ring)
*/

#include "ui/components/GlassPainter.h"
#include "ui/components/OscilSlider.h"

#include <cmath>

namespace oscil
{

void OscilSlider::paint(juce::Graphics& g)
{
    if (variant_ == SliderVariant::Vertical)
        paintVertical(g);
    else
        paintHorizontal(g);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, getLocalBounds().toFloat());
}

void OscilSlider::paintHorizontal(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    float labelHeight = 0.0f;
    if (label_.isNotEmpty())
    {
        labelHeight = 14.0f;
        g.setColour(getTheme().textSecondary.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(label_, bounds.withHeight(labelHeight), juce::Justification::centredLeft);
    }

    float const trackY = bounds.getCentreY() - (TRACK_HEIGHT / 2.0f) + (labelHeight / 2.0f);
    auto trackBounds = juce::Rectangle<float>(THUMB_SIZE / 2.0f, trackY, bounds.getWidth() - THUMB_SIZE,
                                              static_cast<float>(TRACK_HEIGHT));

    paintTrack(g, trackBounds, false);

    if (variant_ == SliderVariant::Range)
    {
        paintThumb(g, getThumbPosition(false), false, false, labelHeight);
        paintThumb(g, getThumbPosition(true), false, true, labelHeight);
    }
    else
    {
        paintThumb(g, getThumbPosition(), false, false, labelHeight);
    }

    if ((isHovered_ || isDragging_) && showValueOnHover_)
        paintValueTooltip(g, getThumbPosition(), false);
}

void OscilSlider::paintVertical(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    float labelHeight = 0.0f;
    if (label_.isNotEmpty())
    {
        labelHeight = 14.0f;
        g.setColour(getTheme().textSecondary.withAlpha(opacity));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(label_, bounds.removeFromBottom(labelHeight), juce::Justification::centred);
    }

    float const trackX = bounds.getCentreX() - (TRACK_HEIGHT / 2.0f);
    auto trackBounds = juce::Rectangle<float>(trackX, THUMB_SIZE / 2.0f, static_cast<float>(TRACK_HEIGHT),
                                              bounds.getHeight() - THUMB_SIZE);

    paintTrack(g, trackBounds, true);
    paintThumb(g, getThumbPosition(), true);

    if ((isHovered_ || isDragging_) && showValueOnHover_)
        paintValueTooltip(g, getThumbPosition(), true);
}

void OscilSlider::paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isVertical)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& glass = getGlass();

    // Track background: bgGlass fill + borderSubtle border, fully rounded
    float const cornerRadius = bounds.getHeight() / 2.0f;

    g.setColour(glass.bgGlass.withAlpha(glass.bgGlass.getFloatAlpha() * opacity));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(glass.borderSubtle.withAlpha(glass.borderSubtle.getFloatAlpha() * opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // Fill portion: accentMuted
    auto fillProportion = static_cast<float>(valueToProportionOfLength(value_));

    juce::Rectangle<float> filledBounds;
    if (variant_ == SliderVariant::Range)
    {
        auto startProp = static_cast<float>(valueToProportionOfLength(rangeStart_));
        auto endProp = static_cast<float>(valueToProportionOfLength(rangeEnd_));

        if (isVertical)
        {
            float const startY = bounds.getBottom() - (bounds.getHeight() * startProp);
            float const endY = bounds.getBottom() - (bounds.getHeight() * endProp);
            filledBounds = juce::Rectangle<float>(bounds.getX(), endY, bounds.getWidth(), startY - endY);
        }
        else
        {
            float const startX = bounds.getX() + (bounds.getWidth() * startProp);
            float const endX = bounds.getX() + (bounds.getWidth() * endProp);
            filledBounds = juce::Rectangle<float>(startX, bounds.getY(), endX - startX, bounds.getHeight());
        }
    }
    else
    {
        if (isVertical)
        {
            filledBounds = bounds.withTop(bounds.getBottom() - (bounds.getHeight() * fillProportion));
        }
        else
        {
            filledBounds = bounds.withWidth(bounds.getWidth() * fillProportion);
        }
    }

    g.setColour(glass.accentMuted.withAlpha(glass.accentMuted.getFloatAlpha() * opacity));
    g.fillRoundedRectangle(filledBounds, cornerRadius);
}

void OscilSlider::paintThumb(juce::Graphics& g, float position, bool isVertical, bool /*isRangeEnd*/, float labelOffset)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& glass = getGlass();
    auto bounds = getLocalBounds().toFloat();

    float const scale = currentThumbScale_;
    float const size = THUMB_SIZE * scale;
    float cx = 0.0f;
    float cy = 0.0f;

    if (isVertical)
    {
        cx = bounds.getCentreX();
        cy = position;
    }
    else
    {
        cx = position;
        cy = bounds.getCentreY() + (labelOffset / 2.0f);
    }

    auto thumbBounds = juce::Rectangle<float>(cx - (size / 2), cy - (size / 2), size, size);

    // Shadow behind thumb
    g.setColour(juce::Colours::black.withAlpha(0.15f * opacity));
    g.fillEllipse(thumbBounds.translated(0, 1).expanded(1.0f));

    // White thumb fill
    g.setColour(juce::Colours::white.withAlpha(opacity));
    g.fillEllipse(thumbBounds);

    // Accent border (2px)
    g.setColour(glass.accent.withAlpha(opacity));
    g.drawEllipse(thumbBounds.reduced(1.0f), 2.0f);
}

void OscilSlider::paintValueTooltip(juce::Graphics& g, float thumbPosition, bool isVertical)
{
    const auto& glass = getGlass();
    juce::String const valueText = formatValue(value_);

    auto font = juce::Font(juce::FontOptions().withHeight(12.0f));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, valueText, 0, 0);
    float const textWidthF = glyphs.getBoundingBox(0, -1, false).getWidth() + (TOOLTIP_PADDING * 2);

    juce::Rectangle<float> tooltipBounds;

    if (isVertical)
    {
        tooltipBounds =
            juce::Rectangle<float>(static_cast<float>(getWidth()) + 4.0f, thumbPosition - (TOOLTIP_HEIGHT / 2.0f),
                                   textWidthF, static_cast<float>(TOOLTIP_HEIGHT));
    }
    else
    {
        tooltipBounds = juce::Rectangle<float>(thumbPosition - (textWidthF / 2.0f), -TOOLTIP_HEIGHT - 4.0f, textWidthF,
                                               static_cast<float>(TOOLTIP_HEIGHT));
    }

    tooltipBounds = tooltipBounds.constrainedWithin(getLocalBounds().toFloat().expanded(50, 30));

    // Glass panel tooltip
    GlassPainter::paintGlassPanel(g, tooltipBounds, glass, ComponentLayout::RADIUS_SM, BorderLevel::Subtle);

    g.setColour(getTheme().textPrimary);
    g.setFont(font);
    g.drawText(valueText, tooltipBounds, juce::Justification::centred);
}

void OscilSlider::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    GlassPainter::paintFocusRing(g, bounds, ComponentLayout::RADIUS_SM, getGlass().accent);
}

juce::String OscilSlider::formatValue(double value) const
{
    if (valueFormatter_)
        return valueFormatter_(value);

    juce::String text = juce::String(value, decimalPlaces_);
    if (suffix_.isNotEmpty())
        text += " " + suffix_;

    return text;
}

float OscilSlider::getThumbPosition(bool isRangeEnd) const
{
    auto bounds = getLocalBounds().toFloat();
    bool const isVertical = variant_ == SliderVariant::Vertical;

    double const value = (variant_ == SliderVariant::Range) ? (isRangeEnd ? rangeEnd_ : rangeStart_) : value_;

    auto proportion = static_cast<float>(valueToProportionOfLength(value));

    if (isVertical)
    {
        float const trackHeight = std::max(1.0f, bounds.getHeight() - THUMB_SIZE);
        return bounds.getBottom() - (THUMB_SIZE / 2.0f) - (trackHeight * proportion);
    }

    float const trackWidth = std::max(1.0f, bounds.getWidth() - THUMB_SIZE);
    return THUMB_SIZE / 2.0f + trackWidth * proportion;
}

} // namespace oscil
