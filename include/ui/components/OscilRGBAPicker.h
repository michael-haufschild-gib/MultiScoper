/*
    Oscil - RGBA Picker Header
    Color selection with RGBA and hex support
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/IThemeService.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"
#include <functional>

namespace oscil
{

/**
 * Color picker component with RGBA sliders and hex input.
 * Supports both #RRGGBB and #RRGGBBAA hex formats.
 */
class OscilRGBAPicker : public juce::Component
{
public:
    explicit OscilRGBAPicker(IThemeService& themeService);
    ~OscilRGBAPicker() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Get the currently selected color
     */
    juce::Colour getColour() const { return currentColour_; }

    /**
     * Set the current color
     */
    void setColour(juce::Colour colour);

    /**
     * Get opacity (0.0 - 1.0)
     */
    float getOpacity() const { return currentColour_.getFloatAlpha(); }

    /**
     * Set opacity (0.0 - 1.0)
     */
    void setOpacity(float opacity);

    /**
     * Set callback for color changes
     */
    void onColourChanged(std::function<void(juce::Colour)> callback)
    {
        colourChangedCallback_ = std::move(callback);
    }

    /**
     * Parse hex color string (#RRGGBB or #RRGGBBAA)
     * Returns true if valid, false otherwise
     */
    static bool parseHexColour(const juce::String& hex, juce::Colour& outColour);

    /**
     * Format color as hex string (#RRGGBBAA)
     */
    static juce::String toHexString(juce::Colour colour);

    // Preferred height for layout
    static constexpr int PREFERRED_HEIGHT = 120;

private:
    void updateFromSliders();
    void updateFromHex();
    void updateControls();
    void notifyColourChanged();

    IThemeService& themeService_;
    juce::Colour currentColour_{ juce::Colours::green };

    // RGBA Sliders
    std::unique_ptr<OscilSlider> redSlider_;
    std::unique_ptr<OscilSlider> greenSlider_;
    std::unique_ptr<OscilSlider> blueSlider_;
    std::unique_ptr<OscilSlider> alphaSlider_;

    // Labels
    std::unique_ptr<juce::Label> redLabel_;
    std::unique_ptr<juce::Label> greenLabel_;
    std::unique_ptr<juce::Label> blueLabel_;
    std::unique_ptr<juce::Label> alphaLabel_;

    // Hex input
    std::unique_ptr<juce::Label> hexLabel_;
    std::unique_ptr<OscilTextField> hexInput_;

    // Color preview
    juce::Rectangle<int> previewBounds_;

    // Callback
    std::function<void(juce::Colour)> colourChangedCallback_;

    // Flag to prevent recursive updates
    bool isUpdating_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilRGBAPicker)
};

} // namespace oscil
