/*
    Oscil - Ease Animation Tests: Interaction
    Tests for user interaction patterns and interpolation
*/

#include "ui/components/SpringAnimation.h"

#include <gtest/gtest.h>

using namespace oscil;

class EaseInteractionTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: getNormalized returns correct value
TEST_F(EaseInteractionTest, GetNormalized)
{
    SpringAnimation anim;
    anim.position = 0.5f;
    anim.target = 1.0f;

    EXPECT_FLOAT_EQ(anim.getNormalized(), 0.5f);

    anim.position = 0.0f;
    EXPECT_FLOAT_EQ(anim.getNormalized(), 0.0f);

    anim.position = 1.0f;
    EXPECT_FLOAT_EQ(anim.getNormalized(), 1.0f);
}

// Test: getNormalized clamps values
TEST_F(EaseInteractionTest, GetNormalizedClamps)
{
    SpringAnimation anim = SpringPresets::medium();

    // Manually set position beyond 0-1 to test clamping
    anim.position = -0.5f;
    EXPECT_FLOAT_EQ(anim.getNormalized(), 0.0f);

    anim.position = 1.5f;
    EXPECT_FLOAT_EQ(anim.getNormalized(), 1.0f);
}

// Test: getNormalized stays in range during animation
TEST_F(EaseInteractionTest, GetNormalizedDuringAnimation)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    for (int i = 0; i < 100; ++i)
    {
        anim.update(1.0f / 60.0f);
        float normalized = anim.getNormalized();
        EXPECT_GE(normalized, 0.0f);
        EXPECT_LE(normalized, 1.0f);
    }
}

// Test: Interpolation at zero
TEST_F(EaseInteractionTest, InterpolateAtZero)
{
    SpringAnimation anim;
    anim.position = 0.0f;

    float result = anim.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: Interpolation at one
TEST_F(EaseInteractionTest, InterpolateAtOne)
{
    SpringAnimation anim;
    anim.position = 1.0f;

    float result = anim.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 200.0f);
}

// Test: Interpolation at middle
TEST_F(EaseInteractionTest, InterpolateAtMiddle)
{
    SpringAnimation anim;
    anim.position = 0.5f;

    float result = anim.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 150.0f);
}

// Test: Interpolation clamps negative
TEST_F(EaseInteractionTest, InterpolateClampsNegative)
{
    SpringAnimation anim;
    anim.position = -0.5f;

    float result = anim.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: Interpolation clamps over one
TEST_F(EaseInteractionTest, InterpolateClampsOverOne)
{
    SpringAnimation anim;
    anim.position = 1.5f;

    float result = anim.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 200.0f);
}

// Test: SetTarget with start position
TEST_F(EaseInteractionTest, SetTargetWithStartPosition)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.velocity = 100.0f;

    anim.setTarget(50.0f, 25.0f);

    EXPECT_FLOAT_EQ(anim.target, 50.0f);
    EXPECT_FLOAT_EQ(anim.position, 25.0f);
    EXPECT_FLOAT_EQ(anim.velocity, 0.0f);
}
