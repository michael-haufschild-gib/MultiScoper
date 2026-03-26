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

struct NeonGlowShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint glowIntensityLoc = -1;
    GLint geometryScaleLoc = -1;
};
#endif

NeonGlowShader::NeonGlowShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

NeonGlowShader::~NeonGlowShader() = default;

#if OSCIL_ENABLE_OPENGL
bool NeonGlowShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled)
        return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String vertexCode =
        juce::String::createStringFromData(BinaryData::neon_glow_vert, BinaryData::neon_glow_vertSize);
    juce::String fragmentCode =
        juce::String::createStringFromData(BinaryData::neon_glow_frag, BinaryData::neon_glow_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) || !gl_->program->addFragmentShader(fragmentCode) ||
        !gl_->program->link())
    {
        DBG("NeonGlowShader compile error: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");
    gl_->geometryScaleLoc = gl_->program->getUniformIDFromName("geometryScale");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void NeonGlowShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
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
    ext.glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(), params.colour.getFloatGreen(),
                    params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->glowIntensityLoc, params.shaderIntensity);
    ext.glUniform1f(gl_->geometryScaleLoc, kGeometryScale);

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float height = params.bounds.getHeight();
    float centerY1, centerY2, amp1, amp2;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    auto programId = gl_->program->getProgramID();
    GLint posLoc = std::max(GLint{0}, ext.glGetAttribLocation(programId, "position"));
    GLint distLoc = std::max(GLint{1}, ext.glGetAttribLocation(programId, "distFromCenter"));

    float visualWidth = params.lineWidth * kGeometryScale;
    auto renderChannel = [&](const std::vector<float>& data, float cy, float amp) {
        std::vector<float> vertices;
        buildLineGeometry(vertices, data, cy, amp, visualWidth, params.bounds.getX(), params.bounds.getWidth());
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
    };

    renderChannel(channel1, centerY1, amp1);
    if (params.isStereo && channel2 && channel2->size() >= 2)
        renderChannel(*channel2, centerY2, amp2);

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
