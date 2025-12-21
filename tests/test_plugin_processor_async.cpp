/*
    Oscil - Plugin Processor Async Tests
    Tests for real-time safety and async behavior in PluginProcessor
*/

#include "OscilTestFixtures.h"

using namespace oscil;
using namespace oscil::test;

class PluginProcessorAsyncTest : public OscilPluginTestFixture
{
};

TEST_F(PluginProcessorAsyncTest, UpdateTrackProperties_IsAsync)
{
    // 1. Setup - get initial state
    SourceId sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid());
    
    auto initialInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(initialInfo.has_value());
    juce::String initialName = initialInfo->name;
    
    // 2. Action - Update properties with a new name
    juce::AudioProcessor::TrackProperties props;
    props.name = "New Async Track Name";
    
    processor->updateTrackProperties(props);
    
    // 3. Assertion 1 - Should NOT change immediately (because we want it deferred)
    auto infoAfterCall = getRegistry().getSource(sourceId);
    ASSERT_TRUE(infoAfterCall.has_value());
    
    // THIS EXPECTATION CONFIRMS ASYNC BEHAVIOR
    // If code is synchronous (buggy), this will FAIL (it will be equal).
    EXPECT_NE(infoAfterCall->name, "New Async Track Name") << "Registry updated synchronously! Real-time violation.";
    
    // 4. Wait for update (pumping messages)
    bool updated = waitForCondition([&]() {
        auto info = getRegistry().getSource(sourceId);
        return info.has_value() && info->name == "New Async Track Name";
    }, 1000); // Wait up to 1 second
    
    EXPECT_TRUE(updated) << "Registry update timed out!";
    
    auto updatedInfo = getRegistry().getSource(sourceId);
    ASSERT_TRUE(updatedInfo.has_value());
    EXPECT_EQ(updatedInfo->name, "New Async Track Name");
    
    // 6. Action - Update again
    props.name = "Another Update";
    processor->updateTrackProperties(props);
    
    // Verify it doesn't crash if we pump
    updated = waitForCondition([&]() {
        auto info = getRegistry().getSource(sourceId);
        return info.has_value() && info->name == "Another Update";
    }, 1000);
    EXPECT_TRUE(updated);
}
