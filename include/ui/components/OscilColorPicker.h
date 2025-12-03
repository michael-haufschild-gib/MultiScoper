/*
    Oscil - Color Picker Component
    Full interactive HSV color picker with alpha support
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * Full interactive color picker
 *
 * Features:
 * - HSV color wheel or square gradient
 * - Separate hue, saturation, brightness sliders
 * - Alpha channel slider
 * - Hex input field
 * - Color preview (current vs original)
 * - Eye dropper tool (optional)
 */
class OscilColorPicker : public juce::Component,
                         public ThemeManagerListener,
                         public TestIdSupport,
                         private juce::Timer
{
public:
    enum class Mode
    {
        Square,     // SV square with hue slider
        Wheel       // HSV color wheel
    };

    OscilColorPicker();
    explicit OscilColorPicker(const juce::String& testId);
    ~OscilColorPicker() override;

    // Color
    void setColor(juce::Colour color, bool notify = true);
    juce::Colour getColor() const { return currentColor_; }

    void setOriginalColor(juce::Colour color);
    juce::Colour getOriginalColor() const { return originalColor_; }

    // Configuration
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }

    void setShowAlpha(bool show);
    bool getShowAlpha() const { return showAlpha_; }

    void setShowHexInput(bool show);
    bool getShowHexInput() const { return showHexInput_; }

    void setShowPreview(bool show);
    bool getShowPreview() const { return showPreview_; }

    // Callbacks
    std::function<void(juce::Colour)> onColorChanged;
    std::function<void(juce::Colour)> onColorChanging;  // During drag

    // Size hints
    int getPreferredWidth() const { return 280; }
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void timerCallback() override;

    void updateFromHSV();
    void updateHexField();

    void paintSquareMode(juce::Graphics& g);
    void paintWheelMode(juce::Graphics& g);
    void paintHueSlider(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintAlphaSlider(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintPreview(juce::Graphics& g, juce::Rectangle<int> bounds);

    juce::Rectangle<int> getGradientBounds() const;
    juce::Rectangle<int> getHueSliderBounds() const;
    juce::Rectangle<int> getAlphaSliderBounds() const;
    juce::Rectangle<int> getPreviewBounds() const;
    juce::Rectangle<int> getHexInputBounds() const;

    enum class DragTarget { None, Gradient, Hue, Alpha };
    DragTarget getDragTarget(juce::Point<int> pos) const;

    void handleGradientDrag(juce::Point<int> pos);
    void handleHueDrag(juce::Point<int> pos);
    void handleAlphaDrag(juce::Point<int> pos);

    juce::Colour currentColor_;
    juce::Colour originalColor_;

    float hue_ = 0.0f;
    float saturation_ = 1.0f;
    float brightness_ = 1.0f;
    float alpha_ = 1.0f;

    Mode mode_ = Mode::Square;
    bool showAlpha_ = true;
    bool showHexInput_ = true;
    bool showPreview_ = true;

    DragTarget currentDragTarget_ = DragTarget::None;

    std::unique_ptr<juce::TextEditor> hexInput_;

    ColorTheme theme_;

    static constexpr int GRADIENT_SIZE = 180;
    static constexpr int SLIDER_HEIGHT = 16;
    static constexpr int SLIDER_SPACING = 12;
    static constexpr int PREVIEW_HEIGHT = 32;

    // Cached gradient
    juce::Image cachedGradientImage_;
    float cachedHue_ = -1.0f;
    juce::Rectangle<int> cachedGradientBounds_;
    void updateGradientCache(const juce::Rectangle<int>& bounds);

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilColorPicker)
};

} // namespace oscil
