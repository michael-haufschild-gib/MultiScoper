/*
    Oscil - Neon Glow Shader Implementation
*/

#include "rendering/shaders/NeonGlowShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;
// GLSL shader sources
static const char* vertexShaderSource = R"(
    #version 330 core

    layout(location = 0) in vec2 position;
    layout(location = 1) in float distFromCenter;

    uniform mat4 projection;

    out float vDistFromCenter;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
    }
)";

static const char* fragmentShaderSource = R"(
    #version 330 core

    in float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float glowIntensity;

    out vec4 fragColor;

    void main()
    {
        // Core line is at center (distFromCenter near 0)
        // Edge is at distFromCenter near +/- 1

        float dist = abs(vDistFromCenter);

        // Core line - full color at center
        float core = 1.0 - smoothstep(0.0, 0.3, dist);

        // Glow - exponential falloff from center
        float glow = exp(-dist * 3.0) * glowIntensity;

        // Combine core and glow
        float intensity = core + glow;

        // Brighten color for glow effect (HDR-like)
        vec3 glowColor = baseColor.rgb * (1.0 + glow * 2.0);

        // Soft clamp for HDR bloom feel
        glowColor = glowColor / (glowColor + vec3(0.5));

        // Final alpha based on intensity
        float alpha = opacity * intensity * baseColor.a;

        fragColor = vec4(glowColor, alpha);
    }
)";

struct NeonGlowShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint glowIntensityLoc = -1;
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
    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    if (!compileShaderProgram(*gl_->program, vertexShaderSource, fragmentShaderSource))
    {
        gl_->program.reset();
        return false;
    }

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->glowIntensityLoc = gl_->program->getUniformIDFromName("glowIntensity");

    // Create VAO and VBO
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void NeonGlowShader::release()
{
    if (!gl_->compiled)
        return;

    if (gl_->vao != 0)
    {
        // Note: We need a current context to delete these
        // This should be called while OpenGL context is active
        gl_->vao = 0;
        gl_->vbo = 0;
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
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;

    // Enable blending for transparency and glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use our shader program
    gl_->program->use();

    // Set up orthographic projection
    float left = params.bounds.getX();
    float right = params.bounds.getRight();
    float top = params.bounds.getY();
    float bottom = params.bounds.getBottom();

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
        amplitude1 = halfHeight * 0.45f;
        amplitude2 = halfHeight * 0.45f;
    }
    else
    {
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amplitude1 = height * 0.45f;
        amplitude2 = amplitude1;
    }

    // Bind VAO
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Build and render channel 1
    std::vector<float> vertices;
    buildLineGeometry(vertices, channel1, centerY1, amplitude1,
        params.lineWidth * 2.0f, static_cast<int>(params.bounds.getWidth()));

    ext.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes
    ext.glEnableVertexAttribArray(0);  // position
    ext.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(1);  // distFromCenter
    ext.glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    // Draw with multiple passes for enhanced glow
    for (int pass = GLOW_PASSES - 1; pass >= 0; --pass)
    {
        float passIntensity = GLOW_INTENSITY * static_cast<float>(GLOW_PASSES - pass) / GLOW_PASSES;
        ext.glUniform1f(gl_->glowIntensityLoc, passIntensity);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
    }

    // Render channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        vertices.clear();
        buildLineGeometry(vertices, *channel2, centerY2, amplitude2,
            params.lineWidth * 2.0f, static_cast<int>(params.bounds.getWidth()));

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
    ext.glDisableVertexAttribArray(0);
    ext.glDisableVertexAttribArray(1);
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
        amplitude1 = halfHeight * 0.45f;
        amplitude2 = halfHeight * 0.45f;
    }
    else
    {
        centerY1 = bounds.getCentreY();
        centerY2 = centerY1;
        amplitude1 = height * 0.45f;
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
