/*
    Oscil - Neon Glow Shader Implementation
*/

#include "rendering/shaders/NeonGlowShader.h"
#include <cmath>
#include <iostream>

namespace oscil
{

// Release-mode logging macro (works in both Debug and Release)
#define NEON_LOG(msg) std::cerr << "[NEON] " << msg << std::endl

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;
// GLSL shader sources - using legacy syntax for JUCE translation
// JUCE's translateToV3 will convert these for the current OpenGL version
static const char* vertexShaderSource = R"(
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

static const char* fragmentShaderSource = R"(
    varying float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float glowIntensity;

    void main()
    {
        // Use the actual base color from the oscillator
        vec3 neonColor = baseColor.rgb;

        float dist = abs(vDistFromCenter);

        // 80s Neon effect: thin bright core with colored glow halo
        // Core line - very thin bright center
        float core = 1.0 - smoothstep(0.0, 0.08, dist);

        // Glow falloff - smooth exponential decay for the halo
        float glow = exp(-dist * 6.0) * glowIntensity;

        // The core is slightly brighter/saturated version of the color
        // The glow keeps the color but fades out
        vec3 coreColor = neonColor * 1.5;  // Brighten core
        vec3 glowColor = neonColor;         // Glow stays true to color

        // Mix: bright core fading into colored glow
        vec3 finalColor = mix(glowColor * glow, coreColor, core);

        // Alpha: solid core, fading glow
        float alpha = opacity * (core + glow * 0.7) * baseColor.a;

        // Clamp to prevent over-saturation
        finalColor = clamp(finalColor, 0.0, 1.0);

        gl_FragColor = vec4(finalColor, alpha);
    }
)";

struct NeonGlowShader::GLResources
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
#endif
}

#if OSCIL_ENABLE_OPENGL
bool NeonGlowShader::compile(juce::OpenGLContext& context)
{
    NEON_LOG("compile() called, already compiled=" << gl_->compiled);

    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    // Use JUCE's shader translation for cross-platform compatibility
    juce::String translatedVertex = juce::OpenGLHelpers::translateVertexShaderToV3(vertexShaderSource);
    juce::String translatedFragment = juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentShaderSource);

    NEON_LOG("Compiling with JUCE translation...");
    NEON_LOG("Vertex shader length: " << translatedVertex.length());
    NEON_LOG("Fragment shader length: " << translatedFragment.length());

    if (!gl_->program->addVertexShader(translatedVertex))
    {
        NEON_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    NEON_LOG("Vertex shader compiled OK");

    if (!gl_->program->addFragmentShader(translatedFragment))
    {
        NEON_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    NEON_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())
    {
        NEON_LOG("Shader program linking FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }

    NEON_LOG("Shader compiled and linked successfully");

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    // Log uniform locations for debugging
    NEON_LOG("Uniform locations - projection=" << gl_->projectionLoc
             << ", baseColor=" << gl_->baseColorLoc
             << ", opacity=" << gl_->opacityLoc
             << ", glowIntensity=" << gl_->glowIntensityLoc);

    // Validate uniform locations - critical uniforms must be found
    bool uniformsValid = true;
    if (gl_->projectionLoc < 0)
    {
        NEON_LOG("Failed to find uniform 'projection'");
        uniformsValid = false;
    }
    if (gl_->baseColorLoc < 0)
    {
        NEON_LOG("Failed to find uniform 'baseColor'");
        uniformsValid = false;
    }
    if (gl_->opacityLoc < 0)
    {
        NEON_LOG("Failed to find uniform 'opacity'");
        uniformsValid = false;
    }
    if (gl_->glowIntensityLoc < 0)
    {
        NEON_LOG("Failed to find uniform 'glowIntensity'");
        uniformsValid = false;
    }

    if (!uniformsValid)
    {
        NEON_LOG("Shader compilation failed - missing uniforms");
        gl_->program.reset();
        return false;
    }

    // Create VAO and VBO (required for OpenGL 3.2 Core Profile on macOS)
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    NEON_LOG("Created VAO=" << gl_->vao << ", VBO=" << gl_->vbo);

    gl_->compiled = true;
    NEON_LOG("Shader fully initialized, compiled=true");
    return true;
}

void NeonGlowShader::release(juce::OpenGLContext& context)
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
    // Log once per second to avoid spamming
    static int renderLogCounter = 0;
    bool shouldLog = (++renderLogCounter >= 60);
    if (shouldLog) renderLogCounter = 0;

    if (shouldLog)
        NEON_LOG("render() called, compiled=" << gl_->compiled << ", ch1 size=" << channel1.size());

    if (!gl_->compiled || channel1.size() < 2)
    {
        if (shouldLog)
            NEON_LOG("render() early exit: compiled=" << gl_->compiled << ", samples=" << channel1.size());
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

    // Projection maps (0, 0) -> (-1, 1) and (viewportWidth, viewportHeight) -> (1, -1)
    // This creates an orthographic projection for the full editor coordinate space
    float left = 0.0f;
    float right = viewportWidth;
    float top = 0.0f;
    float bottom = viewportHeight;

    // Create orthographic projection matrix (column-major for OpenGL)
    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
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

    // Bind VAO
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Build and render channel 1
    // Use geometry wide enough for glow to fade out
    // The fragment shader creates the thin core line, the geometry provides space for the glow halo
    std::vector<float> vertices;
    // Scale glow width based on lineWidth: base 15px at default 1.5 lineWidth, proportionally scaled
    float glowWidth = params.lineWidth * 10.0f;
    buildLineGeometry(vertices, channel1, centerY1, amplitude1,
        glowWidth, params.bounds.getX(), params.bounds.getWidth());

    ext.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes using dynamic locations
    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    if (shouldLog)
        NEON_LOG("Drawing ch1: " << vertices.size() / 4 << " verts, bounds=("
                 << params.bounds.getX() << "," << params.bounds.getY() << ","
                 << params.bounds.getWidth() << "x" << params.bounds.getHeight() << ")"
                 << ", posLoc=" << positionLoc << ", distLoc=" << distFromCenterLoc);

    // Draw with multiple passes for enhanced glow
    for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
    {
        float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
        ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
    }

    // Check for GL errors after draw
    GLenum err = glGetError();
    if (shouldLog && err != GL_NO_ERROR)
        NEON_LOG("GL error after draw: " << err);
    else if (shouldLog)
        NEON_LOG("Draw completed OK");

    // Render channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        vertices.clear();
        buildLineGeometry(vertices, *channel2, centerY2, amplitude2,
            glowWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(), GL_DYNAMIC_DRAW);

        for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
        {
            float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
            ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
        }
    }

    // Cleanup
    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
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
