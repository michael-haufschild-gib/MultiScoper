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

/**
 * Modifier keys for mouse and keyboard events
 */
struct ModifierKeyState
{
    bool shift = false;
    bool alt = false;  // Option on macOS
    bool ctrl = false; // Command on macOS for most shortcuts
    bool cmd = false;  // For explicit Command key on macOS

    juce::ModifierKeys toJuceModifiers() const;
};

/**
 * Result of a scroll operation
 */
struct ScrollResult
{
    bool success = false;
    double previousValue = 0.0;
    double newValue = 0.0;
};

/**
 * Controls UI elements programmatically for automated testing.
 * All interactions are thread-safe and use the message thread.
 */
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
            mm->callAsync([flushed]() { flushed->signal(); });
            flushed->wait(MESSAGE_THREAD_TIMEOUT_MS);
        }
    }

    /**
     * Set track scope for element lookups.
     * When set, getTargetComponent resolves elements within the specified
     * track's editor window instead of using the global first-match.
     */
    void setTrackScope(int trackIndex, TestDAW* daw);

    /**
     * Clear track scope — revert to global element lookup.
     */
    void clearTrackScope();

    // ================== Mouse Interactions ==================

    /**
     * Click on an element by test ID
     */
    bool click(const juce::String& elementId);

    /**
     * Click with modifier keys (Shift, Alt, Ctrl)
     */
    bool clickWithModifiers(const juce::String& elementId, const ModifierKeyState& modifiers);

    /**
     * Double-click on an element
     */
    bool doubleClick(const juce::String& elementId);

    /**
     * Right-click on an element (context menu)
     */
    bool rightClick(const juce::String& elementId);

    /**
     * Hover over an element (for tooltips)
     * @param durationMs How long to hover (tooltips typically need 500-1000ms)
     */
    bool hover(const juce::String& elementId, int durationMs = 500);

    /**
     * Drag from one element to another
     */
    bool drag(const juce::String& fromElementId, const juce::String& toElementId);

    /**
     * Drag with modifier keys (Alt for fine control, Shift for coarse)
     */
    bool dragWithModifiers(const juce::String& fromElementId, const juce::String& toElementId,
                           const ModifierKeyState& modifiers);

    /**
     * Drag by pixel offset (for precise control)
     */
    bool dragByOffset(const juce::String& elementId, int deltaX, int deltaY);

    /**
     * Drag by pixel offset with modifiers
     */
    bool dragByOffsetWithModifiers(const juce::String& elementId, int deltaX, int deltaY,
                                   const ModifierKeyState& modifiers);

    /**
     * Scroll wheel on an element
     * @param deltaY Positive = scroll up, Negative = scroll down
     * @param deltaX Horizontal scroll (optional)
     */
    ScrollResult scroll(const juce::String& elementId, float deltaY, float deltaX = 0.0f);

    /**
     * Scroll with modifier keys
     */
    ScrollResult scrollWithModifiers(const juce::String& elementId, float deltaY, float deltaX,
                                     const ModifierKeyState& modifiers);

    // ================== Keyboard Interactions ==================

    /**
     * Press a single key on focused element or specified element
     * @param keyCode JUCE key code (e.g., juce::KeyPress::escapeKey)
     * @param elementId Optional element to focus first
     */
    bool pressKey(int keyCode, const juce::String& elementId = {});

    /**
     * Press a key with modifiers
     */
    bool pressKeyWithModifiers(int keyCode, const ModifierKeyState& modifiers, const juce::String& elementId = {});

    /**
     * Press Escape key
     */
    bool pressEscape(const juce::String& elementId = {});

    /**
     * Press Enter key
     */
    bool pressEnter(const juce::String& elementId = {});

    /**
     * Press Space key
     */
    bool pressSpace(const juce::String& elementId = {});

    /**
     * Press Tab key (with optional Shift for reverse)
     */
    bool pressTab(bool reverse = false, const juce::String& elementId = {});

    /**
     * Press Arrow key
     * @param direction 0=Up, 1=Down, 2=Left, 3=Right
     */
    bool pressArrow(int direction, const juce::String& elementId = {});

    /**
     * Press Home key
     */
    bool pressHome(const juce::String& elementId = {});

    /**
     * Press End key
     */
    bool pressEnd(const juce::String& elementId = {});

    /**
     * Press Delete/Backspace key
     */
    bool pressDelete(const juce::String& elementId = {});

    /**
     * Type a character sequence (simulates individual key presses)
     */
    bool typeCharacters(const juce::String& text, const juce::String& elementId = {});

    // ================== Form Interactions ==================

    /**
     * Select an item in a ComboBox by item ID
     */
    bool select(const juce::String& elementId, int itemId);

    /**
     * Select an item in a ComboBox by item text
     */
    bool selectByText(const juce::String& elementId, const juce::String& text);

    /**
     * Select an item in an OscilDropdown by string item ID
     */
    bool selectById(const juce::String& elementId, const juce::String& itemId);

    /**
     * Set a toggle button state
     */
    bool toggle(const juce::String& elementId, bool value);

    /**
     * Set a slider value
     */
    bool setSliderValue(const juce::String& elementId, double value);

    /**
     * Increment slider value by one step
     */
    bool incrementSlider(const juce::String& elementId);

    /**
     * Decrement slider value by one step
     */
    bool decrementSlider(const juce::String& elementId);

    /**
     * Reset slider to default value (simulates double-click)
     */
    bool resetSliderToDefault(const juce::String& elementId);

    /**
     * Type text into a text editor
     */
    bool typeText(const juce::String& elementId, const juce::String& text);

    /**
     * Clear text in a text editor
     */
    bool clearText(const juce::String& elementId);

    // ================== Focus Management ==================

    /**
     * Set focus to an element
     */
    bool setFocus(const juce::String& elementId);

    /**
     * Get the currently focused element's test ID
     */
    juce::String getFocusedElementId();

    /**
     * Check if element has focus
     */
    bool hasFocus(const juce::String& elementId);

    /**
     * Move focus to next focusable element
     */
    bool focusNext();

    /**
     * Move focus to previous focusable element
     */
    bool focusPrevious();

    // ================== State Queries ==================

    /**
     * Get the full UI state as JSON
     */
    json getUIState();

    /**
     * Get element info as JSON
     */
    json getElementInfo(const juce::String& elementId);

    /**
     * Check if element is visible
     */
    bool isElementVisible(const juce::String& elementId);

    /**
     * Check if element is enabled
     */
    bool isElementEnabled(const juce::String& elementId);

    /**
     * Check if element is focusable
     */
    bool isElementFocusable(const juce::String& elementId);

    /**
     * Get slider value
     */
    double getSliderValue(const juce::String& elementId);

    /**
     * Get slider range
     */
    std::pair<double, double> getSliderRange(const juce::String& elementId);

    /**
     * Get toggle state
     */
    bool getToggleState(const juce::String& elementId);

    /**
     * Get text content
     */
    juce::String getTextContent(const juce::String& elementId);

    /**
     * Get selected item in combobox
     */
    int getSelectedItemId(const juce::String& elementId);

    // ================== Waits ==================

    /**
     * Wait for an element to appear (with timeout)
     */
    bool waitForElement(const juce::String& elementId, int timeoutMs = 5000);

    /**
     * Wait for element to be visible
     */
    bool waitForVisible(const juce::String& elementId, int timeoutMs = 5000);

    /**
     * Wait for element to be enabled
     */
    bool waitForEnabled(const juce::String& elementId, int timeoutMs = 5000);

    /**
     * Wait for slider to reach a value (within tolerance)
     */
    bool waitForSliderValue(const juce::String& elementId, double value, double tolerance = 0.01, int timeoutMs = 5000);

protected:
    static constexpr int MESSAGE_THREAD_TIMEOUT_MS = 3000;

    /// Run a function on the message thread synchronously, returning its bool result.
    /// Resolves the element via getTargetComponent and passes it (possibly nullptr) to func.
    /// Uses heap-allocated shared state to prevent use-after-free if the timeout expires
    /// before the queued lambda runs. The lambda captures `self_` (weak_ptr) instead of
    /// `this` so a late-executing callback after timeout + controller destruction is safe.
    template <typename Func>
    bool runOnMessageThreadSync(const juce::String& elementId, Func&& func)
    {
        struct State
        {
            bool result = false;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>();
        auto weak = self_;
        juce::MessageManager::callAsync([weak, elementId, state, f = std::forward<Func>(func)]() mutable {
            auto locked = weak.lock();
            if (!locked || !locked->load(std::memory_order_acquire))
            {
                state->done.signal();
                return;
            }
            auto* self = locked->controller;
            auto* component = self->getTargetComponent(elementId);
            state->result = f(component);
            state->done.signal();
        });
        if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
            return false;
        return state->result;
    }

    /// Overload for lambdas that don't need component resolution.
    template <typename Func>
    bool runOnMessageThreadSync(Func&& func)
    {
        struct State
        {
            bool result = false;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>();
        juce::MessageManager::callAsync([state, f = std::forward<Func>(func)]() mutable {
            state->result = f();
            state->done.signal();
        });
        if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
            return false;
        return state->result;
    }

    /// Run a function on the message thread synchronously, returning a value of type T.
    /// Resolves the element via getTargetComponent and passes it to func.
    /// Captures `self_` (weak_ptr) instead of `this` for late-execution safety.
    template <typename T, typename Func>
    T runOnMessageThreadSyncWithResult(const juce::String& elementId, T defaultValue, Func&& func)
    {
        struct State
        {
            T result;
            juce::WaitableEvent done;
        };
        auto state = std::make_shared<State>(State{std::move(defaultValue), {}});
        auto weak = self_;
        juce::MessageManager::callAsync([weak, elementId, state, f = std::forward<Func>(func)]() mutable {
            auto locked = weak.lock();
            if (!locked || !locked->load(std::memory_order_acquire))
            {
                state->done.signal();
                return;
            }
            auto* self = locked->controller;
            auto* component = self->getTargetComponent(elementId);
            state->result = f(component);
            state->done.signal();
        });
        if (!state->done.wait(MESSAGE_THREAD_TIMEOUT_MS))
            return std::move(state->result);
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

    // Control block shared between the controller and queued callAsync lambdas.
    // Lambdas capture a weak_ptr<ControlBlock> so they can safely detect
    // destruction even if the destructor's flush sentinel times out.
    struct ControlBlock : std::atomic<bool>
    {
        explicit ControlBlock(TestUIController* ctrl) : std::atomic<bool>(true), controller(ctrl) {}
        TestUIController* controller;
    };
    std::shared_ptr<ControlBlock> self_ = std::make_shared<ControlBlock>(this);

    // Track scope for multi-instance element resolution
    int trackScopeIndex_ = -1; // -1 = no scope (global)
    TestDAW* trackScopeDaw_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestUIController)
};

} // namespace oscil::test
