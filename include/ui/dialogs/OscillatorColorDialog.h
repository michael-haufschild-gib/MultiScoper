/*
    MultiScoper - Oscillator Color Dialog
    Content component for color selection modal
*/

#pragma once

#include "ui/components/MultiScoperButton.h"
#include "ui/components/MultiScoperColorSwatches.h"
#include "ui/components/ThemedComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace multiscoper
{

class OscillatorColorDialog : public ThemedComponent
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

    int getPreferredWidth() const;
    int getPreferredHeight() const;

private:
    void setupComponents();

    std::unique_ptr<MultiScoperColorSwatches> colorSwatches_;
    std::unique_ptr<MultiScoperButton> okButton_;
    std::unique_ptr<MultiScoperButton> cancelButton_;

    ColorSelectedCallback onColorSelected_;
    std::function<void()> onCancel_;

    static constexpr int BUTTON_HEIGHT = 32;
};

} // namespace multiscoper
