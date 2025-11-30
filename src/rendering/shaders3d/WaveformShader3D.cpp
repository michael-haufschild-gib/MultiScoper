/*
    Oscil - 3D Waveform Shader Base Class Implementation
*/

#include "rendering/shaders3d/WaveformShader3D.h"
#include <cmath>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

bool WaveformShader3D::compileShaderProgram(juce::OpenGLShaderProgram& shader,
                                            const char* vertexSource,
                                            const char* fragmentSource)
{
    juce::String translatedVertex = juce::OpenGLHelpers::translateVertexShaderToV3(vertexSource);
    juce::String translatedFragment = juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentSource);

    if (!shader.addVertexShader(translatedVertex))
    {
        DBG("WaveformShader3D: Vertex shader error: " << shader.getLastError());
        return false;
    }

    if (!shader.addFragmentShader(translatedFragment))
    {
        DBG("WaveformShader3D: Fragment shader error: " << shader.getLastError());
        return false;
    }

    if (!shader.link())
    {
        DBG("WaveformShader3D: Link error: " << shader.getLastError());
        return false;
    }

    return true;
}

void WaveformShader3D::render(juce::OpenGLContext& context,
                              const std::vector<float>& channel1,
                              const std::vector<float>* channel2,
                              const ShaderRenderParams& params)
{
    // Bridge: Convert 2D params to 3D data and call the 3D render method
    WaveformData3D data;
    data.samples = channel1.data();
    data.sampleCount = static_cast<int>(channel1.size());
    data.color = params.colour;
    data.lineThickness = params.lineWidth;
    data.zOffset = 0.0f; // Could be parameterised if multiple waveforms are stacked
    data.amplitude = params.verticalScale;
    data.time = time_; // Use internally updated time

    // If stereo, we might want to render the second channel too
    // For now, just render channel 1. Supporting stereo in 3D would require
    // specific logic per shader (e.g. separate ribbons, or interleaving)
    if (params.isStereo && channel2)
    {
        // Potential future expansion:
        // WaveformData3D data2 = data;
        // data2.samples = channel2->data();
        // data2.zOffset += 0.1f; // Offset slightly
        // render(context, data2, camera_, lighting_);
    }

    // Use stored camera and lighting configuration
    // These must be set via setCamera() and setLighting() before calling render()
    render(context, data, camera_, lighting_);
}

void WaveformShader3D::setMatrixUniforms(juce::OpenGLExtensionFunctions& ext,
                                         GLint modelLoc,
                                         GLint viewLoc,
                                         GLint projLoc,
                                         const Camera3D& camera,
                                         const Matrix4* model)
{
    Matrix4 identity;

    if (modelLoc >= 0)
    {
        const float* modelData = model ? model->data() : identity.data();
        ext.glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelData);
    }

    if (viewLoc >= 0)
    {
        ext.glUniformMatrix4fv(viewLoc, 1, GL_FALSE, camera.getViewMatrix().data());
    }

    if (projLoc >= 0)
    {
        ext.glUniformMatrix4fv(projLoc, 1, GL_FALSE, camera.getProjectionMatrix().data());
    }
}

void WaveformShader3D::setLightingUniforms(juce::OpenGLExtensionFunctions& ext,
                                           GLint lightDirLoc,
                                           GLint ambientLoc,
                                           GLint diffuseLoc,
                                           GLint specularLoc,
                                           GLint specPowerLoc,
                                           const LightingConfig& lighting)
{
    // Normalize light direction
    float lx = lighting.lightDirX;
    float ly = lighting.lightDirY;
    float lz = lighting.lightDirZ;
    float len = std::sqrt(lx*lx + ly*ly + lz*lz);
    if (len > 0) { lx /= len; ly /= len; lz /= len; }

    if (lightDirLoc >= 0)
        ext.glUniform3f(lightDirLoc, lx, ly, lz);

    if (ambientLoc >= 0)
        ext.glUniform3f(ambientLoc, lighting.ambientR, lighting.ambientG, lighting.ambientB);

    if (diffuseLoc >= 0)
        ext.glUniform3f(diffuseLoc, lighting.diffuseR, lighting.diffuseG, lighting.diffuseB);

    if (specularLoc >= 0)
        ext.glUniform4f(specularLoc, lighting.specularR, lighting.specularG,
                        lighting.specularB, lighting.specularIntensity);

    if (specPowerLoc >= 0)
        ext.glUniform1f(specPowerLoc, lighting.specularPower);
}

void WaveformShader3D::generateTubeMesh(const float* samples, int sampleCount,
                                        float radius, int segments,
                                        std::vector<float>& vertices,
                                        std::vector<GLuint>& indices)
{
    vertices.clear();
    indices.clear();

    if (!samples || sampleCount < 2 || segments < 3)
        return;

    // Each vertex: position (3) + normal (3) + uv (2) = 8 floats
    const int floatsPerVertex = 8;
    vertices.reserve(static_cast<size_t>(sampleCount * segments * floatsPerVertex));
    indices.reserve(static_cast<size_t>((sampleCount - 1) * segments * 6));

    // Generate vertices along the path
    for (int i = 0; i < sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);

        // Position on waveform (x = time, y = amplitude, z = 0 base)
        float px = t * 2.0f - 1.0f;  // -1 to 1
        float py = samples[i];
        float pz = 0.0f;

        // Calculate tangent direction
        float tx, ty, tz;
        if (i == 0)
        {
            tx = 1.0f / static_cast<float>(sampleCount);
            ty = samples[1] - samples[0];
            tz = 0.0f;
        }
        else if (i == sampleCount - 1)
        {
            tx = 1.0f / static_cast<float>(sampleCount);
            ty = samples[i] - samples[i - 1];
            tz = 0.0f;
        }
        else
        {
            tx = 2.0f / static_cast<float>(sampleCount);
            ty = samples[i + 1] - samples[i - 1];
            tz = 0.0f;
        }

        // Normalize tangent
        float tLen = std::sqrt(tx*tx + ty*ty + tz*tz);
        if (tLen > 0) { tx /= tLen; ty /= tLen; tz /= tLen; }

        // Calculate perpendicular vectors using Frenet frame
        // Up vector (use Z-up as default reference)
        float upX = 0.0f, upY = 0.0f, upZ = 1.0f;

        // Binormal = tangent x up
        float bx = ty * upZ - tz * upY;
        float by = tz * upX - tx * upZ;
        float bz = tx * upY - ty * upX;

        float bLen = std::sqrt(bx*bx + by*by + bz*bz);
        if (bLen > 0) { bx /= bLen; by /= bLen; bz /= bLen; }

        // Normal = binormal x tangent
        float nx = by * tz - bz * ty;
        float ny = bz * tx - bx * tz;
        float nz = bx * ty - by * tx;

        // Generate ring of vertices around this point
        for (int j = 0; j < segments; ++j)
        {
            float angle = static_cast<float>(j) / static_cast<float>(segments) * 2.0f * 3.14159265f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            // Position on ring
            float rx = px + (nx * cosA + bx * sinA) * radius;
            float ry = py + (ny * cosA + by * sinA) * radius;
            float rz = pz + (nz * cosA + bz * sinA) * radius;

            // Normal (pointing outward from center of tube)
            float rnx = nx * cosA + bx * sinA;
            float rny = ny * cosA + by * sinA;
            float rnz = nz * cosA + bz * sinA;

            // UV coordinates
            float u = t;
            float v = static_cast<float>(j) / static_cast<float>(segments);

            // Add vertex
            vertices.push_back(rx);
            vertices.push_back(ry);
            vertices.push_back(rz);
            vertices.push_back(rnx);
            vertices.push_back(rny);
            vertices.push_back(rnz);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Generate indices for triangle strips between rings
    for (int i = 0; i < sampleCount - 1; ++i)
    {
        for (int j = 0; j < segments; ++j)
        {
            int current = i * segments + j;
            int next = i * segments + ((j + 1) % segments);
            int currentNext = (i + 1) * segments + j;
            int nextNext = (i + 1) * segments + ((j + 1) % segments);

            // Two triangles per quad
            indices.push_back(static_cast<GLuint>(current));
            indices.push_back(static_cast<GLuint>(currentNext));
            indices.push_back(static_cast<GLuint>(next));

            indices.push_back(static_cast<GLuint>(next));
            indices.push_back(static_cast<GLuint>(currentNext));
            indices.push_back(static_cast<GLuint>(nextNext));
        }
    }
}

void WaveformShader3D::generateRibbonMesh(const float* samples, int sampleCount,
                                          float width,
                                          std::vector<float>& vertices,
                                          std::vector<GLuint>& indices)
{
    vertices.clear();
    indices.clear();

    if (!samples || sampleCount < 2)
        return;

    // Each vertex: position (3) + normal (3) + uv (2) = 8 floats
    const int floatsPerVertex = 8;
    vertices.reserve(static_cast<size_t>(sampleCount * 2 * floatsPerVertex));
    indices.reserve(static_cast<size_t>((sampleCount - 1) * 6));

    float halfWidth = width * 0.5f;

    // Generate two vertices per sample (top and bottom of ribbon)
    for (int i = 0; i < sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
        float x = t * 2.0f - 1.0f;  // -1 to 1
        float y = samples[i];

        // Calculate normal from tangent
        float tx, ty;
        if (i == 0)
        {
            tx = 1.0f;
            ty = (samples[1] - samples[0]) * static_cast<float>(sampleCount);
        }
        else if (i == sampleCount - 1)
        {
            tx = 1.0f;
            ty = (samples[i] - samples[i - 1]) * static_cast<float>(sampleCount);
        }
        else
        {
            tx = 1.0f;
            ty = (samples[i + 1] - samples[i - 1]) * static_cast<float>(sampleCount) * 0.5f;
        }

        // Normal is perpendicular to tangent in XY plane, facing Z+
        float len = std::sqrt(tx*tx + ty*ty);
        float nx = -ty / len;
        float ny = tx / len;

        // Front face normal (facing viewer)
        float fnx = 0.0f, fny = 0.0f, fnz = 1.0f;

        // Top vertex
        vertices.push_back(x);
        vertices.push_back(y + halfWidth * ny);
        vertices.push_back(halfWidth * nx);
        vertices.push_back(fnx);
        vertices.push_back(fny);
        vertices.push_back(fnz);
        vertices.push_back(t);
        vertices.push_back(0.0f);

        // Bottom vertex
        vertices.push_back(x);
        vertices.push_back(y - halfWidth * ny);
        vertices.push_back(-halfWidth * nx);
        vertices.push_back(fnx);
        vertices.push_back(fny);
        vertices.push_back(fnz);
        vertices.push_back(t);
        vertices.push_back(1.0f);
    }

    // Generate indices
    for (int i = 0; i < sampleCount - 1; ++i)
    {
        int topLeft = i * 2;
        int bottomLeft = i * 2 + 1;
        int topRight = (i + 1) * 2;
        int bottomRight = (i + 1) * 2 + 1;

        // Two triangles per quad
        indices.push_back(static_cast<GLuint>(topLeft));
        indices.push_back(static_cast<GLuint>(bottomLeft));
        indices.push_back(static_cast<GLuint>(topRight));

        indices.push_back(static_cast<GLuint>(topRight));
        indices.push_back(static_cast<GLuint>(bottomLeft));
        indices.push_back(static_cast<GLuint>(bottomRight));
    }
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
