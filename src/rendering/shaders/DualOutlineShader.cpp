/*
    Oscil - Dual Outline Shader Implementation
*/

#include "rendering/shaders/DualOutlineShader.h"

#include "BinaryData.h"

#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct DualOutlineShader::GLResources : WaveformShader::WaveformGLResources
{
};
#endif

DualOutlineShader::DualOutlineShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

DualOutlineShader::~DualOutlineShader()
{
#if OSCIL_ENABLE_OPENGL
    if (gl_ && gl_->compiled)
    {
        jassertfalse;
        DBG("[DualOutlineShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
// NOLINTNEXTLINE(readability-function-size)
bool DualOutlineShader::compile(juce::OpenGLContext& context)
{
    if (!compileFromBinaryData(*gl_, context, BinaryData::dual_outline_vert, BinaryData::dual_outline_vertSize,
                               BinaryData::dual_outline_frag, BinaryData::dual_outline_fragSize, "DualOutlineShader"))
        return false;
    return true;
}

void DualOutlineShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    releaseGLResources(*gl_);
}

bool DualOutlineShader::isCompiled() const { return gl_->compiled; }

// NOLINTNEXTLINE(readability-function-size)
void DualOutlineShader::drawChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples,
                                    float centerY, float amplitude, float boundsX, float boundsWidth, float lineWidth,
                                    GLint posLoc, GLint distLoc)
{
    juce::ignoreUnused(ext);
    std::vector<float> vertices;
    float const lineGeomWidth = lineWidth * 6.0f;
    buildLineGeometry(vertices, samples, centerY, amplitude, lineGeomWidth, boundsX, boundsWidth);

    juce::OpenGLExtensionFunctions::glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

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
}

void DualOutlineShader::render(juce::OpenGLContext& context, const std::vector<float>& channel1,
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

    juce::OpenGLExtensionFunctions::glBindVertexArray(gl_->vao);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float const height = params.bounds.getHeight();
    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amp1 = 0.0f;
    float amp2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    GLint const posLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint const distLoc =
        juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");

    if (posLoc < 0 || distLoc < 0)
    {
        jassertfalse; // Shader attributes not found
        juce::OpenGLExtensionFunctions::glBindVertexArray(0);
        juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisable(GL_BLEND);
        return;
    }

    drawChannel(ext, channel1, centerY1, amp1, params.bounds.getX(), params.bounds.getWidth(), params.lineWidth, posLoc,
                distLoc);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawChannel(ext, *channel2, centerY2, amp2, params.bounds.getX(), params.bounds.getWidth(), params.lineWidth,
                    posLoc, distLoc);
    }

    juce::OpenGLExtensionFunctions::glBindVertexArray(0);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
