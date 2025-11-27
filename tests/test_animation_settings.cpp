/*
    Oscil - Animation Settings Tests
*/

#include <gtest/gtest.h>
#include "ui/components/AnimationSettings.h"
#include "ui/components/SpringAnimation.h"

using namespace oscil;

class AnimationSettingsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clear any overrides before each test
        AnimationSettings::clearOverride();
        AnimationSettings::setAppPreference(false);
    }

    void TearDown() override
    {
        // Restore default state
        AnimationSettings::clearOverride();
        AnimationSettings::setAppPreference(false);
    }
};

// Test: Default state - no reduced motion
TEST_F(AnimationSettingsTest, DefaultState)
{
    // With no override and no app preference, should not prefer reduced motion
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: App preference setting
TEST_F(AnimationSettingsTest, AppPreference)
{
    AnimationSettings::setAppPreference(true);
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: Override takes precedence
TEST_F(AnimationSettingsTest, OverrideTakesPrecedence)
{
    // Set app preference to false
    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    // Override to true
    AnimationSettings::setOverride(true, true);
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    // Override to false (even when override is enabled)
    AnimationSettings::setOverride(true, false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: Clear override restores app preference
TEST_F(AnimationSettingsTest, ClearOverride)
{
    AnimationSettings::setAppPreference(true);
    AnimationSettings::setOverride(true, false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    AnimationSettings::clearOverride();
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
}

// Test: Animation duration with normal motion
TEST_F(AnimationSettingsTest, NormalAnimationDuration)
{
    AnimationSettings::setAppPreference(false);

    EXPECT_EQ(AnimationSettings::getAnimationDuration(100), 100);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(200), 200);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(100), 0.1f);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(1000), 1.0f);
}

// Test: Animation duration with reduced motion
TEST_F(AnimationSettingsTest, ReducedMotionDuration)
{
    AnimationSettings::setAppPreference(true);

    EXPECT_EQ(AnimationSettings::getAnimationDuration(100), 0);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(200), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(100), 0.0f);
}

// Test: Spring animations preference
TEST_F(AnimationSettingsTest, SpringAnimationsPreference)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_TRUE(AnimationSettings::shouldUseSpringAnimations());

    AnimationSettings::setAppPreference(true);
    EXPECT_FALSE(AnimationSettings::shouldUseSpringAnimations());
}

// Test: Animation intensity
TEST_F(AnimationSettingsTest, AnimationIntensity)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationIntensity(), 1.0f);

    AnimationSettings::setAppPreference(true);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationIntensity(), 0.0f);
}

// Test: ScopedReducedMotion RAII helper
TEST_F(AnimationSettingsTest, ScopedReducedMotion)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    {
        ScopedReducedMotion scope;
        EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
    }

    // After scope ends, should be restored
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: AnimationHelper::lerp with normal motion
TEST_F(AnimationSettingsTest, LerpNormalMotion)
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
TEST_F(AnimationSettingsTest, LerpReducedMotion)
{
    AnimationSettings::setAppPreference(true);

    // With reduced motion, lerp should return the target value directly
    float result = AnimationHelper::lerp(0.0f, 100.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);

    result = AnimationHelper::lerp(0.0f, 100.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: AnimationHelper::easeOut with normal motion
TEST_F(AnimationSettingsTest, EaseOutNormalMotion)
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
TEST_F(AnimationSettingsTest, EaseInOutNormalMotion)
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

// Test: Integration with SpringAnimation
TEST_F(AnimationSettingsTest, SpringAnimationIntegration)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_TRUE(AnimationSettings::shouldUseSpringAnimations());

    SpringAnimation spring = SpringPresets::snappy();
    spring.setTarget(1.0f);
    spring.update(1.0f / 60.0f);

    // With animations enabled, spring should make progress
    EXPECT_GT(spring.position, 0.0f);
}
