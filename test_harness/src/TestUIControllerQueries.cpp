/*
    Oscil Test Harness - UI Controller: Focus Management, State Queries & Waits
*/

#include "TestUIController.h"
#include <thread>
#include <chrono>
#include <cmath>

namespace oscil::test
{

// ================== Focus Management ==================

bool TestUIController::setFocus(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([safeComp, &done]()
    {
        if (auto* comp = safeComp.getComponent())
            comp->grabKeyboardFocus();
        done.signal();
    });
    done.wait(3000);

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

    juce::Component::SafePointer<juce::Component> safeFocused(focused);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([safeFocused, &done]()
    {
        if (auto* comp = safeFocused.getComponent())
            comp->moveKeyboardFocusToSibling(true);
        done.signal();
    });
    done.wait(3000);

    return true;
}

bool TestUIController::focusPrevious()
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeFocused(focused);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([safeFocused, &done]()
    {
        if (auto* comp = safeFocused.getComponent())
            comp->moveKeyboardFocusToSibling(false);
        done.signal();
    });
    done.wait(3000);

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
    // Always log to verify this code path executes
    fprintf(stderr, "[getElementInfo] queried: %s\n", elementId.toRawUTF8());
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
    {
        fprintf(stderr, "[getElementInfo] NOT FOUND: %s\n", elementId.toRawUTF8());
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
