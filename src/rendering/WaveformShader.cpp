/*
    Oscil - Waveform Shader Base Class Implementation
*/

#include "rendering/WaveformShader.h"
#include <cmath>

namespace oscil
{

namespace
{
void strokeChannelPath(juce::Graphics& g,
                       const std::vector<float>& samples,
                       float centerY, float amplitude,
                       float boundsX, float boundsWidth,
                       float lineWidth)
{
    if (samples.size() < 2)
        return;

    juce::Path path;
    float xScale = boundsWidth / static_cast<float>(samples.size() - 1);

    path.startNewSubPath(boundsX, centerY - samples[0] * amplitude);
    for (size_t i = 1; i < samples.size(); ++i)
    {
        float x = boundsX + static_cast<float>(i) * xScale;
        float y = centerY - samples[i] * amplitude;
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(lineWidth));
}
} // namespace

void WaveformShader::renderSoftware(
    juce::Graphics& g,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (channel1.size() < 2)
        return;

    auto bounds = params.bounds;
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    float centerY1, centerY2;
    float amplitude1, amplitude2;

    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = bounds.getY() + halfHeight * 0.5f;
        centerY2 = bounds.getY() + halfHeight * 1.5f;
        amplitude1 = amplitude2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = centerY2 = bounds.getCentreY();
        amplitude1 = amplitude2 = height * 0.45f * params.verticalScale;
    }

    g.setColour(params.colour.withAlpha(params.opacity));

    strokeChannelPath(g, channel1, centerY1, amplitude1,
                      bounds.getX(), width, params.lineWidth);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        strokeChannelPath(g, *channel2, centerY2, amplitude2,
                          bounds.getX(), width, params.lineWidth);
    }
}

#if OSCIL_ENABLE_OPENGL
bool WaveformShader::compileShaderProgram(
    juce::OpenGLShaderProgram& program,
    const char* vertexSource,
    const char* fragmentSource)
{
    if (!program.addVertexShader(vertexSource))
    {
        DBG("Vertex shader compilation failed: " << program.getLastError());
        return false;
    }

    if (!program.addFragmentShader(fragmentSource))
    {
        DBG("Fragment shader compilation failed: " << program.getLastError());
        return false;
    }

    if (!program.link())
    {
        DBG("Shader program linking failed: " << program.getLastError());
        return false;
    }

    return true;
}

void WaveformShader::calculateStereoLayout(const ShaderRenderParams& params,
                                            const std::vector<float>* channel2,
                                            float height,
                                            float& centerY1, float& centerY2,
                                            float& amp1, float& amp2)
{
    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amp1 = amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = centerY2 = params.bounds.getCentreY();
        amp1 = amp2 = height * 0.45f * params.verticalScale;
    }
}

namespace
{
void computeLineNormal(const std::vector<float>& samples, size_t i,
                        float boundsX, float xScale, float centerY, float amplitude,
                        float x, float y, float& nx, float& ny)
{
    nx = 0.0f;
    ny = 1.0f;

    if (i > 0 && i < samples.size() - 1)
    {
        float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
        float prevY = centerY - samples[i - 1] * amplitude;
        float nextX = boundsX + (static_cast<float>(i) + 1.0f) * xScale;
        float nextY = centerY - samples[i + 1] * amplitude;
        float dx = nextX - prevX;
        float dy = nextY - prevY;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.001f) { nx = -dy / len; ny = dx / len; }
    }
    else if (i == 0 && samples.size() > 1)
    {
        float dx = (boundsX + xScale) - x;
        float dy = (centerY - samples[1] * amplitude) - y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.001f) { nx = -dy / len; ny = dx / len; }
    }
    else if (i == samples.size() - 1 && samples.size() > 1)
    {
        float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
        float prevY = centerY - samples[i - 1] * amplitude;
        float dx = x - prevX;
        float dy = y - prevY;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.001f) { nx = -dy / len; ny = dx / len; }
    }
}
} // namespace

void WaveformShader::buildLineGeometry(
    std::vector<float>& vertices,
    const std::vector<float>& samples,
    float centerY,
    float amplitude,
    float lineWidth,
    float boundsX,
    float boundsWidth)
{
    if (samples.size() < 2)
        return;

    vertices.reserve(samples.size() * 2 * 4);
    float xScale = boundsWidth / static_cast<float>(samples.size() - 1);
    float halfWidth = lineWidth * 0.5f;

    for (size_t i = 0; i < samples.size(); ++i)
    {
        float x = boundsX + static_cast<float>(i) * xScale;
        float y = centerY - samples[i] * amplitude;
        float nx, ny;
        computeLineNormal(samples, i, boundsX, xScale, centerY, amplitude, x, y, nx, ny);

        float t = static_cast<float>(i) / static_cast<float>(samples.size() - 1);

        vertices.push_back(x + nx * halfWidth);
        vertices.push_back(y + ny * halfWidth);
        vertices.push_back(1.0f);
        vertices.push_back(t);

        vertices.push_back(x - nx * halfWidth);
        vertices.push_back(y - ny * halfWidth);
        vertices.push_back(-1.0f);
        vertices.push_back(t);
    }
}

void WaveformShader::buildFillGeometry(
    std::vector<float>& vertices,
    const std::vector<float>& samples,
    float centerY,
    float zeroY,
    float amplitude,
    float boundsX,
    float boundsWidth)
{
    if (samples.size() < 2)
        return;

    vertices.reserve(samples.size() * 2 * 4);

    float xScale = boundsWidth / static_cast<float>(samples.size() - 1);

    for (size_t i = 0; i < samples.size(); ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(samples.size() - 1);
        float x = boundsX + static_cast<float>(i) * xScale;
        float yWave = centerY - samples[i] * amplitude;

        // Top vertex (at waveform)
        vertices.push_back(x);
        vertices.push_back(yWave);
        vertices.push_back(1.0f); // v = 1 (top)
        vertices.push_back(t);    // u = t

        // Bottom vertex (at zero/baseline)
        vertices.push_back(x);
        vertices.push_back(zeroY);
        vertices.push_back(0.0f); // v = 0 (bottom)
        vertices.push_back(t);    // u = t
    }
}

bool WaveformShader::setup2DProjection(juce::OpenGLContext& context,
                                       juce::OpenGLExtensionFunctions& ext,
                                       GLint projectionLoc)
{
    auto* target = context.getTargetComponent();
    if (!target) return false;

    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());
    if (w <= 0.0f || h <= 0.0f) return false;

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    ext.glUniformMatrix4fv(projectionLoc, 1, juce::gl::GL_FALSE, projection);
    return true;
}

bool WaveformShader::checkGLError([[maybe_unused]] const char* location)
{
    using namespace juce::gl;

    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return true;

    [[maybe_unused]] const char* errorStr = "Unknown error";
    switch (error)
    {
        case GL_INVALID_ENUM:                  errorStr = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 errorStr = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             errorStr = "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 errorStr = "GL_OUT_OF_MEMORY"; break;
        default: break;
    }

    DBG("OpenGL error at " << location << ": " << errorStr << " (0x" << juce::String::toHexString(static_cast<int>(error)) << ")");

    // Clear any remaining errors
    while (glGetError() != GL_NO_ERROR) {}

    return false;
}
#endif

} // namespace oscil
