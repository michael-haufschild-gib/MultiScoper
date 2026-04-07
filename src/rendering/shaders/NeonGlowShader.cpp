/*
    Oscil - Neon Glow Shader Implementation
*/

#include "rendering/shaders/NeonGlowShader.h"

#include "BinaryData.h"

#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct NeonGlowShader::GLResources : WaveformShader::WaveformGLResources
{
    // Extra uniform locations (beyond base projection, baseColor, opacity)
    GLint glowIntensityLoc = -1;
    GLint geometryScaleLoc = -1;
    // Attribute locations (cached at compile time)
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};
#endif

NeonGlowShader::NeonGlowShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

NeonGlowShader::~NeonGlowShader()
{
#if OSCIL_ENABLE_OPENGL
    if (gl_ && gl_->compiled)
    {
        jassertfalse;
        DBG("[NeonGlowShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
// NOLINTNEXTLINE(readability-function-size)
bool NeonGlowShader::compile(juce::OpenGLContext& context)
{
    if (!compileFromBinaryData(*gl_, context, BinaryData::neon_glow_vert, BinaryData::neon_glow_vertSize,
                               BinaryData::neon_glow_frag, BinaryData::neon_glow_fragSize, "NeonGlowShader"))
    {
        return false;
    }

    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");
    gl_->geometryScaleLoc = gl_->program->getUniformIDFromName("geometryScale");
    gl_->positionLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    gl_->distFromCenterLoc =
        juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");

    if (gl_->glowIntensityLoc < 0 || gl_->geometryScaleLoc < 0 || gl_->positionLoc < 0 || gl_->distFromCenterLoc < 0)
    {
        DBG("NeonGlowShader: Missing required uniforms or attributes");
        releaseGLResources(*gl_);
        return false;
    }

    return true;
}

void NeonGlowShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    releaseGLResources(*gl_);
}

bool NeonGlowShader::isCompiled() const { return gl_->compiled; }

void NeonGlowShader::render(juce::OpenGLContext& context, const std::vector<float>& channel1,
                            const std::vector<float>* channel2, const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    gl_->program->use();

    if (!setup2DProjection(context, ext, gl_->projectionLoc))
        return;

    const float kGeometryScale = 12.0f;
    juce::OpenGLExtensionFunctions::glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(),
                                                params.colour.getFloatGreen(), params.colour.getFloatBlue(),
                                                params.colour.getFloatAlpha());
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->opacityLoc, params.opacity);
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->glowIntensityLoc, params.shaderIntensity);
    juce::OpenGLExtensionFunctions::glUniform1f(gl_->geometryScaleLoc, kGeometryScale);

    juce::OpenGLExtensionFunctions::glBindVertexArray(gl_->vao);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float const height = params.bounds.getHeight();
    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amp1 = 0.0f;
    float amp2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    GLint const posLoc = gl_->positionLoc;
    GLint const distLoc = gl_->distFromCenterLoc;

    float const visualWidth = params.lineWidth * kGeometryScale;
    auto renderChannel = [&](const std::vector<float>& data, float cy, float amp) {
        std::vector<float> vertices;
        buildLineGeometry(vertices, data, cy, amp, visualWidth, params.bounds.getX(), params.bounds.getWidth());
        juce::OpenGLExtensionFunctions::glBufferData(GL_ARRAY_BUFFER,
                                                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                                                     vertices.data(), GL_DYNAMIC_DRAW);
        juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE,
                                                              4 * sizeof(float), nullptr);
        juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
        juce::OpenGLExtensionFunctions::glVertexAttribPointer(
            static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float))); // NOLINT(performance-no-int-to-ptr)
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
    };

    renderChannel(channel1, centerY1, amp1);
    if (params.isStereo && channel2 && channel2->size() >= 2)
        renderChannel(*channel2, centerY2, amp2);

    juce::OpenGLExtensionFunctions::glBindVertexArray(0);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
