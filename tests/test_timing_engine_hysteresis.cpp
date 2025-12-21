#include "OscilTestFixtures.h"
#include "core/dsp/TimingEngine.h"
#include <gtest/gtest.h>

namespace oscil
{

class TimingEngineHysteresisTest : public ::testing::Test
{
protected:
    TimingEngine engine;
};

TEST_F(TimingEngineHysteresisTest, HysteresisClampingLower)
{
    engine.setTriggerHysteresis(-0.5f);
    auto config = engine.getConfig();
    EXPECT_GE(config.triggerHysteresis, 0.0f);
}

TEST_F(TimingEngineHysteresisTest, HysteresisClampingUpper)
{
    engine.setTriggerHysteresis(2.0f);
    auto config = engine.getConfig();
    EXPECT_LE(config.triggerHysteresis, 1.0f);
}

TEST_F(TimingEngineHysteresisTest, DeserializationClamping)
{
    juce::ValueTree state(TimingIds::Timing);
    state.setProperty(TimingIds::TriggerHysteresis, 5.0f, nullptr);
    
    engine.fromValueTree(state);
    
    auto config = engine.getConfig();
    EXPECT_LE(config.triggerHysteresis, 1.0f);
}

}
