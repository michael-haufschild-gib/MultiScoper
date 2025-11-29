/*
    Oscil - Waveform OpenGL Renderer Implementation
*/

#include "rendering/WaveformGLRenderer.h"

#if OSCIL_ENABLE_OPENGL

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"
#include <iostream>

namespace oscil
{

using namespace juce::gl;

// Release-mode logging macro (works in both Debug and Release)
#define GL_LOG(msg) std::cerr << "[GL] " << msg << std::endl

// Debug mode: set to true to draw colored rectangles instead of waveforms
// This bypasses shaders and tests basic GL rendering pipeline
static constexpr bool DEBUG_RENDER_MODE = false;

// Simple solid color shader for debug rendering
// Use legacy GLSL syntax - JUCE's translateToV3 handles platform compatibility
static const char* debugVertexShader = R"(
    attribute vec2 position;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(position, 0.0, 1.0);
    }
)";

static const char* debugFragmentShader = R"(
    uniform vec4 color;
    void main() {
        gl_FragColor = color;
    }
)";

WaveformGLRenderer::WaveformGLRenderer()
{
    GL_LOG("WaveformGLRenderer CREATED");
    DBG("WaveformGLRenderer created");
}

WaveformGLRenderer::~WaveformGLRenderer()
{
    GL_LOG("WaveformGLRenderer DESTROYED");
    DBG("WaveformGLRenderer destroyed");
}

void WaveformGLRenderer::setContext(juce::OpenGLContext* context)
{
    GL_LOG("setContext called, context=" << (context ? "valid" : "nullptr"));
    context_ = context;
}

void WaveformGLRenderer::newOpenGLContextCreated()
{
    GL_LOG("=== newOpenGLContextCreated CALLED ===");
    DBG("WaveformGLRenderer: OpenGL context created");

    if constexpr (DEBUG_RENDER_MODE)
    {
        GL_LOG("DEBUG_RENDER_MODE enabled, compiling debug shader");
        compileDebugShader();
    }
    else
    {
        compileShaders();
    }

    contextReady_.store(true);
    GL_LOG("contextReady_ set to TRUE");
}

void WaveformGLRenderer::compileDebugShader()
{
    GL_LOG("compileDebugShader called, context_=" << (context_ ? "valid" : "nullptr")
           << ", already compiled=" << debugShaderCompiled_);

    if (!context_ || debugShaderCompiled_)
        return;

    GL_LOG("Compiling DEBUG shader...");

    debugShader_ = std::make_unique<juce::OpenGLShaderProgram>(*context_);

    // Use JUCE's shader translation for cross-platform compatibility
    juce::String vertexShader = juce::OpenGLHelpers::translateVertexShaderToV3(debugVertexShader);
    juce::String fragmentShader = juce::OpenGLHelpers::translateFragmentShaderToV3(debugFragmentShader);

    GL_LOG("Translated vertex shader length: " << vertexShader.length());
    GL_LOG("Translated fragment shader length: " << fragmentShader.length());

    if (!debugShader_->addVertexShader(vertexShader))
    {
        GL_LOG("DEBUG SHADER: Vertex shader compilation FAILED: " << debugShader_->getLastError().toStdString());
        debugShader_.reset();
        return;
    }
    GL_LOG("DEBUG SHADER: Vertex shader compiled OK");

    if (!debugShader_->addFragmentShader(fragmentShader))
    {
        GL_LOG("DEBUG SHADER: Fragment shader compilation FAILED: " << debugShader_->getLastError().toStdString());
        debugShader_.reset();
        return;
    }
    GL_LOG("DEBUG SHADER: Fragment shader compiled OK");

    if (!debugShader_->link())
    {
        GL_LOG("DEBUG SHADER: Link FAILED");
        debugShader_.reset();
        return;
    }
    GL_LOG("DEBUG SHADER: Link OK");

    // Get uniform locations
    debugProjectionLoc_ = debugShader_->getUniformIDFromName("projection");
    debugColorLoc_ = debugShader_->getUniformIDFromName("color");

    GL_LOG("DEBUG SHADER: projection loc=" << debugProjectionLoc_ << ", color loc=" << debugColorLoc_);

    // Create VAO and VBO for debug rendering using JUCE's cross-platform extensions
    // Note: OpenGL 3.2 Core Profile is set via setOpenGLVersionRequired() before attachTo()
    auto& ext = context_->extensions;

    // Clear any pre-existing GL errors
    while (glGetError() != GL_NO_ERROR) {}

    // Create VAO (required for OpenGL 3.2 Core Profile on macOS)
    ext.glGenVertexArrays(1, &debugVAO_);
    GLenum vaoErr = glGetError();
    GL_LOG("After glGenVertexArrays: VAO=" << debugVAO_ << ", error=" << vaoErr);

    // Create VBO
    ext.glGenBuffers(1, &debugVBO_);
    GLenum vboErr = glGetError();
    GL_LOG("After glGenBuffers: VBO=" << debugVBO_ << ", error=" << vboErr);

    debugShaderCompiled_ = true;
    GL_LOG("DEBUG SHADER: Compiled successfully, VAO=" << debugVAO_ << ", VBO=" << debugVBO_);
}

void WaveformGLRenderer::compileShaders()
{
    if (!context_ || shadersCompiled_)
        return;

    DBG("WaveformGLRenderer: Compiling shaders...");
    ShaderRegistry::getInstance().compileAll(*context_);
    shadersCompiled_ = true;
    DBG("WaveformGLRenderer: Shaders compiled successfully");
}

void WaveformGLRenderer::renderOpenGL()
{
    if (!contextReady_.load())
        return;

    jassert(juce::OpenGLHelpers::isContextActive());

    // Set viewport to match the component
    const float desktopScale = static_cast<float>(context_->getRenderingScale());
    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent)
        return;

    auto componentWidth = static_cast<float>(targetComponent->getWidth());
    auto componentHeight = static_cast<float>(targetComponent->getHeight());
    auto width = static_cast<GLsizei>(componentWidth * desktopScale);
    auto height = static_cast<GLsizei>(componentHeight * desktopScale);

    glViewport(0, 0, width, height);

    // Clear the framebuffer with the background color.
    juce::OpenGLHelpers::clear(backgroundColour_);

    // Copy waveform data under lock for thread safety
    std::vector<WaveformRenderData> waveformsToRender;
    {
        const juce::SpinLock::ScopedLockType lock(dataLock_);
        waveformsToRender.reserve(waveforms_.size());
        for (const auto& pair : waveforms_)
        {
            // In debug mode, render even if channel1 is empty and not visible
            // (just draw the bounds to test GL rendering pipeline)
            if constexpr (DEBUG_RENDER_MODE)
            {
                // Only require non-empty bounds, not visible flag or data
                if (!pair.second.bounds.isEmpty())
                {
                    waveformsToRender.push_back(pair.second);
                }
            }
            else
            {
                if (pair.second.visible && !pair.second.channel1.empty())
                {
                    waveformsToRender.push_back(pair.second);
                }
            }
        }
    }

    // Debug log once per second (use GL_LOG for release-mode visibility)
    static int frameCounter = 0;
    if (++frameCounter >= 60)
    {
        frameCounter = 0;
        GL_LOG("renderOpenGL: " << waveformsToRender.size() << " waveforms, "
               << waveforms_.size() << " registered, viewport=" << width << "x" << height);
        for (const auto& data : waveformsToRender)
        {
            GL_LOG("  Waveform " << data.id << ": bounds=("
                   << data.bounds.getX() << "," << data.bounds.getY() << ","
                   << data.bounds.getWidth() << "x" << data.bounds.getHeight() << ")"
                   << ", ch1 size=" << data.channel1.size()
                   << ", visible=" << data.visible);
        }
    }

    if constexpr (DEBUG_RENDER_MODE)
    {
        // HARDCODED TEST: Always draw a bright RED rectangle at fixed position
        // This isolates whether the shader/draw call works AT ALL
        renderDebugRect(juce::Rectangle<float>(50.0f, 50.0f, 200.0f, 200.0f),
                        juce::Colour(0xFFFF0000));  // Bright red, fully opaque

        // Also draw a BLUE rectangle at a different position
        renderDebugRect(juce::Rectangle<float>(300.0f, 100.0f, 150.0f, 150.0f),
                        juce::Colour(0xFF0000FF));  // Bright blue

        // DEBUG MODE: Draw colored rectangles at waveform positions
        for (const auto& data : waveformsToRender)
        {
            renderDebugRect(data.bounds, data.colour);
        }
    }
    else
    {
        // Normal mode: render waveforms with shaders
        for (const auto& data : waveformsToRender)
        {
            renderWaveform(data);
        }
    }
}

void WaveformGLRenderer::renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour)
{
    // Check preconditions and log failures once per second
    static int entryLogCounter = 0;
    bool shouldLogEntry = (++entryLogCounter >= 60);
    if (shouldLogEntry)
    {
        entryLogCounter = 0;
        GL_LOG("renderDebugRect entry: shader=" << (debugShader_ ? "valid" : "null")
               << ", compiled=" << debugShaderCompiled_
               << ", context=" << (context_ ? "valid" : "null"));
    }

    if (!debugShader_ || !debugShaderCompiled_ || !context_)
        return;

    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent)
        return;

    auto& ext = context_->extensions;

    // Log once per second
    static int debugFrameCounter = 0;
    bool shouldLog = (++debugFrameCounter >= 60);
    if (shouldLog) debugFrameCounter = 0;

    // Check for any pre-existing GL errors
    GLenum preError = glGetError();
    if (shouldLog && preError != GL_NO_ERROR)
        GL_LOG("Pre-existing GL error: " << preError);

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

    if (shouldLog)
        GL_LOG("renderDebugRect: viewport=" << viewportWidth << "x" << viewportHeight
               << ", bounds=(" << bounds.getX() << "," << bounds.getY()
               << "," << bounds.getWidth() << "x" << bounds.getHeight() << ")"
               << ", projLoc=" << debugProjectionLoc_ << ", colorLoc=" << debugColorLoc_
               << ", VAO=" << debugVAO_ << ", VBO=" << debugVBO_);

    // Create orthographic projection matrix (column-major for OpenGL)
    // Maps (0,0) at top-left to (viewportWidth, viewportHeight) at bottom-right
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

    // Build quad vertices (two triangles)
    float x1 = bounds.getX();
    float y1 = bounds.getY();
    float x2 = bounds.getRight();
    float y2 = bounds.getBottom();

    float vertices[] = {
        x1, y1,  // top-left
        x2, y1,  // top-right
        x1, y2,  // bottom-left
        x2, y1,  // top-right
        x2, y2,  // bottom-right
        x1, y2   // bottom-left
    };

    debugShader_->use();
    GLenum errUse = glGetError();
    if (shouldLog && errUse != GL_NO_ERROR)
        GL_LOG("Error after shader use: " << errUse);

    // Get the actual OpenGL program ID and uniform locations directly
    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);
    if (shouldLog)
        GL_LOG("Current program ID: " << programID);

    // Get uniform locations directly from OpenGL
    GLint projLoc = ext.glGetUniformLocation(programID, "projection");
    GLint colorLoc = ext.glGetUniformLocation(programID, "color");
    if (shouldLog)
        GL_LOG("Direct uniform locations: proj=" << projLoc << ", color=" << colorLoc);

    // Set uniforms using direct locations
    ext.glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    GLenum errProj = glGetError();
    if (shouldLog && errProj != GL_NO_ERROR)
        GL_LOG("Error after projection uniform: " << errProj);

    ext.glUniform4f(colorLoc,
        colour.getFloatRed(),
        colour.getFloatGreen(),
        colour.getFloatBlue(),
        1.0f);  // Full opacity for debugging
    GLenum errColor = glGetError();
    if (shouldLog && errColor != GL_NO_ERROR)
        GL_LOG("Error after color uniform: " << errColor);

    // Disable depth test - we want 2D overlay
    glDisable(GL_DEPTH_TEST);

    // Clear any pre-existing errors
    while (glGetError() != GL_NO_ERROR) {}

    // Bind VAO (required for OpenGL 3.2 Core Profile)
    ext.glBindVertexArray(debugVAO_);

    // Bind VBO and upload vertex data
    ext.glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
    ext.glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // Get the attribute location from the shader
    GLint positionLoc = ext.glGetAttribLocation(static_cast<GLuint>(programID), "position");
    if (shouldLog)
        GL_LOG("Position attribute location: " << positionLoc);

    // Only enable and configure the vertex attribute if it exists (location >= 0)
    if (positionLoc >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    }
    GLenum errAttrib = glGetError();
    if (shouldLog && errAttrib != GL_NO_ERROR)
        GL_LOG("Error after vertex attrib setup: " << errAttrib);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GLenum errDraw = glGetError();
    if (errDraw != GL_NO_ERROR)
        GL_LOG("ERROR after draw: " << errDraw);  // Always log errors

    if (shouldLog)
    {
        GL_LOG("Draw call completed");
        // Log vertex data being sent
        GL_LOG("Vertices: (" << x1 << "," << y1 << ") to (" << x2 << "," << y2 << ")");
        GL_LOG("Color: R=" << colour.getFloatRed() << " G=" << colour.getFloatGreen()
               << " B=" << colour.getFloatBlue() << " A=1.0");
    }

    // Cleanup - only disable the attribute if it was enabled
    if (positionLoc >= 0)
        ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void WaveformGLRenderer::renderWaveform(const WaveformRenderData& data)
{
    if (!context_ || data.channel1.size() < 2)
        return;

    // Get shader from registry
    auto* shader = ShaderRegistry::getInstance().getShader(data.shaderId);
    if (!shader)
    {
        shader = ShaderRegistry::getInstance().getShader(
            ShaderRegistry::getInstance().getDefaultShaderId());
    }

    if (!shader || !shader->isCompiled())
    {
        DBG("WaveformGLRenderer: Shader not available or not compiled: " << data.shaderId);
        return;
    }

    // Set up render parameters
    ShaderRenderParams params;
    params.colour = data.colour;
    params.opacity = data.opacity;
    params.lineWidth = data.lineWidth;
    params.bounds = data.bounds;
    params.isStereo = data.isStereo;
    params.verticalScale = data.verticalScale;

    // Render using the GPU shader
    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void WaveformGLRenderer::openGLContextClosing()
{
    DBG("WaveformGLRenderer: OpenGL context closing");
    contextReady_.store(false);

    // Release debug shader resources
    if (context_ != nullptr)
    {
        auto& ext = context_->extensions;

        if (debugVBO_ != 0)
        {
            ext.glDeleteBuffers(1, &debugVBO_);
            debugVBO_ = 0;
        }

        if (debugVAO_ != 0)
        {
            ext.glDeleteVertexArrays(1, &debugVAO_);
            debugVAO_ = 0;
        }
    }
    debugShader_.reset();
    debugShaderCompiled_ = false;

    // Release all shader resources (context is still active at this point)
    if (context_ != nullptr)
    {
        ShaderRegistry::getInstance().releaseAll(*context_);
    }
    shadersCompiled_ = false;
}

void WaveformGLRenderer::registerWaveform(int id)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    if (waveforms_.find(id) == waveforms_.end())
    {
        WaveformRenderData data;
        data.id = id;
        waveforms_[id] = data;
        DBG("WaveformGLRenderer: Registered waveform " << id);
    }
}

void WaveformGLRenderer::unregisterWaveform(int id)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    waveforms_.erase(id);
    DBG("WaveformGLRenderer: Unregistered waveform " << id);
}

void WaveformGLRenderer::updateWaveform(const WaveformRenderData& data)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    waveforms_[data.id] = data;
}

void WaveformGLRenderer::setBackgroundColour(juce::Colour colour)
{
    backgroundColour_ = colour;
}

int WaveformGLRenderer::getWaveformCount() const
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    return static_cast<int>(waveforms_.size());
}

void WaveformGLRenderer::clearAllWaveforms()
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    waveforms_.clear();
    DBG("WaveformGLRenderer: Cleared all waveforms");
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
