/*
    Oscil - Spring Animation Tests: Interaction
    Tests for user interaction, velocity injection, and interpolation
*/

#include <gtest/gtest.h>
#include "ui/components/SpringAnimation.h"

using namespace oscil;

class SpringInteractionTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: getNormalized returns correct value
TEST_F(SpringInteractionTest, GetNormalized)
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
TEST_F(SpringInteractionTest, GetNormalizedClamps)
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

// Test: Impulse with extreme values
TEST_F(SpringInteractionTest, ImpulseExtreme)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.position = 0.5f;
    spring.target = 0.5f;
    spring.velocity = 0.0f;

    spring.impulse(1000.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 1000.0f);

    spring.update(1.0f / 60.0f);
    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));
}

// Test: Impulse with negative value
TEST_F(SpringInteractionTest, ImpulseNegative)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.velocity = 10.0f;

    spring.impulse(-5.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 5.0f);
}

// Test: Interpolation at zero
TEST_F(SpringInteractionTest, InterpolateAtZero)
{
    SpringAnimation spring;
    spring.position = 0.0f;

    float result = spring.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: Interpolation at one
TEST_F(SpringInteractionTest, InterpolateAtOne)
{
    SpringAnimation spring;
    spring.position = 1.0f;

    float result = spring.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 200.0f);
}

// Test: Interpolation at middle
TEST_F(SpringInteractionTest, InterpolateAtMiddle)
{
    SpringAnimation spring;
    spring.position = 0.5f;

    float result = spring.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 150.0f);
}

// Test: Interpolation clamps negative
TEST_F(SpringInteractionTest, InterpolateClampsNegative)
{
    SpringAnimation spring;
    spring.position = -0.5f;

    float result = spring.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

// Test: Interpolation clamps over one
TEST_F(SpringInteractionTest, InterpolateClampsOverOne)
{
    SpringAnimation spring;
    spring.position = 1.5f;

    float result = spring.interpolate(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(result, 200.0f);
}

// Test: SetTarget with start position
TEST_F(SpringInteractionTest, SetTargetWithStartPosition)
{
    SpringAnimation spring = SpringPresets::snappy();
    spring.velocity = 100.0f; // Some initial velocity

    spring.setTarget(50.0f, 25.0f);

    EXPECT_FLOAT_EQ(spring.target, 50.0f);
    EXPECT_FLOAT_EQ(spring.position, 25.0f);
    EXPECT_FLOAT_EQ(spring.velocity, 0.0f); // Velocity reset
}
