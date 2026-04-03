/*
    Oscil - Basic Shader Implementation
*/

#include "rendering/shaders/BasicShader.h"

#include "BinaryData.h"

#include <algorithm>
#include <cmath>

namespace oscil
{

// Debug-only logging macro — no output in release builds
#define BASIC_LOG(msg) DBG("[BASIC] " << msg)

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct BasicShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations (get these after compilation)
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint glowIntensityLoc = -1;

    // Attribute locations (get these after compilation)
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};
#endif

BasicShader::BasicShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

BasicShader::~BasicShader()
{
#if OSCIL_ENABLE_OPENGL
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

#if OSCIL_ENABLE_OPENGL

void BasicShader::resolveUniforms(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    BASIC_LOG("Uniform locations - projection=" << gl_->projectionLoc << ", baseColor=" << gl_->baseColorLoc
                                                << ", opacity=" << gl_->opacityLoc
                                                << ", glowIntensity=" << gl_->glowIntensityLoc);

    gl_->positionLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    gl_->distFromCenterLoc =
        juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
    gl_->positionLoc = std::max(gl_->positionLoc, 0);
    if (gl_->distFromCenterLoc < 0)
        gl_->distFromCenterLoc = 1;

    BASIC_LOG("Attribute locations - position=" << gl_->positionLoc << ", distFromCenter=" << gl_->distFromCenterLoc);
}

bool BasicShader::validateUniforms() const
{
    bool valid = true;
    if (gl_->projectionLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'projection'");
        valid = false;
    }
    if (gl_->baseColorLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'baseColor'");
        valid = false;
    }
    if (gl_->opacityLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'opacity'");
        valid = false;
    }
    if (gl_->glowIntensityLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'glowIntensity'");
        valid = false;
    }
    return valid;
}

bool BasicShader::compile(juce::OpenGLContext& context)
{
    BASIC_LOG("compile() called, already compiled=" << static_cast<int>(gl_->compiled));

    if (gl_->compiled)
        return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String const vertexCode =
        juce::String::createStringFromData(BinaryData::basic_vert, BinaryData::basic_vertSize);
    juce::String const fragmentCode =
        juce::String::createStringFromData(BinaryData::basic_frag, BinaryData::basic_fragSize);

    if (!compileShaderProgram(*gl_->program, vertexCode.toRawUTF8(), fragmentCode.toRawUTF8()))
    {
        BASIC_LOG("Shader compilation/linking FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    resolveUniforms(context);

    if (!validateUniforms())
    {
        BASIC_LOG("Shader compilation failed - missing uniforms");
        gl_->program.reset();
        return false;
    }

    juce::OpenGLExtensionFunctions::glGenVertexArrays(1, &gl_->vao);
    juce::OpenGLExtensionFunctions::glGenBuffers(1, &gl_->vbo);

    BASIC_LOG("Created VAO=" << static_cast<int>(gl_->vao) << ", VBO=" << static_cast<int>(gl_->vbo));

    gl_->compiled = true;
    BASIC_LOG("Shader fully initialized, compiled=true");
    return true;
}

void BasicShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    if (!gl_->compiled)
        return;

    // Properly delete VAO and VBO to prevent resource leaks
    if (gl_->vbo != 0)
    {
        juce::OpenGLExtensionFunctions::glDeleteBuffers(1, &gl_->vbo);
        gl_->vbo = 0;
    }

    if (gl_->vao != 0)
    {
        juce::OpenGLExtensionFunctions::glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
    }

    gl_->program.reset();
    gl_->compiled = false;
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
        return;

    juce::OpenGLExtensionFunctions::glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(),
                                                params.colour.getFloatGreen(), params.colour.getFloatBlue(),
                                                params.colour.getFloatAlpha());
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->opacityLoc, params.opacity);
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->glowIntensityLoc, GLOW_INTENSITY);

    float const height = params.bounds.getHeight();
    float centerY1;
    float centerY2;
    float amp1;
    float amp2;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    juce::OpenGLExtensionFunctions::glBindVertexArray(gl_->vao);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(gl_->positionLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(gl_->positionLoc), 2, GL_FLOAT, GL_FALSE,
                                                          4 * sizeof(float), nullptr);
    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(gl_->distFromCenterLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(gl_->distFromCenterLoc), 1, GL_FLOAT,
                                                          GL_FALSE, 4 * sizeof(float),
                                                          reinterpret_cast<void*>(2 * sizeof(float)));

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

    float centerY1;
    float centerY2;
    float amp1;
    float amp2;
    if (params.isStereo && channel2 != nullptr)
    {
        float const halfH = height * 0.5f;
        centerY1 = bounds.getY() + (halfH * 0.5f);
        centerY2 = bounds.getY() + (halfH * 1.5f);
        amp1 = amp2 = halfH * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = centerY2 = bounds.getCentreY();
        amp1 = amp2 = height * 0.45f * params.verticalScale;
    }

    drawGlowingChannel(g, channel1, centerY1, amp1, bounds.getX(), bounds.getWidth(), params);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawGlowingChannel(g, *channel2, centerY2, amp2, bounds.getX(), bounds.getWidth(), params);
    }
}

} // namespace oscil
