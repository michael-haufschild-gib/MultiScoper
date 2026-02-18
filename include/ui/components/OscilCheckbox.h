/*
    Oscil - Checkbox Component
    Tri-state checkbox with animated transitions
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/AnimationSettings.h"
#include "ui/components/TestId.h"
#include "ui/animation/OscilAnimationService.h"

namespace oscil
{

/**
 * A themed checkbox component with tri-state support
 *
 * Features:
 * - Three states: Unchecked, Checked, Indeterminate
 * - Animated check mark with spring physics
 * - Optional label
 * - Full keyboard accessibility
 */
class OscilCheckbox : public ThemedComponent,
                      public TestIdSupport
{
public:
    explicit OscilCheckbox(IThemeService& themeService);
    OscilCheckbox(IThemeService& themeService, const juce::String& label);
    OscilCheckbox(IThemeService& themeService, const juce::String& label, const juce::String& testId);
    ~OscilCheckbox() override;

    // State control
    void setChecked(bool checked, bool notify = true);
    bool isChecked() const { return state_ == CheckState::Checked; }

    void setState(CheckState state, bool notify = true);
    CheckState getState() const { return state_; }

    void toggle();

    // Configuration
    void setLabel(const juce::String& label);
    juce::String getLabel() const { return label_; }

    void setLabelOnRight(bool onRight);
    bool isLabelOnRight() const { return labelOnRight_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Callbacks
    std::function<void(bool)> onCheckedChanged;
    std::function<void(CheckState)> onStateChanged;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;


    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void notifyStateChanged();

    // Rendering
    void paintBox(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintCheckMark(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintIndeterminate(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // State
    CheckState state_ = CheckState::Unchecked;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool isPressed_ = false;
    bool hasFocus_ = false;
    juce::String label_;
    bool labelOnRight_ = true;

    // Animation state
    float checkProgress_ = 0.0f;      // 0 = unchecked, 1 = checked
    float hoverProgress_ = 0.0f;
    OscilAnimationService* animService_ = nullptr;
    ScopedAnimator checkAnimator_;
    ScopedAnimator hoverAnimator_;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilCheckbox)
};

} // namespace oscil
