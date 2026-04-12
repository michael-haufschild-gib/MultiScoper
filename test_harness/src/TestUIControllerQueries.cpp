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
#include <limits>
#include <thread>

namespace oscil::test
{

// ================== Focus Management ==================

bool TestUIController::setFocus(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId, [](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        comp->grabKeyboardFocus();
        return true;
    });
}

juce::String TestUIController::getFocusedElementId()
{
    struct State
    {
        juce::String result;
        juce::WaitableEvent done;
    };
    auto state = std::make_shared<State>();
    juce::MessageManager::callAsync([state]() {
        auto* focused = juce::Component::getCurrentlyFocusedComponent();
        if (focused)
        {
            auto elements = TestElementRegistry::getInstance().getAllElements();
            for (const auto& [testId, component] : elements)
            {
                if (component == focused)
                {
                    state->result = testId;
                    break;
                }
            }
        }
        state->done.signal();
    });
    if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
        return {};
    return state->result;
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
    return runOnMessageThreadSync(elementId, [](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        return comp->hasKeyboardFocus(true);
    });
}

bool TestUIController::focusNext()
{
    return runOnMessageThreadSync([]() -> bool {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        {
            focused->moveKeyboardFocusToSibling(true);
            return true;
        }
        return false;
    });
}

bool TestUIController::focusPrevious()
{
    return runOnMessageThreadSync([]() -> bool {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        {
            focused->moveKeyboardFocusToSibling(false);
            return true;
        }
        return false;
    });
}

// ================== State Queries ==================

json TestUIController::getUIState()
{
    struct State
    {
        json result;
        juce::WaitableEvent done;
    };
    auto state = std::make_shared<State>();
    auto weak = self_;
    juce::MessageManager::callAsync([weak, state]() {
        auto locked = weak.lock();
        if (!isControllerAlive(locked))
        {
            state->done.signal();
            return;
        }
        auto* self = locked->controller;
        state->result["elements"] = json::object();
        auto elements = TestElementRegistry::getInstance().getAllElements();
        for (const auto& [testId, component] : elements)
            state->result["elements"][testId.toStdString()] = self->componentToJson(component, testId);
        state->result["focusedElement"] = self->getFocusedElementIdOnMessageThread().toStdString();
        state->done.signal();
    });
    if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
        return json{{"error", "timeout"}};
    return state->result;
}

json TestUIController::getElementInfo(const juce::String& elementId)
{
    return runOnMessageThreadSyncWithResult<json>(elementId, json{{"error", "Element not found"}},
                                                  [this, elementId](juce::Component* component) -> json {
                                                      if (!component)
                                                          return json{{"error", "Element not found"}};
                                                      return componentToJson(component, elementId);
                                                  });
}

bool TestUIController::isElementVisible(const juce::String& elementId)
{
    return runOnMessageThreadSync(
        elementId, [](juce::Component* comp) -> bool { return comp && comp->isVisible() && comp->isShowing(); });
}

bool TestUIController::isElementEnabled(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId, [](juce::Component* comp) -> bool { return comp && comp->isEnabled(); });
}

bool TestUIController::isElementFocusable(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId,
                                  [](juce::Component* comp) -> bool { return comp && comp->getWantsKeyboardFocus(); });
}

double TestUIController::getSliderValue(const juce::String& elementId)
{
    return runOnMessageThreadSyncWithResult<double>(
        elementId, std::numeric_limits<double>::quiet_NaN(), [](juce::Component* component) -> double {
            if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
                return oscilSlider->getValue();
            if (auto* slider = dynamic_cast<juce::Slider*>(component))
                return slider->getValue();
            return std::numeric_limits<double>::quiet_NaN();
        });
}

std::pair<double, double> TestUIController::getSliderRange(const juce::String& elementId)
{
    return runOnMessageThreadSyncWithResult<std::pair<double, double>>(
        elementId, {0.0, 1.0}, [](juce::Component* component) -> std::pair<double, double> {
            if (auto* oscilSlider = dynamic_cast<oscil::OscilSlider*>(component))
                return {oscilSlider->getMinimum(), oscilSlider->getMaximum()};
            if (auto* slider = dynamic_cast<juce::Slider*>(component))
                return {slider->getMinimum(), slider->getMaximum()};
            return {0.0, 1.0};
        });
}

bool TestUIController::getToggleState(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId, [](juce::Component* component) -> bool {
        if (auto* oscilToggle = dynamic_cast<oscil::OscilToggle*>(component))
            return oscilToggle->getValue();
        if (auto* button = dynamic_cast<juce::Button*>(component))
            return button->getToggleState();
        return false;
    });
}

juce::String TestUIController::getTextContent(const juce::String& elementId)
{
    return runOnMessageThreadSyncWithResult<juce::String>(
        elementId, juce::String{}, [](juce::Component* component) -> juce::String {
            if (auto* oscilTextField = dynamic_cast<oscil::OscilTextField*>(component))
                return oscilTextField->getText();
            if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
                return textEditor->getText();
            if (auto* label = dynamic_cast<juce::Label*>(component))
                return label->getText();
            if (auto* button = dynamic_cast<juce::Button*>(component))
                return button->getButtonText();
            return {};
        });
}

int TestUIController::getSelectedItemId(const juce::String& elementId)
{
    return runOnMessageThreadSyncWithResult<int>(elementId, 0, [](juce::Component* component) -> int {
        if (auto* oscilDropdown = dynamic_cast<oscil::OscilDropdown*>(component))
            return oscilDropdown->getSelectedIndex();
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
            return comboBox->getSelectedId();
        return 0;
    });
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
