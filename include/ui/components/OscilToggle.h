/*
    Oscil - Toggle Component
    Animated toggle switch with smooth ease transitions
*/

#pragma once

#include "ui/components/AnimationSettings.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/TestId.h"
#include "ui/components/ThemedComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

/**
 * An animated toggle switch component
 *
 * Features:
 * - Spring-based knob animation with slight overshoot
 * - Celebration "pop" effect on activation
 * - APVTS attachment support for DAW automation
 * - Full keyboard accessibility
 * - Optional label
 * - E2E test automation via testId
 */
class OscilToggle
    : public ThemedComponent
    , public TestIdSupport
    , private juce::Timer
{
public:
    OscilToggle(IThemeService& themeService);
    OscilToggle(IThemeService& themeService, const juce::String& label);
    OscilToggle(IThemeService& themeService, const juce::String& label, const juce::String& testId);
    ~OscilToggle() override;

    // Value control
    void setValue(bool value, bool animate = true);
    bool getValue() const { return value_; }

    void toggle();

    // Configuration
    void setLabel(const juce::String& label);
    juce::String getLabel() const { return label_; }

    void setLabelOnRight(bool onRight);
    bool isLabelOnRight() const { return labelOnRight_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    /// Bind this toggle to an APVTS parameter for two-way synchronization.
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);
    void detachFromParameter();
    bool isAttachedToParameter() const { return attachment_ != nullptr; }

    // Callbacks
    std::function<void(bool)> onValueChanged;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;
    void updateAnimations();
    void notifyValueChanged() const;

    // Rendering
    void paintTrack(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintKnob(juce::Graphics& g, const juce::Rectangle<float>& trackBounds) const;
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // State
    bool value_ = false;
    bool enabled_ = true;
    bool hasFocus_ = false;
    juce::String label_;
    bool labelOnRight_ = true;

    // Animation
    SpringAnimation positionSpring_; // Knob position (0 = off, 1 = on)

    // APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment_;

    // Internal toggle button for APVTS attachment compatibility
    juce::ToggleButton internalButton_;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilToggle)
};

} // namespace oscil
