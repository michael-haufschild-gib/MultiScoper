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
    // Only attempt cleanup if context exists and is active to safely call GL functions
    auto* ctx = context_.load(std::memory_order_acquire);
    if (!cleanupPerformed_.load() && ctx != nullptr && ctx->isActive())
    {
        openGLContextClosing();
    }

    GL_LOG("WaveformGLRenderer DESTROYED");
    DBG("WaveformGLRenderer destroyed");
}

void WaveformGLRenderer::setContext(juce::OpenGLContext* context)
{
    GL_LOG("setContext called, context=" << (context ? "valid" : "nullptr"));
    context_.store(context, std::memory_order_release);
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
    auto* ctx = context_.load(std::memory_order_acquire);
    if (useRenderEngine_ && ctx != nullptr)
    {
        const juce::ScopedWriteLock lock(engineLock_);
        renderEngine_ = std::make_unique<RenderEngine>();
        if (!renderEngine_->initialize(*ctx))
        {
            GL_LOG("WARNING: RenderEngine initialization failed, falling back to basic rendering");
            DBG("WaveformGLRenderer: RenderEngine initialization failed");
            renderEngine_.reset();
        }
        else
        {
            GL_LOG("RenderEngine initialized successfully");
            DBG("WaveformGLRenderer: RenderEngine initialized");
            // Propagate current background color to the new RenderEngine
            renderEngine_->setBackgroundColour(backgroundColour_);
        }
    }

    // Initialize frame timing
    lastFrameTime_ = std::chrono::steady_clock::now();

    contextReady_.store(true);
    GL_LOG("contextReady_ set to TRUE");
}

void WaveformGLRenderer::compileDebugShader()
{
    auto* ctx = context_.load(std::memory_order_acquire);
    GL_LOG("compileDebugShader called, context_=" << (ctx ? "valid" : "nullptr")
           << ", already compiled=" << debugShaderCompiled_);

    if (ctx == nullptr || debugShaderCompiled_)
        return;

    GL_LOG("Compiling DEBUG shader...");

    debugShader_ = std::make_unique<juce::OpenGLShaderProgram>(*ctx);

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
    auto& ext = ctx->extensions;

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
    // Load context pointer atomically once and use the local copy throughout this frame
    // This prevents race conditions with setContext() being called from another thread
    auto* ctx = context_.load(std::memory_order_acquire);

    // Defensive context validation
    if (!contextReady_.load() || ctx == nullptr)
        return;

    // Check for context loss
    if (!juce::OpenGLHelpers::isContextActive())
    {
        handleContextLoss();
        return;
    }

    // Check for context change (host may have recreated context)
    if (ctx != lastKnownContext_)
    {
        DBG("WaveformGLRenderer: Context changed, scheduling reinitialization");
        needsReinitialization_.store(true, std::memory_order_release);
        lastKnownContext_ = ctx;
    }

    // Handle pending reinitialization
    if (needsReinitialization_.load(std::memory_order_acquire))
    {
        if (!attemptReinitialization())
        {
            // Reinitialization failed, skip this frame
            return;
        }
    }

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
    const float desktopScale = static_cast<float>(ctx->getRenderingScale());
    auto* targetComponent = ctx->getTargetComponent();
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
        // Note: Using member variables instead of static to support multiple plugin instances
        if (width != lastResizeWidth_ || height != lastResizeHeight_)
        {
            renderEngine_->resize(width, height);
            lastResizeWidth_ = width;
            lastResizeHeight_ = height;
        }
    }

    // C12 FIX: Process deferred clear request from UI thread
    // This must happen on the render thread to avoid data races with waveformsRead_
    if (clearRequested_.load(std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedLockType lock(dataLock_);
        waveformsWrite_.clear();
        waveformsRead_.clear();
        swapPending_.store(false, std::memory_order_release);
        clearRequested_.store(false, std::memory_order_release);
        DBG("WaveformGLRenderer: Cleared all waveforms on render thread");
    }

    // Swap double-buffers if write buffer has new data
    // This avoids deep-copying WaveformRenderData vectors every frame
    if (swapPending_.load(std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedLockType lock(dataLock_);
        std::swap(waveformsWrite_, waveformsRead_);
        swapPending_.store(false, std::memory_order_release);
    }

    // Build list of visible waveforms from the read buffer (no lock needed - GL thread exclusive)
    // Reuse member vector to avoid per-frame allocation
    waveformsToRender_.clear();
    if (waveformsToRender_.capacity() < waveformsRead_.size())
        waveformsToRender_.reserve(waveformsRead_.size());
    for (const auto& pair : waveformsRead_)
    {
        // In debug mode, render even if channel1 is empty and not visible
        // (just draw the bounds to test GL rendering pipeline)
        if constexpr (DEBUG_RENDER_MODE)
        {
            // Only require non-empty bounds, not visible flag or data
            if (!pair.second.bounds.isEmpty())
            {
                waveformsToRender_.push_back(&pair.second);
            }
        }
        else
        {
            if (pair.second.visible && !pair.second.channel1.empty())
            {
                waveformsToRender_.push_back(&pair.second);
            }
        }
    }

    // Debug log once per second
#if JUCE_DEBUG
    if (++debugFrameCounter_ >= 60)
    {
        debugFrameCounter_ = 0;
        GL_LOG("renderOpenGL: " << waveformsToRender_.size() << " waveforms, "
               << waveformsRead_.size() << " registered, viewport=" << width << "x" << height
               << ", renderEngine=" << (renderEngine_ ? "active" : "off"));
        for (const auto* data : waveformsToRender_)
        {
            GL_LOG("  Waveform " << data->id << ": bounds=("
                   << data->bounds.getX() << "," << data->bounds.getY() << ","
                   << data->bounds.getWidth() << "x" << data->bounds.getHeight() << ")"
                   << ", ch1=" << data->channel1.size()
                   << ", ch2=" << data->channel2.size()
                   << ", isStereo=" << data->isStereo
                   << ", visible=" << data->visible
                   << ", preset=" << data->visualConfig.presetId.toStdString());
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
        for (const auto* data : waveformsToRender_)
        {
            renderDebugRect(data->bounds, data->colour);
        }
    }
    else
    {
        // Lock the engine for the duration of the frame
        const juce::ScopedWriteLock lock(engineLock_);
        
        if (renderEngine_ && renderEngine_->isInitialized())
        {
            // ========== ADVANCED RENDER ENGINE PATH ==========
            // Use the full render engine with post-processing, 3D effects, etc.

            // Sync RenderEngine state: remove any waveform states for IDs
            // that are no longer in the read buffer. This prevents
            // GPU resource leaks (history FBOs) when
            // oscillators are removed or panes are rebuilt.
            {
                std::unordered_set<int> activeIds;
                for (const auto* data : waveformsToRender_)
                    activeIds.insert(data->id);
                renderEngine_->syncWaveforms(activeIds);
            }

            // Begin frame (clears scene FBO, updates timing)
            renderEngine_->beginFrame(deltaTime);

            // ========== BATCHED RENDERING (optimized state changes) ==========
            // Use batch API to reduce shader/FBO/VBO bind calls
            renderEngine_->beginBatch();

            for (const auto* data : waveformsToRender_)
            {
                // Ensure waveform is registered
                if (!renderEngine_->hasWaveform(data->id))
                {
                    renderEngine_->registerWaveform(data->id);
                }

                // Update the visual configuration for this waveform
                renderEngine_->setWaveformConfig(data->id, data->visualConfig);

                // Add to batch for sorted, grouped rendering
                renderEngine_->addToBatch(*data);
            }

            // Flush batch: sorts by shader, batches grids, groups non-PP waveforms
            renderEngine_->flushBatch();

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

    // Process any pending frame captures
    processPendingCaptures(width, height);
}

void WaveformGLRenderer::requestFrameCapture(FrameCaptureCallback callback)
{
    const juce::SpinLock::ScopedLockType lock(captureLock_);
    pendingCaptures_.push_back(std::move(callback));
}

void WaveformGLRenderer::processPendingCaptures(int width, int height)
{
    // Check (unlocked) if there's work to do
    if (pendingCaptures_.empty())
        return;

    std::vector<FrameCaptureCallback> callbacks;
    {
        const juce::SpinLock::ScopedLockType lock(captureLock_);
        if (pendingCaptures_.empty())
            return;
        callbacks = std::move(pendingCaptures_);
        // pendingCaptures_ is now empty
    }

    // Capture the frame using pooled buffer to avoid per-capture allocation
    const size_t requiredSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (captureBuffer_.size() < requiredSize)
    {
        try
        {
            captureBuffer_.resize(requiredSize);
        }
        catch (const std::exception& e)
        {
            juce::Logger::writeToLog("Frame capture resize failed: " + juce::String(e.what()));
            return;
        }
        catch (...)
        {
            juce::Logger::writeToLog("Frame capture resize failed: unknown exception");
            return;
        }
    }

    // Read pixels (RGBA format)
    // Note: glReadPixels reads from bottom-left up
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, captureBuffer_.data());

    // Create the JUCE image (ARGB format)
    juce::Image capturedImage(juce::Image::ARGB, width, height, true);
    juce::Image::BitmapData destData(capturedImage, juce::Image::BitmapData::readWrite);

    // Copy and convert
    // GL (RGBA, Bottom-Up) -> JUCE (ARGB, Top-Down)
    for (int y = 0; y < height; ++y)
    {
        const int srcRowIndex = height - 1 - y; // Flip vertically
        const uint8_t* srcRow = captureBuffer_.data() + (static_cast<size_t>(srcRowIndex) * static_cast<size_t>(width) * 4);
        uint8_t* destRow = destData.getLinePointer(y);

        for (int x = 0; x < width; ++x)
        {
            const uint8_t* srcPixel = srcRow + (x * 4);
            // GL: R, G, B, A
            uint8_t r = srcPixel[0];
            uint8_t g = srcPixel[1];
            uint8_t b = srcPixel[2];
            uint8_t a = srcPixel[3];

            // JUCE ARGB: B, G, R, A (in memory on Little Endian)
            // We use PixelARGB to handle endianness correctly
            juce::PixelARGB* destPixel = reinterpret_cast<juce::PixelARGB*>(destRow + (x * 4));
            destPixel->setARGB(a, r, g, b);
        }
    }

    // Dispatch callbacks on the Message Thread
    // We capture by value to ensure the image lives long enough
    juce::MessageManager::callAsync([callbacks = std::move(callbacks), img = capturedImage]() {
        for (const auto& cb : callbacks)
        {
            cb(img);
        }
    });
}

void WaveformGLRenderer::renderDebugRect(const juce::Rectangle<float>& bounds, juce::Colour colour)
{
    auto* ctx = context_.load(std::memory_order_acquire);

    // Check preconditions and log failures once per second
    // Using member variable instead of static to prevent data races with multiple plugin instances
    bool shouldLogEntry = (++debugEntryLogCounter_ >= 60);
    if (shouldLogEntry)
    {
        debugEntryLogCounter_ = 0;
        GL_LOG("renderDebugRect entry: shader=" << (debugShader_ ? "valid" : "null")
               << ", compiled=" << debugShaderCompiled_
               << ", context=" << (ctx ? "valid" : "null"));
    }

    if (!debugShader_ || !debugShaderCompiled_ || ctx == nullptr)
        return;

    auto* targetComponent = ctx->getTargetComponent();
    if (!targetComponent)
        return;

    auto& ext = ctx->extensions;

    // Use shouldLogEntry from above to control logging frequency
    // (already incremented at the start of this function)
    bool shouldLog = shouldLogEntry;

    // Check for any pre-existing GL errors
    GLenum preError = glGetError();
    if (shouldLog && preError != GL_NO_ERROR)
        GL_LOG("Pre-existing GL error: " << preError);

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

    // Guard against division by zero in projection matrix
    if (viewportWidth < 1.0f) viewportWidth = 1.0f;
    if (viewportHeight < 1.0f) viewportHeight = 1.0f;

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

    // Save GL state before modification
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

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

    // Restore GL state
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);
}

void WaveformGLRenderer::handleContextLoss()
{
    // Mark all GPU resources as invalid
    contextReady_.store(false, std::memory_order_release);
    
    // Schedule reinitialization on next valid frame
    needsReinitialization_.store(true, std::memory_order_release);
    
    // Log for debugging (throttled)
    static std::atomic<int> lossCounter{0};
    if (lossCounter.fetch_add(1, std::memory_order_relaxed) % 60 == 0)
    {
        DBG("WaveformGLRenderer: Context loss detected, scheduling recovery");
    }
}

bool WaveformGLRenderer::attemptReinitialization()
{
    // Check if context is now available
    auto* ctx = context_.load(std::memory_order_acquire);
    if (ctx == nullptr || !juce::OpenGLHelpers::isContextActive())
    {
        consecutiveRecoveryFailures_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    DBG("WaveformGLRenderer: Attempting context recovery...");

    // Shutdown existing resources safely
    {
        const juce::ScopedWriteLock lock(engineLock_);
        if (renderEngine_)
        {
            renderEngine_->shutdown();
            renderEngine_.reset();
        }
    }

    // Release debug shader GL resources BEFORE clearing state
    // C13 FIX: Must delete GL objects while context is still active
    auto& ext = ctx->extensions;
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

    // Clear debug shader state (program is JUCE-managed, reset handles cleanup)
    debugShader_.reset();
    debugShaderCompiled_ = false;
    shadersCompiled_ = false;

    // Reinitialize
    if constexpr (DEBUG_RENDER_MODE)
    {
        compileDebugShader();
    }

    if (useRenderEngine_)
    {
        const juce::ScopedWriteLock lock(engineLock_);
        renderEngine_ = std::make_unique<RenderEngine>();
        if (!renderEngine_->initialize(*ctx))
        {
            DBG("WaveformGLRenderer: RenderEngine reinitialization failed");
            renderEngine_.reset();
            consecutiveRecoveryFailures_.fetch_add(1, std::memory_order_relaxed);

            // If we've failed too many times, stop trying
            if (consecutiveRecoveryFailures_.load(std::memory_order_relaxed) > 10)
            {
                DBG("WaveformGLRenderer: Too many recovery failures, giving up");
                needsReinitialization_.store(false, std::memory_order_release);
            }
            return false;
        }

        // Propagate current background color
        renderEngine_->setBackgroundColour(backgroundColour_);
    }

    // Success!
    needsReinitialization_.store(false, std::memory_order_release);
    contextReady_.store(true, std::memory_order_release);
    consecutiveRecoveryFailures_.store(0, std::memory_order_relaxed);
    lastKnownContext_ = ctx;

    // Reset resize tracking to force FBO resize on next frame
    lastResizeWidth_ = 0;
    lastResizeHeight_ = 0;

    DBG("WaveformGLRenderer: Context recovery successful");
    return true;
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
    auto* ctx = context_.load(std::memory_order_acquire);
    if (ctx != nullptr)
    {
        auto& ext = ctx->extensions;

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
    if (waveformsWrite_.find(id) == waveformsWrite_.end())
    {
        WaveformRenderData data;
        data.id = id;
        waveformsWrite_[id] = data;
        swapPending_.store(true, std::memory_order_release);
        DBG("WaveformGLRenderer: Registered waveform " << id);
    }
}

void WaveformGLRenderer::unregisterWaveform(int id)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    waveformsWrite_.erase(id);
    swapPending_.store(true, std::memory_order_release);
    DBG("WaveformGLRenderer: Unregistered waveform " << id);
}

void WaveformGLRenderer::updateWaveform(const WaveformRenderData& data)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    waveformsWrite_[data.id] = data;
    swapPending_.store(true, std::memory_order_release);
}

void WaveformGLRenderer::updateWaveform(WaveformRenderData&& data)
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    int id = data.id;
    waveformsWrite_[id] = std::move(data);
    swapPending_.store(true, std::memory_order_release);
}

void WaveformGLRenderer::setBackgroundColour(juce::Colour colour)
{
    backgroundColour_ = colour;
    // Propagate to RenderEngine so scene FBO is also cleared with this color
    if (renderEngine_)
    {
        renderEngine_->setBackgroundColour(colour);
    }
}

int WaveformGLRenderer::getWaveformCount() const
{
    const juce::SpinLock::ScopedLockType lock(dataLock_);
    return static_cast<int>(waveformsWrite_.size());
}

void WaveformGLRenderer::clearAllWaveforms()
{
    // C12 FIX: Set atomic flag for deferred clear instead of modifying buffers directly.
    // The actual clear happens on the render thread at the start of the next frame
    // to avoid race conditions with renderOpenGL() accessing waveformsRead_.
    clearRequested_.store(true, std::memory_order_release);
    DBG("WaveformGLRenderer: Clear requested (deferred to render thread)");
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
