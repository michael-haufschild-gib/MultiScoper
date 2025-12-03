/*
    Oscil - Camera3D Interaction Tests
    Tests for camera user interaction, controls, and transformations
*/

#include <gtest/gtest.h>
#include "rendering/Camera3D.h"
#include <cmath>

using namespace oscil;

// =============================================================================
// Camera3D Interaction Tests
// =============================================================================

class Camera3DInteractionTest : public ::testing::Test
{
protected:
    Camera3D camera;
};

TEST_F(Camera3DInteractionTest, DefaultValues)
{
    Vec3 pos = camera.getPosition();
    Vec3 target = camera.getTarget();
    Vec3 up = camera.getUp();

    // Default position is calculated from orbit params (distance=5, yaw=0, pitch=20)
    // Position should be roughly at distance 5 from origin
    float distFromTarget = pos.length();
    EXPECT_NEAR(distFromTarget, 5.0f, 0.1f);

    // Default target is origin
    EXPECT_FLOAT_EQ(target.x, 0.0f);
    EXPECT_FLOAT_EQ(target.y, 0.0f);
    EXPECT_FLOAT_EQ(target.z, 0.0f);

    // Default up is Y
    EXPECT_FLOAT_EQ(up.x, 0.0f);
    EXPECT_FLOAT_EQ(up.y, 1.0f);
    EXPECT_FLOAT_EQ(up.z, 0.0f);

    // Default projection is perspective
    EXPECT_EQ(camera.getProjection(), CameraProjection::Perspective);

    // Default orbit distance is 5
    EXPECT_FLOAT_EQ(camera.getOrbitDistance(), 5.0f);
}

TEST_F(Camera3DInteractionTest, SetPosition)
{
    camera.setPosition(1.0f, 2.0f, 3.0f);
    Vec3 pos = camera.getPosition();

    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
}

TEST_F(Camera3DInteractionTest, SetPositionVec3)
{
    camera.setPosition(Vec3(4.0f, 5.0f, 6.0f));
    Vec3 pos = camera.getPosition();

    EXPECT_FLOAT_EQ(pos.x, 4.0f);
    EXPECT_FLOAT_EQ(pos.y, 5.0f);
    EXPECT_FLOAT_EQ(pos.z, 6.0f);
}

TEST_F(Camera3DInteractionTest, SetTarget)
{
    camera.setTarget(1.0f, 2.0f, 3.0f);
    Vec3 target = camera.getTarget();

    EXPECT_FLOAT_EQ(target.x, 1.0f);
    EXPECT_FLOAT_EQ(target.y, 2.0f);
    EXPECT_FLOAT_EQ(target.z, 3.0f);
}

TEST_F(Camera3DInteractionTest, SetUp)
{
    camera.setUp(1.0f, 0.0f, 0.0f);
    Vec3 up = camera.getUp();

    EXPECT_FLOAT_EQ(up.x, 1.0f);
    EXPECT_FLOAT_EQ(up.y, 0.0f);
    EXPECT_FLOAT_EQ(up.z, 0.0f);
}

TEST_F(Camera3DInteractionTest, SetProjection)
{
    camera.setProjection(CameraProjection::Orthographic);
    EXPECT_EQ(camera.getProjection(), CameraProjection::Orthographic);

    camera.setProjection(CameraProjection::Perspective);
    EXPECT_EQ(camera.getProjection(), CameraProjection::Perspective);
}

TEST_F(Camera3DInteractionTest, SetFOV)
{
    camera.setFOV(60.0f);
    EXPECT_FLOAT_EQ(camera.getFOV(), 60.0f);
}

TEST_F(Camera3DInteractionTest, SetAspectRatio)
{
    camera.setAspectRatio(16.0f / 9.0f);
    EXPECT_NEAR(camera.getAspectRatio(), 1.777f, 0.01f);
}

TEST_F(Camera3DInteractionTest, SetNearFarPlanes)
{
    camera.setNearPlane(0.5f);
    camera.setFarPlane(500.0f);

    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.5f);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 500.0f);
}

TEST_F(Camera3DInteractionTest, SetOrthoSize)
{
    camera.setOrthoSize(10.0f);
    EXPECT_FLOAT_EQ(camera.getOrthoSize(), 10.0f);
}

TEST_F(Camera3DInteractionTest, OrbitControls)
{
    camera.setOrbitDistance(10.0f);
    camera.setOrbitYaw(45.0f);
    camera.setOrbitPitch(30.0f);

    EXPECT_FLOAT_EQ(camera.getOrbitDistance(), 10.0f);
    EXPECT_FLOAT_EQ(camera.getOrbitYaw(), 45.0f);
    EXPECT_FLOAT_EQ(camera.getOrbitPitch(), 30.0f);
}

TEST_F(Camera3DInteractionTest, Orbit)
{
    float initialYaw = camera.getOrbitYaw();
    float initialPitch = camera.getOrbitPitch();

    camera.orbit(10.0f, 5.0f);

    EXPECT_FLOAT_EQ(camera.getOrbitYaw(), initialYaw + 10.0f);
    EXPECT_FLOAT_EQ(camera.getOrbitPitch(), initialPitch + 5.0f);
}

TEST_F(Camera3DInteractionTest, Zoom)
{
    float initialDistance = camera.getOrbitDistance();

    camera.zoom(2.0f);

    EXPECT_FLOAT_EQ(camera.getOrbitDistance(), initialDistance + 2.0f);
}

TEST_F(Camera3DInteractionTest, ZoomNegative)
{
    float initialDistance = camera.getOrbitDistance();

    camera.zoom(-1.0f);

    EXPECT_FLOAT_EQ(camera.getOrbitDistance(), initialDistance - 1.0f);
}

TEST_F(Camera3DInteractionTest, Pan)
{
    Vec3 initialTarget = camera.getTarget();

    camera.pan(1.0f, 1.0f);

    // Target should have moved
    Vec3 newTarget = camera.getTarget();
    EXPECT_NE(newTarget.x, initialTarget.x);
}

TEST_F(Camera3DInteractionTest, Reset)
{
    // Modify camera
    camera.setOrbitDistance(20.0f);
    camera.setOrbitYaw(90.0f);
    camera.setTarget(10.0f, 10.0f, 10.0f);

    camera.reset();

    // Should be back to defaults
    EXPECT_FLOAT_EQ(camera.getOrbitDistance(), 5.0f);
    EXPECT_FLOAT_EQ(camera.getOrbitYaw(), 0.0f);

    Vec3 target = camera.getTarget();
    EXPECT_FLOAT_EQ(target.x, 0.0f);
    EXPECT_FLOAT_EQ(target.y, 0.0f);
    EXPECT_FLOAT_EQ(target.z, 0.0f);
}

TEST_F(Camera3DInteractionTest, GetViewMatrix)
{
    const Matrix4& view = camera.getViewMatrix();

    // View matrix should be valid (not all zeros)
    bool hasNonZero = false;
    for (int i = 0; i < 16; ++i)
    {
        if (view.m[static_cast<size_t>(i)] != 0.0f)
            hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(Camera3DInteractionTest, GetProjectionMatrix)
{
    const Matrix4& proj = camera.getProjectionMatrix();

    // Projection matrix should be valid
    bool hasNonZero = false;
    for (int i = 0; i < 16; ++i)
    {
        if (proj.m[static_cast<size_t>(i)] != 0.0f)
            hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(Camera3DInteractionTest, GetViewProjectionMatrix)
{
    Matrix4 vp = camera.getViewProjectionMatrix();

    // VP matrix should be valid
    bool hasNonZero = false;
    for (int i = 0; i < 16; ++i)
    {
        if (vp.m[static_cast<size_t>(i)] != 0.0f)
            hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}
