/*
    Oscil - Dropdown Component Interaction & Accessibility
    (Core setup and painting are in OscilDropdown.cpp)
*/

#include "ui/components/OscilDropdown.h"

namespace oscil
{

void OscilDropdown::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    if (popupVisible_)
        hidePopup();
    else
        showPopup();
}

void OscilDropdown::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!enabled_)
        return;

    isHovered_ = true;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(1.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 1.0f;
        repaint();
    }
}

void OscilDropdown::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;

    if (AnimationSettings::shouldUseSpringAnimations())
    {
        hoverSpring_.setTarget(0.0f);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        hoverSpring_.position = 0.0f;
        repaint();
    }
}

bool OscilDropdown::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_)
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        if (popupVisible_)
            hidePopup();
        else
            showPopup();
        return true;
    }

    if (key == juce::KeyPress::escapeKey && popupVisible_)
    {
        hidePopup();
        return true;
    }

    if (!popupVisible_ && !items_.empty())
    {
        int const current = getSelectedIndex();

        if (key == juce::KeyPress::upKey)
        {
            setSelectedIndex(std::max(0, current - 1));
            return true;
        }

        if (key == juce::KeyPress::downKey)
        {
            setSelectedIndex(std::min(static_cast<int>(items_.size()) - 1, current + 1));
            return true;
        }
    }

    return false;
}

void OscilDropdown::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void OscilDropdown::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void OscilDropdown::timerCallback()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    hoverSpring_.update(dt);
    chevronSpring_.update(dt);

    if (hoverSpring_.isSettled() && chevronSpring_.isSettled())
        stopTimer();

    repaint();
}

// Custom accessibility handler for dropdown with selection announcements
class OscilDropdownAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilDropdownAccessibilityHandler(OscilDropdown& dropdown)
        : juce::AccessibilityHandler(dropdown, juce::AccessibilityRole::comboBox,
                                     juce::AccessibilityActions()
                                         .addAction(juce::AccessibilityActionType::press,
                                                    [&dropdown] {
                                                        if (dropdown.isEnabled())
                                                            dropdown.showPopup();
                                                    })
                                         .addAction(juce::AccessibilityActionType::showMenu,
                                                    [&dropdown] {
                                                        if (dropdown.isEnabled())
                                                            dropdown.showPopup();
                                                    }))
        , dropdown_(dropdown)
    {
    }

    juce::String getTitle() const override { return dropdown_.getPlaceholder(); }

    juce::String getDescription() const override
    {
        juce::String const selection = dropdown_.getSelectedLabel();
        if (selection.isEmpty())
            return "No selection";

        int const numItems = dropdown_.getNumItems();
        return "Selected: " + selection + " (" + juce::String(numItems) + " options available)";
    }

    juce::String getHelp() const override
    {
        return "Press Space or Enter to open dropdown. Use arrow keys to navigate.";
    }

private:
    OscilDropdown& dropdown_;
};

std::unique_ptr<juce::AccessibilityHandler> OscilDropdown::createAccessibilityHandler()
{
    return std::make_unique<OscilDropdownAccessibilityHandler>(*this);
}

} // namespace oscil
