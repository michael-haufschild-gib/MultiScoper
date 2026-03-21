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
    if (!shader.addVertexShader(vertexSource))
    {
        DBG("WaveformShader3D: Vertex shader error: " << shader.getLastError());
        return false;
    }

    if (!shader.addFragmentShader(fragmentSource))
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
    WaveformData3D data;
    data.samples = channel1.data();
    data.sampleCount = static_cast<int>(channel1.size());
    data.color = params.colour;
    data.lineThickness = params.lineWidth;
    data.time = time_;

    if (params.isStereo && channel2 && channel2->size() >= 2)
    {
        renderStereoChannels(context, data, *channel2, params);
    }
    else
    {
        data.yOffset = 0.0f;
        data.zOffset = 0.0f;
        data.amplitude = params.verticalScale;
        render(context, data, camera_, lighting_);
    }
}

void WaveformShader3D::renderStereoChannels(juce::OpenGLContext& context,
                                             WaveformData3D& data,
                                             const std::vector<float>& channel2,
                                             const ShaderRenderParams& params)
{
    float xSpread, halfHeight;
    calculateCameraSpread(camera_, xSpread, halfHeight);

    // Z-offset compensates for camera pitch so both channels are equidistant
    float pitchRad = camera_.getOrbitPitch() * 3.14159265f / 180.0f;
    float tanPitch = std::tan(pitchRad);

    data.yOffset = 0.5f;
    data.amplitude = params.verticalScale * 0.45f;
    data.zOffset = -(data.yOffset * halfHeight) * tanPitch;
    render(context, data, camera_, lighting_);

    glClear(GL_DEPTH_BUFFER_BIT);

    WaveformData3D data2;
    data2.samples = channel2.data();
    data2.sampleCount = static_cast<int>(channel2.size());
    data2.color = params.colour;
    data2.lineThickness = params.lineWidth;
    data2.yOffset = -0.5f;
    data2.amplitude = params.verticalScale * 0.45f;
    data2.zOffset = -(data2.yOffset * halfHeight) * tanPitch;
    data2.time = time_;
    render(context, data2, camera_, lighting_);
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

void WaveformShader3D::setupPosNormTexAttribs(juce::OpenGLExtensionFunctions& ext, GLuint programID)
{
    GLint posAttrib = ext.glGetAttribLocation(programID, "aPosition");
    GLint normAttrib = ext.glGetAttribLocation(programID, "aNormal");
    GLint texAttrib = ext.glGetAttribLocation(programID, "aTexCoord");

    const int stride = 8 * sizeof(float);

    if (posAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib), 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    }
    if (normAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(normAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(normAttrib), 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(3 * sizeof(float)));
    }
    if (texAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(texAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(texAttrib), 2, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(6 * sizeof(float)));
    }
}

void WaveformShader3D::calculateCameraSpread(const Camera3D& camera, float& xSpread, float& halfHeight)
{
    xSpread = 1.0f;
    halfHeight = 1.0f;

    if (camera.getProjection() == CameraProjection::Orthographic)
    {
        float height = camera.getOrthoSize();
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }
    else
    {
        float dist = (camera.getPosition() - camera.getTarget()).length();
        float fovRad = camera.getFOV() * 3.14159265f / 180.0f;
        float height = 2.0f * dist * std::tan(fovRad * 0.5f);
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }
}

static void generateTubeRingVertices(std::vector<float>& vertices,
                                     const float* samples, int sampleCount,
                                     float xSpread, float radius, int segments)
{
    for (int i = 0; i < sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
        float px = (t * 2.0f - 1.0f) * xSpread;
        float py = samples[i];

        // Calculate tangent direction
        float tx, ty, tz = 0.0f;
        if (i == 0)
        {
            tx = (1.0f / static_cast<float>(sampleCount)) * xSpread;
            ty = samples[1] - samples[0];
        }
        else if (i == sampleCount - 1)
        {
            tx = (1.0f / static_cast<float>(sampleCount)) * xSpread;
            ty = samples[i] - samples[i - 1];
        }
        else
        {
            tx = (2.0f / static_cast<float>(sampleCount)) * xSpread;
            ty = samples[i + 1] - samples[i - 1];
        }

        // Normalize tangent
        float tLen = std::sqrt(tx * tx + ty * ty + tz * tz);
        if (tLen > 0) { tx /= tLen; ty /= tLen; tz /= tLen; }

        // Frenet frame: binormal = tangent x (0,0,1)
        float bx = ty, by = -tx, bz = 0.0f;
        float bLen = std::sqrt(bx * bx + by * by + bz * bz);
        if (bLen > 0) { bx /= bLen; by /= bLen; bz /= bLen; }

        // Normal = binormal x tangent
        float nx = by * tz - bz * ty;
        float ny = bz * tx - bx * tz;
        float nz = bx * ty - by * tx;

        for (int j = 0; j < segments; ++j)
        {
            float angle = static_cast<float>(j) / static_cast<float>(segments) * 2.0f * 3.14159265f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            vertices.push_back(px + (nx * cosA + bx * sinA) * radius);
            vertices.push_back(py + (ny * cosA + by * sinA) * radius);
            vertices.push_back((nz * cosA + bz * sinA) * radius);
            vertices.push_back(nx * cosA + bx * sinA);
            vertices.push_back(ny * cosA + by * sinA);
            vertices.push_back(nz * cosA + bz * sinA);
            vertices.push_back(t);
            vertices.push_back(static_cast<float>(j) / static_cast<float>(segments));
        }
    }
}

static void generateTubeIndices(std::vector<GLuint>& indices, int sampleCount, int segments)
{
    for (int i = 0; i < sampleCount - 1; ++i)
    {
        for (int j = 0; j < segments; ++j)
        {
            int current = i * segments + j;
            int next = i * segments + ((j + 1) % segments);
            int currentNext = (i + 1) * segments + j;
            int nextNext = (i + 1) * segments + ((j + 1) % segments);

            indices.push_back(static_cast<GLuint>(current));
            indices.push_back(static_cast<GLuint>(currentNext));
            indices.push_back(static_cast<GLuint>(next));

            indices.push_back(static_cast<GLuint>(next));
            indices.push_back(static_cast<GLuint>(currentNext));
            indices.push_back(static_cast<GLuint>(nextNext));
        }
    }
}

void WaveformShader3D::generateTubeMesh(const float* samples, int sampleCount,
                                        float xSpread,
                                        float radius, int segments,
                                        std::vector<float>& vertices,
                                        std::vector<GLuint>& indices)
{
    vertices.clear();
    indices.clear();

    if (!samples || sampleCount < 2 || segments < 3)
        return;

    const int floatsPerVertex = 8;
    vertices.reserve(static_cast<size_t>(sampleCount * segments * floatsPerVertex));
    indices.reserve(static_cast<size_t>((sampleCount - 1) * segments * 6));

    generateTubeRingVertices(vertices, samples, sampleCount, xSpread, radius, segments);
    generateTubeIndices(indices, sampleCount, segments);
}

static void generateRibbonVertices(std::vector<float>& vertices,
                                   const float* samples, int sampleCount,
                                   float xSpread, float halfWidth)
{
    for (int i = 0; i < sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
        float x = (t * 2.0f - 1.0f) * xSpread;
        float y = samples[i];

        float tx, ty;
        if (i == 0)
        {
            tx = xSpread;
            ty = (samples[1] - samples[0]) * static_cast<float>(sampleCount);
        }
        else if (i == sampleCount - 1)
        {
            tx = xSpread;
            ty = (samples[i] - samples[i - 1]) * static_cast<float>(sampleCount);
        }
        else
        {
            tx = xSpread;
            ty = (samples[i + 1] - samples[i - 1]) * static_cast<float>(sampleCount) * 0.5f;
        }

        float len = std::sqrt(tx * tx + ty * ty);
        if (len <= 0.0f) len = 1.0f;
        float nx = -ty / len;
        float ny = tx / len;

        // Top vertex: pos(3) + normal(3) + uv(2)
        vertices.push_back(x);
        vertices.push_back(y + halfWidth * ny);
        vertices.push_back(halfWidth * nx);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(t); vertices.push_back(0.0f);

        // Bottom vertex
        vertices.push_back(x);
        vertices.push_back(y - halfWidth * ny);
        vertices.push_back(-halfWidth * nx);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);
        vertices.push_back(t); vertices.push_back(1.0f);
    }
}

static void generateQuadStripIndices(std::vector<GLuint>& indices, int stripCount)
{
    for (int i = 0; i < stripCount; ++i)
    {
        int topLeft = i * 2;
        int bottomLeft = i * 2 + 1;
        int topRight = (i + 1) * 2;
        int bottomRight = (i + 1) * 2 + 1;

        indices.push_back(static_cast<GLuint>(topLeft));
        indices.push_back(static_cast<GLuint>(bottomLeft));
        indices.push_back(static_cast<GLuint>(topRight));
        indices.push_back(static_cast<GLuint>(topRight));
        indices.push_back(static_cast<GLuint>(bottomLeft));
        indices.push_back(static_cast<GLuint>(bottomRight));
    }
}

void WaveformShader3D::generateRibbonMesh(const float* samples, int sampleCount,
                                          float xSpread, float width,
                                          std::vector<float>& vertices,
                                          std::vector<GLuint>& indices)
{
    vertices.clear();
    indices.clear();

    if (!samples || sampleCount < 2)
        return;

    const int floatsPerVertex = 8;
    vertices.reserve(static_cast<size_t>(sampleCount * 2 * floatsPerVertex));
    indices.reserve(static_cast<size_t>((sampleCount - 1) * 6));

    generateRibbonVertices(vertices, samples, sampleCount, xSpread, width * 0.5f);
    generateQuadStripIndices(indices, sampleCount - 1);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
