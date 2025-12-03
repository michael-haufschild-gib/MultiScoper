/*
    Oscil - Camera3D Math Tests
    Tests for Vec3 and Matrix4 mathematical operations
*/

#include <gtest/gtest.h>
#include "rendering/Camera3D.h"
#include <cmath>

using namespace oscil;

// =============================================================================
// Vec3 Tests
// =============================================================================

class Vec3Test : public ::testing::Test
{
};

TEST_F(Vec3Test, DefaultConstruction)
{
    Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST_F(Vec3Test, ParameterizedConstruction)
{
    Vec3 v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST_F(Vec3Test, Addition)
{
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    Vec3 result = a + b;

    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST_F(Vec3Test, Subtraction)
{
    Vec3 a(4.0f, 5.0f, 6.0f);
    Vec3 b(1.0f, 2.0f, 3.0f);
    Vec3 result = a - b;

    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(Vec3Test, ScalarMultiplication)
{
    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 result = v * 2.0f;

    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
}

TEST_F(Vec3Test, Length)
{
    Vec3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);

    Vec3 unit(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(unit.length(), 1.0f);

    Vec3 zero;
    EXPECT_FLOAT_EQ(zero.length(), 0.0f);
}

TEST_F(Vec3Test, Normalized)
{
    Vec3 v(3.0f, 4.0f, 0.0f);
    Vec3 normalized = v.normalized();

    EXPECT_NEAR(normalized.length(), 1.0f, 0.0001f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST_F(Vec3Test, NormalizedZeroVector)
{
    Vec3 zero;
    Vec3 normalized = zero.normalized();

    // Normalizing zero vector should return zero vector
    EXPECT_FLOAT_EQ(normalized.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized.y, 0.0f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST_F(Vec3Test, DotProduct)
{
    Vec3 a(1.0f, 0.0f, 0.0f);
    Vec3 b(0.0f, 1.0f, 0.0f);

    // Perpendicular vectors have dot product of 0
    EXPECT_FLOAT_EQ(Vec3::dot(a, b), 0.0f);

    // Parallel vectors
    Vec3 c(1.0f, 0.0f, 0.0f);
    Vec3 d(2.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(Vec3::dot(c, d), 2.0f);

    // General case
    Vec3 e(1.0f, 2.0f, 3.0f);
    Vec3 f(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(Vec3::dot(e, f), 32.0f);  // 1*4 + 2*5 + 3*6 = 32
}

TEST_F(Vec3Test, CrossProduct)
{
    Vec3 x(1.0f, 0.0f, 0.0f);
    Vec3 y(0.0f, 1.0f, 0.0f);
    Vec3 z = Vec3::cross(x, y);

    // x cross y = z
    EXPECT_FLOAT_EQ(z.x, 0.0f);
    EXPECT_FLOAT_EQ(z.y, 0.0f);
    EXPECT_FLOAT_EQ(z.z, 1.0f);

    // y cross x = -z
    Vec3 negZ = Vec3::cross(y, x);
    EXPECT_FLOAT_EQ(negZ.z, -1.0f);
}

// =============================================================================
// Matrix4 Tests
// =============================================================================

class Matrix4Test : public ::testing::Test
{
};

TEST_F(Matrix4Test, DefaultIsIdentity)
{
    Matrix4 m;

    // Diagonal should be 1
    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(m(2, 2), 1.0f);
    EXPECT_FLOAT_EQ(m(3, 3), 1.0f);

    // Off-diagonal should be 0
    EXPECT_FLOAT_EQ(m(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(m(1, 0), 0.0f);
}

TEST_F(Matrix4Test, Identity)
{
    Matrix4 m;
    m(0, 0) = 5.0f;  // Modify

    m.identity();

    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(0, 1), 0.0f);
}

TEST_F(Matrix4Test, Translation)
{
    Matrix4 t = Matrix4::translation(2.0f, 3.0f, 4.0f);

    EXPECT_FLOAT_EQ(t(0, 3), 2.0f);
    EXPECT_FLOAT_EQ(t(1, 3), 3.0f);
    EXPECT_FLOAT_EQ(t(2, 3), 4.0f);

    // Other elements should be identity
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(3, 3), 1.0f);
}

TEST_F(Matrix4Test, Scale)
{
    Matrix4 s = Matrix4::scale(2.0f, 3.0f, 4.0f);

    EXPECT_FLOAT_EQ(s(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(s(1, 1), 3.0f);
    EXPECT_FLOAT_EQ(s(2, 2), 4.0f);
    EXPECT_FLOAT_EQ(s(3, 3), 1.0f);
}

TEST_F(Matrix4Test, RotationX)
{
    float angle = static_cast<float>(M_PI / 2.0);  // 90 degrees
    Matrix4 r = Matrix4::rotationX(angle);

    // First column stays as (1, 0, 0, 0)
    EXPECT_FLOAT_EQ(r(0, 0), 1.0f);
    EXPECT_NEAR(r(1, 0), 0.0f, 0.0001f);

    // cos(90) = 0, sin(90) = 1
    EXPECT_NEAR(r(1, 1), 0.0f, 0.0001f);   // cos
    EXPECT_NEAR(r(2, 1), 1.0f, 0.0001f);   // sin
}

TEST_F(Matrix4Test, RotationY)
{
    float angle = static_cast<float>(M_PI / 2.0);  // 90 degrees
    Matrix4 r = Matrix4::rotationY(angle);

    EXPECT_NEAR(r(0, 0), 0.0f, 0.0001f);   // cos
    EXPECT_NEAR(r(2, 0), -1.0f, 0.0001f);  // -sin
    EXPECT_FLOAT_EQ(r(1, 1), 1.0f);        // Y unchanged
}

TEST_F(Matrix4Test, RotationZ)
{
    float angle = static_cast<float>(M_PI / 2.0);  // 90 degrees
    Matrix4 r = Matrix4::rotationZ(angle);

    EXPECT_NEAR(r(0, 0), 0.0f, 0.0001f);   // cos
    EXPECT_NEAR(r(1, 0), 1.0f, 0.0001f);   // sin
    EXPECT_FLOAT_EQ(r(2, 2), 1.0f);        // Z unchanged
}

TEST_F(Matrix4Test, MatrixMultiplication)
{
    Matrix4 t = Matrix4::translation(1.0f, 0.0f, 0.0f);
    Matrix4 s = Matrix4::scale(2.0f, 2.0f, 2.0f);

    Matrix4 result = t * s;

    // Scale then translate: scale happens first in column-major
    // The result should be scale first, then translate
    EXPECT_FLOAT_EQ(result(0, 0), 2.0f);  // Scale preserved
}

TEST_F(Matrix4Test, Perspective)
{
    float fov = static_cast<float>(M_PI / 4.0);  // 45 degrees
    float aspect = 16.0f / 9.0f;
    float nearP = 0.1f;
    float farP = 100.0f;

    Matrix4 p = Matrix4::perspective(fov, aspect, nearP, farP);

    // Check that perspective matrix has correct structure
    EXPECT_NE(p(0, 0), 0.0f);
    EXPECT_NE(p(1, 1), 0.0f);
    EXPECT_NE(p(2, 2), 0.0f);
    EXPECT_FLOAT_EQ(p(3, 2), -1.0f);  // Perspective divide
    EXPECT_FLOAT_EQ(p(3, 3), 0.0f);   // w = -z
}

TEST_F(Matrix4Test, Orthographic)
{
    Matrix4 o = Matrix4::orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    // Orthographic should be an identity-like scale
    EXPECT_FLOAT_EQ(o(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(o(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(o(2, 2), -1.0f);  // Z is negated
    EXPECT_FLOAT_EQ(o(3, 3), 1.0f);
}

TEST_F(Matrix4Test, LookAt)
{
    Matrix4 v = Matrix4::lookAt(0, 0, 5,   // Eye at (0, 0, 5)
                                 0, 0, 0,   // Looking at origin
                                 0, 1, 0);  // Up is Y

    // Looking down -Z, so the view matrix should map:
    // - Z axis to look direction
    // - Y axis to up direction
    // - X axis to right

    // The matrix is valid if it's orthonormal (rows/cols are unit length)
    float rowLen = std::sqrt(v(0,0)*v(0,0) + v(0,1)*v(0,1) + v(0,2)*v(0,2));
    EXPECT_NEAR(rowLen, 1.0f, 0.0001f);
}

TEST_F(Matrix4Test, DataPointer)
{
    Matrix4 m;
    const float* data = m.data();

    // Column-major: first column is [1,0,0,0]
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 0.0f);
    EXPECT_FLOAT_EQ(data[2], 0.0f);
    EXPECT_FLOAT_EQ(data[3], 0.0f);
}
