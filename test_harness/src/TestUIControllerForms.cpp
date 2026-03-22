/*
    Oscil Test Harness - UI Controller: Form Interactions, Focus & State Queries
*/

#include "TestUIController.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilButton.h"
#include <thread>
#include <chrono>

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

    juce::MessageManager::callAsync([comboBox, itemId]()
    {
        comboBox->setSelectedId(itemId, juce::sendNotification);
    });

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

    juce::MessageManager::callAsync([comboBox, text]()
    {
        comboBox->setText(text, juce::sendNotification);
    });

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

        juce::MessageManager::callAsync([oscilDropdown, targetIndex]()
        {
            oscilDropdown->setSelectedIndex(targetIndex, true);
        });

        return true;
    }

    // Fallback: try as juce::ComboBox with integer ID
    if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        int intId = itemId.getIntValue();
        if (intId > 0)
        {
            juce::MessageManager::callAsync([comboBox, intId]()
            {
                comboBox->setSelectedId(intId, juce::sendNotification);
            });
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
        juce::MessageManager::callAsync([toggleButton, value]()
        {
            toggleButton->setToggleState(value, juce::sendNotification);
        });
        return true;
    }

    auto* button = dynamic_cast<juce::Button*>(component);
    if (button != nullptr)
    {
        juce::MessageManager::callAsync([button, value]()
        {
            button->setToggleState(value, juce::sendNotification);
        });
        return true;
    }

    return false;
}

bool TestUIController::setSliderValue(const juce::String& elementId, double value)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::MessageManager::callAsync([slider, value]()
    {
        slider->setValue(value, juce::sendNotification);
    });

    return true;
}

bool TestUIController::incrementSlider(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto* slider = dynamic_cast<juce::Slider*>(component);
    if (slider == nullptr)
        return false;

    juce::MessageManager::callAsync([slider]()
    {
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

    juce::MessageManager::callAsync([slider]()
    {
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

    auto* textEditor = dynamic_cast<juce::TextEditor*>(component);
    if (textEditor != nullptr)
    {
        juce::MessageManager::callAsync([textEditor, text]()
        {
            textEditor->setText(text, juce::sendNotification);
        });
        return true;
    }

    auto* label = dynamic_cast<juce::Label*>(component);
    if (label != nullptr && label->isEditable())
    {
        juce::MessageManager::callAsync([label, text]()
        {
            label->setText(text, juce::sendNotification);
        });
        return true;
    }

    return false;
}

bool TestUIController::clearText(const juce::String& elementId)
{
    return typeText(elementId, juce::String());
}

// ================== Focus Management ==================

bool TestUIController::setFocus(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component]()
    {
        component->grabKeyboardFocus();
    });

    return true;
}

juce::String TestUIController::getFocusedElementId()
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == nullptr)
        return {};

    auto elements = TestElementRegistry::getInstance().getAllElements();
    for (const auto& [testId, component] : elements)
    {
        if (component == focused)
            return testId;
    }

    return {};
}

bool TestUIController::hasFocus(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    return component->hasKeyboardFocus(true);
}

bool TestUIController::focusNext()
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == nullptr)
        return false;

    juce::MessageManager::callAsync([focused]()
    {
        focused->moveKeyboardFocusToSibling(true);
    });

    return true;
}

bool TestUIController::focusPrevious()
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == nullptr)
        return false;

    juce::MessageManager::callAsync([focused]()
    {
        focused->moveKeyboardFocusToSibling(false);
    });

    return true;
}

// ================== State Queries ==================

json TestUIController::getUIState()
{
    json state;
    state["elements"] = json::object();

    auto elements = TestElementRegistry::getInstance().getAllElements();
    for (const auto& [testId, component] : elements)
    {
        state["elements"][testId.toStdString()] = componentToJson(component, testId);
    }

    auto focusedId = getFocusedElementId();
    state["focusedElement"] = focusedId.toStdString();

    return state;
}

json TestUIController::getElementInfo(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
    {
        return json{{"error", "Element not found"}};
    }

    return componentToJson(component, elementId);
}

bool TestUIController::isElementVisible(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    return component->isVisible() && component->isShowing();
}

bool TestUIController::isElementEnabled(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    return component->isEnabled();
}

bool TestUIController::isElementFocusable(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    return component->getWantsKeyboardFocus();
}

double TestUIController::getSliderValue(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        return slider->getValue();
    return 0.0;
}

std::pair<double, double> TestUIController::getSliderRange(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        return { slider->getMinimum(), slider->getMaximum() };
    return { 0.0, 1.0 };
}

bool TestUIController::getToggleState(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (auto* button = dynamic_cast<juce::Button*>(component))
        return button->getToggleState();
    return false;
}

juce::String TestUIController::getTextContent(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);

    if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
        return textEditor->getText();

    if (auto* label = dynamic_cast<juce::Label*>(component))
        return label->getText();

    if (auto* button = dynamic_cast<juce::Button*>(component))
        return button->getButtonText();

    return {};
}

int TestUIController::getSelectedItemId(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
        return comboBox->getSelectedId();
    return 0;
}

// ================== Waits ==================

bool TestUIController::waitForElement(const juce::String& elementId, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        if (TestElementRegistry::getInstance().hasElement(elementId))
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool TestUIController::waitForVisible(const juce::String& elementId, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        if (isElementVisible(elementId))
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool TestUIController::waitForEnabled(const juce::String& elementId, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        if (isElementEnabled(elementId))
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool TestUIController::waitForSliderValue(const juce::String& elementId, double value,
                                           double tolerance, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        double currentValue = getSliderValue(elementId);
        if (std::abs(currentValue - value) <= tolerance)
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace oscil::test
