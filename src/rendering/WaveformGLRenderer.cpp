/*
    Oscil - Waveform OpenGL Renderer Implementation
*/

#include "rendering/WaveformGLRenderer.h"

#if OSCIL_ENABLE_OPENGL

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"
#include "rendering/RenderEngine.h"
#include <iostream>
#include <chrono>
#include <unordered_set>

namespace oscil
{

using namespace juce::gl;

// Log to stderr only in Debug builds to avoid spamming Release output
#if JUCE_DEBUG
    #define GL_LOG(msg) std::cerr << "[GL] " << msg << std::endl
#else
    #define GL_LOG(msg)
#endif

// Debug mode: set to true to draw colored rectangles instead of waveforms
// This bypasses shaders and tests basic GL rendering pipeline
static constexpr bool DEBUG_RENDER_MODE = false;

// Simple solid color shader for debug rendering
static const char* debugVertexShader = R"(
    #version 330 core
    in vec2 position;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(position, 0.0, 1.0);
    }
)";

static const char* debugFragmentShader = R"(
    #version 330 core
    uniform vec4 color;
    out vec4 fragColor;
    void main() {
        fragColor = color;
    }
)";

WaveformGLRenderer::WaveformGLRenderer()
{
    GL_LOG("WaveformGLRenderer CREATED");
    DBG("WaveformGLRenderer created");
}

WaveformGLRenderer::~WaveformGLRenderer()
{
    // Ensure cleanup happens even if openGLContextClosing() callback wasn't invoked
    // (e.g., in headless test environments where JUCE callbacks may not fire)
    openGLContextClosing();

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

    // Initialize the advanced render engine
    if (useRenderEngine_ && context_)
    {
        const juce::ScopedWriteLock lock(engineLock_);
        renderEngine_ = std::make_unique<RenderEngine>();
        if (!renderEngine_->initialize(*context_))
        {
            GL_LOG("WARNING: RenderEngine initialization failed, falling back to basic rendering");
            DBG("WaveformGLRenderer: RenderEngine initialization failed");
            renderEngine_.reset();
        }
        else
        {
            GL_LOG("RenderEngine initialized successfully");
            DBG("WaveformGLRenderer: RenderEngine initialized");
        }
    }

    // Initialize frame timing
    lastFrameTime_ = std::chrono::steady_clock::now();

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

    if (!debugShader_->addVertexShader(debugVertexShader))
    {
        GL_LOG("DEBUG SHADER: Vertex shader compilation FAILED: " << debugShader_->getLastError().toStdString());
        debugShader_.reset();
        return;
    }
    GL_LOG("DEBUG SHADER: Vertex shader compiled OK");

    if (!debugShader_->addFragmentShader(debugFragmentShader))
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

void WaveformGLRenderer::renderOpenGL()
{
    if (!contextReady_.load())
        return;

    jassert(juce::OpenGLHelpers::isContextActive());

    // Clear the default framebuffer (screen) to transparent black.
    // This is CRITICAL because the RenderEngine path blits transparent content
    // on top of the screen. If we don't clear here, the previous frame persists,
    // causing infinite accumulation (smearing) of waveforms.
    juce::OpenGLHelpers::clear(juce::Colours::transparentBlack);

    // Calculate delta time for animations
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;
    // Clamp delta time to prevent huge jumps
    deltaTime = std::min(deltaTime, 0.1f);

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

    // Handle resize for render engine
    if (renderEngine_ && renderEngine_->isInitialized())
    {
        // Check if we need to resize the FBOs
        static int lastWidth = 0, lastHeight = 0;
        if (width != lastWidth || height != lastHeight)
        {
            renderEngine_->resize(width, height);
            lastWidth = width;
            lastHeight = height;
        }
    }

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

    // Debug log once per second
#if JUCE_DEBUG
    static int frameCounter = 0;
    if (++frameCounter >= 60)
    {
        frameCounter = 0;
        GL_LOG("renderOpenGL: " << waveformsToRender.size() << " waveforms, "
               << waveforms_.size() << " registered, viewport=" << width << "x" << height
               << ", renderEngine=" << (renderEngine_ ? "active" : "off"));
        for (const auto& data : waveformsToRender)
        {
            GL_LOG("  Waveform " << data.id << ": bounds=("
                   << data.bounds.getX() << "," << data.bounds.getY() << ","
                   << data.bounds.getWidth() << "x" << data.bounds.getHeight() << ")"
                   << ", ch1=" << data.channel1.size()
                   << ", ch2=" << data.channel2.size()
                   << ", isStereo=" << data.isStereo
                   << ", visible=" << data.visible
                   << ", preset=" << data.visualConfig.presetId.toStdString());
        }
    }
#endif

    if constexpr (DEBUG_RENDER_MODE)
    {
        // Clear the framebuffer with the background color.
        juce::OpenGLHelpers::clear(backgroundColour_);

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
        // Lock the engine for the duration of the frame
        const juce::ScopedWriteLock lock(engineLock_);
        
        if (renderEngine_ && renderEngine_->isInitialized())
        {
            // ========== ADVANCED RENDER ENGINE PATH ==========
            // Use the full render engine with post-processing, particles, etc.

            // Sync RenderEngine state: remove any waveform states for IDs
            // that are no longer in the local waveforms_ map. This prevents
            // GPU resource leaks (history FBOs, particle emitters) when
            // oscillators are removed or panes are rebuilt.
            {
                std::unordered_set<int> activeIds;
                for (const auto& data : waveformsToRender)
                    activeIds.insert(data.id);
                renderEngine_->syncWaveforms(activeIds);
            }

            // Begin frame (clears scene FBO, updates timing)
            renderEngine_->beginFrame(deltaTime);

            // Register/update waveform configs with render engine
            for (const auto& data : waveformsToRender)
                {
                    // Ensure waveform is registered
                    if (!renderEngine_->getWaveformConfig(data.id))
                    {
                        renderEngine_->registerWaveform(data.id);
                    }

                    // Update the visual configuration for this waveform
                    renderEngine_->setWaveformConfig(data.id, data.visualConfig);

                    // Render the waveform through the engine
                    renderEngine_->renderWaveform(data);
                }

            // End frame (applies global effects, blits to screen)
            renderEngine_->endFrame();
        }
        else
        {
            // ========== BASIC FALLBACK RENDER PATH ==========
            // RenderEngine failed or not initialized.
            // Fallback to clear screen (no shaders available without RenderEngine)
            juce::OpenGLHelpers::clear(backgroundColour_);
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
    GLint projLoc = ext.glGetUniformLocation(static_cast<GLuint>(programID), "projection");
    GLint colorLoc = ext.glGetUniformLocation(static_cast<GLuint>(programID), "color");
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

    GLuint posAttrib = static_cast<GLuint>(positionLoc >= 0 ? positionLoc : 0);
    ext.glEnableVertexAttribArray(posAttrib);
    ext.glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
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

    // Cleanup
    ext.glDisableVertexAttribArray(posAttrib);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void WaveformGLRenderer::openGLContextClosing()
{
    // Guard against multiple cleanup calls (e.g., callback + destructor)
    bool expected = false;
    if (!cleanupPerformed_.compare_exchange_strong(expected, true))
    {
        DBG("WaveformGLRenderer: Cleanup already performed, skipping");
        return;
    }

    DBG("WaveformGLRenderer: OpenGL context closing");
    contextReady_.store(false);

    // Shutdown render engine first (it has its own resources)
    {
        const juce::ScopedWriteLock lock(engineLock_);
        if (renderEngine_)
        {
            renderEngine_->shutdown();
            renderEngine_.reset();
            DBG("WaveformGLRenderer: RenderEngine shutdown complete");
        }
    }

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
    // Shaders are now managed by RenderEngine, which is already shut down above.
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
