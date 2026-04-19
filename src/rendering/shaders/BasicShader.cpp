/*
    MultiScoper - Basic Shader Implementation
*/

#include "rendering/shaders/BasicShader.h"

#include "BinaryData.h"

#include <algorithm>
#include <cmath>

namespace multiscoper
{

// Debug-only logging macro — no output in release builds
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define BASIC_LOG(msg) DBG("[BASIC] " << msg)

#if MULTISCOPER_ENABLE_OPENGL
using namespace juce::gl;

struct BasicShader::GLResources : WaveformShader::WaveformGLResources
{
    // Extra uniform locations (beyond base projection, baseColor, opacity)
    GLint glowIntensityLoc = -1;

    // Attribute locations
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};
#endif

BasicShader::BasicShader()
#if MULTISCOPER_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

BasicShader::~BasicShader()
{
#if MULTISCOPER_ENABLE_OPENGL
    // Resources should be released before destruction
    // but we can't call OpenGL functions here without context
    if (gl_ && gl_->compiled)
    {
        // GPU resource leak: VBO/VAO/Program not released before destruction.
        // jassertfalse fires in debug builds only to catch this during development.
        jassertfalse;
        DBG("[BasicShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

#if MULTISCOPER_ENABLE_OPENGL

void BasicShader::resolveUniforms(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    // Base uniforms (projection, baseColor, opacity) already resolved by compileFromBinaryData
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    BASIC_LOG("Uniform locations - projection=" << gl_->projectionLoc << ", baseColor=" << gl_->baseColorLoc
                                                << ", opacity=" << gl_->opacityLoc
                                                << ", glowIntensity=" << gl_->glowIntensityLoc);

    gl_->positionLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    gl_->distFromCenterLoc =
        juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");

    if (gl_->positionLoc < 0)
        BASIC_LOG("WARNING: attribute 'position' not found (location=" << gl_->positionLoc << ")");
    if (gl_->distFromCenterLoc < 0)
        BASIC_LOG("WARNING: attribute 'distFromCenter' not found (location=" << gl_->distFromCenterLoc << ")");

    BASIC_LOG("Attribute locations - position=" << gl_->positionLoc << ", distFromCenter=" << gl_->distFromCenterLoc);
}

bool BasicShader::validateUniforms() const
{
    // Base uniforms (projection, baseColor, opacity) already validated by compileFromBinaryData
    if (gl_->glowIntensityLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'glowIntensity'");
        return false;
    }
    if (gl_->positionLoc < 0)
    {
        BASIC_LOG("Failed to find attribute 'position'");
        return false;
    }
    if (gl_->distFromCenterLoc < 0)
    {
        BASIC_LOG("Failed to find attribute 'distFromCenter'");
        return false;
    }
    return true;
}

bool BasicShader::compile(juce::OpenGLContext& context)
{
    BASIC_LOG("compile() called, already compiled=" << static_cast<int>(gl_->compiled));

    if (!compileFromBinaryData(*gl_, context, BinaryData::basic_vert, BinaryData::basic_vertSize,
                               BinaryData::basic_frag, BinaryData::basic_fragSize, "BasicShader"))
    {
        BASIC_LOG("Base shader compilation failed");
        return false;
    }

    resolveUniforms(context);

    if (!validateUniforms())
    {
        BASIC_LOG("Shader compilation failed - missing extra uniforms");
        releaseGLResources(*gl_);
        return false;
    }

    BASIC_LOG("Created VAO=" << static_cast<int>(gl_->vao) << ", VBO=" << static_cast<int>(gl_->vbo));
    BASIC_LOG("Shader fully initialized, compiled=true");
    return true;
}

void BasicShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    releaseGLResources(*gl_);
}

bool BasicShader::isCompiled() const { return gl_->compiled; }

void BasicShader::drawGlowPasses(juce::OpenGLExtensionFunctions& ext, int vertexCount)
{
    juce::ignoreUnused(ext);
    for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
    {
        float const passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
        juce::OpenGLExtensionFunctions::glUniform1f(gl_->glowIntensityLoc, passIntensity);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
    }
}

void BasicShader::renderChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples, float centerY,
                                float amplitude, float glowWidth, float boundsX, float boundsWidth)
{
    vertexBuffer_.clear();
    buildLineGeometry(vertexBuffer_, samples, centerY, amplitude, glowWidth, boundsX, boundsWidth);
    juce::OpenGLExtensionFunctions::glBufferData(GL_ARRAY_BUFFER,
                                                 static_cast<GLsizeiptr>(vertexBuffer_.size() * sizeof(float)),
                                                 vertexBuffer_.data(), GL_DYNAMIC_DRAW);
    drawGlowPasses(ext, static_cast<int>(vertexBuffer_.size() / 4));
}

void BasicShader::render(juce::OpenGLContext& context, const std::vector<float>& channel1,
                         const std::vector<float>* channel2, const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    gl_->program->use();

    if (!setup2DProjection(context, ext, gl_->projectionLoc))
    {
        glDisable(GL_BLEND);
        return;
    }

    juce::OpenGLExtensionFunctions::glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(),
                                                params.colour.getFloatGreen(), params.colour.getFloatBlue(),
                                                params.colour.getFloatAlpha());
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->opacityLoc, params.opacity);
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->glowIntensityLoc, GLOW_INTENSITY);

    float const height = params.bounds.getHeight();
    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amp1 = 0.0f;
    float amp2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    juce::OpenGLExtensionFunctions::glBindVertexArray(gl_->vao);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(gl_->positionLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(gl_->positionLoc), 2, GL_FLOAT, GL_FALSE,
                                                          4 * sizeof(float), nullptr);
    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(gl_->distFromCenterLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(
        static_cast<GLuint>(gl_->distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))); // NOLINT(performance-no-int-to-ptr)

    float const glowWidth = params.lineWidth * 10.0f;
    renderChannel(ext, channel1, centerY1, amp1, glowWidth, params.bounds.getX(), params.bounds.getWidth());

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
        renderChannel(ext, *channel2, centerY2, amp2, glowWidth, params.bounds.getX(), params.bounds.getWidth());

    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(gl_->positionLoc));
    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(gl_->distFromCenterLoc));
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
    juce::OpenGLExtensionFunctions::glBindVertexArray(0);
    glDisable(GL_BLEND);
}
#endif

namespace
{
void drawGlowingChannel(juce::Graphics& g, const std::vector<float>& samples, float centerY, float amplitude,
                        float boundsX, float boundsWidth, const ShaderRenderParams& params)
{
    juce::Path path;
    float const xScale = boundsWidth / static_cast<float>(samples.size() - 1);

    path.startNewSubPath(boundsX, centerY - (samples[0] * amplitude));
    for (size_t i = 1; i < samples.size(); ++i)
    {
        float const x = boundsX + (static_cast<float>(i) * xScale);
        float const y = centerY - (samples[i] * amplitude);
        path.lineTo(x, y);
    }

    for (int pass = BasicShader::GLOW_PASSES; pass >= 0; --pass)
    {
        float const glowWidth = params.lineWidth * (1.0f + (static_cast<float>(pass) * 1.5f));
        float const alpha =
            params.opacity * (0.15f + (0.25f * (1.0f - (static_cast<float>(pass) / BasicShader::GLOW_PASSES))));
        g.setColour(params.colour.withAlpha(alpha));
        g.strokePath(path,
                     juce::PathStrokeType(glowWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(params.colour.withAlpha(params.opacity));
    g.strokePath(path, juce::PathStrokeType(params.lineWidth));

    g.setColour(params.colour.brighter(0.5f).withAlpha(params.opacity * 0.8f));
    g.strokePath(path, juce::PathStrokeType(params.lineWidth * 0.5f));
}
} // namespace

void BasicShader::renderSoftware(juce::Graphics& g, const std::vector<float>& channel1,
                                 const std::vector<float>* channel2, const ShaderRenderParams& params)
{
    if (channel1.size() < 2)
        return;

    auto bounds = params.bounds;
    float const height = bounds.getHeight();

    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amp1 = 0.0f;
    float amp2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    drawGlowingChannel(g, channel1, centerY1, amp1, bounds.getX(), bounds.getWidth(), params);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawGlowingChannel(g, *channel2, centerY2, amp2, bounds.getX(), bounds.getWidth(), params);
    }
}

} // namespace multiscoper
