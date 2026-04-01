/*
    Oscil Test Harness - Testable Component Helper
    Macros and utilities for instrumenting UI components
*/

#pragma once

#include "TestElementRegistry.h"

namespace oscil::test
{

/**
 * RAII helper for automatic element registration/deregistration.
 * Add as a member to any component that needs to be testable.
 *
 * Usage:
 *   class MyButton : public juce::Button {
 *   public:
 *       MyButton() : testRegistration_(*this, "my-button-id") {}
 *   private:
 *       TestRegistration testRegistration_;
 *   };
 */
class TestRegistration
{
public:
    TestRegistration(juce::Component& component, const juce::String& testId) : component_(component), testId_(testId)
    {
        TestElementRegistry::getInstance().registerElement(testId_, &component_);
    }

    ~TestRegistration() { TestElementRegistry::getInstance().unregisterElement(testId_, &component_); }

    const juce::String& getTestId() const { return testId_; }

private:
    juce::Component& component_;
    juce::String testId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestRegistration)
};

/**
 * Mixin class for testable components.
 * Inherit from this alongside your juce::Component to get automatic registration.
 *
 * Usage:
 *   class MySlider : public juce::Slider, public TestableComponentMixin {
 *   public:
 *       MySlider() : TestableComponentMixin(*this, "my-slider") {}
 *   };
 */
class TestableComponentMixin
{
public:
    TestableComponentMixin(juce::Component& component, const juce::String& testId) : registration_(component, testId) {}

    const juce::String& getTestId() const { return registration_.getTestId(); }

private:
    TestRegistration registration_;
};

} // namespace oscil::test

// Convenience macros for quick instrumentation

/**
 * Declare a testable component member.
 * Use in the private section of your component class.
 */
#define DECLARE_TESTABLE() std::unique_ptr<oscil::test::TestRegistration> testRegistration_

/**
 * Register this component for testing.
 * Call in the constructor after component is fully constructed.
 *
 * @param testId The unique identifier for this element
 */
#define REGISTER_TESTABLE(testId) testRegistration_ = std::make_unique<oscil::test::TestRegistration>(*this, testId)

/**
 * Register a child component for testing.
 * Useful when you want to register components you own but don't subclass.
 *
 * @param component The component to register
 * @param testId The unique identifier for this element
 */
#define REGISTER_TESTABLE_CHILD(component, testId) \
    oscil::test::TestElementRegistry::getInstance().registerElement(testId, &component)

/**
 * Unregister a child component.
 *
 * @param testId The identifier used during registration
 */
#define UNREGISTER_TESTABLE_CHILD(testId) oscil::test::TestElementRegistry::getInstance().unregisterElement(testId)

/**
 * Generate a unique testId for indexed elements.
 * Example: TEST_ID_WITH_INDEX("pane", 2) returns "pane-2"
 */
#define TEST_ID_WITH_INDEX(prefix, index) (juce::String(prefix) + "-" + juce::String(index))

/**
 * Generate a compound testId.
 * Example: TEST_ID_COMPOUND("osc", "1", "freq") returns "osc-1-freq"
 */
#define TEST_ID_COMPOUND(a, b, c) (juce::String(a) + "-" + juce::String(b) + "-" + juce::String(c))
