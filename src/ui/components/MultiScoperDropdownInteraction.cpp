/*
    MultiScoper - Dropdown Component Interaction & Accessibility
    (Core setup and painting are in MultiScoperDropdown.cpp)
*/

#include "ui/components/MultiScoperDropdown.h"

namespace multiscoper
{

void MultiScoperDropdown::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (!isEnabled())
        return;

    if (popupVisible_)
        hidePopup();
    else
        showPopup();
}

void MultiScoperDropdown::mouseEnter(const juce::MouseEvent& /*event*/)
{
    if (!isEnabled())
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

void MultiScoperDropdown::mouseExit(const juce::MouseEvent& /*event*/)
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

bool MultiScoperDropdown::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    // Space/return deliberately not handled here — they must pass through to
    // the DAW for transport shortcuts. Use mouse-click to open the popup.
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

void MultiScoperDropdown::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void MultiScoperDropdown::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

void MultiScoperDropdown::timerCallback()
{
    float const dt = AnimationTiming::FRAME_DURATION_60FPS;
    hoverSpring_.update(dt);
    chevronSpring_.update(dt);

    if (hoverSpring_.isSettled() && chevronSpring_.isSettled())
        stopTimer();

    repaint();
}

// Custom accessibility handler for dropdown with selection announcements
class MultiScoperDropdownAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit MultiScoperDropdownAccessibilityHandler(MultiScoperDropdown& dropdown)
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
    MultiScoperDropdown& dropdown_;
};

std::unique_ptr<juce::AccessibilityHandler> MultiScoperDropdown::createAccessibilityHandler()
{
    return std::make_unique<MultiScoperDropdownAccessibilityHandler>(*this);
}

} // namespace multiscoper
