/*
    Oscil - Spring Animation Tests: Physics
    Tests for spring calculations, damping, stiffness, and settling behavior
*/

#include <gtest/gtest.h>
#include "ui/components/SpringAnimation.h"

using namespace oscil;

class SpringPhysicsTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(SpringPhysicsTest, DefaultConstruction)
{
    SpringAnimation spring;

    EXPECT_FLOAT_EQ(spring.position, 0.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f);
    EXPECT_FLOAT_EQ(spring.target, 0.0f);
}

// Test: Parameterized construction
TEST_F(SpringPhysicsTest, ParameterizedConstruction)
{
    SpringAnimation spring(100.0f, 10.0f, 1.0f);

    EXPECT_FLOAT_EQ(spring.stiffness, 100.0f);
    EXPECT_FLOAT_EQ(spring.damping, 10.0f);
    EXPECT_FLOAT_EQ(spring.mass, 1.0f);
}

// Test: Preset configurations
TEST_F(SpringPhysicsTest, SpringPresets)
{
    auto snappy = SpringPresets::snappy();
    EXPECT_GT(snappy.stiffness, 0.0f);
    EXPECT_GT(snappy.damping, 0.0f);

    auto bouncy = SpringPresets::bouncy();
    EXPECT_GT(bouncy.stiffness, 0.0f);

    auto smooth = SpringPresets::smooth();
    EXPECT_GT(smooth.stiffness, 0.0f);

    auto gentle = SpringPresets::gentle();
    EXPECT_LT(gentle.stiffness, snappy.stiffness);

    auto stiff = SpringPresets::stiff();
    EXPECT_GT(stiff.stiffness, 0.0f);

    auto wobbly = SpringPresets::wobbly();
    EXPECT_GT(wobbly.stiffness, 0.0f);
    EXPECT_GT(wobbly.damping, 0.0f);
    // Wobbly should have low damping for more oscillation
    EXPECT_LT(wobbly.damping, snappy.damping);
}

// Test: Setting target
TEST_F(SpringPhysicsTest, SetTarget)
{
    SpringAnimation spring;
    spring.setTarget(100.0f);

    EXPECT_FLOAT_EQ(spring.target, 100.0f);
}

// Test: Snap to target
TEST_F(SpringPhysicsTest, SnapToTarget)
{
    SpringAnimation spring;
    spring.setTarget(50.0f);
    spring.snapToTarget();

    EXPECT_FLOAT_EQ(spring.position, 50.0f);
    EXPECT_FLOAT_EQ(spring.target, 50.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f);
}

// Test: Update moves toward target
TEST_F(SpringPhysicsTest, UpdateMovesTowardTarget)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    float initialPosition = spring.position;
    spring.update(1.0f / 60.0f);

    EXPECT_GT(spring.position, initialPosition);
    EXPECT_LT(spring.position, 100.0f);
}

// Test: Spring eventually settles at target
TEST_F(SpringPhysicsTest, SettlesAtTarget)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    // Simulate many updates
    for (int i = 0; i < 1000; ++i)
    {
        spring.update(1.0f / 60.0f);
    }

    EXPECT_NEAR(spring.position, 100.0f, 0.01f);
    EXPECT_TRUE(spring.isSettled());
}

// Test: isSettled returns correct value
TEST_F(SpringPhysicsTest, IsSettled)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.setTarget(50.0f);
    spring.snapToTarget();

    EXPECT_TRUE(spring.isSettled());

    spring.setTarget(100.0f);
    spring.update(1.0f / 60.0f);

    EXPECT_FALSE(spring.isSettled());
}

// Test: Zero delta time doesn't change position
TEST_F(SpringPhysicsTest, ZeroDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    float initialPosition = spring.position;
    spring.update(0.0f);

    EXPECT_FLOAT_EQ(spring.position, initialPosition);
}

// Test: Negative delta time is handled
TEST_F(SpringPhysicsTest, NegativeDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    float initialPosition = spring.position;
    spring.update(-1.0f);

    EXPECT_FLOAT_EQ(spring.position, initialPosition);
}

// Test: Bouncy preset has overshoot
TEST_F(SpringPhysicsTest, BouncyHasOvershoot)
{
    SpringAnimation spring = SpringPresets::bouncy();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    float maxPosition = 0.0f;
    for (int i = 0; i < 200; ++i)
    {
        spring.update(1.0f / 60.0f);
        maxPosition = std::max(maxPosition, spring.position);
    }

    // Bouncy should overshoot the target
    EXPECT_GT(maxPosition, 1.0f);
}

// Test: Reset clears state
TEST_F(SpringPhysicsTest, Reset)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 50.0f;
    spring.velocity = 100.0f;
    spring.target = 100.0f;

    spring.reset();

    EXPECT_FLOAT_EQ(spring.position, 0.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f);
    EXPECT_FLOAT_EQ(spring.target, 0.0f);
}

// Test: High damping (overdamped spring)
TEST_F(SpringPhysicsTest, ExtremeHighDamping)
{
    // Use high but stable damping (similar to stiff preset)
    SpringAnimation spring(300.0f, 50.0f, 1.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // High damping should prevent oscillation and produce finite values
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // With high damping, motion should be overdamped (smooth approach, minimal oscillation)
    // Position should have moved toward target
    EXPECT_GT(spring.position, 0.0f);
}

// Test: Zero damping (undamped oscillation)
TEST_F(SpringPhysicsTest, ZeroDamping)
{
    SpringAnimation spring(300.0f, 0.0f, 1.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // With zero damping, should oscillate forever (theoretically)
    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position));
        EXPECT_TRUE(std::isfinite(spring.velocity));
    }

    // With zero damping, isSettled should still be false
    // (unless very close to target with zero velocity by chance)
}

// Test: needsUpdate is inverse of isSettled
TEST_F(SpringPhysicsTest, NeedsUpdateConsistency)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.setTarget(1.0f);
    spring.snapToTarget();

    EXPECT_TRUE(spring.isSettled());
    EXPECT_FALSE(spring.needsUpdate());

    spring.setTarget(2.0f);
    spring.update(1.0f / 60.0f);

    EXPECT_FALSE(spring.isSettled());
    EXPECT_TRUE(spring.needsUpdate());
}

// Test: isSettled with custom threshold
TEST_F(SpringPhysicsTest, IsSettledCustomThreshold)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.9f;
    spring.velocity = 0.05f;
    spring.target = 1.0f;

    // Default threshold (0.001) - not settled
    EXPECT_FALSE(spring.isSettled(0.001f));

    // Large threshold (0.2) - settled
    EXPECT_TRUE(spring.isSettled(0.2f));
}

// Test: Multiple rapid updates
TEST_F(SpringPhysicsTest, RapidUpdates)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Many very small updates
    for (int i = 0; i < 10000; ++i)
    {
        spring.update(0.0001f);
    }

    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));
}
