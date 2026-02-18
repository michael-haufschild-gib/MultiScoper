/*
    Oscil - Neon Glow Shader Implementation
*/

#include "rendering/shaders/NeonGlowShader.h"
#include "BinaryData.h"
#include <cmath>
#include <iostream>

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

NeonGlowShader::~NeonGlowShader()
{
#if OSCIL_ENABLE_OPENGL
    // Resources should be released before destruction
    // but we can't call OpenGL functions here without context
    // Check for both compiled state AND orphaned VAO/VBO resources
    if (gl_ && (gl_->compiled || gl_->vao != 0 || gl_->vbo != 0))
    {
        // FATAL: Shader destroyed without release(context) being called.
        // This leaks VBO/VAO/Program on the GPU.
        std::cerr << "[NeonGlowShader] LEAK: Destructor called without release(). "
                  << "GPU resources may have leaked. (compiled=" << gl_->compiled
                  << ", vao=" << gl_->vao << ", vbo=" << gl_->vbo << ")" << std::endl;
        jassertfalse;  // Assert in debug builds
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
bool NeonGlowShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    juce::String vertexCode = juce::String::createStringFromData(BinaryData::neon_glow_vert, BinaryData::neon_glow_vertSize);
    juce::String fragmentCode = juce::String::createStringFromData(BinaryData::neon_glow_frag, BinaryData::neon_glow_fragSize);

    if (!gl_->program->addVertexShader(vertexCode) || !gl_->program->addFragmentShader(fragmentCode) || !gl_->program->link())
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

bool NeonGlowShader::isCompiled() const
{
    return gl_->compiled;
}

void NeonGlowShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;

    // Pure Additive Blending for light simulation
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); 

    gl_->program->use();

    // Setup projection (Matches BasicShader)
    auto* target = context.getTargetComponent();
    if (!target) return;

    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());

    if (w <= 0.0f || h <= 0.0f)
        return;

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f, // Flip Y
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
    if (gl_->glowIntensityLoc >= 0)
        ext.glUniform1f(gl_->glowIntensityLoc, params.shaderIntensity);

    // Wide expansion for the glow tail
    const float kGeometryScale = 12.0f;
    if (gl_->geometryScaleLoc >= 0)
        ext.glUniform1f(gl_->geometryScaleLoc, kGeometryScale);

    // Prepare geometry
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Layout logic
    float height = params.bounds.getHeight();
    float centerY1, centerY2;
    float amp1, amp2;

    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amp1 = halfHeight * 0.45f * params.verticalScale;
        amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amp1 = height * 0.45f * params.verticalScale;
        amp2 = amp1;
    }

    GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint distLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "distFromCenter");
    if (posLoc < 0) posLoc = 0;
    if (distLoc < 0) distLoc = 1;

    auto renderChannel = [&](const std::vector<float>& data, float cy, float amp) {
        std::vector<float> vertices;
        // Calculate the visual width including the glow
        float visualWidth = params.lineWidth * kGeometryScale;
        
        buildLineGeometry(vertices, data, cy, amp, visualWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

        ext.glEnableVertexAttribArray(static_cast<GLuint>(distLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(distLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glDisableVertexAttribArray(static_cast<GLuint>(distLoc));
    };

    renderChannel(channel1, centerY1, amp1);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        renderChannel(*channel2, centerY2, amp2);
    }

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

void NeonGlowShader::renderSoftware(
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

    // Helper lambda to draw neon glow effect (multi-pass glow simulation)
    auto drawNeonWaveform = [&](const std::vector<float>& samples, float centerY, float amp)
    {
        juce::Path path;
        float xScale = width / static_cast<float>(samples.size() - 1);

        float startY = centerY - samples[0] * amp;
        path.startNewSubPath(bounds.getX(), startY);

        for (size_t i = 1; i < samples.size(); ++i)
        {
            float x = bounds.getX() + static_cast<float>(i) * xScale;
            float y = centerY - samples[i] * amp;
            path.lineTo(x, y);
        }

        // Draw multiple passes to simulate glow effect
        // Outer glow (widest, most transparent)
        float glowIntensity = params.shaderIntensity;
        float baseWidth = params.lineWidth;

        // Pass 1: Wide outer glow
        g.setColour(params.colour.withAlpha(params.opacity * 0.15f * glowIntensity));
        g.strokePath(path, juce::PathStrokeType(baseWidth * 6.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pass 2: Medium glow
        g.setColour(params.colour.withAlpha(params.opacity * 0.3f * glowIntensity));
        g.strokePath(path, juce::PathStrokeType(baseWidth * 3.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pass 3: Inner glow (colored)
        g.setColour(params.colour.withAlpha(params.opacity * 0.7f));
        g.strokePath(path, juce::PathStrokeType(baseWidth * 1.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pass 4: Bright white core
        g.setColour(juce::Colours::white.withAlpha(params.opacity * 0.9f));
        g.strokePath(path, juce::PathStrokeType(baseWidth * 0.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    // Draw first channel
    drawNeonWaveform(channel1, centerY1, amplitude1);

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawNeonWaveform(*channel2, centerY2, amplitude2);
    }
}

} // namespace oscil
