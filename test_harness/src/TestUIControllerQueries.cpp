/*
    Oscil Test Harness - UI Controller: Focus Management, State Queries & Waits
*/

#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilToggle.h"

#include "TestDAW.h"
#include "TestUIController.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace oscil::test
{

// ================== Focus Management ==================

bool TestUIController::setFocus(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            comp->grabKeyboardFocus();
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

juce::String TestUIController::getFocusedElementId()
{
    juce::String result;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([&result, &done]() {
        auto* focused = juce::Component::getCurrentlyFocusedComponent();
        if (focused != nullptr)
        {
            auto elements = TestElementRegistry::getInstance().getAllElements();
            for (const auto& [testId, component] : elements)
            {
                if (component == focused)
                {
                    result = testId;
                    break;
                }
            }
        }
        done.signal();
    });
    done.wait(3000);
    return result;
}

juce::String TestUIController::getFocusedElementIdOnMessageThread()
{
    // Must be called from the message thread — no dispatch needed
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
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
            result = comp->hasKeyboardFocus(true);
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::focusNext()
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([&result, &done]() {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        {
            focused->moveKeyboardFocusToSibling(true);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::focusPrevious()
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([&result, &done]() {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        {
            focused->moveKeyboardFocusToSibling(false);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

// ================== State Queries ==================

json TestUIController::getUIState()
{
    json state;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, &state, &done]() {
        state["elements"] = json::object();
        auto elements = TestElementRegistry::getInstance().getAllElements();
        for (const auto& [testId, component] : elements)
            state["elements"][testId.toStdString()] = componentToJson(component, testId);
        state["focusedElement"] = getFocusedElementIdOnMessageThread().toStdString();
        done.signal();
    });
    done.wait(3000);
    return state;
}

json TestUIController::getElementInfo(const juce::String& elementId)
{
    json result;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
            result = json{{"error", "Element not found"}};
        else
            result = componentToJson(component, elementId);
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::isElementVisible(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
            result = comp->isVisible() && comp->isShowing();
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::isElementEnabled(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
            result = comp->isEnabled();
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::isElementFocusable(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
            result = comp->getWantsKeyboardFocus();
        done.signal();
    });
    done.wait(3000);
    return result;
}

double TestUIController::getSliderValue(const juce::String& elementId)
{
    double result = 0.0;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
            result = oscilSlider->getValue();
        else if (auto* slider = dynamic_cast<juce::Slider*>(component))
            result = slider->getValue();
        done.signal();
    });
    done.wait(3000);
    return result;
}

std::pair<double, double> TestUIController::getSliderRange(const juce::String& elementId)
{
    std::pair<double, double> result{0.0, 1.0};
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
            result = {oscilSlider->getMinimum(), oscilSlider->getMaximum()};
        else if (auto* slider = dynamic_cast<juce::Slider*>(component))
            result = {slider->getMinimum(), slider->getMaximum()};
        done.signal();
    });
    done.wait(3000);
    return result;
}

bool TestUIController::getToggleState(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* oscilToggle = dynamic_cast<oscil::OscilToggle*>(component))
            result = oscilToggle->getValue();
        else if (auto* button = dynamic_cast<juce::Button*>(component))
            result = button->getToggleState();
        done.signal();
    });
    done.wait(3000);
    return result;
}

juce::String TestUIController::getTextContent(const juce::String& elementId)
{
    juce::String result;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* oscilTextField = dynamic_cast<oscil::OscilTextField*>(component))
            result = oscilTextField->getText();
        else if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
            result = textEditor->getText();
        else if (auto* label = dynamic_cast<juce::Label*>(component))
            result = label->getText();
        else if (auto* button = dynamic_cast<juce::Button*>(component))
            result = button->getButtonText();
        done.signal();
    });
    done.wait(3000);
    return result;
}

int TestUIController::getSelectedItemId(const juce::String& elementId)
{
    int result = 0;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
            result = oscilDropdown->getSelectedIndex();
        else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
            result = comboBox->getSelectedId();
        done.signal();
    });
    done.wait(3000);
    return result;
}

// ================== Waits ==================

bool TestUIController::waitForElement(const juce::String& elementId, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        if (TestElementRegistry::getInstance().hasElement(elementId))
            return true;

        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

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

        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

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

        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool TestUIController::waitForSliderValue(const juce::String& elementId, double value, double tolerance, int timeoutMs)
{
    auto startTime = std::chrono::steady_clock::now();

    while (true)
    {
        double currentValue = getSliderValue(elementId);
        if (std::abs(currentValue - value) <= tolerance)
            return true;

        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace oscil::test
