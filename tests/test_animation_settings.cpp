/*
    Oscil - Animation Settings Tests
*/

#include <gtest/gtest.h>
#include "ui/components/AnimationSettings.h"
#include "ui/components/SpringAnimation.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cmath>

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

// ============================================================================
// Nested Scope Edge Cases
// ============================================================================

// Test: Nested ScopedReducedMotion behavior
// NOTE: This documents a design limitation - nested scopes don't maintain
// reference counting. Inner scope destructor clears the override.
TEST_F(AnimationSettingsTest, NestedScopedReducedMotion_DocumentLimitation)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    {
        ScopedReducedMotion outer;
        EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

        {
            ScopedReducedMotion inner;
            EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
        }  // inner destructor calls clearOverride()

        // DESIGN LIMITATION: Override is cleared even though outer is in scope
        // This test documents current behavior - outer scope no longer active
        EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
    }

    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: ScopedReducedMotion after manual override
TEST_F(AnimationSettingsTest, ScopedReducedMotion_AfterManualOverride)
{
    // Set manual override to NOT reduce motion
    AnimationSettings::setOverride(true, false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    {
        ScopedReducedMotion scope;
        // Scope overrides to reduce motion
        EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
    }

    // After scope ends, override is cleared (not restored to previous)
    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: Multiple scopes sequential (not nested)
TEST_F(AnimationSettingsTest, MultipleScopesSequential)
{
    AnimationSettings::setAppPreference(false);

    for (int i = 0; i < 5; ++i)
    {
        EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
        {
            ScopedReducedMotion scope;
            EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
        }
        EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
    }
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

// Test: Concurrent app preference changes
TEST_F(AnimationSettingsTest, ConcurrentAppPreferenceChanges)
{
    std::vector<std::thread> threads;
    std::atomic<int> iterations{ 0 };

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&iterations, t]() {
            for (int i = 0; i < 100; ++i)
            {
                AnimationSettings::setAppPreference(t % 2 == 0);
                // Read should always succeed without crash/UB
                bool result = AnimationSettings::prefersReducedMotion();
                (void)result;
                iterations++;
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(iterations.load(), 400);
}

// Test: Concurrent override changes
TEST_F(AnimationSettingsTest, ConcurrentOverrideChanges)
{
    std::vector<std::thread> threads;
    std::atomic<int> iterations{ 0 };

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&iterations, t]() {
            for (int i = 0; i < 100; ++i)
            {
                if (i % 3 == 0)
                    AnimationSettings::setOverride(true, t % 2 == 0);
                else if (i % 3 == 1)
                    AnimationSettings::clearOverride();
                else
                    AnimationSettings::setAppPreference(i % 2 == 0);

                // All reads should be valid (no torn reads)
                bool result = AnimationSettings::prefersReducedMotion();
                (void)result;
                iterations++;
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(iterations.load(), 400);
}

// Test: Concurrent reads while writing
TEST_F(AnimationSettingsTest, ConcurrentReadWhileWriting)
{
    std::atomic<bool> running{ true };
    std::atomic<int> readCount{ 0 };

    // Writer thread
    std::thread writer([&running]() {
        while (running)
        {
            AnimationSettings::setAppPreference(true);
            AnimationSettings::setOverride(true, false);
            AnimationSettings::clearOverride();
            AnimationSettings::setAppPreference(false);
        }
    });

    // Reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 3; ++i)
    {
        readers.emplace_back([&running, &readCount]() {
            while (running)
            {
                AnimationSettings::prefersReducedMotion();
                AnimationSettings::getAnimationDuration(100);
                AnimationSettings::shouldUseSpringAnimations();
                AnimationSettings::getAnimationIntensity();
                readCount++;
            }
        });
    }

    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    writer.join();
    for (auto& reader : readers)
    {
        reader.join();
    }

    EXPECT_GT(readCount.load(), 0);
}

// ============================================================================
// Easing Edge Cases
// ============================================================================

// Test: Lerp with progress below zero
TEST_F(AnimationSettingsTest, LerpProgressBelowZero)
{
    AnimationSettings::setAppPreference(false);

    // Negative progress extrapolates backward
    float result = AnimationHelper::lerp(0.0f, 100.0f, -0.5f);
    EXPECT_FLOAT_EQ(result, -50.0f);
}

// Test: Lerp with progress above one
TEST_F(AnimationSettingsTest, LerpProgressAboveOne)
{
    AnimationSettings::setAppPreference(false);

    // Progress > 1 extrapolates forward
    float result = AnimationHelper::lerp(0.0f, 100.0f, 1.5f);
    EXPECT_FLOAT_EQ(result, 150.0f);
}

// Test: Lerp with negative value range
TEST_F(AnimationSettingsTest, LerpWithNegativeValues)
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
TEST_F(AnimationSettingsTest, LerpWithLargeValues)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::lerp(0.0f, 1e10f, 0.5f);
    EXPECT_NEAR(result, 5e9f, 1e5f);
}

// Test: EaseOut at boundaries
TEST_F(AnimationSettingsTest, EaseOutProgressBoundaries)
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
TEST_F(AnimationSettingsTest, EaseInOutProgressBoundaries)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeInOut(10.0f, 90.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);

    result = AnimationHelper::easeInOut(10.0f, 90.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 90.0f);
}

// Test: EaseOut with negative progress (extrapolation behavior)
TEST_F(AnimationSettingsTest, EaseOutNegativeProgress)
{
    AnimationSettings::setAppPreference(false);

    // Negative progress should extrapolate (may produce values outside range)
    float result = AnimationHelper::easeOut(0.0f, 100.0f, -0.5f);
    EXPECT_TRUE(std::isfinite(result));
}

// Test: EaseInOut with progress > 1 (extrapolation behavior)
TEST_F(AnimationSettingsTest, EaseInOutProgressAboveOne)
{
    AnimationSettings::setAppPreference(false);

    float result = AnimationHelper::easeInOut(0.0f, 100.0f, 1.5f);
    EXPECT_TRUE(std::isfinite(result));
}

// Test: Lerp with same from and to values
TEST_F(AnimationSettingsTest, LerpSameFromTo)
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
TEST_F(AnimationSettingsTest, EasingReducedMotionAlwaysTarget)
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

// ============================================================================
// Duration Edge Cases
// ============================================================================

// Test: Zero duration input
TEST_F(AnimationSettingsTest, ZeroDurationInput)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(0), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(0), 0.0f);

    AnimationSettings::setAppPreference(true);
    EXPECT_EQ(AnimationSettings::getAnimationDuration(0), 0);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(0), 0.0f);
}

// Test: Negative duration input
TEST_F(AnimationSettingsTest, NegativeDurationInput)
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
TEST_F(AnimationSettingsTest, LargeDurationInput)
{
    AnimationSettings::setAppPreference(false);

    int largeDuration = 1000000;  // 1 million ms = ~16 minutes
    EXPECT_EQ(AnimationSettings::getAnimationDuration(largeDuration), largeDuration);
    EXPECT_FLOAT_EQ(AnimationSettings::getAnimationDurationSeconds(largeDuration), 1000.0f);
}

// ============================================================================
// State Consistency Tests
// ============================================================================

// Test: Set override when already overridden
TEST_F(AnimationSettingsTest, OverrideWhenAlreadyOverridden)
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
TEST_F(AnimationSettingsTest, ClearOverrideWhenNotOverridden)
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
TEST_F(AnimationSettingsTest, RapidStateToggle)
{
    for (int i = 0; i < 1000; ++i)
    {
        AnimationSettings::setAppPreference(i % 2 == 0);
        bool expected = (i % 2 == 0);
        EXPECT_EQ(AnimationSettings::prefersReducedMotion(), expected);
    }
}

// Test: Rapid override toggle
TEST_F(AnimationSettingsTest, RapidOverrideToggle)
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
TEST_F(AnimationSettingsTest, StateAfterManyOperations)
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

// Test: performWithMotionPreference helper
TEST_F(AnimationSettingsTest, PerformWithMotionPreference)
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
TEST_F(AnimationSettingsTest, LerpWithIntegerTypes)
{
    AnimationSettings::setAppPreference(false);

    int result = AnimationHelper::lerp(0, 100, 0.5f);
    EXPECT_EQ(result, 50);

    result = AnimationHelper::lerp(0, 100, 0.0f);
    EXPECT_EQ(result, 0);

    result = AnimationHelper::lerp(0, 100, 1.0f);
    EXPECT_EQ(result, 100);
}

// Test: updateFromSystem doesn't crash
TEST_F(AnimationSettingsTest, UpdateFromSystemNoOp)
{
    // Currently a no-op but should not crash
    AnimationSettings::updateFromSystem();
    AnimationSettings::updateFromSystem();
    AnimationSettings::updateFromSystem();

    // State should be unchanged
    AnimationSettings::setAppPreference(true);
    AnimationSettings::updateFromSystem();
    EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
}
