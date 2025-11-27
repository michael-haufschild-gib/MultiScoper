/*
    Oscil - Spring Animation Tests
*/

#include <gtest/gtest.h>
#include "ui/components/SpringAnimation.h"

using namespace oscil;

class SpringAnimationTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(SpringAnimationTest, DefaultConstruction)
{
    SpringAnimation spring;

    EXPECT_FLOAT_EQ(spring.position, 0.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f);
    EXPECT_FLOAT_EQ(spring.target, 0.0f);
}

// Test: Parameterized construction
TEST_F(SpringAnimationTest, ParameterizedConstruction)
{
    SpringAnimation spring(100.0f, 10.0f, 1.0f);

    EXPECT_FLOAT_EQ(spring.stiffness, 100.0f);
    EXPECT_FLOAT_EQ(spring.damping, 10.0f);
    EXPECT_FLOAT_EQ(spring.mass, 1.0f);
}

// Test: Preset configurations
TEST_F(SpringAnimationTest, SpringPresets)
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
}

// Test: Setting target
TEST_F(SpringAnimationTest, SetTarget)
{
    SpringAnimation spring;
    spring.setTarget(100.0f);

    EXPECT_FLOAT_EQ(spring.target, 100.0f);
}

// Test: Snap to target
TEST_F(SpringAnimationTest, SnapToTarget)
{
    SpringAnimation spring;
    spring.setTarget(50.0f);
    spring.snapToTarget();

    EXPECT_FLOAT_EQ(spring.position, 50.0f);
    EXPECT_FLOAT_EQ(spring.target, 50.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f);
}

// Test: Update moves toward target
TEST_F(SpringAnimationTest, UpdateMovesTowardTarget)
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
TEST_F(SpringAnimationTest, SettlesAtTarget)
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
TEST_F(SpringAnimationTest, IsSettled)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.setTarget(50.0f);
    spring.snapToTarget();

    EXPECT_TRUE(spring.isSettled());

    spring.setTarget(100.0f);
    spring.update(1.0f / 60.0f);

    EXPECT_FALSE(spring.isSettled());
}

// Test: getNormalized returns correct value
TEST_F(SpringAnimationTest, GetNormalized)
{
    SpringAnimation spring;
    spring.position = 0.5f;
    spring.target = 1.0f;

    EXPECT_FLOAT_EQ(spring.getNormalized(), 0.5f);

    spring.position = 0.0f;
    EXPECT_FLOAT_EQ(spring.getNormalized(), 0.0f);

    spring.position = 1.0f;
    EXPECT_FLOAT_EQ(spring.getNormalized(), 1.0f);
}

// Test: getNormalized clamps values
TEST_F(SpringAnimationTest, GetNormalizedClamps)
{
    SpringAnimation spring = SpringPresets::bouncy();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    for (int i = 0; i < 100; ++i)
    {
        spring.update(1.0f / 60.0f);
        // getNormalized always returns clamped value
        float normalized = spring.getNormalized();
        EXPECT_GE(normalized, 0.0f);
        EXPECT_LE(normalized, 1.0f);
    }
}

// Test: Reset clears state
TEST_F(SpringAnimationTest, Reset)
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

// Test: Zero delta time doesn't change position
TEST_F(SpringAnimationTest, ZeroDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    float initialPosition = spring.position;
    spring.update(0.0f);

    EXPECT_FLOAT_EQ(spring.position, initialPosition);
}

// Test: Negative delta time is handled
TEST_F(SpringAnimationTest, NegativeDeltaTime)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.0f;
    spring.setTarget(100.0f);

    float initialPosition = spring.position;
    spring.update(-1.0f);

    EXPECT_FLOAT_EQ(spring.position, initialPosition);
}

// Test: Bouncy preset has overshoot
TEST_F(SpringAnimationTest, BouncyHasOvershoot)
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
