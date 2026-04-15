/*
    Oscil - Waveform Shader Base Class Implementation
*/

#include "rendering/WaveformShader.h"

#include <cmath>

namespace oscil
{

namespace
{
void strokeChannelPath(juce::Graphics& g, const std::vector<float>& samples, float centerY, float amplitude,
                       float boundsX, float boundsWidth, float lineWidth)
{
    if (samples.size() < 2)
        return;

    juce::Path path;
    float const xScale = boundsWidth / static_cast<float>(samples.size() - 1);

    path.startNewSubPath(boundsX, centerY - (samples[0] * amplitude));
    for (size_t i = 1; i < samples.size(); ++i)
    {
        float const x = boundsX + (static_cast<float>(i) * xScale);
        float const y = centerY - (samples[i] * amplitude);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(lineWidth));
}
} // namespace

void WaveformShader::calculateStereoLayout(const ShaderRenderParams& params, const std::vector<float>* channel2,
                                           float height, float& centerY1, float& centerY2, float& amp1, float& amp2)
{
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        float const halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + (halfHeight * 0.5f);
        centerY2 = params.bounds.getY() + (halfHeight * 1.5f);
        amp1 = amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = centerY2 = params.bounds.getCentreY();
        amp1 = amp2 = height * 0.45f * params.verticalScale;
    }
}

void WaveformShader::renderSoftware(juce::Graphics& g, const std::vector<float>& channel1,
                                    const std::vector<float>* channel2, const ShaderRenderParams& params)
{
    if (channel1.size() < 2)
        return;

    float const width = params.bounds.getWidth();
    float const height = params.bounds.getHeight();

    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amplitude1 = 0.0f;
    float amplitude2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amplitude1, amplitude2);

    g.setColour(params.colour.withAlpha(params.opacity));

    strokeChannelPath(g, channel1, centerY1, amplitude1, params.bounds.getX(), width, params.lineWidth);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        strokeChannelPath(g, *channel2, centerY2, amplitude2, params.bounds.getX(), width, params.lineWidth);
    }
}

#if OSCIL_ENABLE_OPENGL
bool WaveformShader::compileShaderProgram(juce::OpenGLShaderProgram& program, const char* vertexSource,
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

bool WaveformShader::compileFromBinaryData(WaveformGLResources& gl, juce::OpenGLContext& context, const char* vertData,
                                           int vertSize, const char* fragData, int fragSize,
                                           const juce::String& shaderName)
{
    if (gl.compiled)
        return true;

    gl.program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String const vertexCode = juce::String::createStringFromData(vertData, vertSize);
    juce::String const fragmentCode = juce::String::createStringFromData(fragData, fragSize);

    if (!compileShaderProgram(*gl.program, vertexCode.toRawUTF8(), fragmentCode.toRawUTF8()))
    {
        DBG(shaderName << ": Shader compilation failed: " << gl.program->getLastError());
        gl.program.reset();
        return false;
    }

    gl.projectionLoc = gl.program->getUniformIDFromName("projection");
    gl.baseColorLoc = gl.program->getUniformIDFromName("baseColor");
    gl.opacityLoc = gl.program->getUniformIDFromName("opacity");

    if (gl.projectionLoc < 0 || gl.baseColorLoc < 0 || gl.opacityLoc < 0)
    {
        DBG(shaderName << ": Missing required base uniforms");
        gl.program.reset();
        return false;
    }

    juce::OpenGLExtensionFunctions::glGenVertexArrays(1, &gl.vao);
    juce::OpenGLExtensionFunctions::glGenBuffers(1, &gl.vbo);

    gl.compiled = true;
    return true;
}

void WaveformShader::releaseGLResources(WaveformGLResources& gl)
{
    if (!gl.compiled)
        return;

    if (gl.vbo != 0)
    {
        juce::OpenGLExtensionFunctions::glDeleteBuffers(1, &gl.vbo);
        gl.vbo = 0;
    }
    if (gl.vao != 0)
    {
        juce::OpenGLExtensionFunctions::glDeleteVertexArrays(1, &gl.vao);
        gl.vao = 0;
    }
    gl.program.reset();
    gl.compiled = false;
}

namespace
{
struct NormalContext
{
    const std::vector<float>& samples;
    float boundsX;
    float xScale;
    float centerY;
    float amplitude;
};

// Convert a perpendicular (dx,dy) to a unit normal, returning {nx,ny}.
// Falls back to {0,1} if the segment is degenerate.
void perpendicularUnitNormal(float dx, float dy, float& nx, float& ny)
{
    float const len = std::sqrt((dx * dx) + (dy * dy));
    if (len > 0.001f)
    {
        nx = -dy / len;
        ny = dx / len;
    }
}

void computeLineNormal(const NormalContext& ctx, size_t i, float x, float y, float& nx, float& ny)
{
    nx = 0.0f;
    ny = 1.0f;

    const auto& samples = ctx.samples;
    size_t const n = samples.size();
    if (n < 2)
        return;

    if (i > 0 && i < n - 1)
    {
        float const prevX = ctx.boundsX + ((static_cast<float>(i) - 1.0f) * ctx.xScale);
        float const prevY = ctx.centerY - (samples[i - 1] * ctx.amplitude);
        float const nextX = ctx.boundsX + ((static_cast<float>(i) + 1.0f) * ctx.xScale);
        float const nextY = ctx.centerY - (samples[i + 1] * ctx.amplitude);
        perpendicularUnitNormal(nextX - prevX, nextY - prevY, nx, ny);
    }
    else if (i == 0)
    {
        perpendicularUnitNormal((ctx.boundsX + ctx.xScale) - x, (ctx.centerY - (samples[1] * ctx.amplitude)) - y, nx,
                                ny);
    }
    else // i == n - 1
    {
        float const prevX = ctx.boundsX + ((static_cast<float>(i) - 1.0f) * ctx.xScale);
        float const prevY = ctx.centerY - (samples[i - 1] * ctx.amplitude);
        perpendicularUnitNormal(x - prevX, y - prevY, nx, ny);
    }
}
} // namespace

void WaveformShader::buildLineGeometry(std::vector<float>& vertices, const std::vector<float>& samples, float centerY,
                                       float amplitude, float lineWidth, float boundsX, float boundsWidth)
{
    if (samples.size() < 2)
        return;

    vertices.reserve(samples.size() * 2 * 4);
    float const xScale = boundsWidth / static_cast<float>(samples.size() - 1);
    float const halfWidth = lineWidth * 0.5f;

    NormalContext const ctx{
        .samples = samples, .boundsX = boundsX, .xScale = xScale, .centerY = centerY, .amplitude = amplitude};

    for (size_t i = 0; i < samples.size(); ++i)
    {
        float const x = boundsX + (static_cast<float>(i) * xScale);
        float const y = centerY - (samples[i] * amplitude);
        float nx = NAN;
        float ny = NAN;
        computeLineNormal(ctx, i, x, y, nx, ny);

        float const t = static_cast<float>(i) / static_cast<float>(samples.size() - 1);

        vertices.push_back(x + (nx * halfWidth));
        vertices.push_back(y + (ny * halfWidth));
        vertices.push_back(1.0f);
        vertices.push_back(t);

        vertices.push_back(x - (nx * halfWidth));
        vertices.push_back(y - (ny * halfWidth));
        vertices.push_back(-1.0f);
        vertices.push_back(t);
    }
}

void WaveformShader::buildFillGeometry(std::vector<float>& vertices, const std::vector<float>& samples, float centerY,
                                       float zeroY, float amplitude, float boundsX, float boundsWidth)
{
    if (samples.size() < 2)
        return;

    vertices.reserve(samples.size() * 2 * 4);

    float const xScale = boundsWidth / static_cast<float>(samples.size() - 1);

    for (size_t i = 0; i < samples.size(); ++i)
    {
        float const t = static_cast<float>(i) / static_cast<float>(samples.size() - 1);
        float const x = boundsX + (static_cast<float>(i) * xScale);
        float const yWave = centerY - (samples[i] * amplitude);

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

bool WaveformShader::setup2DProjection(juce::OpenGLContext& context, juce::OpenGLExtensionFunctions& ext,
                                       GLint projectionLoc)
{
    juce::ignoreUnused(ext);
    auto* target = context.getTargetComponent();
    if (!target)
        return false;

    auto const w = static_cast<float>(target->getWidth());
    auto const h = static_cast<float>(target->getHeight());
    if (w <= 0.0f || h <= 0.0f)
        return false;

    float projection[16] = {2.0f / w, 0.0f, 0.0f,  0.0f, 0.0f,  -2.0f / h, 0.0f, 0.0f,
                            0.0f,     0.0f, -1.0f, 0.0f, -1.0f, 1.0f,      0.0f, 1.0f};
    juce::OpenGLExtensionFunctions::glUniformMatrix4fv(projectionLoc, 1, juce::gl::GL_FALSE, projection);
    return true;
}

bool WaveformShader::checkGLError([[maybe_unused]] const char* location)
{
    using namespace juce::gl;

    GLenum const error = glGetError();
    if (error == GL_NO_ERROR)
        return true;

    [[maybe_unused]] const char* errorStr = "Unknown error";
    switch (error)
    {
        case GL_INVALID_ENUM:
            errorStr = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorStr = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorStr = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorStr = "GL_OUT_OF_MEMORY";
            break;
        default:
            break;
    }

    DBG("OpenGL error at " << location << ": " << errorStr << " (0x"
                           << juce::String::toHexString(static_cast<int>(error)) << ")");

    // Clear any remaining errors
    while (glGetError() != GL_NO_ERROR)
    {
    }

    return false;
}
#endif

} // namespace oscil
