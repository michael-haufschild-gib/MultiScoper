/*
    Oscil - Waveform OpenGL Renderer Implementation
*/

#include "rendering/WaveformGLRenderer.h"

#if OSCIL_ENABLE_OPENGL

    #include "rendering/RenderEngine.h"
    #include "rendering/ShaderRegistry.h"
    #include "rendering/WaveformShader.h"

    #include <chrono>
    #include <unordered_set>

namespace oscil
{

using namespace juce::gl;

    // Debug-only logging macro — no output in release builds
    #define GL_LOG(msg) DBG("[GL] " << msg)

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
    cleanupPerformed_.store(false, std::memory_order_release);

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
                                                  << ", already compiled=" << static_cast<int>(debugShaderCompiled_));

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

    debugProjectionLoc_ = debugShader_->getUniformIDFromName("projection");
    debugColorLoc_ = debugShader_->getUniformIDFromName("color");

    auto& ext = context_->extensions;
    while (glGetError() != GL_NO_ERROR)
    {
    }

    ext.glGenVertexArrays(1, &debugVAO_);
    ext.glGenBuffers(1, &debugVBO_);

    debugShaderCompiled_ = true;
    GL_LOG("DEBUG SHADER: Compiled, VAO=" << static_cast<int>(debugVAO_) << ", VBO=" << static_cast<int>(debugVBO_));
}

// renderOpenGL() and renderDebugRect() are in WaveformGLRendererPainting.cpp

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

void WaveformGLRenderer::setBackgroundColour(juce::Colour colour) { backgroundColour_ = colour; }

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
