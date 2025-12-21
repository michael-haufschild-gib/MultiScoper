/*
    Oscil - Button Accessibility Handler
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/OscilButton.h"

namespace oscil
{

class OscilButtonAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilButtonAccessibilityHandler(OscilButton& button)
        : juce::AccessibilityHandler(button,
            button.isToggleable() ? juce::AccessibilityRole::toggleButton : juce::AccessibilityRole::button,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press,
                    [&button] { if (button.isEnabled()) button.triggerClick(); })
          )
        , button_(button)
    {
    }

    juce::String getTitle() const override
    {
        return button_.getText().isNotEmpty() ? button_.getText() : "Button";
    }

    juce::String getDescription() const override
    {
        juce::String desc;

        if (!button_.isEnabled())
            desc = "Disabled";
        else if (button_.isToggleable())
            desc = button_.isToggled() ? "Selected" : "Not selected";

        if (button_.getShortcut().isValid())
        {
            if (desc.isNotEmpty())
                desc += ". ";
            desc += "Shortcut: " + button_.getShortcut().getTextDescription();
        }

        return desc;
    }

    juce::String getHelp() const override
    {
        return "Press Enter or Space to activate.";
    }

    juce::AccessibleState getCurrentState() const override
    {
        auto state = AccessibilityHandler::getCurrentState();
        if (button_.isToggleable() && button_.isToggled())
            state = state.withChecked();
        return state;
    }

private:
    OscilButton& button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilButtonAccessibilityHandler)
};

} // namespace oscil
