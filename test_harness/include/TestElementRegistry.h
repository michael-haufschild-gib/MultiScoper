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
 *
 * Uses juce::Component::SafePointer internally so that destroyed components
 * are automatically detected as null — no dangling pointer dereferences.
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
     * Returns nullptr if not registered or if the component has been destroyed.
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
     * Get all registered elements (only returns live components)
     */
    std::map<juce::String, juce::Component*> getAllElements();

    /**
     * Get all registered test IDs (safe — no component dereference)
     */
    std::vector<juce::String> getAllTestIds();

    /**
     * Clear all registrations
     */
    void clear();

    /**
     * Check if element exists and is still alive
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

    std::map<juce::String, juce::Component::SafePointer<juce::Component>> elements_;
    std::mutex mutex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestElementRegistry)
};

/**
 * Helper macro to register a component for testing.
 * Use in component constructors.
 */
#define TEST_REGISTER_ELEMENT(testId) TestElementRegistry::getInstance().registerElement(testId, this)

/**
 * Helper macro to unregister a component.
 * Use in component destructors.
 */
#define TEST_UNREGISTER_ELEMENT() TestElementRegistry::getInstance().unregisterElement(this)

} // namespace oscil::test
