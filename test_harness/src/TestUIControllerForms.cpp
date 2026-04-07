/*
    Oscil Test Harness - UI Controller: Form Interactions
*/

#include "ui/components/InlineEditLabel.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilToggle.h"

#include "TestDAW.h"
#include "TestUIController.h"

namespace oscil::test
{

// ================== Form Interactions ==================

bool TestUIController::select(const juce::String& elementId, int itemId)
{
    return runOnMessageThreadSync(elementId, [itemId](juce::Component* component) -> bool {
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            comboBox->setSelectedId(itemId, juce::sendNotification);
            return true;
        }
        return false;
    });
}

bool TestUIController::selectByText(const juce::String& elementId, const juce::String& text)
{
    return runOnMessageThreadSync(elementId, [text](juce::Component* component) -> bool {
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            for (int i = 0; i < comboBox->getNumItems(); ++i)
            {
                if (comboBox->getItemText(i) == text)
                {
                    comboBox->setSelectedItemIndex(i, juce::sendNotification);
                    return true;
                }
            }
        }
        return false;
    });
}

bool TestUIController::selectById(const juce::String& elementId, const juce::String& itemId)
{
    return runOnMessageThreadSync(elementId, [itemId](juce::Component* component) -> bool {
        if (!component)
            return false;

        if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
        {
            for (int i = 0; i < oscilDropdown->getNumItems(); ++i)
            {
                if (oscilDropdown->getItem(i).id == itemId)
                {
                    oscilDropdown->setSelectedIndex(i, true);
                    return true;
                }
            }
        }
        else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            int intId = itemId.getIntValue();
            if (intId > 0)
            {
                comboBox->setSelectedId(intId, juce::sendNotification);
                return true;
            }
        }
        return false;
    });
}

bool TestUIController::toggle(const juce::String& elementId, bool value)
{
    return runOnMessageThreadSync(elementId, [value](juce::Component* component) -> bool {
        if (!component)
            return false;

        if (auto* oscilToggle = dynamic_cast<oscil::OscilToggle*>(component))
        {
            oscilToggle->setValue(value);
            return true;
        }
        if (auto* toggleButton = dynamic_cast<juce::ToggleButton*>(component))
        {
            toggleButton->setToggleState(value, juce::sendNotification);
            return true;
        }
        if (auto* button = dynamic_cast<juce::Button*>(component))
        {
            button->setToggleState(value, juce::sendNotification);
            return true;
        }
        return false;
    });
}

bool TestUIController::setSliderValue(const juce::String& elementId, double value)
{
    return runOnMessageThreadSync(elementId, [value](juce::Component* component) -> bool {
        if (!component)
            return false;

        if (auto* slider = dynamic_cast<juce::Slider*>(component))
        {
            slider->setValue(value, juce::sendNotification);
            return true;
        }
        if (auto* textField = dynamic_cast<oscil::OscilTextField*>(component))
        {
            textField->setNumericValue(value, true);
            return true;
        }
        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
        {
            oscilSlider->setValue(value);
            return true;
        }
        return false;
    });
}

bool TestUIController::adjustSlider(const juce::String& elementId, int direction)
{
    return runOnMessageThreadSync(elementId, [direction](juce::Component* component) -> bool {
        if (!component)
            return false;

        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
        {
            double step = oscilSlider->getStep();
            if (step == 0.0)
                step = (oscilSlider->getMaximum() - oscilSlider->getMinimum()) / 100.0;
            oscilSlider->setValue(oscilSlider->getValue() + step * direction);
            return true;
        }
        if (auto* slider = dynamic_cast<juce::Slider*>(component))
        {
            double interval = slider->getInterval();
            if (interval == 0.0)
                interval = (slider->getMaximum() - slider->getMinimum()) / 100.0;
            slider->setValue(slider->getValue() + interval * direction, juce::sendNotification);
            return true;
        }
        return false;
    });
}

bool TestUIController::incrementSlider(const juce::String& elementId) { return adjustSlider(elementId, +1); }

bool TestUIController::decrementSlider(const juce::String& elementId) { return adjustSlider(elementId, -1); }

bool TestUIController::resetSliderToDefault(const juce::String& elementId)
{
    // Simulate double-click which typically resets to default
    return doubleClick(elementId);
}

bool TestUIController::typeText(const juce::String& elementId, const juce::String& text)
{
    return runOnMessageThreadSync(elementId, [text](juce::Component* component) -> bool {
        if (!component)
            return false;

        if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
        {
            textEditor->setText(text, juce::sendNotification);
            return true;
        }
        if (auto* oscilField = dynamic_cast<oscil::OscilTextField*>(component))
        {
            oscilField->setText(text, true);
            return true;
        }
        if (auto* inlineLabel = dynamic_cast<oscil::InlineEditLabel*>(component))
        {
            inlineLabel->setText(text, true);
            return true;
        }
        if (auto* label = dynamic_cast<juce::Label*>(component))
        {
            if (label->isEditable())
            {
                label->setText(text, juce::sendNotification);
                return true;
            }
        }
        return false;
    });
}

bool TestUIController::clearText(const juce::String& elementId) { return typeText(elementId, juce::String()); }

} // namespace oscil::test
