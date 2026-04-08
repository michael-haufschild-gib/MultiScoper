/*
    Oscil - Gradient Fill Shader Implementation
*/

#include "rendering/shaders/GradientFillShader.h"

#include "BinaryData.h"

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct GradientFillShader::GLResources : WaveformShader::WaveformGLResources
{
};
#endif

GradientFillShader::GradientFillShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

GradientFillShader::~GradientFillShader()
{
#if OSCIL_ENABLE_OPENGL
    if (gl_ && gl_->compiled)
    {
        jassertfalse;
        DBG("[GradientFillShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
// NOLINTNEXTLINE(readability-function-size)
bool GradientFillShader::compile(juce::OpenGLContext& context)
{
    if (!compileFromBinaryData(*gl_, context, BinaryData::gradient_fill_vert, BinaryData::gradient_fill_vertSize,
                               BinaryData::gradient_fill_frag, BinaryData::gradient_fill_fragSize,
                               "GradientFillShader"))
        return false;
    return true;
}

void GradientFillShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    releaseGLResources(*gl_);
}

bool GradientFillShader::isCompiled() const { return gl_->compiled; }

void GradientFillShader::drawFillChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples,
                                         float centerY, float amplitude, float boundsX, float boundsWidth, GLint posLoc,
                                         GLint vLoc)
{
    juce::ignoreUnused(ext);
    std::vector<float> vertices;
    buildFillGeometry(vertices, samples, centerY, centerY, amplitude, boundsX, boundsWidth);

    juce::OpenGLExtensionFunctions::glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE,
                                                          4 * sizeof(float), nullptr);
    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(
        static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))); // NOLINT(performance-no-int-to-ptr)

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
}

void GradientFillShader::render(juce::OpenGLContext& context, const std::vector<float>& channel1,
                                const std::vector<float>* channel2, const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    juce::OpenGLExtensionFunctions::glBindVertexArray(gl_->vao);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float const height = params.bounds.getHeight();
    float centerY1 = 0.0f;
    float centerY2 = 0.0f;
    float amp1 = 0.0f;
    float amp2 = 0.0f;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    GLint const posLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint const vLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "vParam");

    if (posLoc < 0 || vLoc < 0)
    {
        DBG("[GradientFillShader] Missing attributes: position=" << posLoc << " vParam=" << vLoc);
        juce::OpenGLExtensionFunctions::glBindVertexArray(0);
        juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisable(GL_BLEND);
        return;
    }

    drawFillChannel(ext, channel1, centerY1, amp1, params.bounds.getX(), params.bounds.getWidth(), posLoc, vLoc);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawFillChannel(ext, *channel2, centerY2, amp2, params.bounds.getX(), params.bounds.getWidth(), posLoc, vLoc);
    }

    juce::OpenGLExtensionFunctions::glBindVertexArray(0);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
