/*
    Oscil - Theme Editor Widgets
    Inner helper components used by ThemeEditorComponent: a swatch button
    that launches a color picker callout, and a titled section that groups
    swatches bound to ColorTheme fields. Split from ThemeEditorComponent.cpp
    which owns the top-level editor panel.
*/

#include "ui/components/ComponentConstants.h"
#include "ui/components/GlassPainter.h"
#include "ui/theme/ThemeEditorComponent.h"

#include <utility>

namespace oscil
{

//==============================================================================
// ColorSwatchButton
//==============================================================================

ColorSwatchButton::ColorSwatchButton(IThemeService& themeService, juce::String label, juce::Colour initialColor)
    : themeService_(themeService)
    , label_(std::move(label))
    , colour_(initialColor)
{
}

void ColorSwatchButton::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    g.setColour(theme.textPrimary);
    g.setFont(12.0f);
    auto labelBounds = bounds.removeFromLeft(100.0f);
    g.drawText(label_, labelBounds.toNearestInt(), juce::Justification::centredRight);

    bounds.removeFromLeft(8.0f);

    auto swatchBounds = bounds.reduced(2.0f);

    GlassPainter::paintCheckerboard(g, swatchBounds.toNearestInt());

    g.setColour(colour_);
    g.fillRect(swatchBounds);

    g.setColour(theme.controlBorder);
    g.drawRect(swatchBounds, 1.0f);
}

void ColorSwatchButton::resized() {}

void ColorSwatchButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        auto colorPicker = std::make_unique<ColorPickerComponent>(themeService_);
        colorPicker->setColour(colour_);
        colorPicker->setSize(280, ColorPickerComponent::PREFERRED_HEIGHT);

        juce::Component::SafePointer<ColorSwatchButton> const safeThis(this);
        colorPicker->onColourChanged([safeThis](juce::Colour newColour) {
            if (safeThis != nullptr)
            {
                safeThis->setColour(newColour);
            }
        });

        juce::CallOutBox::launchAsynchronously(std::move(colorPicker), getScreenBounds(), nullptr);
    }
}

void ColorSwatchButton::setColour(juce::Colour colour)
{
    if (colour_ != colour)
    {
        colour_ = colour;
        repaint();

        if (colourChangedCallback_)
            colourChangedCallback_(colour_);
    }
}

//==============================================================================
// ThemeColorSection
//==============================================================================

ThemeColorSection::ThemeColorSection(IThemeService& themeService, juce::String title)
    : themeService_(themeService)
    , title_(std::move(title))
{
}

void ThemeColorSection::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_DEFAULT).withStyle("Bold"));
    g.drawText(title_, bounds.removeFromTop(24), juce::Justification::centredLeft);

    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(22, 0.0f, static_cast<float>(getWidth()));
}

void ThemeColorSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(28);

    for (auto& swatch : swatches_)
    {
        swatch->setBounds(bounds.removeFromTop(ColorSwatchButton::PREFERRED_HEIGHT));
        bounds.removeFromTop(2);
    }
}

void ThemeColorSection::addColorSwatch(const juce::String& label, juce::Colour* colorRef)
{
    auto swatch = std::make_unique<ColorSwatchButton>(themeService_, label, *colorRef);
    swatch->onColourChanged([this, colorRef](juce::Colour c) {
        *colorRef = c;
        if (colorChangedCallback_)
            colorChangedCallback_();
    });
    addAndMakeVisible(*swatch);
    swatches_.push_back(std::move(swatch));
    colorRefs_.push_back(colorRef);
}

void ThemeColorSection::updateFromTheme(ColorTheme& /*theme*/)
{
    for (size_t i = 0; i < swatches_.size() && i < colorRefs_.size(); ++i)
    {
        swatches_[i]->setColour(*colorRefs_[i]);
    }
}

void ThemeColorSection::setSectionEnabled(bool enabled)
{
    Component::setEnabled(enabled);
    for (auto& swatch : swatches_)
    {
        swatch->setEnabled(enabled);
        swatch->setAlpha(enabled ? 1.0f : 0.5f);
    }
}

int ThemeColorSection::getPreferredHeight() const
{
    return 28 + (static_cast<int>(swatches_.size()) * (ColorSwatchButton::PREFERRED_HEIGHT + 2));
}

} // namespace oscil
