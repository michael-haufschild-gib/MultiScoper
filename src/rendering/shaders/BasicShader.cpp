/*
    Oscil - Basic Shader Implementation
*/

#include "rendering/shaders/BasicShader.h"
#include "BinaryData.h"
#include <cmath>

namespace oscil
{

// Debug-only logging macro — no output in release builds
#define BASIC_LOG(msg) DBG("[BASIC] " << msg)

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

struct BasicShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations (get these after compilation)
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint glowIntensityLoc = -1;

    // Attribute locations (get these after compilation)
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};
#endif

BasicShader::BasicShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

BasicShader::~BasicShader()
{
#if OSCIL_ENABLE_OPENGL
    // Resources should be released before destruction
    // but we can't call OpenGL functions here without context
    if (gl_ && gl_->compiled)
    {
        // GPU resource leak: VBO/VAO/Program not released before destruction.
        // jassertfalse fires in debug builds only to catch this during development.
        jassertfalse;
        DBG("[BasicShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
bool BasicShader::compile(juce::OpenGLContext& context)
{
    BASIC_LOG("compile() called, already compiled=" << static_cast<int>(gl_->compiled));

    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    BASIC_LOG("Compiling shaders (GLSL 3.30 Core)...");

    juce::String vertexCode = juce::String::createStringFromData(BinaryData::basic_vert, BinaryData::basic_vertSize);
    if (!gl_->program->addVertexShader(vertexCode))
    {
        BASIC_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }
    BASIC_LOG("Vertex shader compiled OK");

    juce::String fragmentCode = juce::String::createStringFromData(BinaryData::basic_frag, BinaryData::basic_fragSize);
    if (!gl_->program->addFragmentShader(fragmentCode))
    {
        BASIC_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }
    BASIC_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())

    {
        BASIC_LOG("Shader program linking FAILED: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    BASIC_LOG("Shader compiled and linked successfully");

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    // Log uniform locations for debugging
    BASIC_LOG("Uniform locations - projection=" << gl_->projectionLoc
             << ", baseColor=" << gl_->baseColorLoc
             << ", opacity=" << gl_->opacityLoc
             << ", glowIntensity=" << gl_->glowIntensityLoc);

    // Validate uniform locations - critical uniforms must be found
    bool uniformsValid = true;
    if (gl_->projectionLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'projection'");
        uniformsValid = false;
    }
    if (gl_->baseColorLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'baseColor'");
        uniformsValid = false;
    }
    if (gl_->opacityLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'opacity'");
        uniformsValid = false;
    }
    if (gl_->glowIntensityLoc < 0)
    {
        BASIC_LOG("Failed to find uniform 'glowIntensity'");
        uniformsValid = false;
    }

    if (!uniformsValid)
    {
        BASIC_LOG("Shader compilation failed - missing uniforms");
        gl_->program.reset();
        return false;
    }

    // Create VAO and VBO (required for OpenGL 3.2 Core Profile on macOS)
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    BASIC_LOG("Created VAO=" << static_cast<int>(gl_->vao) << ", VBO=" << static_cast<int>(gl_->vbo));

    gl_->compiled = true;
    BASIC_LOG("Shader fully initialized, compiled=true");
    return true;
}

void BasicShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;

    auto& ext = context.extensions;

    // Properly delete VAO and VBO to prevent resource leaks
    if (gl_->vbo != 0)
    {
        ext.glDeleteBuffers(1, &gl_->vbo);
        gl_->vbo = 0;
    }

    if (gl_->vao != 0)
    {
        ext.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
    }

    gl_->program.reset();
    gl_->compiled = false;
}

bool BasicShader::isCompiled() const
{
    return gl_->compiled;
}

void BasicShader::drawGlowPasses(juce::OpenGLExtensionFunctions& ext, int vertexCount)
{
    for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
    {
        float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
        ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
    }
}

void BasicShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    gl_->program->use();

    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent) return;

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());
    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) return;

    float projection[16] = {
        2.0f / viewportWidth, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / viewportHeight, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);

    ext.glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(),
        params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->glowIntensityLoc, GLOW_INTENSITY);

    float height = params.bounds.getHeight();
    float centerY1, centerY2, amplitude1, amplitude2;
    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amplitude1 = amplitude2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = centerY2 = params.bounds.getCentreY();
        amplitude1 = amplitude2 = height * 0.45f * params.verticalScale;
    }

    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);
    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    GLint distFromCenterLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "distFromCenter");
    if (positionLoc < 0) positionLoc = 0;
    if (distFromCenterLoc < 0) distFromCenterLoc = 1;

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    float glowWidth = params.lineWidth * 10.0f;
    std::vector<float> vertices;
    buildLineGeometry(vertices, channel1, centerY1, amplitude1,
        glowWidth, params.bounds.getX(), params.bounds.getWidth());

    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_DYNAMIC_DRAW);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    drawGlowPasses(ext, static_cast<int>(vertices.size() / 4));

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        vertices.clear();
        buildLineGeometry(vertices, *channel2, centerY2, amplitude2,
            glowWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(), GL_DYNAMIC_DRAW);
        drawGlowPasses(ext, static_cast<int>(vertices.size() / 4));
    }

    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}
#endif

void BasicShader::renderSoftware(
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

    // Calculate layout
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

    // Helper to draw a glowing waveform
    auto drawGlowingWaveform = [&](const std::vector<float>& samples, float centerY, float amplitude)
    {
        juce::Path path;
        float xScale = width / static_cast<float>(samples.size() - 1);

        float startY = centerY - samples[0] * amplitude;
        path.startNewSubPath(bounds.getX(), startY);

        for (size_t i = 1; i < samples.size(); ++i)
        {
            float x = bounds.getX() + static_cast<float>(i) * xScale;
            float y = centerY - samples[i] * amplitude;
            path.lineTo(x, y);
        }

        // Draw glow layers (outer to inner, increasing opacity)
        for (int pass = GLOW_PASSES; pass >= 0; --pass)
        {
            float glowWidth = params.lineWidth * (1.0f + static_cast<float>(pass) * 1.5f);
            float alpha = params.opacity * (0.15f + 0.25f * (1.0f - static_cast<float>(pass) / GLOW_PASSES));

            g.setColour(params.colour.withAlpha(alpha));
            g.strokePath(path, juce::PathStrokeType(glowWidth,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Draw core line
        g.setColour(params.colour.withAlpha(params.opacity));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth));

        // Draw bright center for neon effect
        g.setColour(params.colour.brighter(0.5f).withAlpha(params.opacity * 0.8f));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth * 0.5f));
    };

    // Draw first channel
    drawGlowingWaveform(channel1, centerY1, amplitude1);

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawGlowingWaveform(*channel2, centerY2, amplitude2);
    }
}

} // namespace oscil
