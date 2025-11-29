/*
    Oscil - Holo Matrix Shader Implementation
*/

#include "rendering/shaders/HoloMatrixShader.h"
#include <cmath>
#include <iostream>

namespace oscil
{

// Release-mode logging macro (works in both Debug and Release)
#define HOLO_LOG(msg) std::cerr << "[HOLO] " << msg << std::endl

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

// GLSL shader sources - using legacy syntax for JUCE translation
static const char* vertexShaderSource = R"(
    attribute vec2 position;
    attribute float distFromCenter;

    uniform mat4 projection;
    uniform float time;

    varying float vDistFromCenter;
    varying float vTime;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
        vTime = time;
    }
)";

static const char* fragmentShaderSource = R"(
    varying float vDistFromCenter;
    varying float vTime;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float matrixIntensity;

    void main()
    {
        vec3 holoColor = baseColor.rgb;

        float dist = abs(vDistFromCenter);

        // Matrix-style scanline effect
        float scanline = sin(gl_FragCoord.y * 0.5 + vTime * 2.0) * 0.5 + 0.5;

        // Crystal pattern based on time
        float crystal = sin(dist * 20.0 + vTime * 3.0) * 0.3 + 0.7;

        // Core line - thin bright center
        float core = 1.0 - smoothstep(0.0, 0.1, dist);

        // Glow with matrix pattern
        float glow = exp(-dist * 5.0) * matrixIntensity * crystal;

        // Apply scanline effect
        glow *= (0.7 + scanline * 0.3);

        vec3 coreColor = holoColor * 1.4;
        vec3 glowColor = holoColor * crystal;

        vec3 finalColor = mix(glowColor * glow, coreColor, core);

        float alpha = opacity * (core + glow * 0.6) * baseColor.a;

        finalColor = clamp(finalColor, 0.0, 1.0);

        gl_FragColor = vec4(finalColor, alpha);
    }
)";

struct HoloMatrixShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint matrixIntensityLoc = -1;
    GLint timeLoc = -1;

    // Attribute locations
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
};
#endif

HoloMatrixShader::HoloMatrixShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

HoloMatrixShader::~HoloMatrixShader()
{
#if OSCIL_ENABLE_OPENGL
    // Resources should be released before destruction
#endif
}

void HoloMatrixShader::update(float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;

    time_ += deltaTime;

    // Wrap time to prevent overflow after long periods
    constexpr float TWO_PI = 6.283185307f;
    constexpr float TIME_WRAP = TWO_PI * 100.0f;
    if (time_ > TIME_WRAP)
        time_ -= TIME_WRAP;
}

#if OSCIL_ENABLE_OPENGL
bool HoloMatrixShader::compile(juce::OpenGLContext& context)
{
    HOLO_LOG("compile() called, already compiled=" << gl_->compiled);

    if (gl_->compiled)
        return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    juce::String translatedVertex = juce::OpenGLHelpers::translateVertexShaderToV3(vertexShaderSource);
    juce::String translatedFragment = juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentShaderSource);

    HOLO_LOG("Compiling with JUCE translation...");

    if (!gl_->program->addVertexShader(translatedVertex))
    {
        HOLO_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    HOLO_LOG("Vertex shader compiled OK");

    if (!gl_->program->addFragmentShader(translatedFragment))
    {
        HOLO_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    HOLO_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())
    {
        HOLO_LOG("Shader program linking FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }

    HOLO_LOG("Shader compiled and linked successfully");

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->matrixIntensityLoc = gl_->program->getUniformIDFromName("matrixIntensity");
    gl_->timeLoc = gl_->program->getUniformIDFromName("time");

    HOLO_LOG("Uniform locations - projection=" << gl_->projectionLoc
             << ", baseColor=" << gl_->baseColorLoc
             << ", opacity=" << gl_->opacityLoc
             << ", matrixIntensity=" << gl_->matrixIntensityLoc
             << ", time=" << gl_->timeLoc);

    // Validate uniform locations
    bool uniformsValid = true;
    if (gl_->projectionLoc < 0)
    {
        HOLO_LOG("Failed to find uniform 'projection'");
        uniformsValid = false;
    }
    if (gl_->baseColorLoc < 0)
    {
        HOLO_LOG("Failed to find uniform 'baseColor'");
        uniformsValid = false;
    }

    if (!uniformsValid)
    {
        HOLO_LOG("Shader compilation failed - missing uniforms");
        gl_->program.reset();
        return false;
    }

    // Create VAO and VBO
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    HOLO_LOG("Created VAO=" << gl_->vao << ", VBO=" << gl_->vbo);

    gl_->compiled = true;
    HOLO_LOG("Shader fully initialized, compiled=true");
    return true;
}

void HoloMatrixShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;

    auto& ext = context.extensions;

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

bool HoloMatrixShader::isCompiled() const
{
    return gl_->compiled;
}

void HoloMatrixShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    static int renderLogCounter = 0;
    bool shouldLog = (++renderLogCounter >= 60);
    if (shouldLog) renderLogCounter = 0;

    if (shouldLog)
        HOLO_LOG("render() called, compiled=" << gl_->compiled << ", ch1 size=" << channel1.size());

    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;

    // Enable additive blending for holographic effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    gl_->program->use();

    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
        return;

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

    float left = 0.0f;
    float right = viewportWidth;
    float top = 0.0f;
    float bottom = viewportHeight;

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);

    ext.glUniform4f(gl_->baseColorLoc,
        params.colour.getFloatRed(),
        params.colour.getFloatGreen(),
        params.colour.getFloatBlue(),
        params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->matrixIntensityLoc, MATRIX_INTENSITY);
    ext.glUniform1f(gl_->timeLoc, time_);

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

    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);

    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    GLint distFromCenterLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "distFromCenter");

    if (positionLoc < 0) positionLoc = 0;
    if (distFromCenterLoc < 0) distFromCenterLoc = 1;

    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Build and render channel 1
    vertices_.clear();
    float glowWidth = params.lineWidth * 10.0f;
    buildLineGeometry(vertices_, channel1, centerY1, amplitude1,
        glowWidth, params.bounds.getX(), params.bounds.getWidth());

    ext.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
        vertices_.data(), GL_DYNAMIC_DRAW);

    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices_.size() / 4));

    // Render channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        vertices_.clear();
        buildLineGeometry(vertices_, *channel2, centerY2, amplitude2,
            glowWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
            vertices_.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices_.size() / 4));
    }

    // Cleanup
    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}
#endif

void HoloMatrixShader::renderSoftware(
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

    // Helper to draw holographic waveform
    auto drawHoloWaveform = [&](const std::vector<float>& samples, float centerY, float amplitude)
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

        // Draw with matrix effect approximation
        // Outer glow
        g.setColour(params.colour.withAlpha(params.opacity * 0.2f));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth * 4.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Inner glow
        g.setColour(params.colour.withAlpha(params.opacity * 0.5f));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth * 2.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Core line
        g.setColour(params.colour.withAlpha(params.opacity));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth));

        // Bright center
        g.setColour(params.colour.brighter(0.4f).withAlpha(params.opacity * 0.9f));
        g.strokePath(path, juce::PathStrokeType(params.lineWidth * 0.5f));
    };

    // Draw first channel
    drawHoloWaveform(channel1, centerY1, amplitude1);

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawHoloWaveform(*channel2, centerY2, amplitude2);
    }
}

} // namespace oscil
