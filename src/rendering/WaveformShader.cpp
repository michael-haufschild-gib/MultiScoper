/*
    Oscil - Waveform Shader Base Class Implementation
*/

#include "rendering/WaveformShader.h"
#include <cmath>

namespace oscil
{

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

    // Calculate layout based on stereo/mono
    float centerY1, centerY2;
    float amplitude1, amplitude2;

    if (params.isStereo && channel2 != nullptr)
    {
        // Stereo stacked layout
        float halfHeight = height * 0.5f;
        centerY1 = bounds.getY() + halfHeight * 0.5f;
        centerY2 = bounds.getY() + halfHeight * 1.5f;
        amplitude1 = halfHeight * 0.45f;
        amplitude2 = halfHeight * 0.45f;
    }
    else
    {
        // Mono layout
        centerY1 = bounds.getCentreY();
        centerY2 = centerY1;
        amplitude1 = height * 0.45f;
        amplitude2 = amplitude1;
    }

    g.setColour(params.colour.withAlpha(params.opacity));

    // Draw first channel
    {
        juce::Path path;
        float xScale = width / static_cast<float>(channel1.size() - 1);

        float startY = centerY1 - channel1[0] * amplitude1;
        path.startNewSubPath(bounds.getX(), startY);

        for (size_t i = 1; i < channel1.size(); ++i)
        {
            float x = bounds.getX() + static_cast<float>(i) * xScale;
            float y = centerY1 - channel1[i] * amplitude1;
            path.lineTo(x, y);
        }

        g.strokePath(path, juce::PathStrokeType(params.lineWidth));
    }

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        juce::Path path;
        float xScale = width / static_cast<float>(channel2->size() - 1);

        float startY = centerY2 - (*channel2)[0] * amplitude2;
        path.startNewSubPath(bounds.getX(), startY);

        for (size_t i = 1; i < channel2->size(); ++i)
        {
            float x = bounds.getX() + static_cast<float>(i) * xScale;
            float y = centerY2 - (*channel2)[i] * amplitude2;
            path.lineTo(x, y);
        }

        g.strokePath(path, juce::PathStrokeType(params.lineWidth));
    }
}

#if OSCIL_ENABLE_OPENGL
bool WaveformShader::compileShaderProgram(
    juce::OpenGLShaderProgram& program,
    const char* vertexSource,
    const char* fragmentSource)
{
    juce::String translatedVertex = juce::OpenGLHelpers::translateVertexShaderToV3(vertexSource);
    juce::String translatedFragment = juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentSource);

    if (!program.addVertexShader(translatedVertex))
    {
        DBG("Vertex shader compilation failed: " << program.getLastError());
        return false;
    }

    if (!program.addFragmentShader(translatedFragment))
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

    // Reserve space for triangle strip (2 vertices per sample point, 4 floats per vertex: x, y, distFromCenter, reserved)
    vertices.reserve(samples.size() * 2 * 4);

    float xScale = boundsWidth / static_cast<float>(samples.size() - 1);
    float halfWidth = lineWidth * 0.5f;

    for (size_t i = 0; i < samples.size(); ++i)
    {
        // FIX: Add boundsX to x coordinate for screen-space positioning
        float x = boundsX + static_cast<float>(i) * xScale;
        float y = centerY - samples[i] * amplitude;

        // Calculate normal direction (perpendicular to line segment)
        float nx = 0.0f, ny = 1.0f;

        if (i > 0 && i < samples.size() - 1)
        {
            // Average of normals from adjacent segments
            float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
            float prevY = centerY - samples[i - 1] * amplitude;
            float nextX = boundsX + (static_cast<float>(i) + 1.0f) * xScale;
            float nextY = centerY - samples[i + 1] * amplitude;

            float dx = nextX - prevX;
            float dy = nextY - prevY;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
        }
        else if (i == 0 && samples.size() > 1)
        {
            float nextX = boundsX + xScale;
            float nextY = centerY - samples[1] * amplitude;
            float dx = nextX - x;
            float dy = nextY - y;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
        }
        else if (i == samples.size() - 1 && samples.size() > 1)
        {
            float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
            float prevY = centerY - samples[i - 1] * amplitude;
            float dx = x - prevX;
            float dy = y - prevY;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
        }

        // Add top and bottom vertices of the line strip
        // Vertex 1: position + normal offset (top of line)
        vertices.push_back(x + nx * halfWidth);
        vertices.push_back(y + ny * halfWidth);
        vertices.push_back(1.0f);  // Distance from center (for glow)
        vertices.push_back(static_cast<float>(i) / static_cast<float>(samples.size() - 1)); // Normalized t along wave

        // Vertex 2: position - normal offset (bottom of line)
        vertices.push_back(x - nx * halfWidth);
        vertices.push_back(y - ny * halfWidth);
        vertices.push_back(-1.0f);  // Distance from center (for glow)
        vertices.push_back(static_cast<float>(i) / static_cast<float>(samples.size() - 1)); // Normalized t along wave
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
