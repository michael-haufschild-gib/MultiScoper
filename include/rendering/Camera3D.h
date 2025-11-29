/*
    Oscil - 3D Camera
    Camera system for 3D waveform visualization
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

namespace oscil
{

/**
 * 4x4 matrix for 3D transformations.
 * Column-major order for OpenGL compatibility.
 */
struct Matrix4
{
    std::array<float, 16> m;

    Matrix4()
    {
        identity();
    }

    void identity()
    {
        m = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    float& operator()(int row, int col) { return m[static_cast<size_t>(col * 4 + row)]; }
    float operator()(int row, int col) const { return m[static_cast<size_t>(col * 4 + row)]; }

    const float* data() const { return m.data(); }

    Matrix4 operator*(const Matrix4& other) const
    {
        Matrix4 result;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                result(row, col) = 0;
                for (int k = 0; k < 4; ++k)
                {
                    result(row, col) += (*this)(row, k) * other(k, col);
                }
            }
        }
        return result;
    }

    static Matrix4 translation(float x, float y, float z)
    {
        Matrix4 m;
        m(0, 3) = x;
        m(1, 3) = y;
        m(2, 3) = z;
        return m;
    }

    static Matrix4 scale(float x, float y, float z)
    {
        Matrix4 m;
        m(0, 0) = x;
        m(1, 1) = y;
        m(2, 2) = z;
        return m;
    }

    static Matrix4 rotationX(float radians)
    {
        Matrix4 m;
        float c = std::cos(radians);
        float s = std::sin(radians);
        m(1, 1) = c;  m(1, 2) = -s;
        m(2, 1) = s;  m(2, 2) = c;
        return m;
    }

    static Matrix4 rotationY(float radians)
    {
        Matrix4 m;
        float c = std::cos(radians);
        float s = std::sin(radians);
        m(0, 0) = c;  m(0, 2) = s;
        m(2, 0) = -s; m(2, 2) = c;
        return m;
    }

    static Matrix4 rotationZ(float radians)
    {
        Matrix4 m;
        float c = std::cos(radians);
        float s = std::sin(radians);
        m(0, 0) = c;  m(0, 1) = -s;
        m(1, 0) = s;  m(1, 1) = c;
        return m;
    }

    static Matrix4 perspective(float fovY, float aspect, float nearPlane, float farPlane)
    {
        Matrix4 m;
        m.m.fill(0);

        float tanHalfFov = std::tan(fovY * 0.5f);
        m(0, 0) = 1.0f / (aspect * tanHalfFov);
        m(1, 1) = 1.0f / tanHalfFov;
        m(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        m(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        m(3, 2) = -1.0f;

        return m;
    }

    static Matrix4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        Matrix4 m;
        m(0, 0) = 2.0f / (right - left);
        m(1, 1) = 2.0f / (top - bottom);
        m(2, 2) = -2.0f / (farPlane - nearPlane);
        m(0, 3) = -(right + left) / (right - left);
        m(1, 3) = -(top + bottom) / (top - bottom);
        m(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        return m;
    }

    static Matrix4 lookAt(float eyeX, float eyeY, float eyeZ,
                          float targetX, float targetY, float targetZ,
                          float upX, float upY, float upZ)
    {
        // Calculate forward, right, up vectors
        float fx = targetX - eyeX;
        float fy = targetY - eyeY;
        float fz = targetZ - eyeZ;

        // Normalize forward
        float fLen = std::sqrt(fx*fx + fy*fy + fz*fz);
        if (fLen > 0) { fx /= fLen; fy /= fLen; fz /= fLen; }

        // Right = forward x up
        float rx = fy * upZ - fz * upY;
        float ry = fz * upX - fx * upZ;
        float rz = fx * upY - fy * upX;

        // Normalize right
        float rLen = std::sqrt(rx*rx + ry*ry + rz*rz);
        if (rLen > 0) { rx /= rLen; ry /= rLen; rz /= rLen; }

        // Recalculate up = right x forward
        float ux = ry * fz - rz * fy;
        float uy = rz * fx - rx * fz;
        float uz = rx * fy - ry * fx;

        Matrix4 m;
        m(0, 0) = rx;  m(0, 1) = ry;  m(0, 2) = rz;  m(0, 3) = -(rx*eyeX + ry*eyeY + rz*eyeZ);
        m(1, 0) = ux;  m(1, 1) = uy;  m(1, 2) = uz;  m(1, 3) = -(ux*eyeX + uy*eyeY + uz*eyeZ);
        m(2, 0) = -fx; m(2, 1) = -fy; m(2, 2) = -fz; m(2, 3) = (fx*eyeX + fy*eyeY + fz*eyeZ);
        m(3, 0) = 0;   m(3, 1) = 0;   m(3, 2) = 0;   m(3, 3) = 1;

        return m;
    }
};

/**
 * 3D vector.
 */
struct Vec3
{
    float x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }

    float length() const { return std::sqrt(x*x + y*y + z*z); }

    Vec3 normalized() const
    {
        float len = length();
        return len > 0 ? Vec3{x/len, y/len, z/len} : Vec3{};
    }

    static float dot(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

/**
 * Camera projection mode.
 */
enum class CameraProjection
{
    Perspective,
    Orthographic
};

/**
 * 3D camera for waveform visualization.
 * Supports orbit, pan, zoom controls.
 */
class Camera3D
{
public:
    Camera3D();

    // Position and orientation
    void setPosition(float x, float y, float z);
    void setPosition(const Vec3& pos) { setPosition(pos.x, pos.y, pos.z); }
    [[nodiscard]] Vec3 getPosition() const { return position_; }

    void setTarget(float x, float y, float z);
    void setTarget(const Vec3& target) { setTarget(target.x, target.y, target.z); }
    [[nodiscard]] Vec3 getTarget() const { return target_; }

    void setUp(float x, float y, float z);
    [[nodiscard]] Vec3 getUp() const { return up_; }

    // Projection settings
    void setProjection(CameraProjection projection);
    [[nodiscard]] CameraProjection getProjection() const { return projection_; }

    void setFOV(float fovDegrees);
    [[nodiscard]] float getFOV() const { return fov_; }

    void setAspectRatio(float aspect);
    [[nodiscard]] float getAspectRatio() const { return aspectRatio_; }

    void setNearPlane(float near);
    void setFarPlane(float far);
    [[nodiscard]] float getNearPlane() const { return nearPlane_; }
    [[nodiscard]] float getFarPlane() const { return farPlane_; }

    // Orthographic specific
    void setOrthoSize(float size);
    [[nodiscard]] float getOrthoSize() const { return orthoSize_; }

    // Orbit controls (spherical coordinates around target)
    void setOrbitDistance(float distance);
    void setOrbitYaw(float yawDegrees);
    void setOrbitPitch(float pitchDegrees);

    [[nodiscard]] float getOrbitDistance() const { return orbitDistance_; }
    [[nodiscard]] float getOrbitYaw() const { return orbitYaw_; }
    [[nodiscard]] float getOrbitPitch() const { return orbitPitch_; }

    /**
     * Orbit camera around target.
     * @param deltaYaw Yaw change in degrees
     * @param deltaPitch Pitch change in degrees
     */
    void orbit(float deltaYaw, float deltaPitch);

    /**
     * Zoom camera (change orbit distance).
     * @param deltaDistance Distance change (positive = zoom out)
     */
    void zoom(float deltaDistance);

    /**
     * Pan camera and target together.
     * @param deltaX Horizontal pan
     * @param deltaY Vertical pan
     */
    void pan(float deltaX, float deltaY);

    /**
     * Reset camera to default position.
     */
    void reset();

    /**
     * Animate camera smoothly.
     * @param deltaTime Time step in seconds
     */
    void update(float deltaTime);

    // Matrix getters
    [[nodiscard]] const Matrix4& getViewMatrix() const { return viewMatrix_; }
    [[nodiscard]] const Matrix4& getProjectionMatrix() const { return projectionMatrix_; }
    [[nodiscard]] Matrix4 getViewProjectionMatrix() const { return projectionMatrix_ * viewMatrix_; }

    // Animation
    void setAnimationSpeed(float speed) { animationSpeed_ = speed; }
    void animateTo(const Vec3& position, const Vec3& target, float duration = 1.0f);
    [[nodiscard]] bool isAnimating() const { return isAnimating_; }

private:
    void updateViewMatrix();
    void updateProjectionMatrix();
    void updatePositionFromOrbit();

    // Position and orientation
    Vec3 position_{0, 0, 5};
    Vec3 target_{0, 0, 0};
    Vec3 up_{0, 1, 0};

    // Orbit controls
    float orbitDistance_ = 5.0f;
    float orbitYaw_ = 0.0f;      // Degrees
    float orbitPitch_ = 20.0f;   // Degrees
    bool useOrbitMode_ = true;

    // Projection
    CameraProjection projection_ = CameraProjection::Perspective;
    float fov_ = 45.0f;          // Degrees
    float aspectRatio_ = 1.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 100.0f;
    float orthoSize_ = 5.0f;

    // Matrices (cached)
    Matrix4 viewMatrix_;
    Matrix4 projectionMatrix_;
    bool viewDirty_ = true;
    bool projectionDirty_ = true;

    // Animation
    float animationSpeed_ = 5.0f;
    bool isAnimating_ = false;
    Vec3 animTargetPosition_;
    Vec3 animTargetTarget_;
    float animDuration_ = 0.0f;
    float animProgress_ = 0.0f;
};

} // namespace oscil
