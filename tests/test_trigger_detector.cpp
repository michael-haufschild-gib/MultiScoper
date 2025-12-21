#include <gtest/gtest.h>
#include "core/dsp/TriggerDetector.h"
#include <vector>

using namespace oscil;

class TriggerDetectorTest : public ::testing::Test
{
protected:
    TriggerDetector detector;
    TriggerDetector::Config config;
};

TEST_F(TriggerDetectorTest, NoTriggerWhenModeIsNone)
{
    config.mode = WaveformTriggerMode::None;
    std::vector<float> samples = { -1.0f, 0.0f, 1.0f };
    EXPECT_FALSE(detector.process(samples.data(), samples.size(), config));
}

TEST_F(TriggerDetectorTest, RisingEdgeTrigger)
{
    config.mode = WaveformTriggerMode::RisingEdge;
    config.threshold = 0.5f;
    config.hysteresis = 0.1f;

    // Test 1: Simple crossing
    // -0.0 -> 0.0 -> 0.6 (trigger)
    std::vector<float> samples = { 0.0f, 0.6f };
    detector.reset();
    EXPECT_TRUE(detector.process(samples.data(), samples.size(), config));

    // Test 2: No trigger if already above
    // 0.6 -> 0.7 (no trigger, state is already "triggered" from prev test? No, we reset or new instance?)
    // detector is member, state persists. Need reset if we want fresh start.
    detector.reset();
    // Start below, go above
    std::vector<float> s1 = { 0.0f };
    EXPECT_FALSE(detector.process(s1.data(), s1.size(), config));
    std::vector<float> s2 = { 0.6f };
    EXPECT_TRUE(detector.process(s2.data(), s2.size(), config));
}

TEST_F(TriggerDetectorTest, HysteresisRisingEdge)
{
    config.mode = WaveformTriggerMode::RisingEdge;
    config.threshold = 0.5f;
    config.hysteresis = 0.1f; // Re-arm at 0.4

    detector.reset();

    // 1. Cross up -> Trigger
    std::vector<float> s1 = { 0.6f };
    EXPECT_TRUE(detector.process(s1.data(), s1.size(), config));

    // 2. Go down a bit (0.45), but not below hysteresis (0.4) -> Should NOT re-arm
    std::vector<float> s2 = { 0.45f };
    EXPECT_FALSE(detector.process(s2.data(), s2.size(), config));

    // 3. Go up again -> Should NOT trigger (still "triggered" state)
    std::vector<float> s3 = { 0.7f };
    EXPECT_FALSE(detector.process(s3.data(), s3.size(), config));

    // 4. Go below hysteresis (0.3) -> Should Re-arm (but returns false)
    std::vector<float> s4 = { 0.3f };
    EXPECT_FALSE(detector.process(s4.data(), s4.size(), config));

    // 5. Go up again -> Trigger!
    std::vector<float> s5 = { 0.6f };
    EXPECT_TRUE(detector.process(s5.data(), s5.size(), config));
}

TEST_F(TriggerDetectorTest, FallingEdgeTrigger)
{
    config.mode = WaveformTriggerMode::FallingEdge;
    config.threshold = 0.5f;
    config.hysteresis = 0.1f; // Re-arm at 0.6

    detector.reset();

    // Start high (above re-arm)
    std::vector<float> s0 = { 0.8f };
    // Process to ensure state is Armed (previousTriggerState_ false)
    // Actually falling edge logic:
    // If state=false (Armed): check if sample <= thresh -> Trigger
    // So if input is high, we stay Armed.
    EXPECT_FALSE(detector.process(s0.data(), s0.size(), config));

    // Cross down -> Trigger
    std::vector<float> s1 = { 0.4f };
    EXPECT_TRUE(detector.process(s1.data(), s1.size(), config));
}

TEST_F(TriggerDetectorTest, LevelTrigger)
{
    config.mode = WaveformTriggerMode::Level;
    config.threshold = 0.5f;
    config.hysteresis = 0.1f;

    detector.reset();

    // Below
    std::vector<float> s0 = { 0.2f };
    EXPECT_FALSE(detector.process(s0.data(), s0.size(), config));

    // Above (abs)
    std::vector<float> s1 = { -0.6f };
    EXPECT_TRUE(detector.process(s1.data(), s1.size(), config));
}
