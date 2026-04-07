/*
    Oscil - Spring Mode Animation Tests
    Tests for spring physics mode: overshoot, settling, velocity tracking,
    and spring preset configurations. Existing ease-out tests live in
    test_spring_physics.cpp / test_spring_interaction.cpp / test_spring_edge.cpp.
*/

#include "ui/components/SpringAnimation.h"

#include <gtest/gtest.h>

using namespace oscil;

class SpringModeTest : public ::testing::Test
{
protected:
    /// Create a spring-mode animation with given stiffness/damping/mass
    static SpringAnimation makeSpring(float stiffness, float damping, float mass = 1.0f)
    {
        SpringAnimation s{stiffness, damping, mass};
        s.setMode(SpringMode::Spring);
        return s;
    }
};

// =============================================================================
// Spring mode: settling behavior
// =============================================================================

TEST_F(SpringModeTest, SettlesToTargetWithinReasonableIterations)
{
    auto spring = SpringPresets::springSwitch();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // 600 frames at 60fps = 10 seconds, more than enough for any spring
    for (int i = 0; i < 600; ++i)
        spring.update(1.0f / 60.0f);

    EXPECT_NEAR(spring.position, 1.0f, 0.001f);
    EXPECT_TRUE(spring.isSettled());
}

TEST_F(SpringModeTest, SettlesToNegativeTarget)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 0.0f;
    spring.setTarget(-5.0f);

    for (int i = 0; i < 600; ++i)
        spring.update(1.0f / 60.0f);

    EXPECT_NEAR(spring.position, -5.0f, 0.001f);
}

// =============================================================================
// Overshoot behavior: underdamped springs MUST overshoot
// =============================================================================

TEST_F(SpringModeTest, UnderdampedSpringOvershoots)
{
    // High stiffness + low damping = underdamped = overshoot expected
    auto spring = makeSpring(700.0f, 10.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    bool overshot = false;
    for (int i = 0; i < 600; ++i)
    {
        spring.update(1.0f / 60.0f);
        if (spring.position > 1.0f)
        {
            overshot = true;
            break;
        }
    }

    EXPECT_TRUE(overshot) << "Underdamped spring (stiff=700, damp=10) should overshoot target";
}

TEST_F(SpringModeTest, EaseOutModeDoesNotOvershoot)
{
    auto anim = SpringPresets::fast();
    // EaseOut is the default mode for fast()
    ASSERT_EQ(anim.mode, SpringMode::EaseOut);

    anim.position = 0.0f;
    anim.setTarget(1.0f);

    for (int i = 0; i < 300; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_LE(anim.position, 1.0f + 0.0001f) << "EaseOut overshot at frame " << i;
    }
}

// =============================================================================
// isSettled accounts for velocity in Spring mode
// =============================================================================

TEST_F(SpringModeTest, IsSettledRequiresLowVelocity)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 1.0f;
    spring.target = 1.0f;
    spring.velocity = 5.0f; // High velocity even though position == target

    EXPECT_FALSE(spring.isSettled()) << "Spring with high velocity should not be settled even when position == target";
}

TEST_F(SpringModeTest, IsSettledTrueWhenPositionAndVelocityNearZero)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 1.0f;
    spring.target = 1.0f;
    spring.velocity = 0.0f;

    EXPECT_TRUE(spring.isSettled());
}

TEST_F(SpringModeTest, EaseOutIgnoresVelocityForSettled)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 1.0f;
    anim.target = 1.0f;
    anim.velocity = 999.0f; // EaseOut doesn't use velocity for settling

    EXPECT_TRUE(anim.isSettled());
}

// =============================================================================
// Spring presets produce Spring mode
// =============================================================================

TEST_F(SpringModeTest, SpringSwitchPresetUsesSpringMode)
{
    auto s = SpringPresets::springSwitch();
    EXPECT_EQ(s.mode, SpringMode::Spring);
    EXPECT_NEAR(s.stiffness, 700.0f, 0.01f);
    EXPECT_NEAR(s.damping, 30.0f, 0.01f);
}

TEST_F(SpringModeTest, SpringIndicatorPresetUsesSpringMode)
{
    auto s = SpringPresets::springIndicator();
    EXPECT_EQ(s.mode, SpringMode::Spring);
    EXPECT_NEAR(s.stiffness, 500.0f, 0.01f);
    EXPECT_NEAR(s.damping, 30.0f, 0.01f);
}

TEST_F(SpringModeTest, SpringPopupPresetUsesSpringMode)
{
    auto s = SpringPresets::springPopup();
    EXPECT_EQ(s.mode, SpringMode::Spring);
    EXPECT_NEAR(s.stiffness, 400.0f, 0.01f);
    EXPECT_NEAR(s.damping, 25.0f, 0.01f);
}

TEST_F(SpringModeTest, SpringGentlePresetUsesSpringMode)
{
    auto s = SpringPresets::springGentle();
    EXPECT_EQ(s.mode, SpringMode::Spring);
    EXPECT_NEAR(s.stiffness, 300.0f, 0.01f);
    EXPECT_NEAR(s.damping, 20.0f, 0.01f);
}

// =============================================================================
// EaseOut presets use EaseOut mode
// =============================================================================

TEST_F(SpringModeTest, EaseOutPresetsUseEaseOutMode)
{
    EXPECT_EQ(SpringPresets::fast().mode, SpringMode::EaseOut);
    EXPECT_EQ(SpringPresets::medium().mode, SpringMode::EaseOut);
    EXPECT_EQ(SpringPresets::slow().mode, SpringMode::EaseOut);
}

// =============================================================================
// setMode switches correctly
// =============================================================================

TEST_F(SpringModeTest, SetModeSwitchesToSpring)
{
    SpringAnimation anim = SpringPresets::medium();
    EXPECT_EQ(anim.mode, SpringMode::EaseOut);

    anim.setMode(SpringMode::Spring);
    EXPECT_EQ(anim.mode, SpringMode::Spring);
}

TEST_F(SpringModeTest, SetModeSwitchesBackToEaseOut)
{
    auto spring = SpringPresets::springSwitch();
    EXPECT_EQ(spring.mode, SpringMode::Spring);

    spring.setMode(SpringMode::EaseOut);
    EXPECT_EQ(spring.mode, SpringMode::EaseOut);
}

// =============================================================================
// Spring physics: velocity drives motion correctly
// =============================================================================

TEST_F(SpringModeTest, SpringMovesTowardTarget)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 0.0f;
    spring.setTarget(10.0f);

    spring.update(1.0f / 60.0f);

    EXPECT_GT(spring.position, 0.0f) << "Spring should move toward target on first update";
    EXPECT_GT(spring.velocity, 0.0f) << "Spring should have positive velocity when moving toward positive target";
}

TEST_F(SpringModeTest, SpringVelocityDecaysToZeroAtSettling)
{
    auto spring = SpringPresets::springIndicator();
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    for (int i = 0; i < 600; ++i)
        spring.update(1.0f / 60.0f);

    EXPECT_NEAR(spring.velocity, 0.0f, 0.01f);
}

// =============================================================================
// Numerical stability
// =============================================================================

TEST_F(SpringModeTest, StableWithLargeDeltaTime)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    // Large delta clamped to MAX_DELTA_TIME
    spring.update(100.0f);

    EXPECT_TRUE(std::isfinite(spring.position));
    EXPECT_TRUE(std::isfinite(spring.velocity));
}

TEST_F(SpringModeTest, StableWithZeroDeltaTime)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 0.5f;
    spring.setTarget(1.0f);

    float posBefore = spring.position;
    spring.update(0.0f);

    EXPECT_FLOAT_EQ(spring.position, posBefore);
}

TEST_F(SpringModeTest, StableWithNegativeDeltaTime)
{
    auto spring = makeSpring(500.0f, 30.0f);
    spring.position = 0.5f;
    spring.setTarget(1.0f);

    float posBefore = spring.position;
    spring.update(-1.0f);

    EXPECT_FLOAT_EQ(spring.position, posBefore);
}

// Semi-implicit Euler is conditionally stable: stiffness must be bounded
// relative to timestep. The highest preset is springSwitch at 700.
// Test the upper range that real presets might use (1500).
TEST_F(SpringModeTest, StableWithHighStiffness)
{
    auto spring = makeSpring(1500.0f, 60.0f);
    spring.position = 0.0f;
    spring.setTarget(1.0f);

    for (int i = 0; i < 600; ++i)
    {
        spring.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(spring.position)) << "Diverged at frame " << i;
        EXPECT_TRUE(std::isfinite(spring.velocity)) << "Velocity diverged at frame " << i;
    }

    EXPECT_NEAR(spring.position, 1.0f, 0.01f);
}

// =============================================================================
// All spring presets settle (regression test)
// =============================================================================

TEST_F(SpringModeTest, AllSpringPresetsSettle)
{
    auto testPreset = [](SpringAnimation spring, const char* name) {
        spring.position = 0.0f;
        spring.setTarget(1.0f);

        for (int i = 0; i < 600; ++i)
            spring.update(1.0f / 60.0f);

        EXPECT_NEAR(spring.position, 1.0f, 0.001f) << "Preset '" << name << "' did not settle to target";
        EXPECT_TRUE(spring.isSettled()) << "Preset '" << name << "' reports not settled";
    };

    testPreset(SpringPresets::springSwitch(), "springSwitch");
    testPreset(SpringPresets::springIndicator(), "springIndicator");
    testPreset(SpringPresets::springPopup(), "springPopup");
    testPreset(SpringPresets::springGentle(), "springGentle");
}

// =============================================================================
// Higher stiffness spring reaches target faster
// =============================================================================

TEST_F(SpringModeTest, HigherStiffnessSettlesFaster)
{
    auto fast = makeSpring(700.0f, 30.0f);
    auto slow = makeSpring(200.0f, 30.0f);
    fast.position = 0.0f;
    fast.setTarget(1.0f);
    slow.position = 0.0f;
    slow.setTarget(1.0f);

    // After a few frames the higher-stiffness spring should be closer to target
    for (int i = 0; i < 10; ++i)
    {
        fast.update(1.0f / 60.0f);
        slow.update(1.0f / 60.0f);
    }

    EXPECT_GT(fast.position, slow.position) << "Higher stiffness spring should be closer to target after 10 frames";
}
