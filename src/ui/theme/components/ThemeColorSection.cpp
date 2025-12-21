/*
    Oscil - Theme Color Section Implementation
*/

#include "ui/theme/components/ThemeColorSection.h"

namespace oscil
{

ThemeColorSection::ThemeColorSection(IThemeService& themeService, const juce::String& title)
    : themeService_(themeService)
    , title_(title)
{
}

void ThemeColorSection::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    // Draw section header
    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(title_, bounds.removeFromTop(24), juce::Justification::centredLeft);

    // Draw separator
    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(22, 0.0f, static_cast<float>(getWidth()));
}

void ThemeColorSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(28);  // Header

    for (auto& swatch : swatches_)
    {
        swatch->setBounds(bounds.removeFromTop(ColorSwatchButton::PREFERRED_HEIGHT));
        bounds.removeFromTop(2);
    }
}

void ThemeColorSection::addColorSwatch(const juce::String& label, juce::Colour* colorRef)
{
    auto swatch = std::make_unique<ColorSwatchButton>(themeService_, label, *colorRef);
    swatch->onColourChanged = [this, colorRef](juce::Colour c)
    {
        *colorRef = c;
        if (onColorChanged)
            onColorChanged();
    };
    addAndMakeVisible(*swatch);
    swatches_.push_back(std::move(swatch));
    colorRefs_.push_back(colorRef);
}

void ThemeColorSection::updateFromTheme(ColorTheme& /*theme*/)
{
    // Update swatch colors from their refs
    for (size_t i = 0; i < swatches_.size() && i < colorRefs_.size(); ++i)
    {
        swatches_[i]->setColour(*colorRefs_[i]);
    }
}

void ThemeColorSection::setEnabled(bool enabled)
{
    enabled_ = enabled;
    for (auto& swatch : swatches_)
    {
        swatch->setEnabled(enabled);
        swatch->setAlpha(enabled ? 1.0f : 0.5f);
    }
}

int ThemeColorSection::getPreferredHeight() const
{
    return 28 + static_cast<int>(swatches_.size()) * (ColorSwatchButton::PREFERRED_HEIGHT + 2);
}

} // namespace oscil
