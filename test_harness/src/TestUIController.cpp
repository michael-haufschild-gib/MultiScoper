/*
    Oscil Test Harness - UI Controller: Mouse & Keyboard Interactions
*/

#include "TestUIController.h"

#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"

#include "TestDAW.h"

#include <chrono>
#include <thread>

namespace oscil::test
{

// ================== Track Scope ==================

void TestUIController::setTrackScope(int trackIndex, TestDAW* daw)
{
    trackScopeIndex_ = trackIndex;
    trackScopeDaw_ = daw;
}

void TestUIController::clearTrackScope()
{
    trackScopeIndex_ = -1;
    trackScopeDaw_ = nullptr;
}

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

    auto& registry = TestElementRegistry::getInstance();
    if (trackScopeIndex_ >= 0 && trackScopeDaw_ != nullptr)
        return registry.findValidElementForTrack(elementId, trackScopeIndex_, *trackScopeDaw_);
    return registry.findValidElement(elementId);
}

juce::Component* TestUIController::getCurrentFocusedComponent()
{
    return juce::Component::getCurrentlyFocusedComponent();
}

// ================== Mouse Interactions ==================

bool TestUIController::click(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseClick(comp);
            result = true;
        }
        // Post a second callback to drain any pending callAsync items
        // (e.g., pendingRefresh_ from refreshPanels reentrancy guard)
        juce::MessageManager::callAsync([&done]() { done.signal(); });
    });
    done.wait(3000);

    return result;
}

bool TestUIController::clickWithModifiers(const juce::String& elementId, const ModifierKeyState& modifiers)
{
    auto mods = modifiers.toJuceModifiers();
    return runOnMessageThreadSync(elementId, [this, mods](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateMouseClick(comp, false, mods);
        return true;
    });
}

bool TestUIController::doubleClick(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId, [this](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateMouseClick(comp, true);
        return true;
    });
}

bool TestUIController::rightClick(const juce::String& elementId)
{
    return runOnMessageThreadSync(elementId, [this](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateMouseRightClick(comp);
        return true;
    });
}

bool TestUIController::hover(const juce::String& elementId, int durationMs)
{
    return runOnMessageThreadSync(elementId, [this, durationMs](juce::Component* component) -> bool {
        if (!component)
            return false;
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
        return true;
    });
}

bool TestUIController::drag(const juce::String& fromElementId, const juce::String& toElementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, fromElementId, toElementId, &result, &done]() {
        auto* fromComp = getTargetComponent(fromElementId);
        auto* toComp = getTargetComponent(toElementId);
        if (fromComp != nullptr && toComp != nullptr)
        {
            simulateMouseDrag(fromComp, toComp);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::dragWithModifiers(const juce::String& fromElementId, const juce::String& toElementId,
                                         const ModifierKeyState& modifiers)
{
    bool result = false;
    auto mods = modifiers.toJuceModifiers();
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, fromElementId, toElementId, mods, &result, &done]() {
        auto* fromComp = getTargetComponent(fromElementId);
        auto* toComp = getTargetComponent(toElementId);
        if (fromComp != nullptr && toComp != nullptr)
        {
            simulateMouseDrag(fromComp, toComp, mods);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::dragByOffset(const juce::String& elementId, int deltaX, int deltaY)
{
    return runOnMessageThreadSync(elementId, [this, deltaX, deltaY](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateMouseDragOffset(comp, deltaX, deltaY);
        return true;
    });
}

bool TestUIController::dragByOffsetWithModifiers(const juce::String& elementId, int deltaX, int deltaY,
                                                 const ModifierKeyState& modifiers)
{
    auto mods = modifiers.toJuceModifiers();
    return runOnMessageThreadSync(elementId, [this, deltaX, deltaY, mods](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateMouseDragOffset(comp, deltaX, deltaY, mods);
        return true;
    });
}

ScrollResult TestUIController::scroll(const juce::String& elementId, float deltaY, float deltaX)
{
    return runOnMessageThreadSyncWithResult<ScrollResult>(
        elementId, ScrollResult{}, [this, deltaX, deltaY](juce::Component* comp) -> ScrollResult {
            if (!comp)
                return {};
            simulateMouseWheel(comp, deltaX, deltaY);
            return {true, 0.0, 0.0};
        });
}

ScrollResult TestUIController::scrollWithModifiers(const juce::String& elementId, float deltaY, float deltaX,
                                                   const ModifierKeyState& modifiers)
{
    auto mods = modifiers.toJuceModifiers();
    return runOnMessageThreadSyncWithResult<ScrollResult>(
        elementId, ScrollResult{}, [this, deltaX, deltaY, mods](juce::Component* comp) -> ScrollResult {
            if (!comp)
                return {};
            simulateMouseWheel(comp, deltaX, deltaY, mods);
            return {true, 0.0, 0.0};
        });
}

// ================== Keyboard Interactions ==================

bool TestUIController::pressKey(int keyCode, const juce::String& elementId)
{
    juce::KeyPress key(keyCode);
    return runOnMessageThreadSync(elementId, [this, key](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateKeyPress(comp, key);
        return true;
    });
}

bool TestUIController::pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers,
                                             const juce::String& elementId)
{
    juce::KeyPress key(keyCode, modifiers.toJuceModifiers(), 0);
    return runOnMessageThreadSync(elementId, [this, key](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        simulateKeyPress(comp, key);
        return true;
    });
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
    return runOnMessageThreadSync(elementId, [this, text](juce::Component* comp) -> bool {
        if (!comp)
            return false;
        for (int i = 0; i < text.length(); ++i)
        {
            juce::juce_wchar ch = text[i];
            juce::KeyPress key(ch);
            simulateKeyPress(comp, key);
        }
        return true;
    });
}

} // namespace oscil::test
