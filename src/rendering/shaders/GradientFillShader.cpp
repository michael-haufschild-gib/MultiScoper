/*
    Oscil - Gradient Fill Shader Implementation
*/

#include "rendering/shaders/GradientFillShader.h"

#include "BinaryData.h"

#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct GradientFillShader::GLResources
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

GradientFillShader::GradientFillShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

GradientFillShader::~GradientFillShader() = default;

#if OSCIL_ENABLE_OPENGL
bool GradientFillShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled)
        return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String vertexCode =
        juce::String::createStringFromData(BinaryData::gradient_fill_vert, BinaryData::gradient_fill_vertSize);
    juce::String fragmentCode =
        juce::String::createStringFromData(BinaryData::gradient_fill_frag, BinaryData::gradient_fill_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) || !gl_->program->addFragmentShader(fragmentCode) ||
        !gl_->program->link())
    {
        DBG("GradientFillShader: Shader compilation failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void GradientFillShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool GradientFillShader::isCompiled() const { return gl_->compiled; }

void GradientFillShader::drawFillChannel(juce::OpenGLExtensionFunctions& ext, const std::vector<float>& samples,
                                         float centerY, float amplitude, float boundsX, float boundsWidth, GLint posLoc,
                                         GLint vLoc)
{
    std::vector<float> vertices;
    buildFillGeometry(vertices, samples, centerY, centerY, amplitude, boundsX, boundsWidth);

    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(),
                     GL_DYNAMIC_DRAW);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void*) (2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

    ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
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
    GLint vLoc = std::max(1, static_cast<int>(ext.glGetAttribLocation(gl_->program->getProgramID(), "vParam")));

    drawFillChannel(ext, channel1, centerY1, amp1, params.bounds.getX(), params.bounds.getWidth(), posLoc, vLoc);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawFillChannel(ext, *channel2, centerY2, amp2, params.bounds.getX(), params.bounds.getWidth(), posLoc, vLoc);
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
