/*
    Oscil - Slider Painter
    Handles rendering logic for OscilSlider
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

class OscilSlider;

class SliderPainter
{
public:
    static void paint(juce::Graphics& g, OscilSlider& slider);

private:
    static void paintHorizontal(juce::Graphics& g, OscilSlider& slider);
    static void paintVertical(juce::Graphics& g, OscilSlider& slider);
    static void paintTrack(juce::Graphics& g, OscilSlider& slider, const juce::Rectangle<float>& bounds, bool isVertical);
    static void paintThumb(juce::Graphics& g, OscilSlider& slider, float position, bool isVertical, bool isRangeEnd, float labelOffset);
    static void paintValueTooltip(juce::Graphics& g, OscilSlider& slider, float thumbPosition, bool isVertical);
    static void paintFocusRing(juce::Graphics& g, OscilSlider& slider, const juce::Rectangle<float>& bounds);
};

} // namespace oscil
