/*
    MultiScoper - Radio Button and Radio Group Components
    Mutually exclusive selection controls with animations
*/

#pragma once

#include "ui/components/AnimationSettings.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/TestId.h"
#include "ui/components/ThemedComponent.h"
#include "ui/theme/IThemeService.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace multiscoper
{

// Forward declaration
class MultiScoperRadioGroup;

/**
 * Individual radio button with animated selection
 *
 * Features:
 * - Animated selection dot with spring physics
 * - Optional label (left or right)
 * - Focus ring for keyboard navigation
 * - Designed to work within MultiScoperRadioGroup
 */
class MultiScoperRadioButton
    : public ThemedComponent
    , public TestIdSupport
    , private juce::Timer
{
public:
    explicit MultiScoperRadioButton(IThemeService& themeService);
    MultiScoperRadioButton(IThemeService& themeService, const juce::String& label);
    MultiScoperRadioButton(IThemeService& themeService, const juce::String& label, const juce::String& testId);
    ~MultiScoperRadioButton() override;

    // State
    void setSelected(bool selected, bool notify = true);
    bool isSelected() const { return selected_; }

    // Configuration
    void setLabel(const juce::String& label);
    juce::String getLabel() const { return label_; }

    void setLabelOnRight(bool onRight);
    bool isLabelOnRight() const { return labelOnRight_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Callbacks
    std::function<void()> onSelected;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    friend class MultiScoperRadioGroup;

    void timerCallback() override;
    void updateAnimations();
    void triggerSelection();
    int findOwnIndexInGroup() const;

    // Rendering
    void paintCircle(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintDot(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // State
    bool selected_ = false;
    bool enabled_ = true;
    bool isHovered_ = false;
    bool isPressed_ = false;
    bool hasFocus_ = false;
    juce::String label_;
    bool labelOnRight_ = true;

    // Parent group (if any)
    MultiScoperRadioGroup* parentGroup_ = nullptr;

    // Animation
    SpringAnimation selectionSpring_;
    SpringAnimation hoverSpring_;
    SpringAnimation scaleSpring_;

    // Theme

    static constexpr int RADIO_SIZE = 18;
    static constexpr int DOT_SIZE = 8;

    // TestIdSupport
    void registerTestId() override;
    MULTISCOPER_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScoperRadioButton)
};

/**
 * Container for radio buttons with mutual exclusivity
 *
 * Features:
 * - Manages collection of radio buttons
 * - Ensures only one selected at a time
 * - Horizontal or vertical layout
 * - Keyboard navigation between options
 */
class MultiScoperRadioGroup : public ThemedComponent

{
public:
    enum class Orientation : std::uint8_t
    {
        Horizontal,
        Vertical
    };

    explicit MultiScoperRadioGroup(IThemeService& themeService);
    MultiScoperRadioGroup(IThemeService& themeService, Orientation orientation);
    ~MultiScoperRadioGroup() override;

    // Options management
    void addOption(const juce::String& label);
    void addOptions(const std::initializer_list<juce::String>& labels);
    void clearOptions();

    int getNumOptions() const { return static_cast<int>(buttons_.size()); }

    // Selection
    void setSelectedIndex(int index, bool notify = true);
    int getSelectedIndex() const { return selectedIndex_; }

    juce::String getSelectedLabel() const;

    // Configuration
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return orientation_; }

    void setSpacing(int spacing);
    int getSpacing() const { return spacing_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Callbacks
    std::function<void(int)> onSelectionChanged;
    std::function<void(const juce::String&)> onSelectionChangedLabel;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    // Accessibility
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    // Access buttons (for internal use by MultiScoperRadioButton)
    MultiScoperRadioButton* getButton(int index) const
    {
        return (index >= 0 && static_cast<size_t>(index) < buttons_.size()) ? buttons_[static_cast<size_t>(index)].get()
                                                                            : nullptr;
    }

private:
    void handleButtonSelected(int index);
    void updateButtonStates();
    void layoutButtons();

    std::vector<std::unique_ptr<MultiScoperRadioButton>> buttons_;
    int selectedIndex_ = -1;
    Orientation orientation_ = Orientation::Vertical;
    int spacing_ = ComponentLayout::SPACING_SM;
    bool enabled_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScoperRadioGroup)
};

} // namespace multiscoper
