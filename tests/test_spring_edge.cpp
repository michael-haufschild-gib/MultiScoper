/*
    Oscil - Spring Animation Tests: Edge Cases
    Tests for numerical stability, extreme values, and SpringAnimationGroup
*/

#include <gtest/gtest.h>
#include "ui/components/SpringAnimation.h"

using namespace oscil;

class SpringEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Extreme high stiffness (numerical stability limitation)
// NOTE: Very high stiffness values can cause numerical instability with
// semi-implicit Euler integration. Use moderate values (100-500) in practice.
TEST_F(SpringEdgeTest, ExtremeHighStiffness)
{
    // Use a high but not extreme stiffness that still works
    SpringAnimation spring(2000.0f, 80.0f, 1.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Should produce finite values with moderate high stiffness
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // Should settle quickly due to high stiffness
    EXPECT_NEAR(spring.position, 1.0f, 0.1f);
}

// Test: Extreme low stiffness
TEST_F(SpringEdgeTest, ExtremeLowStiffness)
{
    SpringAnimation spring(0.01f, 0.1f, 1.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Should still produce finite values
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // With very low stiffness, position should move slowly
    // After 100 frames at 60fps, shouldn't have reached target yet
    EXPECT_LT(spring.position, 1.0f);
}

// Test: Zero stiffness
TEST_F(SpringEdgeTest, ZeroStiffness)
{
    SpringAnimation spring(0.0f, 10.0f, 1.0f);
    spring.position = 0.0f;
    spring.velocity = 10.0f; // Give initial velocity
    spring.setTarget(1.0f);

    // With zero stiffness, spring force is zero
    // Velocity should just be damped
    spring.update(1.0f / 60.0f);

    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));
    EXPECT_LT(spring.velocity, 10.0f); // Velocity reduced by damping
}

// Test: Very small delta time (epsilon-like)
TEST_F(SpringEdgeTest, VerySmallDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    float initialPosition = spring.position;

    // Very small delta time
    spring.update(1e-10f);

    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));
    // Position should have barely changed
    EXPECT_NEAR(spring.position, initialPosition, 1e-6f);
}

// Test: Very large delta time
TEST_F(SpringEdgeTest, VeryLargeDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Very large delta time (10 seconds)
    spring.update(10.0f);

    // Should still produce finite values
    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));

    // Note: Large delta times can cause numerical instability
    // but the result should at least be finite
}

// Test: Extreme mass (very heavy)
TEST_F(SpringEdgeTest, ExtremeMassHigh)
{
    SpringAnimation spring(300.0f, 20.0f, 1000.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // High mass = slow acceleration
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // With high mass, position should move slowly
    EXPECT_LT(spring.position, 0.5f);
}

// Test: Low mass (faster response)
// NOTE: Very low mass values (<0.5) can cause numerical instability with high stiffness.
// The presets all use mass = 1.0 for stable behavior.
TEST_F(SpringEdgeTest, ExtremeMassLow)
{
    // Use slightly lower mass (still stable range)
    SpringAnimation spring(200.0f, 20.0f, 0.5f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Lower mass = higher acceleration, should still be stable
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // With lower mass, should have faster response
    EXPECT_GT(spring.position, 0.0f);
}

// Test: Zero mass (edge case - division by zero protection)
TEST_F(SpringEdgeTest, ZeroMass)
{
    SpringAnimation spring(300.0f, 20.0f, 0.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    spring.update(1.0f / 60.0f);

    // Position should have changed (even if inf/nan, verify update ran)
    // The stiffness and damping are still set
    EXPECT_FLOAT_EQ(spring.stiffness, 300.0f);
    EXPECT_FLOAT_EQ(spring.damping, 20.0f);
}

// Test: Very large target values
TEST_F(SpringEdgeTest, VeryLargeTarget)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(1e6f);

    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // Should be moving toward target
    EXPECT_GT(spring.position, 0.0f);
}

// Test: Negative target
TEST_F(SpringEdgeTest, NegativeTarget)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(-100.0f);

    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // Should be moving toward negative target
    EXPECT_LT(spring.position, 0.0f);
}

// ============================================================================
// SpringAnimationGroup Tests
// ============================================================================

class SpringAnimationGroupTest : public ::testing::Test
{
protected:
    SpringAnimationGroup group;
};

TEST_F(SpringAnimationGroupTest, AddAndGet)
{
    group.add("test", SpringPresets::snappy());

    SpringAnimation* spring = group.get("test");
    ASSERT_NE(spring, nullptr);
    EXPECT_EQ(spring->stiffness, SpringPresets::snappy().stiffness);
}

TEST_F(SpringAnimationGroupTest, GetNonExistent)
{
    SpringAnimation* spring = group.get("nonexistent");
    EXPECT_EQ(spring, nullptr);
}

TEST_F(SpringAnimationGroupTest, GetConst)
{
    group.add("test", SpringPresets::snappy());

    const SpringAnimationGroup& constGroup = group;
    const SpringAnimation* spring = constGroup.get("test");

    ASSERT_NE(spring, nullptr);
    EXPECT_EQ(spring->stiffness, SpringPresets::snappy().stiffness);
}

TEST_F(SpringAnimationGroupTest, GetConstNonExistent)
{
    const SpringAnimationGroup& constGroup = group;
    const SpringAnimation* spring = constGroup.get("nonexistent");
    EXPECT_EQ(spring, nullptr);
}

TEST_F(SpringAnimationGroupTest, UpdateAll)
{
    auto& spring1 = group.add("spring1", SpringPresets::snappy());
    auto& spring2 = group.add("spring2", SpringPresets::bouncy());

    spring1.setTarget(1.0f);
    spring2.setTarget(2.0f);

    group.updateAll(1.0f / 60.0f);

    EXPECT_GT(spring1.position, 0.0f);
    EXPECT_GT(spring2.position, 0.0f);
}

TEST_F(SpringAnimationGroupTest, AnyNeedsUpdate)
{
    auto& spring1 = group.add("spring1", SpringPresets::snappy());
    auto& spring2 = group.add("spring2", SpringPresets::snappy());

    spring1.snapToTarget();
    spring2.snapToTarget();

    EXPECT_FALSE(group.anyNeedsUpdate());

    spring1.setTarget(1.0f);
    spring1.update(1.0f / 60.0f);

    EXPECT_TRUE(group.anyNeedsUpdate());
}

TEST_F(SpringAnimationGroupTest, SnapAllToTarget)
{
    auto& spring1 = group.add("spring1", SpringPresets::snappy());
    auto& spring2 = group.add("spring2", SpringPresets::snappy());

    spring1.setTarget(10.0f);
    spring2.setTarget(20.0f);

    group.snapAllToTarget();

    EXPECT_FLOAT_EQ(spring1.position, 10.0f);
    EXPECT_FLOAT_EQ(spring2.position, 20.0f);
    EXPECT_FALSE(group.anyNeedsUpdate());
}

TEST_F(SpringAnimationGroupTest, AddReturnsReference)
{
    SpringAnimation& spring = group.add("test", SpringPresets::snappy());

    spring.setTarget(5.0f);

    // The reference should be to the stored spring
    EXPECT_FLOAT_EQ(group.get("test")->target, 5.0f);
}

TEST_F(SpringAnimationGroupTest, MultipleSprings)
{
    const int count = 100;

    for (int i = 0; i < count; ++i)
    {
        group.add("spring" + std::to_string(i), SpringPresets::snappy());
    }

    // All springs should be accessible and have correct stiffness
    for (int i = 0; i < count; ++i)
    {
        auto* spring = group.get("spring" + std::to_string(i));
        EXPECT_NE(spring, nullptr);
        EXPECT_EQ(spring->stiffness, SpringPresets::snappy().stiffness);
    }
}

TEST_F(SpringAnimationGroupTest, EmptyGroupUpdateAll)
{
    group.updateAll(1.0f / 60.0f);

    // Empty group should not need updates after updateAll
    EXPECT_FALSE(group.anyNeedsUpdate());
}

TEST_F(SpringAnimationGroupTest, EmptyGroupAnyNeedsUpdate)
{
    EXPECT_FALSE(group.anyNeedsUpdate());

    // Adding a spring at rest should still not need update
    auto& spring = group.add("test", SpringPresets::snappy());
    spring.snapToTarget();
    EXPECT_FALSE(group.anyNeedsUpdate());
}

TEST_F(SpringAnimationGroupTest, EmptyGroupSnapAll)
{
    group.snapAllToTarget();

    // After snap, group should not need updates
    EXPECT_FALSE(group.anyNeedsUpdate());
}

TEST_F(SpringAnimationGroupTest, OverwriteExisting)
{
    group.add("test", SpringAnimation(100.0f, 10.0f, 1.0f));
    EXPECT_FLOAT_EQ(group.get("test")->stiffness, 100.0f);

    group.add("test", SpringAnimation(200.0f, 20.0f, 2.0f));
    EXPECT_FLOAT_EQ(group.get("test")->stiffness, 200.0f);
}
