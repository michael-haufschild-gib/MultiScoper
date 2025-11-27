/*
    Oscil Test Harness - UI Controller Implementation
*/

#include "TestUIController.h"
#include <thread>
#include <chrono>

namespace oscil::test
{

bool TestUIController::click(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    // Execute on message thread
    juce::MessageManager::callAsync([component, this]()
    {
        simulateMouseClick(component, false);
    });

    return true;
}

bool TestUIController::doubleClick(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, this]()
    {
        simulateMouseClick(component, true);
    });

    return true;
}

bool TestUIController::select(const juce::String& elementId, int itemId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

bool TestUIController::toggle(const juce::String& elementId, bool value)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

bool TestUIController::drag(const juce::String& fromElementId, const juce::String& toElementId)
{
    auto* fromComponent = TestElementRegistry::getInstance().findElement(fromElementId);
    auto* toComponent = TestElementRegistry::getInstance().findElement(toElementId);

    if (fromComponent == nullptr || toComponent == nullptr)
        return false;

    juce::MessageManager::callAsync([fromComponent, toComponent, this]()
    {
        simulateMouseDrag(fromComponent, toComponent);
    });

    return true;
}

bool TestUIController::typeText(const juce::String& elementId, const juce::String& text)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

json TestUIController::getUIState()
{
    json state;
    state["elements"] = json::object();

    auto elements = TestElementRegistry::getInstance().getAllElements();
    for (const auto& [testId, component] : elements)
    {
        state["elements"][testId.toStdString()] = componentToJson(component, testId);
    }

    return state;
}

json TestUIController::getElementInfo(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
    {
        return json{{"error", "Element not found"}};
    }

    return componentToJson(component, elementId);
}

bool TestUIController::isElementVisible(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    return component->isVisible() && component->isShowing();
}

bool TestUIController::isElementEnabled(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    return component->isEnabled();
}

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

void TestUIController::simulateMouseClick(juce::Component* component, bool doubleClick)
{
    if (component == nullptr)
        return;

    auto bounds = component->getLocalBounds();
    auto center = bounds.getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    // Create mouse event
    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        juce::ModifierKeys(),
        0.0f, // pressure
        0.0f, // orientation
        0.0f, // rotation
        0.0f, // tiltX
        0.0f, // tiltY
        component,
        component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        doubleClick ? 2 : 1, // click count
        false // mouse was dragged
    );

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);

    if (doubleClick)
    {
        component->mouseDoubleClick(mouseDown);
    }
}

void TestUIController::simulateMouseDrag(juce::Component* from, juce::Component* to)
{
    if (from == nullptr || to == nullptr)
        return;

    auto fromCenter = from->getLocalBounds().getCentre();
    auto toCenter = to->getScreenBounds().getCentre() - from->getScreenPosition();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    // Mouse down on source
    juce::MouseEvent mouseDown(
        mouseSource,
        fromCenter.toFloat(),
        juce::ModifierKeys(),
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        from, from,
        juce::Time::getCurrentTime(),
        fromCenter.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    from->mouseDown(mouseDown);

    // Mouse drag
    juce::MouseEvent mouseDrag(
        mouseSource,
        toCenter.toFloat(),
        juce::ModifierKeys(),
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        from, from,
        juce::Time::getCurrentTime(),
        fromCenter.toFloat(),
        juce::Time::getCurrentTime(),
        1, true
    );

    from->mouseDrag(mouseDrag);

    // Mouse up
    from->mouseUp(mouseDrag);
}

json TestUIController::componentToJson(juce::Component* component, const juce::String& testId)
{
    json info;
    info["testId"] = testId.toStdString();
    info["visible"] = component->isVisible();
    info["enabled"] = component->isEnabled();

    auto bounds = component->getBounds();
    info["bounds"] = {
        {"x", bounds.getX()},
        {"y", bounds.getY()},
        {"width", bounds.getWidth()},
        {"height", bounds.getHeight()}
    };

    // Determine type and add type-specific info
    if (auto* button = dynamic_cast<juce::Button*>(component))
    {
        info["type"] = "button";
        info["text"] = button->getButtonText().toStdString();
        info["toggled"] = button->getToggleState();
    }
    else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        info["type"] = "combobox";
        info["selectedId"] = comboBox->getSelectedId();
        info["selectedText"] = comboBox->getText().toStdString();
        info["numItems"] = comboBox->getNumItems();
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        info["type"] = "slider";
        info["value"] = slider->getValue();
        info["minimum"] = slider->getMinimum();
        info["maximum"] = slider->getMaximum();
    }
    else if (auto* label = dynamic_cast<juce::Label*>(component))
    {
        info["type"] = "label";
        info["text"] = label->getText().toStdString();
        info["editable"] = label->isEditable();
    }
    else if (auto* textEditor = dynamic_cast<juce::TextEditor*>(component))
    {
        info["type"] = "textEditor";
        info["text"] = textEditor->getText().toStdString();
    }
    else
    {
        info["type"] = "component";
        info["componentName"] = component->getName().toStdString();
    }

    return info;
}

} // namespace oscil::test
