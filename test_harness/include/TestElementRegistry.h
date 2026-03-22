/*
    Oscil Test Harness - Element Registry
    Tracks UI elements by test IDs for automated interaction
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <mutex>

namespace oscil::test
{

/**
 * Singleton registry that tracks UI components by test IDs.
 * Components register themselves when created and unregister on destruction.
 */
class TestElementRegistry
{
public:
    static TestElementRegistry& getInstance();

    /**
     * Register a component with a test ID
     */
    void registerElement(const juce::String& testId, juce::Component* component);

    /**
     * Unregister a component
     */
    void unregisterElement(juce::Component* component);

    /**
     * Unregister by test ID
     */
    void unregisterElement(const juce::String& testId);

    /**
     * Find a component by test ID.
     * Returns nullptr if not registered.
     * WARNING: The returned pointer may become stale if the component is
     * deleted on the message thread after this call returns.
     */
    juce::Component* findElement(const juce::String& testId);

    /**
     * Find a component by test ID, returning nullptr if the component
     * is registered but no longer part of a live component hierarchy
     * (i.e., has no parent and is not on screen). This is safer than
     * findElement() when the result will be used for rendering/interaction.
     */
    juce::Component* findValidElement(const juce::String& testId);

    /**
     * Get all registered elements
     */
    std::map<juce::String, juce::Component*> getAllElements();

    /**
     * Clear all registrations
     */
    void clear();

    /**
     * Check if element exists
     */
    bool hasElement(const juce::String& testId);

    /**
     * Get element bounds in screen coordinates
     */
    juce::Rectangle<int> getElementScreenBounds(const juce::String& testId);

    /**
     * Get element bounds relative to its top-level window
     */
    juce::Rectangle<int> getElementBounds(const juce::String& testId);

private:
    TestElementRegistry() = default;
    ~TestElementRegistry() = default;

    std::map<juce::String, juce::Component*> elements_;
    std::mutex mutex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestElementRegistry)
};

/**
 * Helper macro to register a component for testing.
 * Use in component constructors.
 */
#define TEST_REGISTER_ELEMENT(testId) \
    TestElementRegistry::getInstance().registerElement(testId, this)

/**
 * Helper macro to unregister a component.
 * Use in component destructors.
 */
#define TEST_UNREGISTER_ELEMENT() \
    TestElementRegistry::getInstance().unregisterElement(this)

} // namespace oscil::test
