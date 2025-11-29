/*
    Oscil - Vector Flow Shader Implementation
*/

#include "rendering/shaders/VectorFlowShader.h"
#include <cmath>
#include <iostream>

namespace oscil
{

// Release-mode logging macro (works in both Debug and Release)
#define FLOW_LOG(msg) std::cerr << "[FLOW] " << msg << std::endl

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

// GLSL shader sources - using legacy syntax for JUCE translation
static const char* vertexShaderSource = R"(
    attribute vec2 position;
    attribute float distFromCenter;
    attribute float pathPosition;

    uniform mat4 projection;

    varying float vDistFromCenter;
    varying float vPathPosition;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
        vPathPosition = pathPosition;
    }
)";

static const char* fragmentShaderSource = R"(
    varying float vDistFromCenter;
    varying float vPathPosition;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float time;
    uniform float segmentLength;
    uniform float gapLength;

    void main()
    {
        // Calculate dash pattern based on path position and time
        float patternLength = segmentLength + gapLength;
        float animatedPos = vPathPosition + time;
        float patternPos = mod(animatedPos, patternLength);
        
        // Smooth edges for anti-aliasing
        float dashAlpha = smoothstep(0.0, 0.02, patternPos) * 
                          smoothstep(segmentLength + 0.02, segmentLength, patternPos);
        
        // Distance-based edge softening
        float dist = abs(vDistFromCenter);
        float edgeAlpha = 1.0 - smoothstep(0.7, 1.0, dist);
        
        // Combine alphas
        float alpha = opacity * dashAlpha * edgeAlpha * baseColor.a;
        
        gl_FragColor = vec4(baseColor.rgb, alpha);
    }
)";

struct VectorFlowShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint timeLoc = -1;
    GLint segmentLengthLoc = -1;
    GLint gapLengthLoc = -1;

    // Attribute locations
    GLint positionLoc = -1;
    GLint distFromCenterLoc = -1;
    GLint pathPositionLoc = -1;
};

// Helper to build geometry with path position for dashed effect
static void buildFlowGeometry(
    std::vector<float>& vertices,
    const std::vector<float>& samples,
    float centerY,
    float amplitude,
    float lineWidth,
    float boundsX,
    float boundsWidth);
#endif

VectorFlowShader::VectorFlowShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

VectorFlowShader::~VectorFlowShader()
{
#if OSCIL_ENABLE_OPENGL
    // Resources should be released before destruction
    // but we can't call OpenGL functions here without context
#endif
}

void VectorFlowShader::update(float deltaTime)
{
    time_ += deltaTime * flowSpeed_;
    
    // Wrap time to prevent floating point precision issues
    const float wrapThreshold = 1000.0f;
    if (time_ > wrapThreshold)
    {
        time_ = std::fmod(time_, segmentLength_ + gapLength_);
    }
}

#if OSCIL_ENABLE_OPENGL
bool VectorFlowShader::compile(juce::OpenGLContext& context)
{
    FLOW_LOG("compile() called, already compiled=" << gl_->compiled);

    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    // Use JUCE's shader translation for cross-platform compatibility
    juce::String translatedVertex = juce::OpenGLHelpers::translateVertexShaderToV3(vertexShaderSource);
    juce::String translatedFragment = juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentShaderSource);

    FLOW_LOG("Compiling with JUCE translation...");

    if (!gl_->program->addVertexShader(translatedVertex))
    {
        FLOW_LOG("Vertex shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    FLOW_LOG("Vertex shader compiled OK");

    if (!gl_->program->addFragmentShader(translatedFragment))
    {
        FLOW_LOG("Fragment shader compilation FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }
    FLOW_LOG("Fragment shader compiled OK");

    if (!gl_->program->link())
    {
        FLOW_LOG("Shader program linking FAILED: " << gl_->program->getLastError().toStdString());
        gl_->program.reset();
        return false;
    }

    FLOW_LOG("Shader compiled and linked successfully");

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->timeLoc = gl_->program->getUniformIDFromName("time");
    gl_->segmentLengthLoc = gl_->program->getUniformIDFromName("segmentLength");
    gl_->gapLengthLoc = gl_->program->getUniformIDFromName("gapLength");

    // Log uniform locations for debugging
    FLOW_LOG("Uniform locations - projection=" << gl_->projectionLoc
             << ", baseColor=" << gl_->baseColorLoc
             << ", opacity=" << gl_->opacityLoc
             << ", time=" << gl_->timeLoc);

    // Validate uniform locations - critical uniforms must be found
    bool uniformsValid = true;
    if (gl_->projectionLoc < 0)
    {
        FLOW_LOG("Failed to find uniform 'projection'");
        uniformsValid = false;
    }
    if (gl_->baseColorLoc < 0)
    {
        FLOW_LOG("Failed to find uniform 'baseColor'");
        uniformsValid = false;
    }
    if (gl_->opacityLoc < 0)
    {
        FLOW_LOG("Failed to find uniform 'opacity'");
        uniformsValid = false;
    }

    if (!uniformsValid)
    {
        FLOW_LOG("Shader compilation failed - missing uniforms");
        gl_->program.reset();
        return false;
    }

    // Create VAO and VBO (required for OpenGL 3.2 Core Profile on macOS)
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    FLOW_LOG("Created VAO=" << gl_->vao << ", VBO=" << gl_->vbo);

    gl_->compiled = true;
    FLOW_LOG("Shader fully initialized, compiled=true");
    return true;
}

void VectorFlowShader::release(juce::OpenGLContext& context)
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

bool VectorFlowShader::isCompiled() const
{
    return gl_->compiled;
}

void VectorFlowShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use our shader program
    gl_->program->use();

    // Set up orthographic projection using FULL VIEWPORT dimensions
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
        return;

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

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
    
    // Set animation uniforms
    if (gl_->timeLoc >= 0)
        ext.glUniform1f(gl_->timeLoc, time_);
    if (gl_->segmentLengthLoc >= 0)
        ext.glUniform1f(gl_->segmentLengthLoc, segmentLength_);
    if (gl_->gapLengthLoc >= 0)
        ext.glUniform1f(gl_->gapLengthLoc, gapLength_);

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

    // Get attribute locations dynamically
    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    GLint distFromCenterLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "distFromCenter");
    GLint pathPositionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "pathPosition");

    // Fallback if not found
    if (positionLoc < 0) positionLoc = 0;
    if (distFromCenterLoc < 0) distFromCenterLoc = 1;
    if (pathPositionLoc < 0) pathPositionLoc = 2;

    // Bind VAO
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Build and render channel 1
    std::vector<float> vertices;
    buildFlowGeometry(vertices, channel1, centerY1, amplitude1,
        params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

    ext.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes (5 floats per vertex: x, y, distFromCenter, pathPosition, reserved)
    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc), 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    ext.glEnableVertexAttribArray(static_cast<GLuint>(pathPositionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(pathPositionLoc), 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float)));

    // Draw channel 1
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 5));

    // Render channel 2 if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        vertices.clear();
        buildFlowGeometry(vertices, *channel2, centerY2, amplitude2,
            params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 5));
    }

    // Cleanup
    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(pathPositionLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}

// Helper to build geometry with path position for dashed effect
static void buildFlowGeometry(
    std::vector<float>& vertices,
    const std::vector<float>& samples,
    float centerY,
    float amplitude,
    float lineWidth,
    float boundsX,
    float boundsWidth)
{
    if (samples.size() < 2)
        return;

    // Reserve space for triangle strip (2 vertices per sample point, 5 floats per vertex)
    vertices.reserve(samples.size() * 2 * 5);

    float xScale = boundsWidth / static_cast<float>(samples.size() - 1);
    float halfWidth = lineWidth * 0.5f;
    float pathPosition = 0.0f;

    for (size_t i = 0; i < samples.size(); ++i)
    {
        float x = boundsX + static_cast<float>(i) * xScale;
        float y = centerY - samples[i] * amplitude;

        // Calculate normal direction
        float nx = 0.0f, ny = 1.0f;

        if (i > 0 && i < samples.size() - 1)
        {
            float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
            float prevY = centerY - samples[i - 1] * amplitude;
            float nextX = boundsX + (static_cast<float>(i) + 1.0f) * xScale;
            float nextY = centerY - samples[i + 1] * amplitude;

            float dx = nextX - prevX;
            float dy = nextY - prevY;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
            
            // Accumulate path length
            if (i > 0)
            {
                float segDx = x - prevX;
                float segDy = y - prevY;
                pathPosition += std::sqrt(segDx * segDx + segDy * segDy) / boundsWidth;
            }
        }
        else if (i == 0 && samples.size() > 1)
        {
            float nextX = boundsX + xScale;
            float nextY = centerY - samples[1] * amplitude;
            float dx = nextX - x;
            float dy = nextY - y;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
        }
        else if (i == samples.size() - 1 && samples.size() > 1)
        {
            float prevX = boundsX + (static_cast<float>(i) - 1.0f) * xScale;
            float prevY = centerY - samples[i - 1] * amplitude;
            float dx = x - prevX;
            float dy = y - prevY;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f)
            {
                nx = -dy / len;
                ny = dx / len;
            }
            
            pathPosition += std::sqrt(dx * dx + dy * dy) / boundsWidth;
        }

        // Vertex 1: position + normal offset (top of line)
        vertices.push_back(x + nx * halfWidth);
        vertices.push_back(y + ny * halfWidth);
        vertices.push_back(1.0f);       // Distance from center
        vertices.push_back(pathPosition); // Path position for dash animation
        vertices.push_back(0.0f);       // Reserved

        // Vertex 2: position - normal offset (bottom of line)
        vertices.push_back(x - nx * halfWidth);
        vertices.push_back(y - ny * halfWidth);
        vertices.push_back(-1.0f);      // Distance from center
        vertices.push_back(pathPosition); // Path position for dash animation
        vertices.push_back(0.0f);       // Reserved
    }
}
#endif

void VectorFlowShader::renderSoftware(
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

    // Helper to draw a flowing dashed waveform
    auto drawFlowingWaveform = [&](const std::vector<float>& samples, float centerY, float amplitude)
    {
        // Calculate dash pattern lengths in pixels
        float dashLength = segmentLength_ * width;
        float gapPixels = gapLength_ * width;
        float patternLength = dashLength + gapPixels;
        
        // Animated offset
        float offset = std::fmod(time_ * width, patternLength);
        
        // Set up dashed stroke
        juce::Array<float> dashLengths;
        dashLengths.add(dashLength);
        dashLengths.add(gapPixels);
        
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

        g.setColour(params.colour.withAlpha(params.opacity));
        
        // Create a dashed stroke type
        juce::PathStrokeType strokeType(params.lineWidth,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        
        // Create dashed path
        juce::Path dashedPath;
        strokeType.createDashedStroke(dashedPath, path, dashLengths.data(), 
            dashLengths.size(), juce::AffineTransform::translation(-offset, 0.0f));
        
        g.fillPath(dashedPath);
    };

    // Draw first channel
    drawFlowingWaveform(channel1, centerY1, amplitude1);

    // Draw second channel if stereo
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawFlowingWaveform(*channel2, centerY2, amplitude2);
    }
}

} // namespace oscil
