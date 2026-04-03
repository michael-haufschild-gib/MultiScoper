/*
    Oscil - Ease Animation Tests: Core Behavior
    Tests for exponential decay, settling, and preset configurations
*/

#include "ui/components/SpringAnimation.h"

#include <gtest/gtest.h>

using namespace oscil;

class EaseAnimationTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(EaseAnimationTest, DefaultConstruction)
{
    SpringAnimation anim;

    EXPECT_FLOAT_EQ(anim.position, 0.0f);
    EXPECT_FLOAT_EQ(anim.velocity, 0.0f);
    EXPECT_FLOAT_EQ(anim.target, 0.0f);
}

TEST_F(EaseAnimationTest, ParameterizedConstruction)
{
    SpringAnimation anim(100.0f, 10.0f, 1.0f);

    EXPECT_FLOAT_EQ(anim.stiffness, 100.0f);
    EXPECT_FLOAT_EQ(anim.damping, 10.0f);
    EXPECT_FLOAT_EQ(anim.mass, 1.0f);
}

TEST_F(EaseAnimationTest, Presets)
{
    auto fast = SpringPresets::fast();
    EXPECT_GT(fast.stiffness, 0.0f);

    auto medium = SpringPresets::medium();
    EXPECT_GT(medium.stiffness, 0.0f);
    EXPECT_LT(medium.stiffness, fast.stiffness);

    auto slow = SpringPresets::slow();
    EXPECT_GT(slow.stiffness, 0.0f);
    EXPECT_LT(slow.stiffness, medium.stiffness);
}

// Test: Fast preset settles quicker than slow
TEST_F(EaseAnimationTest, FastSettlesBeforeSlow)
{
    SpringAnimation fast = SpringPresets::fast();
    SpringAnimation slow = SpringPresets::slow();

    fast.setTarget(1.0f);
    slow.setTarget(1.0f);

    for (int i = 0; i < 5; ++i)
    {
        fast.update(1.0f / 60.0f);
        slow.update(1.0f / 60.0f);
    }

    EXPECT_GT(fast.position, slow.position);
}

TEST_F(EaseAnimationTest, SetTarget)
{
    SpringAnimation anim;
    anim.setTarget(100.0f);

    EXPECT_FLOAT_EQ(anim.target, 100.0f);
}

TEST_F(EaseAnimationTest, SnapToTarget)
{
    SpringAnimation anim;
    anim.setTarget(50.0f);
    anim.snapToTarget();

    EXPECT_FLOAT_EQ(anim.position, 50.0f);
    EXPECT_FLOAT_EQ(anim.target, 50.0f);
    EXPECT_FLOAT_EQ(anim.velocity, 0.0f);
}

TEST_F(EaseAnimationTest, UpdateMovesTowardTarget)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(100.0f);

    float initialPosition = anim.position;
    anim.update(1.0f / 60.0f);

    EXPECT_GT(anim.position, initialPosition);
    EXPECT_LT(anim.position, 100.0f);
}

// Test: Animation approaches target monotonically (no overshoot)
TEST_F(EaseAnimationTest, NoOvershoot)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    float prevPosition = 0.0f;
    for (int i = 0; i < 120; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_GE(anim.position, prevPosition) << "Overshoot at frame " << i;
        EXPECT_LE(anim.position, 1.0f) << "Exceeded target at frame " << i;
        prevPosition = anim.position;
    }
}

// Test: No overshoot when approaching from above
TEST_F(EaseAnimationTest, NoOvershootFromAbove)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 1.0f;
    anim.setTarget(0.0f);

    float prevPosition = 1.0f;
    for (int i = 0; i < 120; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_LE(anim.position, prevPosition) << "Overshoot at frame " << i;
        EXPECT_GE(anim.position, 0.0f) << "Below target at frame " << i;
        prevPosition = anim.position;
    }
}

// Test: Animation eventually settles at target
TEST_F(EaseAnimationTest, SettlesAtTarget)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(100.0f);

    for (int i = 0; i < 120; ++i)
        anim.update(1.0f / 60.0f);

    EXPECT_NEAR(anim.position, 100.0f, 0.01f);
    EXPECT_TRUE(anim.isSettled());
}

TEST_F(EaseAnimationTest, IsSettled)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.setTarget(50.0f);
    anim.snapToTarget();

    EXPECT_TRUE(anim.isSettled());

    anim.setTarget(100.0f);
    anim.update(1.0f / 60.0f);

    EXPECT_FALSE(anim.isSettled());
}

TEST_F(EaseAnimationTest, ZeroDeltaTime)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(100.0f);

    float initialPosition = anim.position;
    anim.update(0.0f);

    EXPECT_FLOAT_EQ(anim.position, initialPosition);
}

TEST_F(EaseAnimationTest, NegativeDeltaTime)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(100.0f);

    float initialPosition = anim.position;
    anim.update(-1.0f);

    EXPECT_FLOAT_EQ(anim.position, initialPosition);
}

TEST_F(EaseAnimationTest, Reset)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 50.0f;
    anim.velocity = 100.0f;
    anim.target = 100.0f;

    anim.reset();

    EXPECT_FLOAT_EQ(anim.position, 0.0f);
    EXPECT_FLOAT_EQ(anim.velocity, 0.0f);
    EXPECT_FLOAT_EQ(anim.target, 0.0f);
}

TEST_F(EaseAnimationTest, NeedsUpdateConsistency)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.setTarget(1.0f);
    anim.snapToTarget();

    EXPECT_TRUE(anim.isSettled());
    EXPECT_FALSE(anim.needsUpdate());

    anim.setTarget(2.0f);
    anim.update(1.0f / 60.0f);

    EXPECT_FALSE(anim.isSettled());
    EXPECT_TRUE(anim.needsUpdate());
}

TEST_F(EaseAnimationTest, IsSettledCustomThreshold)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.9f;
    anim.target = 1.0f;

    EXPECT_FALSE(anim.isSettled(0.001f));
    EXPECT_TRUE(anim.isSettled(0.2f));
}

TEST_F(EaseAnimationTest, VeryLargeDeltaTimeIsClamped)
{
    SpringAnimation animHuge = SpringPresets::medium();
    animHuge.position = 0.0f;
    animHuge.setTarget(1.0f);
    animHuge.update(10.0f);

    SpringAnimation animClamped = SpringPresets::medium();
    animClamped.position = 0.0f;
    animClamped.setTarget(1.0f);
    animClamped.update(SpringAnimation::MAX_DELTA_TIME);

    EXPECT_TRUE(std::isfinite(animHuge.position));
    EXPECT_FLOAT_EQ(animHuge.position, animClamped.position);
}

TEST_F(EaseAnimationTest, NegativeTarget)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(-100.0f);

    for (int i = 0; i < 120; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(anim.position));
    }

    EXPECT_LT(anim.position, 0.0f);
}

TEST_F(EaseAnimationTest, VeryLargeTarget)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(1e6f);

    for (int i = 0; i < 30; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(anim.position));
    }

    EXPECT_GT(anim.position, 0.0f);
}

TEST_F(EaseAnimationTest, RapidUpdates)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    for (int i = 0; i < 1000; ++i)
        anim.update(0.0001f);

    EXPECT_TRUE(std::isfinite(anim.position));
}
