/*
    Oscil Test Harness - UI Controller Implementation
    Comprehensive UI interaction for automated E2E testing.
*/

#include "TestUIController.h"
#include <thread>
#include <chrono>

namespace oscil::test
{

// ================== ModifierKeyState ==================

juce::ModifierKeys ModifierKeyState::toJuceModifiers() const
{
    int flags = 0;
    if (shift) flags |= juce::ModifierKeys::shiftModifier;
    if (alt)   flags |= juce::ModifierKeys::altModifier;
    if (ctrl)  flags |= juce::ModifierKeys::ctrlModifier;
    if (cmd)   flags |= juce::ModifierKeys::commandModifier;
    return juce::ModifierKeys(flags);
}

// ================== Helper Methods ==================

juce::Component* TestUIController::getTargetComponent(const juce::String& elementId)
{
    if (elementId.isEmpty())
        return getCurrentFocusedComponent();
    return TestElementRegistry::getInstance().findElement(elementId);
}

juce::Component* TestUIController::getCurrentFocusedComponent()
{
    return juce::Component::getCurrentlyFocusedComponent();
}

// ================== Mouse Interactions ==================

bool TestUIController::click(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, this]()
    {
        simulateMouseClick(component, false);
    });

    return true;
}

bool TestUIController::clickWithModifiers(const juce::String& elementId, const ModifierKeyState& modifiers)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync([component, mods, this]()
    {
        simulateMouseClick(component, false, mods);
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

bool TestUIController::rightClick(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, this]()
    {
        simulateMouseRightClick(component);
    });

    return true;
}

bool TestUIController::hover(const juce::String& elementId, int durationMs)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, durationMs, this]()
    {
        simulateMouseHover(component);

        // Wait for tooltip to potentially appear
        juce::Timer::callAfterDelay(durationMs, [component]()
        {
            // Tooltip should have appeared by now if it exists
            // The test can check for tooltip presence separately
        });
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

bool TestUIController::dragWithModifiers(const juce::String& fromElementId,
                                          const juce::String& toElementId,
                                          const ModifierKeyState& modifiers)
{
    auto* fromComponent = TestElementRegistry::getInstance().findElement(fromElementId);
    auto* toComponent = TestElementRegistry::getInstance().findElement(toElementId);

    if (fromComponent == nullptr || toComponent == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync([fromComponent, toComponent, mods, this]()
    {
        simulateMouseDrag(fromComponent, toComponent, mods);
    });

    return true;
}

bool TestUIController::dragByOffset(const juce::String& elementId, int deltaX, int deltaY)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, deltaX, deltaY, this]()
    {
        simulateMouseDragOffset(component, deltaX, deltaY);
    });

    return true;
}

bool TestUIController::dragByOffsetWithModifiers(const juce::String& elementId,
                                                  int deltaX, int deltaY,
                                                  const ModifierKeyState& modifiers)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync([component, deltaX, deltaY, mods, this]()
    {
        simulateMouseDragOffset(component, deltaX, deltaY, mods);
    });

    return true;
}

ScrollResult TestUIController::scroll(const juce::String& elementId, float deltaY, float deltaX)
{
    ScrollResult result;
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return result;

    // Get previous value if it's a slider
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        result.previousValue = slider->getValue();

    result.success = true;

    juce::MessageManager::callAsync([component, deltaX, deltaY, this]()
    {
        simulateMouseWheel(component, deltaX, deltaY);
    });

    // Give time for the event to process
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Get new value if it's a slider
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        result.newValue = slider->getValue();

    return result;
}

ScrollResult TestUIController::scrollWithModifiers(const juce::String& elementId,
                                                    float deltaY, float deltaX,
                                                    const ModifierKeyState& modifiers)
{
    ScrollResult result;
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return result;

    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        result.previousValue = slider->getValue();

    result.success = true;
    auto mods = modifiers.toJuceModifiers();

    juce::MessageManager::callAsync([component, deltaX, deltaY, mods, this]()
    {
        simulateMouseWheel(component, deltaX, deltaY, mods);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        result.newValue = slider->getValue();

    return result;
}

// ================== Keyboard Interactions ==================

bool TestUIController::pressKey(int keyCode, const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::KeyPress key(keyCode);
    juce::MessageManager::callAsync([component, key, this]()
    {
        simulateKeyPress(component, key);
    });

    return true;
}

bool TestUIController::pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers,
                                              const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::KeyPress key(keyCode, modifiers.toJuceModifiers(), 0);
    juce::MessageManager::callAsync([component, key, this]()
    {
        simulateKeyPress(component, key);
    });

    return true;
}

bool TestUIController::pressEscape(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::escapeKey, elementId);
}

bool TestUIController::pressEnter(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::returnKey, elementId);
}

bool TestUIController::pressSpace(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::spaceKey, elementId);
}

bool TestUIController::pressTab(bool reverse, const juce::String& elementId)
{
    if (reverse)
    {
        ModifierKeyState mods;
        mods.shift = true;
        return pressKeyWithModifiers(juce::KeyPress::tabKey, mods, elementId);
    }
    return pressKey(juce::KeyPress::tabKey, elementId);
}

bool TestUIController::pressArrow(int direction, const juce::String& elementId)
{
    int keyCode;
    switch (direction)
    {
        case 0: keyCode = juce::KeyPress::upKey; break;
        case 1: keyCode = juce::KeyPress::downKey; break;
        case 2: keyCode = juce::KeyPress::leftKey; break;
        case 3: keyCode = juce::KeyPress::rightKey; break;
        default: return false;
    }
    return pressKey(keyCode, elementId);
}

bool TestUIController::pressHome(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::homeKey, elementId);
}

bool TestUIController::pressEnd(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::endKey, elementId);
}

bool TestUIController::pressDelete(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::deleteKey, elementId);
}

bool TestUIController::typeCharacters(const juce::String& text, const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, text, this]()
    {
        for (int i = 0; i < text.length(); ++i)
        {
            juce::juce_wchar ch = text[i];
            juce::KeyPress key(ch);
            simulateKeyPress(component, key);
        }
    });

    return true;
}

// ================== Form Interactions ==================

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

bool TestUIController::incrementSlider(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

bool TestUIController::clearText(const juce::String& elementId)
{
    return typeText(elementId, juce::String());
}

// ================== Focus Management ==================

bool TestUIController::setFocus(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

    // Search for the test ID of the focused component
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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

    // Add focused element
    auto focusedId = getFocusedElementId();
    state["focusedElement"] = focusedId.toStdString();

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

bool TestUIController::isElementFocusable(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (component == nullptr)
        return false;

    return component->getWantsKeyboardFocus();
}

double TestUIController::getSliderValue(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        return slider->getValue();
    return 0.0;
}

std::pair<double, double> TestUIController::getSliderRange(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (auto* slider = dynamic_cast<juce::Slider*>(component))
        return { slider->getMinimum(), slider->getMaximum() };
    return { 0.0, 1.0 };
}

bool TestUIController::getToggleState(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
    if (auto* button = dynamic_cast<juce::Button*>(component))
        return button->getToggleState();
    return false;
}

juce::String TestUIController::getTextContent(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findElement(elementId);

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
    auto* component = TestElementRegistry::getInstance().findElement(elementId);
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

// ================== Private Simulation Methods ==================

void TestUIController::simulateMouseClick(juce::Component* component, bool doubleClick,
                                           const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    auto bounds = component->getLocalBounds();
    auto center = bounds.getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        doubleClick ? 2 : 1,
        false
    );

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);

    if (doubleClick)
    {
        component->mouseDoubleClick(mouseDown);
    }
}

void TestUIController::simulateMouseRightClick(juce::Component* component)
{
    if (component == nullptr)
        return;

    auto bounds = component->getLocalBounds();
    auto center = bounds.getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    // Right-click uses popup menu modifier
    juce::ModifierKeys mods(juce::ModifierKeys::popupMenuClickModifier);

    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    component->mouseDown(mouseDown);
    component->mouseUp(mouseDown);
}

void TestUIController::simulateMouseDrag(juce::Component* from, juce::Component* to,
                                          const juce::ModifierKeys& mods)
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
        mods,
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
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        from, from,
        juce::Time::getCurrentTime(),
        fromCenter.toFloat(),
        juce::Time::getCurrentTime(),
        1, true
    );

    from->mouseDrag(mouseDrag);
    from->mouseUp(mouseDrag);
}

void TestUIController::simulateMouseDragOffset(juce::Component* component, int deltaX, int deltaY,
                                                const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto endPoint = center + juce::Point<int>(deltaX, deltaY);
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    // Mouse down
    juce::MouseEvent mouseDown(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    component->mouseDown(mouseDown);

    // Mouse drag
    juce::MouseEvent mouseDrag(
        mouseSource,
        endPoint.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, true
    );

    component->mouseDrag(mouseDrag);
    component->mouseUp(mouseDrag);
}

void TestUIController::simulateMouseWheel(juce::Component* component, float deltaX, float deltaY,
                                           const juce::ModifierKeys& mods)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseEvent(
        mouseSource,
        center.toFloat(),
        mods,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        1, false
    );

    juce::MouseWheelDetails wheel;
    wheel.deltaX = deltaX;
    wheel.deltaY = deltaY;
    wheel.isReversed = false;
    wheel.isSmooth = true;
    wheel.isInertial = false;

    component->mouseWheelMove(mouseEvent, wheel);
}

void TestUIController::simulateMouseHover(juce::Component* component)
{
    if (component == nullptr)
        return;

    auto center = component->getLocalBounds().getCentre();
    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();

    juce::MouseEvent mouseEvent(
        mouseSource,
        center.toFloat(),
        juce::ModifierKeys(),
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        component, component,
        juce::Time::getCurrentTime(),
        center.toFloat(),
        juce::Time::getCurrentTime(),
        0, false
    );

    component->mouseEnter(mouseEvent);
    component->mouseMove(mouseEvent);
}

void TestUIController::simulateKeyPress(juce::Component* component, const juce::KeyPress& key)
{
    if (component == nullptr)
        return;

    component->keyPressed(key);
}

json TestUIController::componentToJson(juce::Component* component, const juce::String& testId)
{
    json info;
    info["testId"] = testId.toStdString();
    info["visible"] = component->isVisible();
    info["showing"] = component->isShowing();
    info["enabled"] = component->isEnabled();
    info["hasFocus"] = component->hasKeyboardFocus(false);
    info["focusable"] = component->getWantsKeyboardFocus();

    auto bounds = component->getBounds();
    info["bounds"] = {
        {"x", bounds.getX()},
        {"y", bounds.getY()},
        {"width", bounds.getWidth()},
        {"height", bounds.getHeight()}
    };

    auto screenBounds = component->getScreenBounds();
    info["screenBounds"] = {
        {"x", screenBounds.getX()},
        {"y", screenBounds.getY()},
        {"width", screenBounds.getWidth()},
        {"height", screenBounds.getHeight()}
    };

    // Determine type and add type-specific info
    if (auto* button = dynamic_cast<juce::Button*>(component))
    {
        info["type"] = "button";
        info["text"] = button->getButtonText().toStdString();
        info["toggled"] = button->getToggleState();
        info["toggleable"] = button->getClickingTogglesState();
    }
    else if (auto* comboBox = dynamic_cast<juce::ComboBox*>(component))
    {
        info["type"] = "combobox";
        info["selectedId"] = comboBox->getSelectedId();
        info["selectedText"] = comboBox->getText().toStdString();
        info["numItems"] = comboBox->getNumItems();

        json items = json::array();
        for (int i = 0; i < comboBox->getNumItems(); ++i)
        {
            items.push_back({
                {"id", comboBox->getItemId(i)},
                {"text", comboBox->getItemText(i).toStdString()}
            });
        }
        info["items"] = items;
    }
    else if (auto* slider = dynamic_cast<juce::Slider*>(component))
    {
        info["type"] = "slider";
        info["value"] = slider->getValue();
        info["minimum"] = slider->getMinimum();
        info["maximum"] = slider->getMaximum();
        info["interval"] = slider->getInterval();
        info["skewFactor"] = slider->getSkewFactor();
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
        info["readOnly"] = textEditor->isReadOnly();
        info["multiLine"] = textEditor->isMultiLine();
    }
    else
    {
        info["type"] = "component";
        info["componentName"] = component->getName().toStdString();
        info["componentId"] = component->getComponentID().toStdString();
    }

    return info;
}

} // namespace oscil::test
