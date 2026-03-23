/*
    Oscil - Animation Settings Tests: Validation
    Tests for settings validation, duration calculations, and intensity
*/

#include "ui/components/AnimationSettings.h"
#include "ui/components/SpringAnimation.h"

#include <gtest/gtest.h>

using namespace oscil;

class AnimationSettingsValidationTest : public ::testing::Test
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
TEST_F(AnimationSettingsValidationTest, DefaultState)
{
    // With no override and no app preference, should not prefer reduced motion
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: App preference setting
TEST_F(AnimationSettingsValidationTest, AppPreference)
{
    AnimationSettings::setAppPreference(true);
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: Override takes precedence
TEST_F(AnimationSettingsValidationTest, OverrideTakesPrecedence)
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
TEST_F(AnimationSettingsValidationTest, ClearOverride)
{
    AnimationSettings::setAppPreference(true);
    AnimationSettings::setOverride(true, false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    AnimationSettings::clearOverride();
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
}

// Test: Animation duration with normal motion
TEST_F(AnimationSettingsValidationTest, NormalAnimationDuration)
{
    AnimationSettings::setAppPreference(false);

    EXPECT_EQ(AnimationSettings::getAnimationDuration(100), 100);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(200), 200);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(100), 0.1f);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(1000), 1.0f);
}

// Test: Animation duration with reduced motion
TEST_F(AnimationSettingsValidationTest, ReducedMotionDuration)
{
    AnimationSettings::setAppPreference(true);

    EXPECT_EQ(AnimationSettings::getAnimationDuration(100), 0);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(200), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(100), 0.0f);
}

// Test: Spring animations preference
TEST_F(AnimationSettingsValidationTest, SpringAnimationsPreference)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_TRUE(AnimationSettings::shouldUseSpringAnimations());

    AnimationSettings::setAppPreference(true);
    EXPECT_FALSE(AnimationSettings::shouldUseSpringAnimations());
}

// Test: Animation intensity
TEST_F(AnimationSettingsValidationTest, AnimationIntensity)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationIntensity(), 1.0f);

    AnimationSettings::setAppPreference(true);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationIntensity(), 0.0f);
}

// Test: Integration with SpringAnimation
TEST_F(AnimationSettingsValidationTest, SpringAnimationIntegration)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_TRUE(AnimationSettings::shouldUseSpringAnimations());

    SpringAnimation spring = SpringPresets::snappy();
    spring.setTarget(1.0f);
    spring.update(1.0f / 60.0f);

    // With animations enabled, spring should make progress
    EXPECT_GT(spring.position, 0.0f);
}

// Test: Zero duration input
TEST_F(AnimationSettingsValidationTest, ZeroDurationInput)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(0), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(0), 0.0f);

    AnimationSettings::setAppPreference(true);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(0), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(0), 0.0f);
}

// Test: Negative duration input
TEST_F(AnimationSettingsValidationTest, NegativeDurationInput)
{
    AnimationSettings::setAppPreference(false);

    // Negative duration passes through when motion is enabled
    EXPECT_EQ(AnimationSettings::getAnimationDuration(-100), -100);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(-100), -0.1f);

    AnimationSettings::setAppPreference(true);
    // With reduced motion, returns 0 regardless
    EXPECT_EQ(AnimationSettings::getAnimationDuration(-100), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(-100), 0.0f);
}

// Test: Very large duration input
TEST_F(AnimationSettingsValidationTest, LargeDurationInput)
{
    AnimationSettings::setAppPreference(false);

    int largeDuration = 1000000; // 1 million ms = ~16 minutes
    EXPECT_EQ(AnimationSettings::getAnimationDuration(largeDuration), largeDuration);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(largeDuration), 1000.0f);
}

// Test: Set override when already overridden
TEST_F(AnimationSettingsValidationTest, OverrideWhenAlreadyOverridden)
{
    AnimationSettings::setOverride(true, true);
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    // Override with different value
    AnimationSettings::setOverride(true, false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    // Override back
    AnimationSettings::setOverride(true, true);
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
}

// Test: Clear override when not overridden
TEST_F(AnimationSettingsValidationTest, ClearOverrideWhenNotOverridden)
{
    AnimationSettings::setAppPreference(true);

    // Clear when no override is set - should be safe
    AnimationSettings::clearOverride();
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    // Clear multiple times
    AnimationSettings::clearOverride();
    AnimationSettings::clearOverride();
    AnimationSettings::clearOverride();
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
}

// Test: Rapid state toggle
TEST_F(AnimationSettingsValidationTest, RapidStateToggle)
{
    for (int i = 0; i < 1000; ++i)
    {
        AnimationSettings::setAppPreference(i % 2 == 0);
        bool expected = (i % 2 == 0);
        EXPECT_EQ(AnimationSettings::prefersReducedMotion(), expected);
    }
}

// Test: Rapid override toggle
TEST_F(AnimationSettingsValidationTest, RapidOverrideToggle)
{
    AnimationSettings::setAppPreference(false);

    for (int i = 0; i < 1000; ++i)
    {
        if (i % 2 == 0)
        {
            AnimationSettings::setOverride(true, true);
            EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
        }
        else
        {
            AnimationSettings::clearOverride();
            EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
        }
    }
}

// Test: State after many mixed operations
TEST_F(AnimationSettingsValidationTest, StateAfterManyOperations)
{
    // Perform many operations
    for (int i = 0; i < 100; ++i)
    {
        AnimationSettings::setAppPreference(true);
        AnimationSettings::setOverride(true, false);
        AnimationSettings::clearOverride();
        AnimationSettings::setAppPreference(false);
        AnimationSettings::setOverride(true, true);
    }

    // Final state should be: override=true, reducedMotion=true
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

    AnimationSettings::clearOverride();
    // App preference is false
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: updateFromSystem doesn't crash
TEST_F(AnimationSettingsValidationTest, UpdateFromSystemNoOp)
{
    AnimationSettings::updateFromSystem();
    AnimationSettings::updateFromSystem();
    AnimationSettings::updateFromSystem();

    // After multiple calls, prefersReducedMotion should return a consistent boolean
    bool reduced1 = AnimationSettings::prefersReducedMotion();
    bool reduced2 = AnimationSettings::prefersReducedMotion();
    EXPECT_EQ(reduced1, reduced2);
}
