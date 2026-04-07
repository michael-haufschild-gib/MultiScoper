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
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, itemId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            comboBox->setSelectedId(itemId, juce::sendNotification);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::selectByText(const juce::String& elementId, const juce::String& text)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, text, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            for (int i = 0; i < comboBox->getNumItems(); ++i)
            {
                if (comboBox->getItemText(i) == text)
                {
                    comboBox->setSelectedItemIndex(i, juce::sendNotification);
                    result = true;
                    break;
                }
            }
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::selectById(const juce::String& elementId, const juce::String& itemId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, itemId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
        {
            for (int i = 0; i < oscilDropdown->getNumItems(); ++i)
            {
                if (oscilDropdown->getItem(i).id == itemId)
                {
                    oscilDropdown->setSelectedIndex(i, true);
                    result = true;
                    break;
                }
            }
        }
        else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        {
            int intId = itemId.getIntValue();
            if (intId > 0)
            {
                comboBox->setSelectedId(intId, juce::sendNotification);
                result = true;
            }
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::toggle(const juce::String& elementId, bool value)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, value, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* oscilToggle = dynamic_cast<oscil::OscilToggle*>(component))
        {
            oscilToggle->setValue(value);
            result = true;
        }
        else if (auto* toggleButton = dynamic_cast<juce::ToggleButton*>(component))
        {
            toggleButton->setToggleState(value, juce::sendNotification);
            result = true;
        }
        else if (auto* button = dynamic_cast<juce::Button*>(component))
        {
            button->setToggleState(value, juce::sendNotification);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::setSliderValue(const juce::String& elementId, double value)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, value, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* slider = dynamic_cast<juce::Slider*>(component))
        {
            slider->setValue(value, juce::sendNotification);
            result = true;
        }
        else if (auto* textField = dynamic_cast<oscil::OscilTextField*>(component))
        {
            textField->setNumericValue(value, true);
            result = true;
        }
        else if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
        {
            oscilSlider->setValue(value);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::incrementSlider(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
        {
            double step = oscilSlider->getStep();
            if (step == 0.0)
                step = (oscilSlider->getMaximum() - oscilSlider->getMinimum()) / 100.0;
            oscilSlider->setValue(oscilSlider->getValue() + step);
            result = true;
        }
        else if (auto* slider = dynamic_cast<juce::Slider*>(component))
        {
            double interval = slider->getInterval();
            if (interval == 0.0)
                interval = (slider->getMaximum() - slider->getMinimum()) / 100.0;
            slider->setValue(slider->getValue() + interval, juce::sendNotification);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::decrementSlider(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
        {
            double step = oscilSlider->getStep();
            if (step == 0.0)
                step = (oscilSlider->getMaximum() - oscilSlider->getMinimum()) / 100.0;
            oscilSlider->setValue(oscilSlider->getValue() - step);
            result = true;
        }
        else if (auto* slider = dynamic_cast<juce::Slider*>(component))
        {
            double interval = slider->getInterval();
            if (interval == 0.0)
                interval = (slider->getMaximum() - slider->getMinimum()) / 100.0;
            slider->setValue(slider->getValue() - interval, juce::sendNotification);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::resetSliderToDefault(const juce::String& elementId)
{
    // Simulate double-click which typically resets to default
    return doubleClick(elementId);
}

bool TestUIController::typeText(const juce::String& elementId, const juce::String& text)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, text, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }

        if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
        {
            textEditor->setText(text, juce::sendNotification);
            result = true;
        }
        else if (auto* oscilField = dynamic_cast<oscil::OscilTextField*>(component))
        {
            oscilField->setText(text, true);
            result = true;
        }
        else if (auto* inlineLabel = dynamic_cast<oscil::InlineEditLabel*>(component))
        {
            inlineLabel->setText(text, true);
            result = true;
        }
        else if (auto* label = dynamic_cast<juce::Label*>(component))
        {
            if (label->isEditable())
            {
                label->setText(text, juce::sendNotification);
                result = true;
            }
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::clearText(const juce::String& elementId) { return typeText(elementId, juce::String()); }

} // namespace oscil::test
