/*
    Oscil - Gradient Fill Shader Implementation
*/

#include "rendering/shaders/GradientFillShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* fillVertexShader = R"(
    #version 330 core
    in vec2 position;
    in float vParam; // 0 at baseline, 1 at waveform
    in float tParam; // 0 to 1 along wave

    uniform mat4 projection;

    out float vV;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vV = vParam;
    }
)";

static const char* fillFragmentShader = R"(
    #version 330 core
    in float vV;

    uniform vec4 baseColor;
    uniform float opacity;

    out vec4 fragColor;

    void main()
    {
        // Gradient from baseColor (at top) to transparent (at bottom)
        vec4 topColor = baseColor;
        vec4 bottomColor = vec4(baseColor.rgb, 0.0);
        
        // Apply opacity
        topColor.a *= opacity;
        bottomColor.a *= opacity;

        // Simple linear interpolation
        fragColor = mix(bottomColor, topColor, vV);
    }
)";

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
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    if (!gl_->program->addVertexShader(fillVertexShader) || !gl_->program->addFragmentShader(fillFragmentShader) || !gl_->program->link())
    {
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
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool GradientFillShader::isCompiled() const
{
    return gl_->compiled;
}

void GradientFillShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl_->program->use();

    auto* target = context.getTargetComponent();
    if (!target) return;

    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    ext.glUniform4f(gl_->baseColorLoc,
        params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Calculate layout based on stereo/mono mode
    float height = params.bounds.getHeight();
    float centerY1, centerY2;
    float amp1, amp2;

    if (params.isStereo && channel2 != nullptr)
    {
        // Stereo stacked layout: L on top half, R on bottom half
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amp1 = halfHeight * 0.45f * params.verticalScale;
        amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        // Mono layout: single channel centered
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amp1 = height * 0.45f * params.verticalScale;
        amp2 = amp1;
    }

    // Get attribute locations once
    GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint vLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "vParam");
    if (posLoc < 0) posLoc = 0;
    if (vLoc < 0) vLoc = 1;

    // Render Channel 1
    {
        std::vector<float> vertices;
        // Fill down to centerY (zero line for this channel)
        buildFillGeometry(vertices, channel1, centerY1, centerY1, amp1, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
    }

    // Render Channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        std::vector<float> vertices;
        buildFillGeometry(vertices, *channel2, centerY2, centerY2, amp2, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
