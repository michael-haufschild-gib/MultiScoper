/*
    Oscil - Slider Accessibility Handler
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/OscilSlider.h"

namespace oscil
{

class OscilSliderAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilSliderAccessibilityHandler(OscilSlider& slider)
        : juce::AccessibilityHandler(slider, juce::AccessibilityRole::slider,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press, [&slider] { slider.setValue(slider.getDefaultValue(), true); })
        )
        , slider_(slider)
    {
    }

    juce::String getTitle() const override
    {
        if (slider_.getLabel().isNotEmpty())
            return slider_.getLabel();
        return "Slider";
    }

    juce::String getDescription() const override
    {
        // Provide current value and range for screen readers
        juce::String text = juce::String(slider_.getValue(), slider_.getDecimalPlaces());
        if (slider_.getSuffix().isNotEmpty())
            text += " " + slider_.getSuffix();

        juce::String range = " (Range: " + juce::String(slider_.getMinimum(), slider_.getDecimalPlaces())
            + " to " + juce::String(slider_.getMaximum(), slider_.getDecimalPlaces()) + ")";

        return "Value: " + text + range;
    }

    juce::String getHelp() const override
    {
        return "Use arrow keys to adjust. Double-click to reset to default.";
    }

private:
    OscilSlider& slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilSliderAccessibilityHandler)
};

} // namespace oscil
