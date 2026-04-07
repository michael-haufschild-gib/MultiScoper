/*
    Oscil - Slider Component
    Horizontal/vertical slider with APVTS support, magnetic snapping, and fine control
*/

#pragma once

#include "ui/components/AnimationSettings.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/MagneticSnapController.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/TestId.h"
#include "ui/components/ThemedComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace oscil
{

/**
 * A themed slider component with APVTS integration and premium interactions
 *
 * Features:
 * - Single, Range, and Vertical variants
 * - APVTS attachment for DAW automation
 * - Magnetic snapping to meaningful values
 * - Alt+drag for fine control (0.1x sensitivity)
 * - Shift+drag for coarse control (10x sensitivity)
 * - Double-click to reset to default
 * - Scroll wheel support
 * - Value tooltip on hover
 * - Full keyboard accessibility
 * - E2E test automation via testId
 */
class OscilSlider
    : public ThemedComponent
    , public TestIdSupport
    , private juce::Timer
{
public:
    OscilSlider(IThemeService& themeService);
    OscilSlider(IThemeService& themeService, SliderVariant variant);
    OscilSlider(IThemeService& themeService, const juce::String& testId);
    OscilSlider(IThemeService& themeService, SliderVariant variant, const juce::String& testId);
    ~OscilSlider() override;

    // Variant
    void setVariant(SliderVariant variant);
    SliderVariant getVariant() const { return variant_; }

    // Value control (single value mode)
    void setValue(double value, bool notify = true);
    double getValue() const { return value_; }

    // Range control (Range variant)
    void setRangeValues(double start, double end, bool notify = true);
    double getRangeStart() const { return rangeStart_; }
    double getRangeEnd() const { return rangeEnd_; }

    // Configuration
    void setRange(double min, double max);
    double getMinimum() const { return minValue_; }
    double getMaximum() const { return maxValue_; }

    void setStep(double step);
    double getStep() const { return step_; }

    void setDefaultValue(double defaultValue);
    double getDefaultValue() const { return defaultValue_; }

    void setSkewFactor(double skew);
    double getSkewFactor() const { return skewFactor_; }

    // Display
    void setLabel(const juce::String& label);
    juce::String getLabel() const { return label_; }

    void setSuffix(const juce::String& suffix);
    juce::String getSuffix() const { return suffix_; }

    void setDecimalPlaces(int places);
    int getDecimalPlaces() const { return decimalPlaces_; }

    void setShowValueOnHover(bool show);
    bool getShowValueOnHover() const { return showValueOnHover_; }

    // Magnetic snapping
    void setMagneticSnappingEnabled(bool enabled);
    bool isMagneticSnappingEnabled() const { return snapController_.isEnabled(); }

    void setMagneticPoints(const std::vector<double>& points);
    void addMagneticPoint(double point);
    void clearMagneticPoints();

    // Custom value formatting
    void setValueFormatter(Callbacks::FormatCallback formatter);

    // State
    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
    void setEnabled(bool enabled);
    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
    bool isEnabled() const { return enabled_; }

    /// Bind this slider to an APVTS parameter for two-way synchronization.
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);
    void detachFromParameter();
    bool isAttachedToParameter() const { return attachment_ != nullptr; }

    // Callbacks
    std::function<void(double)> onValueChanged;
    std::function<void(double, double)> onRangeChanged;
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;
    bool hitTest(int x, int y) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void updateAnimations();

    // Value handling
    double constrainValue(double value) const;
    double valueToProportionOfLength(double value) const;
    double proportionOfLengthToValue(double proportion) const;
    void triggerSnapFeedback();

    // Rendering
    void paintHorizontal(juce::Graphics& g);
    void paintVertical(juce::Graphics& g);
    void paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool isVertical);
    void paintThumb(juce::Graphics& g, float position, bool isVertical, bool isRangeEnd = false,
                    float labelOffset = 0.0f);
    void paintValueTooltip(juce::Graphics& g, float thumbPosition, bool isVertical);
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    juce::String formatValue(double value) const;

    // Hit testing
    bool hitTestThumb(const juce::Point<int>& point, float thumbPosition, bool isVertical) const;
    float getThumbPosition(bool isRangeEnd = false) const;

    // Configuration
    SliderVariant variant_ = SliderVariant::Single;
    double value_ = 0.0;
    double rangeStart_ = 0.0;
    double rangeEnd_ = 100.0;
    double minValue_ = 0.0;
    double maxValue_ = 100.0;
    double step_ = 1.0;
    double defaultValue_ = 0.0;
    double skewFactor_ = 1.0;
    juce::String label_;
    juce::String suffix_;
    int decimalPlaces_ = 1;
    bool enabled_ = true;
    bool showValueOnHover_ = true;

    // Magnetic snapping
    MagneticSnapController snapController_;
    bool justSnapped_ = false; // Local flag for animation feedback

    // State
    bool isHovered_ = false;
    bool isDragging_ = false;
    bool draggingRangeEnd_ = false;
    bool hasFocus_ = false;
    double dragStartValue_ = 0.0;
    juce::Point<int> dragStartPoint_;

    // Animation
    SpringAnimation thumbScale_;
    float currentThumbScale_ = 1.0f;

    // APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;
    juce::Slider internalSlider_;

    // Custom formatter
    Callbacks::FormatCallback valueFormatter_;

    // Layout constants
    static constexpr int TRACK_HEIGHT = 4;
    static constexpr int THUMB_SIZE = 14;
    static constexpr int TOOLTIP_HEIGHT = 24;
    static constexpr int TOOLTIP_PADDING = 8;
    static constexpr float THUMB_HIT_EXTRA = 15.0f; // Increased for WCAG 44px touch target

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilSlider)
};

} // namespace oscil
