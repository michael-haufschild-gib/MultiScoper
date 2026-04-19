/*
    MultiScoper - Slider Component Painting
    Flat-surface rendering for MultiScoperSlider (track, thumb, tooltip, focus ring).
    (Historical: "glassmorphism rendering" prior to the 2026-Q2 uplift.)
*/

#include "ui/components/MultiScoperSlider.h"
#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

#include <cmath>

namespace multiscoper
{

namespace
{
constexpr float ROTARY_START_ANGLE = -5.0f * juce::MathConstants<float>::pi / 6.0f;
constexpr float ROTARY_END_ANGLE = 5.0f * juce::MathConstants<float>::pi / 6.0f;
constexpr float ROTARY_SWEEP = ROTARY_END_ANGLE - ROTARY_START_ANGLE;
constexpr float ROTARY_KNOB_INSET = 4.0f;
constexpr float ROTARY_ARC_GAP = 3.0f;
constexpr float ROTARY_ARC_THICKNESS = 2.0f;
constexpr float ROTARY_TICK_THICKNESS = 1.5f;
constexpr float ROTARY_TICK_LENGTH_RATIO = 0.8f;
} // namespace

void MultiScoperSlider::paint(juce::Graphics& g)
{
    if (variant_ == SliderVariant::Rotary)
    {
        paintRotary(g);
        return;
    }

    if (variant_ == SliderVariant::Vertical)
        paintVertical(g);
    else
        paintHorizontal(g);

    if (hasFocus_ && enabled_)
        paintFocusRing(g, getLocalBounds().toFloat());
}

void MultiScoperSlider::paintHorizontal(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    float labelHeight = 0.0f;
    if (label_.isNotEmpty())
    {
        labelHeight = 14.0f;
        g.setColour(getTheme().textSecondary.withAlpha(opacity));
        g.setFont(Typography::caption());
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

void MultiScoperSlider::paintVertical(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;

    float labelHeight = 0.0f;
    if (label_.isNotEmpty())
    {
        labelHeight = 14.0f;
        g.setColour(getTheme().textSecondary.withAlpha(opacity));
        g.setFont(Typography::caption());
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

void MultiScoperSlider::paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isVertical)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& surface = getSurface();

    // Track background: bgGlass fill + borderSubtle border, flat rectangle
    // (the 2026 design language removed the pill track shape).
    g.setColour(surface.bgGlass.withAlpha(surface.bgGlass.getFloatAlpha() * opacity));
    g.fillRect(bounds);

    g.setColour(surface.borderSubtle.withAlpha(surface.borderSubtle.getFloatAlpha() * opacity));
    g.drawRect(bounds, 1.0f);

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

    g.setColour(surface.accentMuted.withAlpha(surface.accentMuted.getFloatAlpha() * opacity));
    g.fillRect(filledBounds);
}

void MultiScoperSlider::paintThumb(juce::Graphics& g, float position, bool isVertical, bool /*isRangeEnd*/,
                                   float labelOffset)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& surface = getSurface();
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
    g.setColour(surface.accent.withAlpha(opacity));
    g.drawEllipse(thumbBounds.reduced(1.0f), 2.0f);
}

void MultiScoperSlider::paintValueTooltip(juce::Graphics& g, float thumbPosition, bool isVertical)
{
    const auto& surface = getSurface();
    juce::String const valueText = formatValue(value_);

    auto font = Typography::small();
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

    // Surface panel tooltip (flat rectangle)
    SurfacePainter::paintPanel(g, tooltipBounds, surface, 0.0f, BorderLevel::Subtle);

    g.setColour(getTheme().textPrimary);
    g.setFont(font);
    g.drawText(valueText, tooltipBounds, juce::Justification::centred);
}

void MultiScoperSlider::paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    SurfacePainter::paintFocusRing(g, bounds, 0.0f, getSurface().accent);
}

juce::String MultiScoperSlider::formatValue(double value) const
{
    if (valueFormatter_)
        return valueFormatter_(value);

    juce::String text = juce::String(value, decimalPlaces_);
    if (suffix_.isNotEmpty())
        text += " " + suffix_;

    return text;
}

juce::Rectangle<float> MultiScoperSlider::getRotaryKnobBounds() const
{
    auto bounds = getLocalBounds().toFloat();

    float reserved = 0.0f;
    if (label_.isNotEmpty())
        reserved += static_cast<float>(ROTARY_LABEL_HEIGHT);
    if (showValue_)
        reserved += static_cast<float>(ROTARY_VALUE_HEIGHT);

    auto knobArea = bounds.withTrimmedBottom(reserved);
    float const side = std::min(knobArea.getWidth(), knobArea.getHeight()) - (ROTARY_KNOB_INSET * 2.0f);
    float const safeSide = std::max(side, 8.0f);

    return juce::Rectangle<float>(safeSide, safeSide).withCentre(knobArea.getCentre());
}

juce::Colour MultiScoperSlider::getEffectiveArcColour() const
{
    if (arcColour_.getAlpha() == 0)
        return getSurface().accent;
    return arcColour_;
}

void MultiScoperSlider::paintRotary(juce::Graphics& g)
{
    float const opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    const auto& theme = getTheme();
    const auto& surface = getSurface();

    auto knobBounds = getRotaryKnobBounds();

    g.setColour(theme.controlBackground.withAlpha(opacity));
    g.fillEllipse(knobBounds);
    g.setColour(surface.borderDefault.withAlpha(surface.borderDefault.getFloatAlpha() * opacity));
    g.drawEllipse(knobBounds.reduced(0.5f), 1.0f);

    paintRotaryArcs(g, knobBounds, opacity);

    // Indicator tick.
    float const radius = knobBounds.getWidth() * 0.5f;
    float const cx = knobBounds.getCentreX();
    float const cy = knobBounds.getCentreY();
    auto proportion = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(value_)));
    float const valueAngle = ROTARY_START_ANGLE + (proportion * ROTARY_SWEEP);

    juce::Colour tickColour = theme.textPrimary;
    if (isDragging_)
        tickColour = getEffectiveArcColour();
    else if (isHovered_)
        tickColour = theme.textHighlight;
    tickColour = tickColour.withMultipliedAlpha(opacity);

    float const tickLen = radius * ROTARY_TICK_LENGTH_RATIO;
    // JUCE convention: 0 rad = 12 o'clock, positive = CW.
    float const sinA = std::sin(valueAngle);
    float const cosA = std::cos(valueAngle);
    g.setColour(tickColour);
    g.drawLine(cx, cy, cx + (sinA * tickLen), cy - (cosA * tickLen), ROTARY_TICK_THICKNESS);

    paintRotaryLabelAndValue(g, knobBounds, opacity);

    if (hasFocus_ && enabled_)
        SurfacePainter::paintFocusRing(g, knobBounds, knobBounds.getWidth() * 0.5f, surface.accent);
}

void MultiScoperSlider::paintRotaryArcs(juce::Graphics& g, juce::Rectangle<float> knobBounds, float opacity)
{
    float const radius = knobBounds.getWidth() * 0.5f;
    float const cx = knobBounds.getCentreX();
    float const cy = knobBounds.getCentreY();
    float const arcRadius = radius + ROTARY_ARC_GAP;
    auto proportion = static_cast<float>(juce::jlimit(0.0, 1.0, valueToProportionOfLength(value_)));

    juce::Colour arcColour = getEffectiveArcColour();
    if (isHovered_ && !isDragging_)
        arcColour = arcColour.brighter(0.1f);
    arcColour = arcColour.withMultipliedAlpha(opacity);

    float const valueAngle = ROTARY_START_ANGLE + (proportion * ROTARY_SWEEP);
    float const arcFromAngle = bipolar_ ? 0.0f : ROTARY_START_ANGLE;

    juce::Path arc;
    arc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, arcFromAngle, valueAngle, true);
    g.setColour(arcColour);
    g.strokePath(
        arc, juce::PathStrokeType(ROTARY_ARC_THICKNESS, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (std::abs(modAmount_) <= 1e-5f)
        return;

    float const modEnd = juce::jlimit(ROTARY_START_ANGLE, ROTARY_END_ANGLE, valueAngle + (modAmount_ * ROTARY_SWEEP));
    juce::Path modArc;
    modArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, valueAngle, modEnd, true);
    g.setColour(modColour_.withMultipliedAlpha(opacity));
    g.strokePath(modArc, juce::PathStrokeType(ROTARY_ARC_THICKNESS, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    auto glyphArea = juce::Rectangle<float>(knobBounds.getRight() - 10.0f, knobBounds.getY() - 2.0f, 10.0f, 10.0f);
    g.setFont(Typography::caption().withHeight(8.0f));
    // Show the actual modulation polarity: the arc direction already flips
    // with sign; the badge must match or the badge misreports the control.
    g.drawText(modAmount_ >= 0.0f ? "+" : "-", glyphArea, juce::Justification::centred);
}

void MultiScoperSlider::paintRotaryLabelAndValue(juce::Graphics& g, juce::Rectangle<float> knobBounds, float opacity)
{
    const auto& theme = getTheme();
    const auto& surface = getSurface();

    auto const labelHeight = static_cast<float>(ROTARY_LABEL_HEIGHT);
    auto const valueHeight = static_cast<float>(ROTARY_VALUE_HEIGHT);

    float labelY = knobBounds.getBottom() + 2.0f;
    if (label_.isNotEmpty())
    {
        auto labelArea = juce::Rectangle<float>(0.0f, labelY, static_cast<float>(getWidth()), labelHeight);
        g.setColour(theme.textSecondary.withAlpha(opacity));
        g.setFont(Typography::caption());
        g.drawText(label_, labelArea, juce::Justification::centred);
        labelY += labelHeight;
    }

    if (showValue_)
    {
        auto valueArea = juce::Rectangle<float>(0.0f, labelY, static_cast<float>(getWidth()), valueHeight);
        g.setColour(surface.accent.withMultipliedAlpha(opacity));
        g.setFont(Typography::small());
        g.drawText(formatValue(value_), valueArea, juce::Justification::centred);
    }
}

float MultiScoperSlider::getThumbPosition(bool isRangeEnd) const
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

} // namespace multiscoper
