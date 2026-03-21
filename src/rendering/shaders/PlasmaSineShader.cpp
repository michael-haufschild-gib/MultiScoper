/*
    Oscil - Plasma Sine Shader Implementation
*/

#include "rendering/shaders/PlasmaSineShader.h"
#include "BinaryData.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct PlasmaSineShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint timeLoc = -1;
    GLint passIndexLoc = -1;
    GLint jitterAmountLoc = -1;
    GLint jitterSpeedLoc = -1;
};
#endif

PlasmaSineShader::PlasmaSineShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

PlasmaSineShader::~PlasmaSineShader() = default;

#if OSCIL_ENABLE_OPENGL
bool PlasmaSineShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String vertexCode = juce::String::createStringFromData(BinaryData::plasma_sine_vert, BinaryData::plasma_sine_vertSize);
    juce::String fragmentCode = juce::String::createStringFromData(BinaryData::plasma_sine_frag, BinaryData::plasma_sine_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) ||
        !gl_->program->addFragmentShader(fragmentCode) ||
        !gl_->program->link())
    {
        DBG("PlasmaSineShader: Shader compilation failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->timeLoc = gl_->program->getUniformIDFromName("time");
    gl_->passIndexLoc = gl_->program->getUniformIDFromName("passIndex");
    gl_->jitterAmountLoc = gl_->program->getUniformIDFromName("jitterAmount");
    gl_->jitterSpeedLoc = gl_->program->getUniformIDFromName("jitterSpeed");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void PlasmaSineShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool PlasmaSineShader::isCompiled() const
{
    return gl_->compiled;
}

void PlasmaSineShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    
    // Additive Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); 
    glDisable(GL_CULL_FACE);

    gl_->program->use();

    auto* target = context.getTargetComponent();
    if (!target) return;

    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());

    if (w <= 0.0f || h <= 0.0f)
        return;

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->timeLoc, params.time);

    float height = params.bounds.getHeight();
    float centerY1, centerY2, amp1, amp2;
    calculateStereoLayout(params, channel2, height, centerY1, centerY2, amp1, amp2);

    // Attributes
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint vLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "vParam");
    GLint tLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "tParam");

    if (posLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    }
    if (vLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    if (tLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(tLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(tLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    // Helper: Draw Jittered Layers
    auto drawChannel = [&](const std::vector<float>& channel, float centerY, float amp) {
        std::vector<float> vertices;
        
        // --- LAYER 1: OUTER HAZE (Static, Wide) ---
        float hazeWidth = 250.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, hazeWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 0);
        ext.glUniform1f(gl_->jitterAmountLoc, 0.0f); // No jitter for haze
        ext.glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        // --- LAYER 2: ELECTRIC ARCS (High Jitter, Medium Width) ---
        vertices.clear();
        float arcWidth = 80.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, arcWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 1);
        ext.glUniform1f(gl_->jitterAmountLoc, 30.0f * params.verticalScale); // Heavy Jitter
        ext.glUniform1f(gl_->jitterSpeedLoc, 5.0f);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        // --- LAYER 3: CORE (No Jitter, Thin) ---
        vertices.clear();
        float coreWidth = 6.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, coreWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 2);
        ext.glUniform1f(gl_->jitterAmountLoc, 0.0f); // Stable core
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
    };

    // Render Channels
    drawChannel(channel1, centerY1, amp1);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawChannel(*channel2, centerY2, amp2);
    }

    if (posLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    if (vLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
    if (tLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(tLoc));

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil
