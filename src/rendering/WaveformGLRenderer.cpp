/*
    Oscil - Waveform OpenGL Renderer Implementation
*/

#include "rendering/WaveformGLRenderer.h"

#if OSCIL_ENABLE_OPENGL

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"

namespace oscil
{

using namespace juce::gl;

WaveformGLRenderer::WaveformGLRenderer()
{
    DBG("WaveformGLRenderer created");
}

WaveformGLRenderer::~WaveformGLRenderer()
{
    DBG("WaveformGLRenderer destroyed");
}

void WaveformGLRenderer::setContext(juce::OpenGLContext* context)
{
    context_ = context;
}

void WaveformGLRenderer::newOpenGLContextCreated()
{
    DBG("WaveformGLRenderer: OpenGL context created");
    compileShaders();
    contextReady_.store(true);
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
    auto width = static_cast<GLsizei>(targetComponent->getWidth() * desktopScale);
    auto height = static_cast<GLsizei>(targetComponent->getHeight() * desktopScale);

    glViewport(0, 0, width, height);

    // Clear with background color
    juce::OpenGLHelpers::clear(backgroundColour_);

    // Copy waveform data under lock for thread safety
    std::vector<WaveformRenderData> waveformsToRender;
    {
        const juce::SpinLock::ScopedLockType lock(dataLock_);
        waveformsToRender.reserve(waveforms_.size());
        for (const auto& pair : waveforms_)
        {
            if (pair.second.visible && !pair.second.channel1.empty())
            {
                waveformsToRender.push_back(pair.second);
            }
        }
    }

    // Render each waveform
    for (const auto& data : waveformsToRender)
    {
        renderWaveform(data);
    }
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

    // Render using the GPU shader
    const std::vector<float>* channel2Ptr = data.isStereo ? &data.channel2 : nullptr;
    shader->render(*context_, data.channel1, channel2Ptr, params);
}

void WaveformGLRenderer::openGLContextClosing()
{
    DBG("WaveformGLRenderer: OpenGL context closing");
    contextReady_.store(false);

    // Release all shader resources
    ShaderRegistry::getInstance().releaseAll();
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

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
