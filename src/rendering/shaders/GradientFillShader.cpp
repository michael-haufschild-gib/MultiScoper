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

GradientFillShader::~GradientFillShader()
{
#if OSCIL_ENABLE_OPENGL
    // Resources should be released before destruction
    // but we can't call OpenGL functions here without context
    // Check for both compiled state AND orphaned VAO/VBO resources
    if (gl_ && (gl_->compiled || gl_->vao != 0 || gl_->vbo != 0))
    {
        // FATAL: Shader destroyed without release(context) being called.
        // This leaks VBO/VAO/Program on the GPU.
        std::cerr << "[GradientFillShader] LEAK: Destructor called without release(). "
                  << "GPU resources may have leaked. (compiled=" << gl_->compiled
                  << ", vao=" << gl_->vao << ", vbo=" << gl_->vbo << ")" << std::endl;
        jassertfalse;  // Assert in debug builds
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
bool GradientFillShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String vertexCode = juce::String::createStringFromData(BinaryData::gradient_fill_vert, BinaryData::gradient_fill_vertSize);
    juce::String fragmentCode = juce::String::createStringFromData(BinaryData::gradient_fill_frag, BinaryData::gradient_fill_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) || !gl_->program->addFragmentShader(fragmentCode) || !gl_->program->link())
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
    // Delete VBO if allocated (check != 0 to avoid double-free)
    if (gl_->vbo != 0)
    {
        context.extensions.glDeleteBuffers(1, &gl_->vbo);
        gl_->vbo = 0;  // Reset to prevent double-free
    }

    // Delete VAO if allocated (check != 0 to avoid double-free)
    if (gl_->vao != 0)
    {
        context.extensions.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;  // Reset to prevent double-free
    }

    // Release shader program
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

    if (w <= 0.0f || h <= 0.0f)
        return;

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    // Set uniforms with validation (H25 fix: validate before use)
    if (gl_->projectionLoc >= 0)
        ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    if (gl_->baseColorLoc >= 0)
        ext.glUniform4f(gl_->baseColorLoc,
            params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    if (gl_->opacityLoc >= 0)
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

void GradientFillShader::renderSoftware(
    juce::Graphics& g,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (channel1.size() < 2)
        return;

    auto bounds = params.bounds;
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    // Calculate layout based on stereo/mono
    float centerY1, centerY2;
    float amplitude1, amplitude2;

    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = bounds.getY() + halfHeight * 0.5f;
        centerY2 = bounds.getY() + halfHeight * 1.5f;
        amplitude1 = halfHeight * 0.45f * params.verticalScale;
        amplitude2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = bounds.getCentreY();
        centerY2 = centerY1;
        amplitude1 = height * 0.45f * params.verticalScale;
        amplitude2 = amplitude1;
    }

    // Helper lambda to draw a filled waveform with gradient
    auto drawFilledWaveform = [&](const std::vector<float>& samples, float centerY, float amp)
    {
        juce::Path path;
        float xScale = width / static_cast<float>(samples.size() - 1);

        // Start from bottom-left corner at the baseline
        path.startNewSubPath(bounds.getX(), centerY);

        // Draw the waveform line
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float x = bounds.getX() + static_cast<float>(i) * xScale;
            float y = centerY - samples[i] * amp;
            path.lineTo(x, y);
        }

        // Close path back to baseline
        path.lineTo(bounds.getRight(), centerY);
        path.closeSubPath();

        // Fill with gradient from color at top to transparent at bottom
        juce::ColourGradient gradient(
            params.colour.withAlpha(params.opacity),
            0.0f, centerY - amp,  // Top (full color)
            params.colour.withAlpha(0.0f),
            0.0f, centerY,  // Bottom (transparent)
            false);

        g.setGradientFill(gradient);
        g.fillPath(path);
    };

    // Draw first channel
    drawFilledWaveform(channel1, centerY1, amplitude1);

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawFilledWaveform(*channel2, centerY2, amplitude2);
    }
}

} // namespace oscil
