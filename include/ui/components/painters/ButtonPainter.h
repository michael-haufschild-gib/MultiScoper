/*
    Oscil - Button Painter
    Handles rendering logic for OscilButton
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

class OscilButton;

class ButtonPainter
{
public:
    static void paint(juce::Graphics& g, OscilButton& button);

private:
    static void paintButton(juce::Graphics& g, OscilButton& button, const juce::Rectangle<float>& bounds);
    static void paintFocusRing(juce::Graphics& g, OscilButton& button, const juce::Rectangle<float>& bounds);
    static void updatePathCache(OscilButton& button, const juce::Rectangle<float>& bounds);

    static juce::Colour getBackgroundColour(const OscilButton& button);
    static juce::Colour getTextColour(const OscilButton& button);
    static juce::Colour getBorderColour(const OscilButton& button);
};

} // namespace oscil
