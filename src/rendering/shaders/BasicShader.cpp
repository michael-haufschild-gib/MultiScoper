/*
    Oscil - Basic Shader Implementation
*/

#include "rendering/shaders/BasicShader.h"
#include "rendering/FramebufferPool.h"  // For VertexBufferPool
#include "BinaryData.h"
#include <cmath>
#include <iostream>

namespace oscil
{

// Release-mode logging macro (works in both Debug and Release)
#define BASIC_LOG(msg) std::cerr << "[BASIC] " << msg << std::endl

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

    // Logging counter (per-instance, not static)
    int renderLogCounter = 0;
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
    // Check for both compiled state AND orphaned VAO/VBO resources
    if (gl_ && (gl_->compiled || gl_->vao != 0 || gl_->vbo != 0))
    {
        // FATAL: Shader destroyed without release(context) being called.
        // This leaks VBO/VAO/Program on the GPU.
        std::cerr << "[BasicShader] LEAK: Destructor called without release(). "
                  << "GPU resources may have leaked. (compiled=" << gl_->compiled
                  << ", vao=" << gl_->vao << ", vbo=" << gl_->vbo << ")" << std::endl;
        jassertfalse;  // Assert in debug builds
    }
#endif
}

#if OSCIL_ENABLE_OPENGL
bool BasicShader::compile(juce::OpenGLContext& context)
{
    BASIC_LOG("compile() called, already compiled=" << gl_->compiled);

    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    BASIC_LOG("Compiling shaders (GLSL 3.30 Core)...");

    juce::String vertexCode = juce::String::createStringFromData(BinaryData::basic_vert, BinaryData::basic_vertSize);
    if (!gl_->program->addVertexShader(vertexCode))
    {
        BASIC_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    BASIC_LOG("Vertex shader compiled OK");

    juce::String fragmentCode = juce::String::createStringFromData(BinaryData::basic_frag, BinaryData::basic_fragSize);
    if (!gl_->program->addFragmentShader(fragmentCode))
    {
        BASIC_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    BASIC_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())

    {
        BASIC_LOG("Shader program linking FAILED: " << gl_->program->getLastError().toStdString());
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

    BASIC_LOG("Created VAO=" << gl_->vao << ", VBO=" << gl_->vbo);

    // Check for GL errors after VAO/VBO creation
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        BASIC_LOG("GL error after VAO/VBO creation: " << err);
        // Clean up any resources that were created
        if (gl_->vbo != 0)
        {
            context.extensions.glDeleteBuffers(1, &gl_->vbo);
            gl_->vbo = 0;
        }
        if (gl_->vao != 0)
        {
            context.extensions.glDeleteVertexArrays(1, &gl_->vao);
            gl_->vao = 0;
        }
        gl_->program.reset();
        return false;
    }

    // Validate VAO/VBO were actually created (non-zero handles)
    if (gl_->vao == 0 || gl_->vbo == 0)
    {
        BASIC_LOG("Failed to create VAO/VBO: vao=" << gl_->vao << ", vbo=" << gl_->vbo);
        if (gl_->vbo != 0)
        {
            context.extensions.glDeleteBuffers(1, &gl_->vbo);
            gl_->vbo = 0;
        }
        if (gl_->vao != 0)
        {
            context.extensions.glDeleteVertexArrays(1, &gl_->vao);
            gl_->vao = 0;
        }
        gl_->program.reset();
        return false;
    }

    gl_->compiled = true;
    BASIC_LOG("Shader fully initialized, compiled=true");
    return true;
}

void BasicShader::release(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    // Always clean up VAO/VBO if they were created, regardless of compile status
    // This prevents resource leaks if compile() fails after creating these resources
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

void BasicShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    // Log once per second to avoid spamming (per-instance counter)
    bool shouldLog = (++gl_->renderLogCounter >= 60);
    if (shouldLog) gl_->renderLogCounter = 0;

    if (shouldLog)
        BASIC_LOG("render() called, compiled=" << gl_->compiled << ", ch1 size=" << channel1.size());

    if (!gl_->compiled || channel1.size() < 2)
    {
        if (shouldLog)
            BASIC_LOG("render() early exit: compiled=" << gl_->compiled << ", samples=" << channel1.size());
        return;
    }

    auto& ext = context.extensions;

    // Enable ADDITIVE blending for glow effect (light adds up, creating luminosity)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Use our shader program
    gl_->program->use();

    // Set up orthographic projection using FULL VIEWPORT dimensions (not waveform bounds)
    // The viewport is set to the entire editor, so we need to map editor coordinates to NDC.
    // The waveform geometry is built with editor-relative coordinates (from populateGLRenderData).
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
        return;

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f)
        return;

    // Projection maps (0, 0) -> (-1, 1) and (viewportWidth, viewportHeight) -> (1, -1)
    // This creates an orthographic projection for the full editor coordinate space
    float left = 0.0f;
    float right = viewportWidth;
    float top = 0.0f;
    float bottom = viewportHeight;

    // Create orthographic projection matrix
    // Layout: Column-major order as required by OpenGL (glUniformMatrix4fv with GL_FALSE)
    //
    // Matrix math: P = | 2/(r-l)    0         0    -(r+l)/(r-l) |
    //                  |   0     2/(t-b)      0    -(t+b)/(t-b) |
    //                  |   0        0        -1         0        |
    //                  |   0        0         0         1        |
    //
    // Column-major 1D array layout: [col0.x, col0.y, col0.z, col0.w, col1.x, ...]
    // This matches GLSL's mat4 column-major convention.
    //
    // Shader expects: gl_Position = projection * vec4(position, 0.0, 1.0)
    // where position is in screen coordinates (0,0 = top-left).
    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,                         // Column 0
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,                         // Column 1
        0.0f, 0.0f, -1.0f, 0.0f,                                         // Column 2
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f  // Column 3
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);

    // Set color uniforms
    ext.glUniform4f(gl_->baseColorLoc,
        params.colour.getFloatRed(),
        params.colour.getFloatGreen(),
        params.colour.getFloatBlue(),
        params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->glowIntensityLoc, GLOW_INTENSITY);

    // Calculate layout
    float height = params.bounds.getHeight();
    float centerY1, centerY2;
    float amplitude1, amplitude2;

    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amplitude1 = halfHeight * 0.45f * params.verticalScale;
        amplitude2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amplitude1 = height * 0.45f * params.verticalScale;
        amplitude2 = amplitude1;
    }

    // Get the OpenGL program ID for attribute lookup
    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);

    // Get attribute locations dynamically (JUCE's translation may rename them)
    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    GLint distFromCenterLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "distFromCenter");

    // Fallback to index 0 and 1 if not found (should not happen with correct shader)
    if (positionLoc < 0) positionLoc = 0;
    if (distFromCenterLoc < 0) distFromCenterLoc = 1;

    // Check if we should use the pooled VBO for batched rendering
    bool usePool = (vertexBufferPool_ != nullptr && vertexBufferPool_->isInitialized());

    if (usePool)
    {
        // Pool is already bound by caller (RenderEngine) - just use it
        // Note: VAO and VBO are bound by pool->bind() before batch starts
    }
    else
    {
        // Use shader's own VAO/VBO (non-batched path)
        ext.glBindVertexArray(gl_->vao);
        ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);
    }

    // Build and render channel 1
    // Use geometry wide enough for glow to fade out
    // The fragment shader creates the thin core line, the geometry provides space for the glow halo
    std::vector<float> vertices;
    // Scale glow width based on lineWidth: base 15px at default 1.5 lineWidth, proportionally scaled
    float glowWidth = params.lineWidth * 10.0f;
    buildLineGeometry(vertices, channel1, centerY1, amplitude1,
        glowWidth, params.bounds.getX(), params.bounds.getWidth());

    ptrdiff_t ch1Offset = 0;
    size_t ch1VertexCount = vertices.size() / 4;

    if (usePool)
    {
        // Allocate in pool (uploads via glBufferSubData)
        ch1Offset = vertexBufferPool_->allocate(vertices.data(), ch1VertexCount);
        if (ch1Offset < 0)
        {
            // Pool full - fallback to own VBO
            usePool = false;
            ext.glBindVertexArray(gl_->vao);
            ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);
            ext.glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                vertices.data(), GL_DYNAMIC_DRAW);
            ch1Offset = 0;
        }
    }
    else
    {
        // Upload to shader's own VBO
        ext.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(), GL_DYNAMIC_DRAW);
    }

    // Set up vertex attributes with appropriate offset
    void* offsetPtr = reinterpret_cast<void*>(ch1Offset);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 
        4 * sizeof(float), offsetPtr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 
        4 * sizeof(float), reinterpret_cast<void*>(ch1Offset + 2 * sizeof(float)));

    if (shouldLog)
        BASIC_LOG("Drawing ch1: " << ch1VertexCount << " verts, bounds=("
                 << params.bounds.getX() << "," << params.bounds.getY() << ","
                 << params.bounds.getWidth() << "x" << params.bounds.getHeight() << ")"
                 << ", posLoc=" << positionLoc << ", distLoc=" << distFromCenterLoc
                 << ", usePool=" << (usePool ? "true" : "false"));

    // Draw with multiple passes for enhanced glow
    for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
    {
        float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
        ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(ch1VertexCount));
    }

    // Check for GL errors after draw
    GLenum err = glGetError();
    if (shouldLog && err != GL_NO_ERROR)
        BASIC_LOG("GL error after draw: " << err);
    else if (shouldLog)
        BASIC_LOG("Draw completed OK");

    // Render channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        if (shouldLog)
            BASIC_LOG("Drawing ch2: isStereo=" << params.isStereo << ", ch2 size=" << channel2->size());

        vertices.clear();
        buildLineGeometry(vertices, *channel2, centerY2, amplitude2,
            glowWidth, params.bounds.getX(), params.bounds.getWidth());

        size_t ch2VertexCount = vertices.size() / 4;
        ptrdiff_t ch2Offset = 0;

        if (usePool)
        {
            ch2Offset = vertexBufferPool_->allocate(vertices.data(), ch2VertexCount);
            if (ch2Offset < 0)
            {
                // Pool full - skip channel 2 (or could fallback, but this is rare)
                if (shouldLog)
                    BASIC_LOG("ch2 skipped: pool full");
            }
            else
            {
                // Update vertex attribute pointers for ch2 offset
                void* ch2OffsetPtr = reinterpret_cast<void*>(ch2Offset);
                ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE,
                    4 * sizeof(float), ch2OffsetPtr);
                ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE,
                    4 * sizeof(float), reinterpret_cast<void*>(ch2Offset + 2 * sizeof(float)));

                if (shouldLog)
                    BASIC_LOG("Drawing ch2: " << ch2VertexCount << " verts, centerY2=" << centerY2);

                for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
                {
                    float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
                    ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(ch2VertexCount));
                }
            }
        }
        else
        {
            // Non-pooled path
            if (shouldLog)
                BASIC_LOG("Drawing ch2: " << ch2VertexCount << " verts, centerY2=" << centerY2);

            ext.glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                vertices.data(), GL_DYNAMIC_DRAW);

            for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
            {
                float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
                ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(ch2VertexCount));
            }
        }
    }
    else if (shouldLog && params.isStereo)
    {
        BASIC_LOG("ch2 NOT rendered: channel2=" << (channel2 ? "valid" : "null")
                 << ", ch2 size=" << (channel2 ? channel2->size() : 0));
    }

    // Cleanup - only unbind if we bound our own VAO
    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    if (!usePool)
    {
        ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
        ext.glBindVertexArray(0);
    }
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
