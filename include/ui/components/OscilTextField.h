/*
    Oscil - Text Field Component
    Text input with variants for text, search, and number entry
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"

namespace oscil
{

class OscilButton;

/**
 * A themed text field component with multiple variants
 *
 * Features:
 * - Text, Search, and Number variants
 * - Input validation with error states
 * - Number variant with stepper buttons and scroll support
 * - Double-click to reset to default (Number)
 * - Full keyboard accessibility
 * - APVTS support for Number variant
 */
class OscilTextField : public juce::Component,
                       public ThemeManagerListener,
                       private juce::Timer
{
public:
    OscilTextField();
    explicit OscilTextField(TextFieldVariant variant);
    ~OscilTextField() override;

    // Variant configuration
    void setVariant(TextFieldVariant variant);
    TextFieldVariant getVariant() const { return variant_; }

    // Text content
    void setText(const juce::String& text, bool notify = true);
    juce::String getText() const;

    void setPlaceholder(const juce::String& placeholder);
    juce::String getPlaceholder() const { return placeholder_; }

    // Number variant configuration
    void setRange(double min, double max);
    void setDefaultValue(double defaultValue);
    void setStep(double step);
    void setDecimalPlaces(int places);
    void setSuffix(const juce::String& suffix);

    double getNumericValue() const { return numValue_; }
    void setNumericValue(double value, bool notify = true);

    // Validation
    void setValidator(Callbacks::ValidationCallback validator);
    void setError(const juce::String& errorMessage);
    void clearError();
    bool hasError() const { return !errorMessage_.isEmpty(); }

    // State
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // APVTS integration (Number variant only)
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId);
    void detachFromParameter();

    // Callbacks
    std::function<void(const juce::String&)> onTextChanged;
    std::function<void(double)> onValueChanged;
    std::function<void()> onReturnPressed;
    std::function<void()> onEscapePressed;

    // Size hints
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;

    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void setupComponents();
    void updateEditorStyle();
    void validateAndUpdate();
    void incrementValue();
    void decrementValue();
    void applyNumericConstraints(double& value);
    void updateFromNumericValue();
    void notifyTextChanged();
    void notifyValueChanged();

    // Rendering
    void paintBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintSearchIcon(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // Configuration
    TextFieldVariant variant_ = TextFieldVariant::Text;
    juce::String placeholder_ = "Enter value...";
    bool enabled_ = true;
    juce::String errorMessage_;

    // Number variant
    double numValue_ = 0.0;
    double defaultValue_ = 0.0;
    double minValue_ = 0.0;
    double maxValue_ = 100.0;
    double step_ = 1.0;
    int decimalPlaces_ = 0;
    juce::String suffix_;

    // Child components
    std::unique_ptr<juce::TextEditor> editor_;
    std::unique_ptr<OscilButton> decrementButton_;
    std::unique_ptr<OscilButton> incrementButton_;

    // APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;
    juce::Slider internalSlider_;

    // Validation
    Callbacks::ValidationCallback validator_;

    // Animation
    SpringAnimation focusSpring_;
    float focusAmount_ = 0.0f;

    // Theme
    ColorTheme theme_;

    // Layout constants
    static constexpr int ICON_WIDTH = 32;
    static constexpr int STEPPER_WIDTH = 28;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilTextField)
};

} // namespace oscil
