/*
    Oscil - Slider Painter Implementation
*/

#include "ui/components/painters/SliderPainter.h"
#include "ui/components/OscilSlider.h"

namespace oscil
{

void SliderPainter::paint(juce::Graphics& g, OscilSlider& slider)
{
    if (slider.variant_ == SliderVariant::Vertical)
        paintVertical(g, slider);
    else
        paintHorizontal(g, slider);

    if (slider.hasFocus_)
    {
        auto bounds = slider.getLocalBounds().toFloat();
        if (slider.variant_ == SliderVariant::Vertical)
            bounds.reduce(2.0f, 0.0f);
        else
            bounds.reduce(0.0f, 2.0f);
        paintFocusRing(g, slider, bounds);
    }
}

void SliderPainter::paintHorizontal(juce::Graphics& g, OscilSlider& slider)
{
    auto bounds = slider.getLocalBounds().toFloat();
    auto trackBounds = bounds.withHeight(OscilSlider::TRACK_HEIGHT).withCentre(bounds.getCentre());

    // Adjust track width for label
    if (slider.label_.isNotEmpty())
    {
        float labelWidth = 60.0f; // Or measure text
        trackBounds.removeFromLeft(labelWidth);
        
        g.setColour(slider.getTheme().textPrimary);
        g.setFont(14.0f);
        g.drawText(slider.label_, bounds.removeFromLeft(labelWidth), juce::Justification::centredLeft);
    }

    paintTrack(g, slider, trackBounds, false);

    if (slider.variant_ == SliderVariant::Range)
    {
        float startPos = slider.getThumbPosition(false);
        float endPos = slider.getThumbPosition(true);
        
        paintThumb(g, slider, startPos, false, false, 0.0f);
        paintThumb(g, slider, endPos, false, true, 0.0f);

        if (slider.isHovered_ && slider.showValueOnHover_)
        {
            // Show tooltip for dragged thumb or nearest
            if (slider.draggingRangeEnd_)
                paintValueTooltip(g, slider, endPos, false);
            else
                paintValueTooltip(g, slider, startPos, false);
        }
    }
    else
    {
        float thumbPos = slider.getThumbPosition(false);
        paintThumb(g, slider, thumbPos, false, false, 0.0f);

        if (slider.isHovered_ && slider.showValueOnHover_ && !slider.draggingRangeEnd_)
        {
            paintValueTooltip(g, slider, thumbPos, false);
        }
    }
}

void SliderPainter::paintVertical(juce::Graphics& g, OscilSlider& slider)
{
    auto bounds = slider.getLocalBounds().toFloat();
    auto trackBounds = bounds.withWidth(OscilSlider::TRACK_HEIGHT).withCentre(bounds.getCentre());

    // Draw label at bottom if present
    if (slider.label_.isNotEmpty())
    {
        float labelHeight = 20.0f;
        auto labelBounds = bounds.removeFromBottom(labelHeight);
        
        g.setColour(slider.getTheme().textPrimary);
        g.setFont(12.0f);
        g.drawText(slider.label_, labelBounds, juce::Justification::centred);
    }

    paintTrack(g, slider, trackBounds, true);

    float thumbPos = slider.getThumbPosition(false);
    paintThumb(g, slider, thumbPos, true, false, 0.0f);

    if (slider.isHovered_ && slider.showValueOnHover_)
    {
        paintValueTooltip(g, slider, thumbPos, true);
    }
}

void SliderPainter::paintTrack(juce::Graphics& g, OscilSlider& slider, const juce::Rectangle<float>& bounds, bool isVertical)
{
    // Background
    g.setColour(slider.getTheme().controlBackground);
    g.fillRoundedRectangle(bounds, 2.0f);

    // Fill
    g.setColour(slider.isEnabled() ? slider.getTheme().controlActive : slider.getTheme().textSecondary.withAlpha(0.5f));
    
    juce::Rectangle<float> fillBounds = bounds;
    float startPos, endPos;

    if (slider.variant_ == SliderVariant::Range)
    {
        startPos = slider.getThumbPosition(false);
        endPos = slider.getThumbPosition(true);
        
        if (isVertical)
        {
            // Invert Y for vertical
            fillBounds.setTop(endPos);
            fillBounds.setBottom(startPos);
        }
        else
        {
            fillBounds.setLeft(startPos);
            fillBounds.setRight(endPos);
        }
    }
    else
    {
        startPos = isVertical ? bounds.getBottom() : bounds.getX();
        endPos = slider.getThumbPosition(false);

        if (isVertical)
        {
            fillBounds.setTop(endPos);
            fillBounds.setBottom(startPos);
        }
        else
        {
            fillBounds.setWidth(endPos - startPos);
        }
    }

    g.fillRoundedRectangle(fillBounds, 2.0f);
}

void SliderPainter::paintThumb(juce::Graphics& g, OscilSlider& slider, float position, bool isVertical, bool, float)
{
    auto bounds = slider.getLocalBounds().toFloat();
    float size = OscilSlider::THUMB_SIZE * slider.currentThumbScale_;
    
    // Snap feedback pulse
    if (slider.snapPulse_.position > 0.01f)
    {
        float pulseSize = size * (1.0f + slider.snapPulse_.position * 0.5f);
        float alpha = 1.0f - slider.snapPulse_.position;
        
        juce::Rectangle<float> pulseRect;
        if (isVertical)
            pulseRect = juce::Rectangle<float>(0, 0, pulseSize, pulseSize).withCentre({bounds.getCentreX(), position});
        else
            pulseRect = juce::Rectangle<float>(0, 0, pulseSize, pulseSize).withCentre({position, bounds.getCentreY()});

        g.setColour(slider.getTheme().controlActive.withAlpha(alpha * 0.5f));
        g.fillEllipse(pulseRect);
    }

    juce::Rectangle<float> thumbRect;
    if (isVertical)
        thumbRect = juce::Rectangle<float>(0, 0, size, size).withCentre({bounds.getCentreX(), position});
    else
        thumbRect = juce::Rectangle<float>(0, 0, size, size).withCentre({position, bounds.getCentreY()});

    g.setColour(slider.getTheme().backgroundPrimary);
    g.fillEllipse(thumbRect);
    
    g.setColour(slider.isEnabled() ? slider.getTheme().controlActive : slider.getTheme().textSecondary);
    g.drawEllipse(thumbRect, 2.0f);
}

void SliderPainter::paintValueTooltip(juce::Graphics& g, OscilSlider& slider, float thumbPosition, bool isVertical)
{
    double val = slider.variant_ == SliderVariant::Range && slider.draggingRangeEnd_ ? slider.rangeEnd_ : slider.value_;
    juce::String text = slider.formatValue(val);
    
    juce::Font font(juce::FontOptions(12.0f));
    int textWidth = (int)std::ceil(font.getStringWidthFloat(text)) + OscilSlider::TOOLTIP_PADDING * 2;
    
    juce::Rectangle<int> tooltipBounds;
    
    if (isVertical)
    {
        tooltipBounds = juce::Rectangle<int>(0, 0, textWidth, OscilSlider::TOOLTIP_HEIGHT)
            .withCentre({(int)slider.getWidth() / 2, (int)thumbPosition - 25});
    }
    else
    {
        tooltipBounds = juce::Rectangle<int>(0, 0, textWidth, OscilSlider::TOOLTIP_HEIGHT)
            .withCentre({(int)thumbPosition, (int)slider.getHeight() / 2 - 20});
    }

    g.setColour(slider.getTheme().backgroundSecondary);
    g.fillRoundedRectangle(tooltipBounds.toFloat(), 4.0f);
    
    g.setColour(slider.getTheme().controlBorder);
    g.drawRoundedRectangle(tooltipBounds.toFloat(), 4.0f, 1.0f);
    
    g.setColour(slider.getTheme().textPrimary);
    g.setFont(font);
    g.drawText(text, tooltipBounds, juce::Justification::centred);
}

void SliderPainter::paintFocusRing(juce::Graphics& g, OscilSlider& slider, const juce::Rectangle<float>& bounds)
{
    g.setColour(slider.getTheme().controlActive.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
}

} // namespace oscil
