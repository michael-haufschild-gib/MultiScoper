/*
    Oscil - Theme Color Section
    A section of color swatches in the theme editor
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/IThemeService.h"
#include "ui/theme/components/ColorSwatchButton.h"
#include "ui/theme/ThemeManager.h" 

namespace oscil
{

class ThemeColorSection : public juce::Component
{
public:
    ThemeColorSection(IThemeService& themeService, const juce::String& title);

    void addColorSwatch(const juce::String& label, juce::Colour* colorRef);
    void updateFromTheme(ColorTheme& theme);
    void setEnabled(bool enabled);
    int getPreferredHeight() const;

    std::function<void()> onColorChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    IThemeService& themeService_;
    juce::String title_;
    bool enabled_ = true;

    std::vector<std::unique_ptr<ColorSwatchButton>> swatches_;
    std::vector<juce::Colour*> colorRefs_;
};

} // namespace oscil
