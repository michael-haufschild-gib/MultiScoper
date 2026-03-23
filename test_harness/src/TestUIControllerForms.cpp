/*
    Oscil Test Harness - UI Controller: Form Interactions
*/

#include "ui/components/InlineEditLabel.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"

#include "TestUIController.h"

namespace oscil::test
{

// ================== Form Interactions ==================

bool TestUIController::select(const juce::String& elementId, int itemId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* comboBox = dynamic_cast<juce::ComboBox*>(component);
    if (comboBox == nullptr)
        return false;

    juce::MessageManager::callAsync([comboBox, itemId]() { comboBox->setSelectedId(itemId, juce::sendNotification); });

    return true;
}

bool TestUIController::selectByText(const juce::String& elementId, const juce::String& text)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* comboBox = dynamic_cast<juce::ComboBox*>(component);
    if (comboBox == nullptr)
        return false;

    juce::MessageManager::callAsync([comboBox, text]() { comboBox->setText(text, juce::sendNotification); });

    return true;
}

bool TestUIController::selectById(const juce::String& elementId, const juce::String& itemId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    // Handle OscilDropdown
    if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
    {
        int targetIndex = -1;
        for (int i = 0; i < oscilDropdown->getNumItems(); ++i)
        {
            if (oscilDropdown->getItem(i).id == itemId)
            {
                targetIndex = i;
                break;
            }
        }

        if (targetIndex < 0)
            return false;

        juce::MessageManager::callAsync(
            [oscilDropdown, targetIndex]() { oscilDropdown->setSelectedIndex(targetIndex, true); });

        return true;
    }

    // Fallback: try as juce::ComboBox with integer ID
    if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        int intId = itemId.getIntValue();
        if (intId > 0)
        {
            juce::MessageManager::callAsync(
                [comboBox, intId]() { comboBox->setSelectedId(intId, juce::sendNotification); });
            return true;
        }
    }

    return false;
}

bool TestUIController::toggle(const juce::String& elementId, bool value)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* toggleButton = dynamic_cast<juce::ToggleButton*>(component);
    if (toggleButton != nullptr)
    {
        juce::MessageManager::callAsync(
            [toggleButton, value]() { toggleButton->setToggleState(value, juce::sendNotification); });
        return true;
    }

    auto* button = dynamic_cast<juce::Button*>(component);
    if (button != nullptr)
    {
        juce::MessageManager::callAsync([button, value]() { button->setToggleState(value, juce::sendNotification); });
        return true;
    }

    return false;
}

bool TestUIController::setSliderValue(const juce::String& elementId, double value)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        juce::Component::SafePointer<juce::Slider> safe(slider);
        juce::MessageManager::callAsync([safe, value]() {
            if (auto* s = safe.getComponent())
                s->setValue(value, juce::sendNotification);
        });
        return true;
    }

    if (auto* textField = dynamic_cast<oscil::OscilTextField*>(component))
    {
        juce::Component::SafePointer<oscil::OscilTextField> safe(textField);
        juce::MessageManager::callAsync([safe, value]() {
            if (auto* tf = safe.getComponent())
                tf->setNumericValue(value, true);
        });
        return true;
    }

    if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
    {
        juce::Component::SafePointer<oscil::OscilSlider> safe(oscilSlider);
        juce::MessageManager::callAsync([safe, value]() {
            if (auto* s = safe.getComponent())
                s->setValue(value);
        });
        return true;
    }

    return false;
}

bool TestUIController::incrementSlider(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::MessageManager::callAsync([slider]() {
        double interval = slider->getInterval();
        if (interval == 0.0)
            interval = (slider->getMaximum() - slider->getMinimum()) / 100.0;
        slider->setValue(slider->getValue() + interval, juce::sendNotification);
    });

    return true;
}

bool TestUIController::decrementSlider(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::MessageManager::callAsync([slider]() {
        double interval = slider->getInterval();
        if (interval == 0.0)
            interval = (slider->getMaximum() - slider->getMinimum()) / 100.0;
        slider->setValue(slider->getValue() - interval, juce::sendNotification);
    });

    return true;
}

bool TestUIController::resetSliderToDefault(const juce::String& elementId)
{
    // Simulate double-click which typically resets to default
    return doubleClick(elementId);
}

bool TestUIController::typeText(const juce::String& elementId, const juce::String& text)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
    {
        juce::Component::SafePointer<juce::TextEditor> safe(textEditor);
        juce::MessageManager::callAsync([safe, text]() {
            if (auto* te = safe.getComponent())
                te->setText(text, juce::sendNotification);
        });
        return true;
    }

    if (auto* oscilField = dynamic_cast<oscil::OscilTextField*>(component))
    {
        juce::Component::SafePointer<oscil::OscilTextField> safe(oscilField);
        juce::MessageManager::callAsync([safe, text]() {
            if (auto* tf = safe.getComponent())
                tf->setText(text, true);
        });
        return true;
    }

    if (auto* inlineLabel = dynamic_cast<oscil::InlineEditLabel*>(component))
    {
        juce::Component::SafePointer<oscil::InlineEditLabel> safe(inlineLabel);
        juce::MessageManager::callAsync([safe, text]() {
            if (auto* il = safe.getComponent())
                il->setText(text, true);
        });
        return true;
    }

    if (auto* label = dynamic_cast<juce::Label*>(component))
    {
        if (label->isEditable())
        {
            juce::Component::SafePointer<juce::Label> safe(label);
            juce::MessageManager::callAsync([safe, text]() {
                if (auto* l = safe.getComponent())
                    l->setText(text, juce::sendNotification);
            });
            return true;
        }
    }

    return false;
}

bool TestUIController::clearText(const juce::String& elementId) { return typeText(elementId, juce::String()); }

} // namespace oscil::test
