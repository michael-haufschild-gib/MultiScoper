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

struct DualOutlineShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
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
    if (gl_->compiled)
        return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String vertexCode =
        juce::String::createStringFromData(BinaryData::dual_outline_vert, BinaryData::dual_outline_vertSize);
    juce::String fragmentCode =
        juce::String::createStringFromData(BinaryData::dual_outline_frag, BinaryData::dual_outline_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) || !gl_->program->addFragmentShader(fragmentCode) ||
        !gl_->program->link())
    {
        DBG("DualOutlineShader: Shader compilation failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");

    if (gl_->projectionLoc < 0 || gl_->baseColorLoc < 0 || gl_->opacityLoc < 0)
    {
        DBG("DualOutlineShader: Missing required uniforms");
        gl_->program.reset();
        return false;
    }

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void DualOutlineShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool DualOutlineShader::isCompiled() const { return gl_->compiled; }

void DualOutlineShader::drawChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples,
                                    float centerY, float amplitude, float boundsX, float boundsWidth, float lineWidth,
                                    GLint posLoc, GLint distLoc)
{
    std::vector<float> vertices;
    float lineGeomWidth = lineWidth * 6.0f;
    buildLineGeometry(vertices, samples, centerY, amplitude, lineGeomWidth, boundsX, boundsWidth);

    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(),
                     GL_DYNAMIC_DRAW);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void*) (2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

    ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
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

    ext.glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(), params.colour.getFloatGreen(),
                    params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float height = params.bounds.getHeight();
    float centerY1, centerY2, amp1, amp2;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    GLint posLoc = std::max(0, static_cast<int>(ext.glGetAttribLocation(gl_->program->getProgramID(), "position")));
    GLint distLoc =
        std::max(1, static_cast<int>(ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter")));

    drawChannel(ext, channel1, centerY1, amp1, params.bounds.getX(), params.bounds.getWidth(), params.lineWidth, posLoc,
                distLoc);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawChannel(ext, *channel2, centerY2, amp2, params.bounds.getX(), params.bounds.getWidth(), params.lineWidth,
                    posLoc, distLoc);
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
