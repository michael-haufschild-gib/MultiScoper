/*
    Oscil Test Harness - UI Controller: Mouse & Keyboard Interactions
*/

#include "TestUIController.h"

#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"

#include <chrono>
#include <thread>

namespace oscil::test
{

// ================== ModifierKeyState ==================

juce::ModifierKeys ModifierKeyState::toJuceModifiers() const
{
    int flags = 0;
    if (shift)
        flags |= juce::ModifierKeys::shiftModifier;
    if (alt)
        flags |= juce::ModifierKeys::altModifier;
    if (ctrl)
        flags |= juce::ModifierKeys::ctrlModifier;
    if (cmd)
        flags |= juce::ModifierKeys::commandModifier;
    return juce::ModifierKeys(flags);
}

// ================== Helper Methods ==================

juce::Component* TestUIController::getTargetComponent(const juce::String& elementId)
{
    if (elementId.isEmpty())
    {
        auto* focused = getCurrentFocusedComponent();
        if (focused)
            return focused;
        // No focused component — find the topmost visible modal overlay
        // so global keys like Escape can reach it.
        auto& registry = TestElementRegistry::getInstance();
        for (const auto& id : registry.getAllTestIds())
        {
            if (id.contains("Modal") || id.contains("Dialog") || id.contains("modal") || id.contains("dialog"))
            {
                auto* comp = registry.findValidElement(id);
                if (comp && comp->isVisible())
                    return comp;
            }
        }
        return nullptr;
    }
    return TestElementRegistry::getInstance().findValidElement(elementId);
}

juce::Component* TestUIController::getCurrentFocusedComponent()
{
    return juce::Component::getCurrentlyFocusedComponent();
}

// ================== Mouse Interactions ==================

bool TestUIController::click(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([safeComp, this, &done]() {
        if (auto* comp = safeComp.getComponent())
            simulateMouseClick(comp);
        // Post a second callback to drain any pending callAsync items
        // (e.g., pendingRefresh_ from refreshPanels reentrancy guard)
        juce::MessageManager::callAsync([&done]() { done.signal(); });
    });
    done.wait(3000);

    return true;
}

bool TestUIController::clickWithModifiers(const juce::String& elementId, const ModifierKeyState& modifiers)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::MessageManager::callAsync([safeComp, mods, this]() {
        if (auto* comp = safeComp.getComponent())
            simulateMouseClick(comp, false, mods);
    });

    return true;
}

bool TestUIController::doubleClick(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::MessageManager::callAsync([safeComp, this]() {
        if (auto* comp = safeComp.getComponent())
            simulateMouseClick(comp, true);
    });

    return true;
}

bool TestUIController::rightClick(const juce::String& elementId)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::MessageManager::callAsync([safeComp, this]() {
        if (auto* comp = safeComp.getComponent())
            simulateMouseRightClick(comp);
    });

    return true;
}

bool TestUIController::hover(const juce::String& elementId, int durationMs)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::Component::SafePointer<juce::Component> safeComp(component);
    juce::MessageManager::callAsync([safeComp, durationMs, this]() {
        auto* component = safeComp.getComponent();
        if (component == nullptr)
            return;
        simulateMouseHover(component);

        // Schedule hover end
        juce::Component::SafePointer<juce::Component> hoverSafe(component);
        juce::Timer::callAfterDelay(durationMs, [hoverSafe]() {
            auto* comp = hoverSafe.getComponent();
            if (comp == nullptr)
                return;

            auto bounds = comp->getLocalBounds();
            auto center = bounds.getCentre();
            auto& desktop = juce::Desktop::getInstance();
            auto mouseSource = desktop.getMainMouseSource();

            juce::MouseEvent mouseEvent(mouseSource, center.toFloat(), juce::ModifierKeys(), 0.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, comp, comp, juce::Time::getCurrentTime(), center.toFloat(),
                                        juce::Time::getCurrentTime(), 0, false);

            comp->mouseExit(mouseEvent);
        });
    });

    return true;
}

bool TestUIController::drag(const juce::String& fromElementId, const juce::String& toElementId)
{
    auto* fromComp = TestElementRegistry::getInstance().findValidElement(fromElementId);
    auto* toComp = TestElementRegistry::getInstance().findValidElement(toElementId);
    if (fromComp == nullptr || toComp == nullptr)
        return false;

    juce::WaitableEvent done;
    juce::MessageManager::callAsync([fromComp, toComp, this, &done]() {
        simulateMouseDrag(fromComp, toComp);
        done.signal();
    });
    done.wait(3000);

    return true;
}

bool TestUIController::dragWithModifiers(const juce::String& fromElementId, const juce::String& toElementId,
                                         const ModifierKeyState& modifiers)
{
    auto* fromComp = TestElementRegistry::getInstance().findValidElement(fromElementId);
    auto* toComp = TestElementRegistry::getInstance().findValidElement(toElementId);
    if (fromComp == nullptr || toComp == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync([fromComp, toComp, mods, this]() { simulateMouseDrag(fromComp, toComp, mods); });

    return true;
}

bool TestUIController::dragByOffset(const juce::String& elementId, int deltaX, int deltaY)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    juce::WaitableEvent done;
    juce::MessageManager::callAsync([component, deltaX, deltaY, this, &done]() {
        simulateMouseDragOffset(component, deltaX, deltaY);
        done.signal();
    });
    done.wait(3000);

    return true;
}

bool TestUIController::dragByOffsetWithModifiers(const juce::String& elementId, int deltaX, int deltaY,
                                                 const ModifierKeyState& modifiers)
{
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return false;

    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync(
        [component, deltaX, deltaY, mods, this]() { simulateMouseDragOffset(component, deltaX, deltaY, mods); });

    return true;
}

ScrollResult TestUIController::scroll(const juce::String& elementId, float deltaY, float deltaX)
{
    ScrollResult result;
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return result;

    result.success = true;
    juce::MessageManager::callAsync(
        [component, deltaX, deltaY, this]() { simulateMouseWheel(component, deltaX, deltaY); });

    return result;
}

ScrollResult TestUIController::scrollWithModifiers(const juce::String& elementId, float deltaY, float deltaX,
                                                   const ModifierKeyState& modifiers)
{
    ScrollResult result;
    auto* component = TestElementRegistry::getInstance().findValidElement(elementId);
    if (component == nullptr)
        return result;

    result.success = true;
    auto mods = modifiers.toJuceModifiers();
    juce::MessageManager::callAsync(
        [component, deltaX, deltaY, mods, this]() { simulateMouseWheel(component, deltaX, deltaY, mods); });

    return result;
}

// ================== Keyboard Interactions ==================

bool TestUIController::pressKey(int keyCode, const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::KeyPress key(keyCode);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([component, key, this, &done]() {
        simulateKeyPress(component, key);
        done.signal();
    });
    done.wait(3000);

    return true;
}

bool TestUIController::pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers,
                                             const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::KeyPress key(keyCode, modifiers.toJuceModifiers(), 0);
    juce::MessageManager::callAsync([component, key, this]() { simulateKeyPress(component, key); });

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
        case 0:
            keyCode = juce::KeyPress::upKey;
            break;
        case 1:
            keyCode = juce::KeyPress::downKey;
            break;
        case 2:
            keyCode = juce::KeyPress::leftKey;
            break;
        case 3:
            keyCode = juce::KeyPress::rightKey;
            break;
        default:
            return false;
    }
    return pressKey(keyCode, elementId);
}

bool TestUIController::pressHome(const juce::String& elementId) { return pressKey(juce::KeyPress::homeKey, elementId); }

bool TestUIController::pressEnd(const juce::String& elementId) { return pressKey(juce::KeyPress::endKey, elementId); }

bool TestUIController::pressDelete(const juce::String& elementId)
{
    return pressKey(juce::KeyPress::deleteKey, elementId);
}

bool TestUIController::typeCharacters(const juce::String& text, const juce::String& elementId)
{
    auto* component = getTargetComponent(elementId);
    if (component == nullptr)
        return false;

    juce::MessageManager::callAsync([component, text, this]() {
        for (int i = 0; i < text.length(); ++i)
        {
            juce::juce_wchar ch = text[i];
            juce::KeyPress key(ch);
            simulateKeyPress(component, key);
        }
    });

    return true;
}

} // namespace oscil::test
