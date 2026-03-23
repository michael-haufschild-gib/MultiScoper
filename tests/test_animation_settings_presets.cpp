/*
    Oscil - Animation Settings Tests: Easing Presets
    Tests for easing functions, lerp, and animation helpers
*/

#include "ui/components/AnimationSettings.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace oscil;

class AnimationSettingsPresetsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        AnimationSettings::clearOverride();
        AnimationSettings::setAppPreference(false);
    }

    void TearDown() override
    {
        AnimationSettings::clearOverride();
        AnimationSettings::setAppPreference(false);
    }
};

// Test: AnimationHelper::lerp with normal motion
TEST_F(AnimationSettingsPresetsTest, LerpNormalMotion)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::lerp(0.0f, 100.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 50.0f);

    result = AnimationHelper::lerp(0.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);

    result = AnimationHelper::lerp(0.0f, 100.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: AnimationHelper::lerp with reduced motion
TEST_F(AnimationSettingsPresetsTest, LerpReducedMotion)
{
    AnimationSettings::setAppPreference(true);

    // With reduced motion, lerp should return the target value directly
    float result = AnimationHelper::lerp(0.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);

    result = AnimationHelper::lerp(0.0f, 100.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: AnimationHelper::easeOut with normal motion
TEST_F(AnimationSettingsPresetsTest, EaseOutNormalMotion)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeOut(0.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);

    result = AnimationHelper::easeOut(0.0f, 100.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);

    // Ease out should progress faster at the start
    float midpoint = AnimationHelper::easeOut(0.0f, 100.0f, 0.5f);
    EXPECT_GT(midpoint, 50.0f);
}

// Test: AnimationHelper::easeInOut with normal motion
TEST_F(AnimationSettingsPresetsTest, EaseInOutNormalMotion)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeInOut(0.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);

    result = AnimationHelper::easeInOut(0.0f, 100.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);

    // Midpoint should be approximately halfway with easeInOut
    float midpoint = AnimationHelper::easeInOut(0.0f, 100.0f, 0.5f);
    EXPECT_NEAR(midpoint, 50.0f, 1.0f);
}

// Test: Lerp with progress below zero
TEST_F(AnimationSettingsPresetsTest, LerpProgressBelowZero)
{
    AnimationSettings::setAppPreference(false);

    // Negative progress extrapolates backward
    float result = AnimationHelper::lerp(0.0f, 100.0f, -0.5f);
    EXPECT_FLOAT_EQ(result, -50.0f);
}

// Test: Lerp with progress above one
TEST_F(AnimationSettingsPresetsTest, LerpProgressAboveOne)
{
    AnimationSettings::setAppPreference(false);

    // Progress > 1 extrapolates forward
    float result = AnimationHelper::lerp(0.0f, 100.0f, 1.5f);
    EXPECT_FLOAT_EQ(result, 150.0f);
}

// Test: Lerp with negative value range
TEST_F(AnimationSettingsPresetsTest, LerpWithNegativeValues)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::lerp(-100.0f, 100.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 0.0f);

    result = AnimationHelper::lerp(-100.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, -100.0f);

    result = AnimationHelper::lerp(-100.0f, 100.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: Lerp with large values
TEST_F(AnimationSettingsPresetsTest, LerpWithLargeValues)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::lerp(0.0f, 1e10f, 0.5f);
    EXPECT_NEAR(result, 5e9f, 1e5f);
}

// Test: EaseOut at boundaries
TEST_F(AnimationSettingsPresetsTest, EaseOutProgressBoundaries)
{
    AnimationSettings::setAppPreference(false);

    // At 0, should be at start
    float result = AnimationHelper::easeOut(10.0f, 90.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);

    // At 1, should be at end
    result = AnimationHelper::easeOut(10.0f, 90.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 90.0f);
}

// Test: EaseInOut at boundaries
TEST_F(AnimationSettingsPresetsTest, EaseInOutProgressBoundaries)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeInOut(10.0f, 90.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);

    result = AnimationHelper::easeInOut(10.0f, 90.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 90.0f);
}

// Test: EaseOut with negative progress (extrapolation behavior)
TEST_F(AnimationSettingsPresetsTest, EaseOutNegativeProgress)
{
    AnimationSettings::setAppPreference(false);

    // Negative progress should extrapolate (may produce values outside range)
    float result = AnimationHelper::easeOut(0.0f, 100.0f, -0.5f);
    EXPECT_TRUE(std::isfinite(result));
}

// Test: EaseInOut with progress > 1 (extrapolation behavior)
TEST_F(AnimationSettingsPresetsTest, EaseInOutProgressAboveOne)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeInOut(0.0f, 100.0f, 1.5f);
    EXPECT_TRUE(std::isfinite(result));
}

// Test: Lerp with same from and to values
TEST_F(AnimationSettingsPresetsTest, LerpSameFromTo)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::lerp(50.0f, 50.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 50.0f);

    result = AnimationHelper::lerp(50.0f, 50.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 50.0f);

    result = AnimationHelper::lerp(50.0f, 50.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 50.0f);
}

// Test: Easing with reduced motion returns target for any progress
TEST_F(AnimationSettingsPresetsTest, EasingReducedMotionAlwaysTarget)
{
    AnimationSettings::setAppPreference(true);

    // All progress values should return target (to) value
    for (float p : {-1.0f, 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f})
    {
        EXPECT_FLOAT_EQ(AnimationHelper::lerp(0.0f, 100.0f, p), 100.0f);
        EXPECT_FLOAT_EQ(AnimationHelper::easeOut(0.0f, 100.0f, p), 100.0f);
        EXPECT_FLOAT_EQ(AnimationHelper::easeInOut(0.0f, 100.0f, p), 100.0f);
    }
}

// Test: performWithMotionPreference helper
TEST_F(AnimationSettingsPresetsTest, PerformWithMotionPreference)
{
    bool animatedCalled = false;
    bool instantCalled = false;

    auto animated = [&animatedCalled]() { animatedCalled = true; };
    auto instant = [&instantCalled]() { instantCalled = true; };

    // With motion enabled, animated should be called
    AnimationSettings::setAppPreference(false);
    AnimationHelper::performWithMotionPreference(animated, instant);
    EXPECT_TRUE(animatedCalled);
    EXPECT_FALSE(instantCalled);

    // Reset
    animatedCalled = false;
    instantCalled = false;

    // With reduced motion, instant should be called
    AnimationSettings::setAppPreference(true);
    AnimationHelper::performWithMotionPreference(animated, instant);
    EXPECT_FALSE(animatedCalled);
    EXPECT_TRUE(instantCalled);
}

// Test: Lerp with integer types
TEST_F(AnimationSettingsPresetsTest, LerpWithIntegerTypes)
{
    AnimationSettings::setAppPreference(false);

    int result = AnimationHelper::lerp(0, 100, 0.5f);
    EXPECT_EQ(result, 50);

    result = AnimationHelper::lerp(0, 100, 0.0f);
    EXPECT_EQ(result, 0);

    result = AnimationHelper::lerp(0, 100, 1.0f);
    EXPECT_EQ(result, 100);
}
