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
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    auto* comboBox = dynamic_cast<juce::ComboBox*>(component);
    if (comboBox == nullptr)
        return false;

    juce::Component::SafePointer<juce::ComboBox> safe(comboBox);
    return juce::MessageManager::callAsync([safe, itemId]() {
        if (auto* cb = safe.getComponent())
            cb->setSelectedId(itemId, juce::sendNotification);
    });
}

bool TestUIController::selectByText(const juce::String& elementId, const juce::String& text)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    auto* comboBox = dynamic_cast<juce::ComboBox*>(component);
    if (comboBox == nullptr)
        return false;

    juce::Component::SafePointer<juce::ComboBox> safe(comboBox);
    return juce::MessageManager::callAsync([safe, text]() {
        if (auto* cb = safe.getComponent())
        {
            for (int i = 0; i < cb->getNumItems(); ++i)
            {
                if (cb->getItemText(i) == text)
                {
                    cb->setSelectedItemIndex(i, juce::sendNotification);
                    return;
                }
            }
        }
    });
}

bool TestUIController::selectById(const juce::String& elementId, const juce::String& itemId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    // Handle OscilDropdown — resolve inside async callback in case items change
    if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
    {
        juce::Component::SafePointer<oscil::OscilDropdown> safe(oscilDropdown);
        return juce::MessageManager::callAsync([safe, itemId]() {
            if (auto* dd = safe.getComponent())
            {
                for (int i = 0; i < dd->getNumItems(); ++i)
                {
                    if (dd->getItem(i).id == itemId)
                    {
                        dd->setSelectedIndex(i, true);
                        return;
                    }
                }
            }
        });
    }

    // Fallback: try as juce::ComboBox with integer ID
    if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        int intId = itemId.getIntValue();
        if (intId > 0)
        {
            juce::Component::SafePointer<juce::ComboBox> safe(comboBox);
            return juce::MessageManager::callAsync([safe, intId]() {
                if (auto* cb = safe.getComponent())
                    cb->setSelectedId(intId, juce::sendNotification);
            });
        }
    }

    return false;
}

bool TestUIController::toggle(const juce::String& elementId, bool value)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    if (auto* oscilToggle = dynamic_cast<oscil::OscilToggle*>(component))
    {
        juce::Component::SafePointer<oscil::OscilToggle> safe(oscilToggle);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* t = safe.getComponent())
                t->setValue(value);
        });
    }

    auto* toggleButton = dynamic_cast<juce::ToggleButton*>(component);
    if (toggleButton != nullptr)
    {
        juce::Component::SafePointer<juce::ToggleButton> safe(toggleButton);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* tb = safe.getComponent())
                tb->setToggleState(value, juce::sendNotification);
        });
    }

    auto* button = dynamic_cast<juce::Button*>(component);
    if (button != nullptr)
    {
        juce::Component::SafePointer<juce::Button> safe(button);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* b = safe.getComponent())
                b->setToggleState(value, juce::sendNotification);
        });
    }

    return false;
}

bool TestUIController::setSliderValue(const juce::String& elementId, double value)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        juce::Component::SafePointer<juce::Slider> safe(slider);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* s = safe.getComponent())
                s->setValue(value, juce::sendNotification);
        });
    }

    if (auto* textField = dynamic_cast<oscil::OscilTextField*>(component))
    {
        juce::Component::SafePointer<oscil::OscilTextField> safe(textField);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* tf = safe.getComponent())
                tf->setNumericValue(value, true);
        });
    }

    if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
    {
        juce::Component::SafePointer<oscil::OscilSlider> safe(oscilSlider);
        return juce::MessageManager::callAsync([safe, value]() {
            if (auto* s = safe.getComponent())
                s->setValue(value);
        });
    }

    return false;
}

bool TestUIController::incrementSlider(const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
    {
        juce::Component::SafePointer<oscil::OscilSlider> safe(oscilSlider);
        return juce::MessageManager::callAsync([safe]() {
            if (auto* s = safe.getComponent())
            {
                double step = s->getStep();
                if (step == 0.0)
                    step = (s->getMaximum() - s->getMinimum()) / 100.0;
                s->setValue(s->getValue() + step);
            }
        });
    }

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::Component::SafePointer<juce::Slider> safeSlider(slider);
    return juce::MessageManager::callAsync([safeSlider]() {
        if (auto* s = safeSlider.getComponent())
        {
            double interval = s->getInterval();
            if (interval == 0.0)
                interval = (s->getMaximum() - s->getMinimum()) / 100.0;
            s->setValue(s->getValue() + interval, juce::sendNotification);
        }
    });
}

bool TestUIController::decrementSlider(const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
    {
        juce::Component::SafePointer<oscil::OscilSlider> safe(oscilSlider);
        return juce::MessageManager::callAsync([safe]() {
            if (auto* s = safe.getComponent())
            {
                double step = s->getStep();
                if (step == 0.0)
                    step = (s->getMaximum() - s->getMinimum()) / 100.0;
                s->setValue(s->getValue() - step);
            }
        });
    }

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::Component::SafePointer<juce::Slider> safeSlider(slider);
    return juce::MessageManager::callAsync([safeSlider]() {
        if (auto* s = safeSlider.getComponent())
        {
            double interval = s->getInterval();
            if (interval == 0.0)
                interval = (s->getMaximum() - s->getMinimum()) / 100.0;
            s->setValue(s->getValue() - interval, juce::sendNotification);
        }
    });
}

bool TestUIController::resetSliderToDefault(const juce::String& elementId)
{
    // Simulate double-click which typically resets to default
    return doubleClick(elementId);
}

bool TestUIController::typeText(const juce::String& elementId, const juce::String& text)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
    {
        juce::Component::SafePointer<juce::TextEditor> safe(textEditor);
        return juce::MessageManager::callAsync([safe, text]() {
            if (auto* te = safe.getComponent())
                te->setText(text, juce::sendNotification);
        });
    }

    if (auto* oscilField = dynamic_cast<oscil::OscilTextField*>(component))
    {
        juce::Component::SafePointer<oscil::OscilTextField> safe(oscilField);
        return juce::MessageManager::callAsync([safe, text]() {
            if (auto* tf = safe.getComponent())
                tf->setText(text, true);
        });
    }

    if (auto* inlineLabel = dynamic_cast<oscil::InlineEditLabel*>(component))
    {
        juce::Component::SafePointer<oscil::InlineEditLabel> safe(inlineLabel);
        return juce::MessageManager::callAsync([safe, text]() {
            if (auto* il = safe.getComponent())
                il->setText(text, true);
        });
    }

    if (auto* label = dynamic_cast<juce::Label*>(component))
    {
        if (label->isEditable())
        {
            juce::Component::SafePointer<juce::Label> safe(label);
            return juce::MessageManager::callAsync([safe, text]() {
                if (auto* l = safe.getComponent())
                    l->setText(text, juce::sendNotification);
            });
        }
    }

    return false;
}

bool TestUIController::clearText(const juce::String& elementId) { return typeText(elementId, juce::String()); }

} // namespace oscil::test
