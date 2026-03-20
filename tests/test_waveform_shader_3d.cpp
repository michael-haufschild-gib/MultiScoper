/*
    Oscil - WaveformShader3D Tests
*/

#include <gtest/gtest.h>
#include "rendering/shaders3d/WaveformShader3D.h"
#include <vector>
#include <algorithm>
#include <cmath>

using namespace oscil;

// Ensure GLuint is available. If not, it might be because OpenGL headers aren't fully included in test env.
// But WaveformShader3D.h includes juce_opengl.h.
// If GLuint is not found, we might be on a platform where it's not defined in the header without context.
// However, usually it is.
// Let's try using `GLuint` global.

TEST(WaveformShader3DTest, GenerateTubeMeshXSpread)
{
    // Create dummy sample data
    std::vector<float> samples = {0.0f, 0.5f, -0.5f, 0.0f};
    int sampleCount = static_cast<int>(samples.size());
    float radius = 0.1f;
    int segments = 4;
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    // Test 1: xSpread = 1.0f
    // Should result in X range roughly [-1, 1] (plus radius)
    WaveformShader3D::generateTubeMesh(samples.data(), sampleCount, 1.0f, radius, segments, vertices, indices);
    
    ASSERT_FALSE(vertices.empty());
    
    float minX = 1000.0f;
    float maxX = -1000.0f;
    
    // Vertices layout: x, y, z, nx, ny, nz, u, v (8 floats)
    for (size_t i = 0; i < vertices.size(); i += 8)
    {
        float x = vertices[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
    }
    
    // With radius 0.1, max extent should be roughly 1.0 + 0.1 = 1.1
    // But due to tube orientation (tangent slope), the X extent of the ring might be less than radius.
    // We just want to verify it's roughly correct (scaling works).
    EXPECT_NEAR(minX, -1.0f, 0.15f);
    EXPECT_NEAR(maxX, 1.0f, 0.15f);

    // Test 2: xSpread = 2.0f
    // Should result in X range roughly [-2, 2] (plus radius)
    WaveformShader3D::generateTubeMesh(samples.data(), sampleCount, 2.0f, radius, segments, vertices, indices);
    
    minX = 1000.0f;
    maxX = -1000.0f;
    
    for (size_t i = 0; i < vertices.size(); i += 8)
    {
        float x = vertices[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
    }
    
    EXPECT_NEAR(minX, -2.0f, 0.15f);
    EXPECT_NEAR(maxX, 2.0f, 0.15f);
    
    // Test 3: xSpread = 5.0f
    WaveformShader3D::generateTubeMesh(samples.data(), sampleCount, 5.0f, radius, segments, vertices, indices);
    
    minX = 1000.0f;
    maxX = -1000.0f;
    
    for (size_t i = 0; i < vertices.size(); i += 8)
    {
        float x = vertices[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
    }
    
    EXPECT_NEAR(minX, -5.0f, 0.15f);
    EXPECT_NEAR(maxX, 5.0f, 0.15f);
}

TEST(WaveformShader3DTest, GenerateRibbonMeshXSpread)
{
    // Create dummy sample data
    std::vector<float> samples = {0.0f, 0.0f};
    int sampleCount = 2;
    float width = 0.2f; // Ribbon thickness
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    // Test 1: xSpread = 10.0f
    WaveformShader3D::generateRibbonMesh(samples.data(), sampleCount, 10.0f, width, vertices, indices);
    
    float minX = 1000.0f;
    float maxX = -1000.0f;
    
    for (size_t i = 0; i < vertices.size(); i += 8)
    {
        float x = vertices[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
    }
    
    // Ribbon is flat in Z (mostly), but has thickness.
    // Since samples are 0, tangent is X-axis. Normal is Y-axis (or Z depending on impl).
    // Logic: 
    // Top vertex: x, y + halfWidth*ny, ...
    // If flat line, ny is 1 (or similar).
    // x range should be -10 to 10.
    
    EXPECT_NEAR(minX, -10.0f, 0.001f);
    EXPECT_NEAR(maxX, 10.0f, 0.001f);
}
