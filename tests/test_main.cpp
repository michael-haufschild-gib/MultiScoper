/*
    Oscil - Test Main Entry Point
*/

#include <gtest/gtest.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

// Initialize JUCE for tests
class JuceInitializer
{
public:
    JuceInitializer()
    {
        juce::initialiseJuce_GUI();
    }

    ~JuceInitializer()
    {
        juce::shutdownJuce_GUI();
    }
};

static JuceInitializer juceInit;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
