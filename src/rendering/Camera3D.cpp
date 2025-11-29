/*
    Oscil - 3D Camera Implementation
*/

#include "rendering/Camera3D.h"
#include <algorithm>

namespace oscil
{

static constexpr float PI = 3.14159265358979323846f;
static constexpr float DEG_TO_RAD = PI / 180.0f;

Camera3D::Camera3D()
{
    updatePositionFromOrbit();
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera3D::setPosition(float x, float y, float z)
{
    position_ = {x, y, z};
    useOrbitMode_ = false;
    viewDirty_ = true;
}

void Camera3D::setTarget(float x, float y, float z)
{
    target_ = {x, y, z};
    viewDirty_ = true;
}

void Camera3D::setUp(float x, float y, float z)
{
    up_ = Vec3{x, y, z}.normalized();
    viewDirty_ = true;
}

void Camera3D::setProjection(CameraProjection projection)
{
    if (projection_ != projection)
    {
        projection_ = projection;
        projectionDirty_ = true;
    }
}

void Camera3D::setFOV(float fovDegrees)
{
    fov_ = std::clamp(fovDegrees, 10.0f, 120.0f);
    projectionDirty_ = true;
}

void Camera3D::setAspectRatio(float aspect)
{
    aspectRatio_ = std::max(0.1f, aspect);
    projectionDirty_ = true;
}

void Camera3D::setNearPlane(float near)
{
    nearPlane_ = std::max(0.001f, near);
    projectionDirty_ = true;
}

void Camera3D::setFarPlane(float far)
{
    farPlane_ = std::max(nearPlane_ + 0.1f, far);
    projectionDirty_ = true;
}

void Camera3D::setOrthoSize(float size)
{
    orthoSize_ = std::max(0.1f, size);
    projectionDirty_ = true;
}

void Camera3D::setOrbitDistance(float distance)
{
    orbitDistance_ = std::max(0.1f, distance);
    useOrbitMode_ = true;
    updatePositionFromOrbit();
    viewDirty_ = true;
}

void Camera3D::setOrbitYaw(float yawDegrees)
{
    orbitYaw_ = yawDegrees;
    useOrbitMode_ = true;
    updatePositionFromOrbit();
    viewDirty_ = true;
}

void Camera3D::setOrbitPitch(float pitchDegrees)
{
    orbitPitch_ = std::clamp(pitchDegrees, -89.0f, 89.0f);
    useOrbitMode_ = true;
    updatePositionFromOrbit();
    viewDirty_ = true;
}

void Camera3D::orbit(float deltaYaw, float deltaPitch)
{
    orbitYaw_ += deltaYaw;
    orbitPitch_ = std::clamp(orbitPitch_ + deltaPitch, -89.0f, 89.0f);
    useOrbitMode_ = true;
    updatePositionFromOrbit();
    viewDirty_ = true;
}

void Camera3D::zoom(float deltaDistance)
{
    orbitDistance_ = std::max(0.5f, orbitDistance_ + deltaDistance);
    if (useOrbitMode_)
    {
        updatePositionFromOrbit();
        viewDirty_ = true;
    }
}

void Camera3D::pan(float deltaX, float deltaY)
{
    // Calculate right and up vectors from view
    Vec3 forward = (target_ - position_).normalized();
    Vec3 right = Vec3::cross(forward, up_).normalized();
    Vec3 actualUp = Vec3::cross(right, forward);

    // Apply pan
    Vec3 offset = right * deltaX + actualUp * deltaY;
    position_ = position_ + offset;
    target_ = target_ + offset;

    viewDirty_ = true;
}

void Camera3D::reset()
{
    orbitDistance_ = 5.0f;
    orbitYaw_ = 0.0f;
    orbitPitch_ = 20.0f;
    target_ = {0, 0, 0};
    useOrbitMode_ = true;

    updatePositionFromOrbit();
    viewDirty_ = true;
}

void Camera3D::update(float deltaTime)
{
    // Handle animation
    if (isAnimating_)
    {
        animProgress_ += deltaTime / animDuration_;

        if (animProgress_ >= 1.0f)
        {
            animProgress_ = 1.0f;
            isAnimating_ = false;
            position_ = animTargetPosition_;
            target_ = animTargetTarget_;
        }
        else
        {
            // Smooth interpolation (ease out)
            float t = 1.0f - std::pow(1.0f - animProgress_, 3.0f);

            Vec3 startPos = position_;
            Vec3 startTarget = target_;

            position_.x = startPos.x + (animTargetPosition_.x - startPos.x) * t;
            position_.y = startPos.y + (animTargetPosition_.y - startPos.y) * t;
            position_.z = startPos.z + (animTargetPosition_.z - startPos.z) * t;

            target_.x = startTarget.x + (animTargetTarget_.x - startTarget.x) * t;
            target_.y = startTarget.y + (animTargetTarget_.y - startTarget.y) * t;
            target_.z = startTarget.z + (animTargetTarget_.z - startTarget.z) * t;
        }

        viewDirty_ = true;
    }

    // Update matrices if needed
    if (viewDirty_)
    {
        updateViewMatrix();
        viewDirty_ = false;
    }

    if (projectionDirty_)
    {
        updateProjectionMatrix();
        projectionDirty_ = false;
    }
}

void Camera3D::animateTo(const Vec3& position, const Vec3& target, float duration)
{
    animTargetPosition_ = position;
    animTargetTarget_ = target;
    animDuration_ = std::max(0.01f, duration);
    animProgress_ = 0.0f;
    isAnimating_ = true;
}

void Camera3D::updateViewMatrix()
{
    viewMatrix_ = Matrix4::lookAt(
        position_.x, position_.y, position_.z,
        target_.x, target_.y, target_.z,
        up_.x, up_.y, up_.z
    );
}

void Camera3D::updateProjectionMatrix()
{
    if (projection_ == CameraProjection::Perspective)
    {
        projectionMatrix_ = Matrix4::perspective(
            fov_ * DEG_TO_RAD,
            aspectRatio_,
            nearPlane_,
            farPlane_
        );
    }
    else
    {
        float halfHeight = orthoSize_ * 0.5f;
        float halfWidth = halfHeight * aspectRatio_;
        projectionMatrix_ = Matrix4::orthographic(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            nearPlane_, farPlane_
        );
    }
}

void Camera3D::updatePositionFromOrbit()
{
    float yawRad = orbitYaw_ * DEG_TO_RAD;
    float pitchRad = orbitPitch_ * DEG_TO_RAD;

    // Spherical to Cartesian
    float cosPitch = std::cos(pitchRad);
    position_.x = target_.x + orbitDistance_ * cosPitch * std::sin(yawRad);
    position_.y = target_.y + orbitDistance_ * std::sin(pitchRad);
    position_.z = target_.z + orbitDistance_ * cosPitch * std::cos(yawRad);
}

} // namespace oscil
