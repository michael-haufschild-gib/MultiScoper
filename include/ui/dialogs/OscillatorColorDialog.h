/*
    Oscil - Oscillator Color Dialog
    Content component for color selection modal
*/

#pragma once

#include "ui/components/OscilButton.h"
#include "ui/components/OscilColorSwatches.h"
#include "ui/theme/IThemeService.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace oscil
{

class OscillatorColorDialog
    : public juce::Component
    , public ThemeManagerListener
{
public:
    using ColorSelectedCallback = std::function<void(juce::Colour)>;

    explicit OscillatorColorDialog(IThemeService& themeService);
    ~OscillatorColorDialog() override;

    void setColors(const std::vector<juce::Colour>& colors);
    void setSelectedColor(juce::Colour color);
    void setOnColorSelected(ColorSelectedCallback callback);
    void setOnCancel(std::function<void()> callback);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void themeChanged(const ColorTheme& newTheme) override;

    int getPreferredWidth() const;
    int getPreferredHeight() const;

private:
    void setupComponents();

    IThemeService& themeService_;
    std::unique_ptr<OscilColorSwatches> colorSwatches_;
    std::unique_ptr<OscilButton> okButton_;
    std::unique_ptr<OscilButton> cancelButton_;

    ColorSelectedCallback onColorSelected_;
    std::function<void()> onCancel_;

    static constexpr int BUTTON_HEIGHT = 32;
};

} // namespace oscil
