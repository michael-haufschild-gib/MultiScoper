/*
    MultiScoper Test Harness - UI Controller: Form Interactions
*/

#include "ui/components/InlineEditLabel.h"
#include "ui/components/MultiScoperButton.h"
#include "ui/components/MultiScoperDropdown.h"
#include "ui/components/MultiScoperSlider.h"
#include "ui/components/MultiScoperTextField.h"
#include "ui/components/MultiScoperToggle.h"

#include "TestDAW.h"
#include "TestUIController.h"

namespace multiscoper::test
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

        if (auto* multiscoperDropdown = dynamic_cast<multiscoper::MultiScoperDropdown*>(component))
        {
            for (int i = 0; i < multiscoperDropdown->getNumItems(); ++i)
            {
                if (multiscoperDropdown->getItem(i).id == itemId)
                {
                    multiscoperDropdown->setSelectedIndex(i, true);
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

        if (auto* multiscoperToggle = dynamic_cast<multiscoper::MultiScoperToggle*>(component))
        {
            multiscoperToggle->setValue(value);
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
        if (auto* textField = dynamic_cast<multiscoper::MultiScoperTextField*>(component))
        {
            textField->setNumericValue(value, true);
            return true;
        }
        if (auto* multiscoperSlider = dynamic_cast<multiscoper::MultiScoperSlider*>(component))
        {
            multiscoperSlider->setValue(value);
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

        if (auto* multiscoperSlider = dynamic_cast<multiscoper::MultiScoperSlider*>(component))
        {
            double step = multiscoperSlider->getStep();
            if (step == 0.0)
                step = (multiscoperSlider->getMaximum() - multiscoperSlider->getMinimum()) / 100.0;
            multiscoperSlider->setValue(multiscoperSlider->getValue() + step * direction);
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
        if (auto* multiscoperField = dynamic_cast<multiscoper::MultiScoperTextField*>(component))
        {
            multiscoperField->setText(text, true);
            return true;
        }
        if (auto* inlineLabel = dynamic_cast<multiscoper::InlineEditLabel*>(component))
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

} // namespace multiscoper::test
