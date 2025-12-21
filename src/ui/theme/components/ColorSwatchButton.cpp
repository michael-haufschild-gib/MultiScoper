/*
    Oscil - Color Swatch Button Implementation
*/

#include "ui/theme/components/ColorSwatchButton.h"

namespace oscil
{

ColorSwatchButton::ColorSwatchButton(IThemeService& themeService, const juce::String& label, juce::Colour initialColor)
    : themeService_(themeService)
    , label_(label)
    , colour_(initialColor)
{
}

void ColorSwatchButton::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Draw label
    g.setColour(theme.textPrimary);
    g.setFont(12.0f);
    auto labelBounds = bounds.removeFromLeft(100.0f);
    g.drawText(label_, labelBounds.toNearestInt(), juce::Justification::centredRight);

    bounds.removeFromLeft(8.0f);

    // Draw color swatch
    auto swatchBounds = bounds.reduced(2.0f);

    // Checkerboard for transparency
    const int checkSize = 6;
    auto swatchRect = swatchBounds.toNearestInt();
    for (int y = swatchRect.getY(); y < swatchRect.getBottom(); y += checkSize)
    {
        for (int x = swatchRect.getX(); x < swatchRect.getRight(); x += checkSize)
        {
            bool isLight = ((x - swatchRect.getX()) / checkSize + (y - swatchRect.getY()) / checkSize) % 2 == 0;
            g.setColour(isLight ? juce::Colours::white : juce::Colours::lightgrey);
            g.fillRect(x, y, checkSize, checkSize);
        }
    }

    // Draw color
    g.setColour(colour_);
    g.fillRect(swatchBounds);

    // Draw border
    g.setColour(theme.controlBorder);
    g.drawRect(swatchBounds, 1.0f);
}

void ColorSwatchButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        // Create a color picker popup using modern C++ pattern
        auto colorPicker = std::make_unique<OscilRGBAPicker>(themeService_);
        colorPicker->setColour(colour_);
        colorPicker->setSize(280, OscilRGBAPicker::PREFERRED_HEIGHT);

        // Set up callback
        juce::Component::SafePointer<ColorSwatchButton> safeThis(this);
        colorPicker->onColourChanged([safeThis](juce::Colour newColour)
        {
            if (safeThis != nullptr)
            {
                safeThis->setColour(newColour);
            }
        });

        juce::CallOutBox::launchAsynchronously(
            std::move(colorPicker),
            getScreenBounds(),
            nullptr);
    }
}

void ColorSwatchButton::setColour(juce::Colour colour)
{
    if (colour_ != colour)
    {
        colour_ = colour;
        repaint();

        if (onColourChanged)
            onColourChanged(colour_);
    }
}

} // namespace oscil
