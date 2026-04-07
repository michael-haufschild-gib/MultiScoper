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
    bool result = false;
    auto mods = modifiers.toJuceModifiers();
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, mods, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseClick(comp, false, mods);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::doubleClick(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseClick(comp, true);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::rightClick(const juce::String& elementId)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseRightClick(comp);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::hover(const juce::String& elementId, int durationMs)
{
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, durationMs, &result, &done]() {
        auto* component = getTargetComponent(elementId);
        if (component == nullptr)
        {
            done.signal();
            return;
        }
        simulateMouseHover(component);
        result = true;

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
        done.signal();
    });
    done.wait(3000);

    return result;
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
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, deltaX, deltaY, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseDragOffset(comp, deltaX, deltaY);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::dragByOffsetWithModifiers(const juce::String& elementId, int deltaX, int deltaY,
                                                 const ModifierKeyState& modifiers)
{
    bool result = false;
    auto mods = modifiers.toJuceModifiers();
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, deltaX, deltaY, mods, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseDragOffset(comp, deltaX, deltaY, mods);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

ScrollResult TestUIController::scroll(const juce::String& elementId, float deltaY, float deltaX)
{
    ScrollResult result;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, deltaX, deltaY, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseWheel(comp, deltaX, deltaY);
            result.success = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

ScrollResult TestUIController::scrollWithModifiers(const juce::String& elementId, float deltaY, float deltaX,
                                                   const ModifierKeyState& modifiers)
{
    ScrollResult result;
    auto mods = modifiers.toJuceModifiers();
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, deltaX, deltaY, mods, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateMouseWheel(comp, deltaX, deltaY, mods);
            result.success = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

// ================== Keyboard Interactions ==================

bool TestUIController::pressKey(int keyCode, const juce::String& elementId)
{
    bool result = false;
    juce::KeyPress key(keyCode);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, key, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateKeyPress(comp, key);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

bool TestUIController::pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers,
                                             const juce::String& elementId)
{
    bool result = false;
    juce::KeyPress key(keyCode, modifiers.toJuceModifiers(), 0);
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, key, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            simulateKeyPress(comp, key);
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
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
    bool result = false;
    juce::WaitableEvent done;
    juce::MessageManager::callAsync([this, elementId, text, &result, &done]() {
        if (auto* comp = getTargetComponent(elementId))
        {
            for (int i = 0; i < text.length(); ++i)
            {
                juce::juce_wchar ch = text[i];
                juce::KeyPress key(ch);
                simulateKeyPress(comp, key);
            }
            result = true;
        }
        done.signal();
    });
    done.wait(3000);

    return result;
}

} // namespace oscil::test
