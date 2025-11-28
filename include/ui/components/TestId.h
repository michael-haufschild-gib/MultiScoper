/*
    Oscil - Test ID Support for UI Components

    Provides testId registration that works with or without the test harness.
    When TEST_HARNESS is not defined, macros become no-ops.
*/

#pragma once

#include <juce_core/juce_core.h>

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    // Test harness is active - include full registration
    #include "TestableComponent.h"

    // Note: These macros expand to member declarations, use with semicolon: OSCIL_TESTABLE();
    #define OSCIL_TESTABLE() std::unique_ptr<oscil::test::TestRegistration> testRegistration_
    #define OSCIL_REGISTER_TEST_ID(id) \
        testRegistration_ = std::make_unique<oscil::test::TestRegistration>(*this, id)
    #define OSCIL_REGISTER_CHILD_TEST_ID(component, id) REGISTER_TESTABLE_CHILD(component, id)
    #define OSCIL_UNREGISTER_CHILD_TEST_ID(id) UNREGISTER_TESTABLE_CHILD(id)
#else
    // No test harness - provide empty stubs that are semicolon-safe
    #define OSCIL_TESTABLE() static_assert(true, "")
    #define OSCIL_REGISTER_TEST_ID(id) ((void)0)
    #define OSCIL_REGISTER_CHILD_TEST_ID(component, id) ((void)0)
    #define OSCIL_UNREGISTER_CHILD_TEST_ID(id) ((void)0)
#endif

namespace oscil
{

/**
 * Mixin class for components that support testId.
 * Add this to any component that needs to be testable.
 *
 * Usage:
 *   class MySlider : public juce::Slider, public TestIdSupport {
 *   public:
 *       MySlider(const juce::String& testId = {}) {
 *           setTestId(testId);
 *       }
 *   };
 */
class TestIdSupport
{
public:
    virtual ~TestIdSupport() = default;

    void setTestId(const juce::String& testId)
    {
        if (testId.isEmpty())
            return;

        testId_ = testId;

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
        // Registration happens via the component that inherits this
        registerTestId();
#endif
    }

    juce::String getTestId() const { return testId_; }
    bool hasTestId() const { return testId_.isNotEmpty(); }

protected:
    // Override in component to perform actual registration
    virtual void registerTestId() {}

    juce::String testId_;
};

} // namespace oscil
