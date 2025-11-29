/*
    Oscil - Dual Outline Shader Implementation
*/

#include "rendering/shaders/DualOutlineShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* dualVertexShader = R"(
    attribute vec2 position;
    attribute float distFromCenter;

    uniform mat4 projection;

    varying float vDistFromCenter;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
    }
)";

static const char* dualFragmentShader = R"(
    varying float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;

    void main()
    {
        float dist = abs(vDistFromCenter);

        // Inner line: Center to 0.3
        float inner = 1.0 - smoothstep(0.2, 0.3, dist);

        // Outer line: 0.6 to 0.9
        float outer = smoothstep(0.6, 0.7, dist) * (1.0 - smoothstep(0.8, 0.9, dist));

        // Combine
        float alpha = (inner + outer * 0.5) * opacity * baseColor.a;

        gl_FragColor = vec4(baseColor.rgb, alpha);
    }
)";

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

DualOutlineShader::~DualOutlineShader() = default;

#if OSCIL_ENABLE_OPENGL
bool DualOutlineShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String v = juce::OpenGLHelpers::translateVertexShaderToV3(dualVertexShader);
    juce::String f = juce::OpenGLHelpers::translateFragmentShaderToV3(dualFragmentShader);

    if (!gl_->program->addVertexShader(v) || !gl_->program->addFragmentShader(f) || !gl_->program->link())
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

void DualOutlineShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool DualOutlineShader::isCompiled() const
{
    return gl_->compiled;
}

void DualOutlineShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    juce::ignoreUnused(channel2);
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive works well for glowing outlines

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

    float height = params.bounds.getHeight();
    float centerY = params.bounds.getCentreY();
    float amp = height * 0.45f * params.verticalScale;

    // Render Channel 1
    {
        std::vector<float> vertices;
        // Make line wide enough to accommodate both inner and outer outlines
        float width = params.lineWidth * 6.0f;
        buildLineGeometry(vertices, channel1, centerY, amp, width, params.bounds.getX(), params.bounds.getWidth());
        
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
        GLint distLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
        
        if (posLoc < 0) posLoc = 0;
        if (distLoc < 0) distLoc = 1;

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        
        ext.glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        
        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
