/*
    Oscil Test Harness - UI Controller
    Provides programmatic UI interaction for automated E2E testing.

    Supports:
    - Mouse interactions (click, double-click, drag, scroll)
    - Keyboard interactions (key press, shortcuts, text input)
    - Focus management
    - Modifier keys (Shift, Alt/Option, Ctrl/Cmd)
    - Context menus
    - Hover/tooltip triggering
*/

#pragma once

#include "TestElementRegistry.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>

namespace oscil::test
{

using json = nlohmann::json;

struct ModifierKeyState
{
    bool shift = false;
    bool alt = false;  // Option on macOS
    bool ctrl = false; // Command on macOS for most shortcuts
    bool cmd = false;  // For explicit Command key on macOS

    juce::ModifierKeys toJuceModifiers() const;
};

struct ScrollResult
{
    bool success = false;
    double previousValue = 0.0;
    double newValue = 0.0;
};

// Forward declaration for track-scoped element lookup
class TestDAW;

class TestUIController
{
public:
    TestUIController() = default;
    ~TestUIController()
    {
        // Mark the control block as dead. Lambdas holding a weak_ptr to it will
        // see the flag (or fail to lock entirely once the shared_ptr is gone).
        self_->store(false, std::memory_order_release);
        self_->controller = nullptr;
        // Flush pending message-thread callbacks.  Even though lambdas now use
        // weak_ptr and cannot dereference a dead controller, flushing ensures
        // orderly completion of any in-flight work.
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating();
            mm != nullptr && !mm->isThisTheMessageThread())
        {
            auto flushed = std::make_shared<juce::WaitableEvent>();
            if (mm->callAsync([flushed]() { flushed->signal(); }))
                flushed->wait(MESSAGE_THREAD_TIMEOUT_MS);
        }
    }

    void setTrackScope(int trackIndex, TestDAW* daw);

    void clearTrackScope();

    // ================== Mouse Interactions ==================

    bool click(const juce::String& elementId);

    bool clickWithModifiers(const juce::String& elementId, const ModifierKeyState& modifiers);

    bool doubleClick(const juce::String& elementId);

    bool rightClick(const juce::String& elementId);

    /**
     * Hover over an element (for tooltips)
     * @param durationMs How long to hover (tooltips typically need 500-1000ms)
     */
    bool hover(const juce::String& elementId, int durationMs = 500);

    bool drag(const juce::String& fromElementId, const juce::String& toElementId);

    bool dragWithModifiers(const juce::String& fromElementId, const juce::String& toElementId,
                           const ModifierKeyState& modifiers);

    bool dragByOffset(const juce::String& elementId, int deltaX, int deltaY);

    bool dragByOffsetWithModifiers(const juce::String& elementId, int deltaX, int deltaY,
                                   const ModifierKeyState& modifiers);

    /**
     * Scroll wheel on an element
     * @param deltaY Positive = scroll up, Negative = scroll down
     * @param deltaX Horizontal scroll (optional)
     */
    ScrollResult scroll(const juce::String& elementId, float deltaY, float deltaX = 0.0f);

    ScrollResult scrollWithModifiers(const juce::String& elementId, float deltaY, float deltaX,
                                     const ModifierKeyState& modifiers);

    // ================== Keyboard Interactions ==================

    /**
     * Press a single key on focused element or specified element
     * @param keyCode JUCE key code (e.g., juce::KeyPress::escapeKey)
     * @param elementId Optional element to focus first
     */
    bool pressKey(int keyCode, const juce::String& elementId = {});

    bool pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers, const juce::String& elementId = {});

    bool pressEscape(const juce::String& elementId = {});

    bool pressEnter(const juce::String& elementId = {});

    bool pressSpace(const juce::String& elementId = {});

    bool pressTab(bool reverse = false, const juce::String& elementId = {});

    /**
     * Press Arrow key
     * @param direction 0=Up, 1=Down, 2=Left, 3=Right
     */
    bool pressArrow(int direction, const juce::String& elementId = {});

    bool pressHome(const juce::String& elementId = {});

    bool pressEnd(const juce::String& elementId = {});

    bool pressDelete(const juce::String& elementId = {});

    bool typeCharacters(const juce::String& text, const juce::String& elementId = {});

    // ================== Form Interactions ==================

    bool select(const juce::String& elementId, int itemId);

    bool selectByText(const juce::String& elementId, const juce::String& text);

    bool selectById(const juce::String& elementId, const juce::String& itemId);

    bool toggle(const juce::String& elementId, bool value);

    bool setSliderValue(const juce::String& elementId, double value);

    bool incrementSlider(const juce::String& elementId);

    bool decrementSlider(const juce::String& elementId);

    bool resetSliderToDefault(const juce::String& elementId);

    bool typeText(const juce::String& elementId, const juce::String& text);

    bool clearText(const juce::String& elementId);

    // ================== Focus Management ==================

    bool setFocus(const juce::String& elementId);

    juce::String getFocusedElementId();

    bool hasFocus(const juce::String& elementId);

    bool focusNext();

    bool focusPrevious();

    // ================== State Queries ==================

    json getUIState();

    json getElementInfo(const juce::String& elementId);

    bool isElementVisible(const juce::String& elementId);

    bool isElementEnabled(const juce::String& elementId);

    bool isElementFocusable(const juce::String& elementId);

    double getSliderValue(const juce::String& elementId);

    std::pair<double, double> getSliderRange(const juce::String& elementId);

    bool getToggleState(const juce::String& elementId);

    juce::String getTextContent(const juce::String& elementId);

    int getSelectedItemId(const juce::String& elementId);

    // ================== Waits ==================

    bool waitForElement(const juce::String& elementId, int timeoutMs = 5000);

    bool waitForVisible(const juce::String& elementId, int timeoutMs = 5000);

    bool waitForEnabled(const juce::String& elementId, int timeoutMs = 5000);

    bool waitForSliderValue(const juce::String& elementId, double value, double tolerance = 0.01, int timeoutMs = 5000);

protected:
    static constexpr int MESSAGE_THREAD_TIMEOUT_MS = 3000;

    // Control block shared between the controller and queued callAsync lambdas.
    // Lambdas capture a weak_ptr<ControlBlock> so they can safely detect
    // destruction even if the destructor's flush sentinel times out.
    struct ControlBlock : std::atomic<bool>
    {
        explicit ControlBlock(TestUIController* ctrl) : std::atomic<bool>(true), controller(ctrl) {}
        TestUIController* controller;
    };

    // Returns true if the ControlBlock is alive and controller is non-null.
    static bool isControllerAlive(const std::shared_ptr<ControlBlock>& locked)
    {
        return locked && locked->load(std::memory_order_acquire) && locked->controller != nullptr;
    }

    template <typename Func>
    bool runOnMessageThreadSync(const juce::String& elementId, Func&& func)
    {
        // If called from the message thread, run inline — posting callAsync
        // and waiting would deadlock (we'd be waiting for a lambda queued
        // behind us on our own thread).
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating();
            mm != nullptr && mm->isThisTheMessageThread())
        {
            return func(getTargetComponent(elementId));
        }

        struct State
        {
            bool result = false;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>();
        std::weak_ptr<ControlBlock> weak = self_;
        bool posted = juce::MessageManager::callAsync([weak, elementId, state, f = std::forward<Func>(func)]() mutable {
            auto locked = weak.lock();
            if (!isControllerAlive(locked))
            {
                state->done.signal();
                return;
            }
            state->result = f(locked->controller->getTargetComponent(elementId));
            state->done.signal();
        });
        if (!posted)
            return false;
        if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
            return false;
        return state->result;
    }

    template <typename Func>
    bool runOnMessageThreadSync(Func&& func)
    {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating();
            mm != nullptr && mm->isThisTheMessageThread())
        {
            return func();
        }

        struct State
        {
            bool result = false;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>();
        std::weak_ptr<ControlBlock> weak = self_;
        bool posted = juce::MessageManager::callAsync([weak, state, f = std::forward<Func>(func)]() mutable {
            auto locked = weak.lock();
            if (!isControllerAlive(locked))
            {
                state->done.signal();
                return;
            }
            state->result = f();
            state->done.signal();
        });
        if (!posted)
            return false;
        if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
            return false;
        return state->result;
    }

    template <typename T, typename Func>
    T runOnMessageThreadSyncWithResult(const juce::String& elementId, T defaultValue, Func&& func)
    {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating();
            mm != nullptr && mm->isThisTheMessageThread())
        {
            return func(getTargetComponent(elementId));
        }

        struct State
        {
            T result;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>();
        state->result = std::move(defaultValue);
        std::weak_ptr<ControlBlock> weak = self_;
        bool posted = juce::MessageManager::callAsync([weak, elementId, state, f = std::forward<Func>(func)]() mutable {
            auto locked = weak.lock();
            if (!isControllerAlive(locked))
            {
                state->done.signal();
                return;
            }
            state->result = f(locked->controller->getTargetComponent(elementId));
            state->done.signal();
        });
        if (!posted)
            return std::move(state->result);
        state->done.wait(MESSAGE_THREAD_TIMEOUT_MS);
        return std::move(state->result);
    }

private:
    void simulateMouseClick(juce::Component* component, bool doubleClick = false, const juce::ModifierKeys& mods = {});
    void simulateMouseRightClick(juce::Component* component);
    void simulateMouseDrag(juce::Component* from, juce::Component* to, const juce::ModifierKeys& mods = {});
    void simulateMouseDragOffset(juce::Component* component, int deltaX, int deltaY,
                                 const juce::ModifierKeys& mods = {});
    void simulateMouseWheel(juce::Component* component, float deltaX, float deltaY,
                            const juce::ModifierKeys& mods = {});
    void simulateMouseHover(juce::Component* component);
    void simulateKeyPress(juce::Component* component, const juce::KeyPress& key);

    bool tryClickFastPath(juce::Component* component);
    bool tryDragFastPathListItems(juce::Component* from, juce::Component* to);
    bool tryDragOffsetFastPathSidebar(juce::Component* component, int deltaX);

    json componentToJson(juce::Component* component, const juce::String& testId);
    void appendComponentTypeInfo(json& info, juce::Component* component);
    bool appendOscilTypeInfo(json& info, juce::Component* component);
    juce::Component* getTargetComponent(const juce::String& elementId);
    juce::Component* getCurrentFocusedComponent();
    juce::String getFocusedElementIdOnMessageThread();

    bool adjustSlider(const juce::String& elementId, int direction);

    std::shared_ptr<ControlBlock> self_ = std::make_shared<ControlBlock>(this);

    // Track scope for multi-instance element resolution
    int trackScopeIndex_ = -1; // -1 = no scope (global)
    TestDAW* trackScopeDaw_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestUIController)
};

} // namespace oscil::test
