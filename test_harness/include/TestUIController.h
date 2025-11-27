/*
    Oscil Test Harness - UI Controller
    Provides programmatic UI interaction
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "TestElementRegistry.h"
#include <nlohmann/json.hpp>

namespace oscil::test
{

using json = nlohmann::json;

/**
 * Controls UI elements programmatically for automated testing.
 * All interactions are thread-safe and use the message thread.
 */
class TestUIController
{
public:
    TestUIController() = default;
    ~TestUIController() = default;

    /**
     * Click on an element by test ID
     */
    bool click(const juce::String& elementId);

    /**
     * Double-click on an element
     */
    bool doubleClick(const juce::String& elementId);

    /**
     * Select an item in a ComboBox by item ID
     */
    bool select(const juce::String& elementId, int itemId);

    /**
     * Select an item in a ComboBox by item text
     */
    bool selectByText(const juce::String& elementId, const juce::String& text);

    /**
     * Set a toggle button state
     */
    bool toggle(const juce::String& elementId, bool value);

    /**
     * Set a slider value
     */
    bool setSliderValue(const juce::String& elementId, double value);

    /**
     * Drag from one element to another
     */
    bool drag(const juce::String& fromElementId, const juce::String& toElementId);

    /**
     * Type text into a text editor
     */
    bool typeText(const juce::String& elementId, const juce::String& text);

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
     * Wait for an element to appear (with timeout)
     */
    bool waitForElement(const juce::String& elementId, int timeoutMs = 5000);

private:
    void simulateMouseClick(juce::Component* component, bool doubleClick = false);
    void simulateMouseDrag(juce::Component* from, juce::Component* to);
    json componentToJson(juce::Component* component, const juce::String& testId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestUIController)
};

} // namespace oscil::test
