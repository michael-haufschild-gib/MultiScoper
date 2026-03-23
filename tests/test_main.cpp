/*
    Oscil - Test Main Entry Point

    NOTE: JUCE initialization is deferred to SetUp() rather than using a static
    initializer to avoid crashes during gtest_discover_tests, which runs the
    executable with --gtest_list_tests to enumerate tests at build time.
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <gtest/gtest.h>

// GoogleTest environment for JUCE initialization
// This is only invoked when tests actually run, not during test discovery
class JuceTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override { juce::initialiseJuce_GUI(); }

    void TearDown() override { juce::shutdownJuce_GUI(); }
};

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // Register JUCE environment - SetUp() is only called when running tests,
    // not when listing tests (--gtest_list_tests) during test discovery
    ::testing::AddGlobalTestEnvironment(new JuceTestEnvironment());

    return RUN_ALL_TESTS();
}
