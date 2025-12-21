/*
    Oscil - Color Swatch Button
    A button that displays a color and opens a picker on click
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/IThemeService.h"
#include "ui/components/OscilRGBAPicker.h"

namespace oscil
{

class ColorSwatchButton : public juce::Component
{
public:
    ColorSwatchButton(IThemeService& themeService, const juce::String& label, juce::Colour initialColor);

    void setColour(juce::Colour colour);
    juce::Colour getColour() const { return colour_; }

    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void(juce::Colour)> onColourChanged;

    static constexpr int PREFERRED_HEIGHT = 24;

private:
    IThemeService& themeService_;
    juce::String label_;
    juce::Colour colour_;
    std::function<void(juce::Colour)> colourChangedCallback_;
};

} // namespace oscil
