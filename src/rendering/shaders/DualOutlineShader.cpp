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
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
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
bool DualOutlineShader::compile(juce::OpenGLContext& context)
{
    if (!compileFromBinaryData(*gl_, context, BinaryData::dual_outline_vert, BinaryData::dual_outline_vertSize,
                               BinaryData::dual_outline_frag, BinaryData::dual_outline_fragSize, "DualOutlineShader"))
        return false;

    gl_->positionLoc = juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "position");
    gl_->distFromCenterLoc =
        juce::OpenGLExtensionFunctions::glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");

    if (gl_->positionLoc < 0 || gl_->distFromCenterLoc < 0)
    {
        DBG("[DualOutlineShader] Missing required attributes");
        releaseGLResources(*gl_);
        return false;
    }

    return true;
}

void DualOutlineShader::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);
    releaseGLResources(*gl_);
}

bool DualOutlineShader::isCompiled() const { return gl_->compiled; }

void DualOutlineShader::drawChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples,
                                    const DrawArgs& args)
{
    juce::ignoreUnused(ext);
    vertexBuffer_.clear();
    float const lineGeomWidth = args.lineWidth * 6.0f;
    buildLineGeometry(vertexBuffer_, samples, args.centerY, args.amplitude, lineGeomWidth, args.boundsX,
                      args.boundsWidth);

    juce::OpenGLExtensionFunctions::glBufferData(GL_ARRAY_BUFFER,
                                                 static_cast<GLsizeiptr>(vertexBuffer_.size() * sizeof(float)),
                                                 vertexBuffer_.data(), GL_DYNAMIC_DRAW);

    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(args.posLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(static_cast<GLuint>(args.posLoc), 2, GL_FLOAT, GL_FALSE,
                                                          4 * sizeof(float), nullptr);
    juce::OpenGLExtensionFunctions::glEnableVertexAttribArray(static_cast<GLuint>(args.distLoc));
    juce::OpenGLExtensionFunctions::glVertexAttribPointer(
        static_cast<GLuint>(args.distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))); // NOLINT(performance-no-int-to-ptr)

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertexBuffer_.size() / 4));

    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(args.posLoc));
    juce::OpenGLExtensionFunctions::glDisableVertexAttribArray(static_cast<GLuint>(args.distLoc));
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

    DrawArgs args{.centerY = centerY1,
                  .amplitude = amp1,
                  .boundsX = params.bounds.getX(),
                  .boundsWidth = params.bounds.getWidth(),
                  .lineWidth = params.lineWidth,
                  .posLoc = gl_->positionLoc,
                  .distLoc = gl_->distFromCenterLoc};
    drawChannel(ext, channel1, args);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        args.centerY = centerY2;
        args.amplitude = amp2;
        drawChannel(ext, *channel2, args);
    }

    juce::OpenGLExtensionFunctions::glBindVertexArray(0);
    juce::OpenGLExtensionFunctions::glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
