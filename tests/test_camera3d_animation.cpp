/*
    Oscil - Camera3D Animation Tests
    Tests for camera animation and smooth transitions
*/

#include <gtest/gtest.h>
#include "rendering/Camera3D.h"

using namespace oscil;

// =============================================================================
// Camera3D Animation Tests
// =============================================================================

class Camera3DAnimationTest : public ::testing::Test
{
protected:
    Camera3D camera;
};

TEST_F(Camera3DAnimationTest, AnimateTo)
{
    Vec3 newPos(10.0f, 10.0f, 10.0f);
    Vec3 newTarget(5.0f, 5.0f, 5.0f);

    camera.animateTo(newPos, newTarget, 1.0f);

    EXPECT_TRUE(camera.isAnimating());
}

TEST_F(Camera3DAnimationTest, IsAnimatingInitiallyFalse)
{
    EXPECT_FALSE(camera.isAnimating());
}

TEST_F(Camera3DAnimationTest, Update)
{
    camera.animateTo(Vec3(10.0f, 10.0f, 10.0f), Vec3(0.0f, 0.0f, 0.0f), 1.0f);

    // Update with partial progress
    camera.update(0.5f);

    // Should still be animating
    EXPECT_TRUE(camera.isAnimating());

    // Complete the animation
    camera.update(0.6f);

    // Animation should be complete
    EXPECT_FALSE(camera.isAnimating());
}

TEST_F(Camera3DAnimationTest, UpdateNoAnimation)
{
    // Should not crash when updating without animation
    camera.update(0.016f);
    EXPECT_FALSE(camera.isAnimating());
}
