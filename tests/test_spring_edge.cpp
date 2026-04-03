/*
    Oscil - Ease Animation Tests: Edge Cases
    Tests for numerical stability, extreme values, and SpringAnimationGroup
*/

#include "ui/components/SpringAnimation.h"

#include <gtest/gtest.h>

using namespace oscil;

class EaseEdgeTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Extreme high stiffness (very fast animation)
TEST_F(EaseEdgeTest, ExtremeHighStiffness)
{
    SpringAnimation anim(2000.0f, 0.0f, 1.0f);
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    for (int i = 0; i < 10; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(anim.position));
    }

    EXPECT_NEAR(anim.position, 1.0f, 0.1f);
}

// Test: Extreme low stiffness (very slow animation)
TEST_F(EaseEdgeTest, ExtremeLowStiffness)
{
    SpringAnimation anim(0.01f, 0.0f, 1.0f);
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    for (int i = 0; i < 60; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(anim.position));
    }

    // Very slow — should have barely moved
    EXPECT_LT(anim.position, 1.0f);
}

// Test: Zero stiffness (no movement)
TEST_F(EaseEdgeTest, ZeroStiffness)
{
    SpringAnimation anim(0.0f, 0.0f, 1.0f);
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    anim.update(1.0f / 60.0f);

    EXPECT_TRUE(std::isfinite(anim.position));
    // With zero stiffness, speed is 0 — no movement
    EXPECT_FLOAT_EQ(anim.position, 0.0f);
}

// Test: Very small delta time
TEST_F(EaseEdgeTest, VerySmallDeltaTime)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(1.0f);

    float initialPosition = anim.position;

    anim.update(1e-10f);

    EXPECT_TRUE(std::isfinite(anim.position));
    // Tiny delta should barely move
    EXPECT_NEAR(anim.position, initialPosition, 1e-4f);
}

// Test: Very large delta time is clamped
TEST_F(EaseEdgeTest, VeryLargeDeltaTimeIsClamped)
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

// Test: Very large target value
TEST_F(EaseEdgeTest, VeryLargeTarget)
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

// Test: Negative target
TEST_F(EaseEdgeTest, NegativeTarget)
{
    SpringAnimation anim = SpringPresets::medium();
    anim.position = 0.0f;
    anim.setTarget(-100.0f);

    for (int i = 0; i < 60; ++i)
    {
        anim.update(1.0f / 60.0f);
        EXPECT_TRUE(std::isfinite(anim.position));
    }

    EXPECT_LT(anim.position, 0.0f);
}

// Test: Position never overshoots (monotonic for all stiffness)
TEST_F(EaseEdgeTest, MonotonicForAllStiffness)
{
    for (float stiffness : {10.0f, 100.0f, 500.0f, 1000.0f, 2000.0f})
    {
        SpringAnimation anim(stiffness, 0.0f, 1.0f);
        anim.position = 0.0f;
        anim.setTarget(1.0f);

        float prev = 0.0f;
        for (int i = 0; i < 120; ++i)
        {
            anim.update(1.0f / 60.0f);
            EXPECT_GE(anim.position, prev) << "Overshoot with stiffness=" << stiffness << " at frame " << i;
            EXPECT_LE(anim.position, 1.0f + 0.001f) << "Exceeded target with stiffness=" << stiffness;
            prev = anim.position;
        }
    }
}

//==============================================================================
// SpringAnimationGroup Tests
//==============================================================================

class EaseAnimationGroupTest : public ::testing::Test
{
    SpringAnimationGroup group;

public:
    EaseAnimationGroupTest() = default;

protected:
    SpringAnimationGroup& getGroup() { return group; }
};

TEST_F(EaseAnimationGroupTest, AddAndGet)
{
    getGroup().add("test", SpringPresets::medium());

    SpringAnimation* anim = getGroup().get("test");
    ASSERT_NE(anim, nullptr);
    EXPECT_EQ(anim->stiffness, SpringPresets::medium().stiffness);
}

TEST_F(EaseAnimationGroupTest, GetNonExistent)
{
    SpringAnimation* anim = getGroup().get("nonexistent");
    EXPECT_EQ(anim, nullptr);
}

TEST_F(EaseAnimationGroupTest, GetConst)
{
    getGroup().add("test", SpringPresets::medium());

    const SpringAnimationGroup& constGroup = getGroup();
    const SpringAnimation* anim = constGroup.get("test");

    ASSERT_NE(anim, nullptr);
    EXPECT_EQ(anim->stiffness, SpringPresets::medium().stiffness);
}

TEST_F(EaseAnimationGroupTest, GetConstNonExistent)
{
    const SpringAnimationGroup& constGroup = getGroup();
    const SpringAnimation* anim = constGroup.get("nonexistent");
    EXPECT_EQ(anim, nullptr);
}

TEST_F(EaseAnimationGroupTest, UpdateAll)
{
    auto& anim1 = getGroup().add("anim1", SpringPresets::medium());
    auto& anim2 = getGroup().add("anim2", SpringPresets::fast());

    anim1.setTarget(1.0f);
    anim2.setTarget(1.0f);

    getGroup().updateAll(1.0f / 60.0f);

    EXPECT_GT(anim1.position, 0.0f);
    EXPECT_GT(anim2.position, 0.0f);
}

TEST_F(EaseAnimationGroupTest, AnyNeedsUpdate)
{
    auto& anim1 = getGroup().add("anim1", SpringPresets::medium());
    auto& anim2 = getGroup().add("anim2", SpringPresets::medium());

    anim1.setTarget(1.0f);
    anim1.snapToTarget();
    anim2.setTarget(1.0f);
    anim2.snapToTarget();

    EXPECT_FALSE(getGroup().anyNeedsUpdate());

    anim1.setTarget(2.0f);
    EXPECT_TRUE(getGroup().anyNeedsUpdate());
}

TEST_F(EaseAnimationGroupTest, SnapAllToTarget)
{
    auto& anim1 = getGroup().add("anim1", SpringPresets::medium());
    auto& anim2 = getGroup().add("anim2", SpringPresets::medium());

    anim1.setTarget(5.0f);
    anim2.setTarget(10.0f);

    getGroup().snapAllToTarget();

    EXPECT_FLOAT_EQ(anim1.position, 5.0f);
    EXPECT_FLOAT_EQ(anim2.position, 10.0f);
}

TEST_F(EaseAnimationGroupTest, AddReturnsReference)
{
    SpringAnimation& anim = getGroup().add("test", SpringPresets::medium());
    anim.setTarget(42.0f);

    SpringAnimation* retrieved = getGroup().get("test");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_FLOAT_EQ(retrieved->target, 42.0f);
}

TEST_F(EaseAnimationGroupTest, ManyAnimations)
{
    for (int i = 0; i < 50; ++i)
        getGroup().add("anim" + std::to_string(i), SpringPresets::medium());

    for (int i = 0; i < 50; ++i)
    {
        auto* anim = getGroup().get("anim" + std::to_string(i));
        ASSERT_NE(anim, nullptr);
        EXPECT_EQ(anim->stiffness, SpringPresets::medium().stiffness);
    }
}

TEST_F(EaseAnimationGroupTest, DefaultPreset)
{
    auto& anim = getGroup().add("test");
    EXPECT_EQ(anim.stiffness, SpringPresets::medium().stiffness);
}
