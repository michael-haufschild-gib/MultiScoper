/*
    Oscil - Animation Settings Tests: Persistence & Threading
    Tests for scope overrides, state persistence, and thread safety
*/

#include "ui/components/AnimationSettings.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace oscil;

class AnimationSettingsPersistenceTest : public ::testing::Test
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

// Test: ScopedReducedMotion RAII helper
TEST_F(AnimationSettingsPersistenceTest, ScopedReducedMotion)
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

// Test: Nested ScopedReducedMotion behavior
// NOTE: This documents a design limitation - nested scopes don't maintain
// reference counting. Inner scope destructor clears the override.
TEST_F(AnimationSettingsPersistenceTest, NestedScopedReducedMotion_DocumentLimitation)
{
    AnimationSettings::setAppPreference(false);
    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());

    {
        ScopedReducedMotion outer;
        EXPECT_TRUE(AnimationSettings::prefersReducedMotion());

        {
            ScopedReducedMotion inner;
            EXPECT_TRUE(AnimationSettings::prefersReducedMotion());
        } // inner destructor calls clearOverride()

        // DESIGN LIMITATION: Override is cleared even though outer is in scope
        // This test documents current behavior - outer scope no longer active
        EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
    }

    EXPECT_FALSE(AnimationSettings::prefersReducedMotion());
}

// Test: ScopedReducedMotion after manual override
TEST_F(AnimationSettingsPersistenceTest, ScopedReducedMotion_AfterManualOverride)
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
TEST_F(AnimationSettingsPersistenceTest, MultipleScopesSequential)
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

// Test: Concurrent app preference changes
TEST_F(AnimationSettingsPersistenceTest, ConcurrentAppPreferenceChanges)
{
    std::vector<std::thread> threads;
    std::atomic<int> iterations{0};

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&iterations, t]() {
            for (int i = 0; i < 100; ++i)
            {
                AnimationSettings::setAppPreference(t % 2 == 0);
                // Read should always succeed without crash/UB
                bool result = AnimationSettings::prefersReducedMotion();
                (void) result;
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
TEST_F(AnimationSettingsPersistenceTest, ConcurrentOverrideChanges)
{
    std::vector<std::thread> threads;
    std::atomic<int> iterations{0};

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
                (void) result;
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
TEST_F(AnimationSettingsPersistenceTest, ConcurrentReadWhileWriting)
{
    std::atomic<bool> running{true};
    std::atomic<int> readCount{0};

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
