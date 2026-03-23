/*
    Oscil - Color Picker Component Implementation
    Color selection with RGBA and hex support
*/

#include "ui/theme/ColorPickerComponent.h"

namespace oscil
{

ColorPickerComponent::ColorPickerComponent(IThemeService& themeService) : themeService_(themeService)
{
    // Create sliders for RGBA channels
    auto createSlider = [this](std::unique_ptr<OscilSlider>& slider) {
        slider = std::make_unique<OscilSlider>(themeService_);
        slider->setRange(0, 255);
        slider->setStep(1);
        slider->setDecimalPlaces(0);
        slider->setShowValueOnHover(true);
        slider->onValueChanged = [this](double /*value*/) { updateFromSliders(); };
        addAndMakeVisible(*slider);
    };

    createSlider(redSlider_);
    createSlider(greenSlider_);
    createSlider(blueSlider_);
    createSlider(alphaSlider_);

    // Create labels
    auto createLabel = [this](std::unique_ptr<juce::Label>& label, const juce::String& text) {
        label = std::make_unique<juce::Label>("", text);
        label->setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(*label);
    };

    createLabel(redLabel_, "R:");
    createLabel(greenLabel_, "G:");
    createLabel(blueLabel_, "B:");
    createLabel(alphaLabel_, "A:");
    createLabel(hexLabel_, "Hex:");

    // Create hex input
    hexInput_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Text, "");
    hexInput_->setPlaceholder("#RRGGBBAA");
    hexInput_->onReturnPressed = [this]() { updateFromHex(); };
    addAndMakeVisible(*hexInput_);

    // Initialize controls
    updateControls();
}

void ColorPickerComponent::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();

    // Draw background
    g.setColour(theme.backgroundSecondary);
    g.fillRect(getLocalBounds());

    // Draw color preview with checkerboard for transparency
    if (!previewBounds_.isEmpty())
    {
        // Draw checkerboard pattern for transparency visualization
        const int checkSize = 8;
        for (int y = previewBounds_.getY(); y < previewBounds_.getBottom(); y += checkSize)
        {
            for (int x = previewBounds_.getX(); x < previewBounds_.getRight(); x += checkSize)
            {
                bool isLight =
                    ((x - previewBounds_.getX()) / checkSize + (y - previewBounds_.getY()) / checkSize) % 2 == 0;
                g.setColour(isLight ? juce::Colours::white : juce::Colours::lightgrey);
                g.fillRect(x, y, checkSize, checkSize);
            }
        }

        // Draw the actual color on top
        g.setColour(currentColour_);
        g.fillRect(previewBounds_);

        // Draw border
        g.setColour(theme.controlBorder);
        g.drawRect(previewBounds_, 1);
    }
}

void ColorPickerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    const int labelWidth = 30;
    const int sliderHeight = 22;
    const int spacing = 2;
    const int previewWidth = 60;

    // Preview area on the right
    previewBounds_ = bounds.removeFromRight(previewWidth);
    bounds.removeFromRight(10); // spacing

    // Layout sliders vertically
    auto layoutRow = [&](juce::Label* label, OscilSlider* slider) {
        auto row = bounds.removeFromTop(sliderHeight);
        label->setBounds(row.removeFromLeft(labelWidth));
        slider->setBounds(row);
        bounds.removeFromTop(spacing);
    };

    layoutRow(redLabel_.get(), redSlider_.get());
    layoutRow(greenLabel_.get(), greenSlider_.get());
    layoutRow(blueLabel_.get(), blueSlider_.get());
    layoutRow(alphaLabel_.get(), alphaSlider_.get());

    // Hex input
    auto hexRow = bounds.removeFromTop(sliderHeight);
    hexLabel_->setBounds(hexRow.removeFromLeft(labelWidth));
    hexInput_->setBounds(hexRow.removeFromLeft(100));
}

void ColorPickerComponent::setColour(juce::Colour colour)
{
    if (currentColour_ != colour)
    {
        currentColour_ = colour;
        updateControls();
    }
}

void ColorPickerComponent::setOpacity(float opacity)
{
    opacity = juce::jlimit(0.0f, 1.0f, opacity);
    auto newColour = currentColour_.withAlpha(opacity);
    if (currentColour_ != newColour)
    {
        currentColour_ = newColour;
        updateControls();
    }
}

bool ColorPickerComponent::parseHexColour(const juce::String& hex, juce::Colour& outColour)
{
    juce::String cleaned = hex.trim();

    // Remove # prefix if present
    if (cleaned.startsWith("#"))
        cleaned = cleaned.substring(1);

    // Validate length
    if (cleaned.length() != 6 && cleaned.length() != 8)
        return false;

    // Validate characters (hex digits only)
    for (int i = 0; i < cleaned.length(); ++i)
    {
        auto c = cleaned[i];
        bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
        if (!isHex)
            return false;
    }

    // Parse values
    int r = cleaned.substring(0, 2).getHexValue32();
    int g = cleaned.substring(2, 4).getHexValue32();
    int b = cleaned.substring(4, 6).getHexValue32();
    int a = (cleaned.length() == 8) ? cleaned.substring(6, 8).getHexValue32() : 255;

    outColour = juce::Colour(static_cast<juce::uint8>(r), static_cast<juce::uint8>(g), static_cast<juce::uint8>(b),
                             static_cast<juce::uint8>(a));
    return true;
}

juce::String ColorPickerComponent::toHexString(juce::Colour colour)
{
    return juce::String::formatted("#%02X%02X%02X%02X", colour.getRed(), colour.getGreen(), colour.getBlue(),
                                   colour.getAlpha());
}

void ColorPickerComponent::updateFromSliders()
{
    if (isUpdating_)
        return;

    isUpdating_ = true;

    currentColour_ = juce::Colour(
        static_cast<juce::uint8>(redSlider_->getValue()), static_cast<juce::uint8>(greenSlider_->getValue()),
        static_cast<juce::uint8>(blueSlider_->getValue()), static_cast<juce::uint8>(alphaSlider_->getValue()));

    hexInput_->setText(toHexString(currentColour_), false);
    repaint();
    notifyColourChanged();

    isUpdating_ = false;
}

void ColorPickerComponent::updateFromHex()
{
    if (isUpdating_)
        return;

    juce::Colour newColour;
    if (parseHexColour(hexInput_->getText(), newColour))
    {
        isUpdating_ = true;

        currentColour_ = newColour;
        redSlider_->setValue(currentColour_.getRed(), false);
        greenSlider_->setValue(currentColour_.getGreen(), false);
        blueSlider_->setValue(currentColour_.getBlue(), false);
        alphaSlider_->setValue(currentColour_.getAlpha(), false);
        repaint();
        notifyColourChanged();

        isUpdating_ = false;
    }
    else
    {
        // Restore valid value
        hexInput_->setText(toHexString(currentColour_), false);
    }
}

void ColorPickerComponent::updateControls()
{
    if (isUpdating_)
        return;

    isUpdating_ = true;

    redSlider_->setValue(currentColour_.getRed(), false);
    greenSlider_->setValue(currentColour_.getGreen(), false);
    blueSlider_->setValue(currentColour_.getBlue(), false);
    alphaSlider_->setValue(currentColour_.getAlpha(), false);
    hexInput_->setText(toHexString(currentColour_), false);

    repaint();

    isUpdating_ = false;
}

void ColorPickerComponent::notifyColourChanged()
{
    if (colourChangedCallback_)
    {
        colourChangedCallback_(currentColour_);
    }
}

} // namespace oscil
