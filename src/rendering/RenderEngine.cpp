/*
    Oscil - Render Engine Implementation
*/

#include "rendering/RenderEngine.h"

#include "rendering/ShaderRegistry.h"
#include "rendering/effects/PostProcessEffect.h"

#include <cmath>

namespace oscil
{

using namespace juce::gl;

RenderEngine::RenderEngine()
    : bootstrapper_(std::make_unique<RenderBootstrapper>())
    , effectPipeline_(std::make_unique<EffectPipeline>())
    , waveformPass_(std::make_unique<WaveformPass>())
{
}

RenderEngine::~RenderEngine()
{
    // If we're being destroyed while still initialized, GPU resources will leak.
    // shutdown() must be called while the OpenGL context is still active.
    // We can't call OpenGL functions here (no guaranteed context), so we just warn.
    if (initialized_)
    {
        DBG("[RenderEngine] LEAK: Destructor called without shutdown(). "
            "GPU resources (shaders, FBOs, textures) will leak.");
    }
}

bool RenderEngine::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        shutdown();

    context_ = &context;

    // Get initial dimensions from target component
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
    {
        RE_LOG("RenderEngine: No target component");
        return false;
    }

    // Use physical (scaled) dimensions for internal state and FBOs
    double scale = context.getRenderingScale();
    currentWidth_ = static_cast<int>(targetComponent->getWidth() * scale);
    currentHeight_ = static_cast<int>(targetComponent->getHeight() * scale);

    if (currentWidth_ <= 0 || currentHeight_ <= 0)
    {
        RE_LOG("RenderEngine: Invalid dimensions " << currentWidth_ << "x" << currentHeight_);
        return false;
    }

    // Initialize subsystems
    if (!bootstrapper_->initialize(context))
    {
        RE_LOG("RenderEngine: Failed to initialize bootstrapper");
        return false;
    }

    if (!effectPipeline_->initialize(context, currentWidth_, currentHeight_))
    {
        RE_LOG("RenderEngine: Failed to initialize effect pipeline");
        bootstrapper_->shutdown(context);
        return false;
    }

    if (!waveformPass_->initialize(context, currentWidth_, currentHeight_))
    {
        RE_LOG("RenderEngine: Failed to initialize waveform pass");
        effectPipeline_->shutdown(context);
        bootstrapper_->shutdown(context);
        return false;
    }

    initialized_ = true;
    RE_LOG("RenderEngine: Initialized " << currentWidth_ << "x" << currentHeight_);
    return true;
}

void RenderEngine::shutdown()
{
    if (!initialized_ || !context_)
        return;

    // Release all waveform states
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
        waveformStates_.clear();
    }

    // Shutdown subsystems
    waveformPass_->shutdown(*context_);
    effectPipeline_->shutdown(*context_);
    bootstrapper_->shutdown(*context_);

    context_ = nullptr;
    initialized_ = false;

    RE_LOG("RenderEngine: Shutdown complete");
}

void RenderEngine::resize(int width, int height)
{
    if (!initialized_ || !context_ || width <= 0 || height <= 0)
        return;

    if (width == currentWidth_ && height == currentHeight_)
        return;

    currentWidth_ = width;
    currentHeight_ = height;

    // Resize subsystems
    effectPipeline_->resize(*context_, width, height);
    // WaveformPass doesn't have resize, but it might need it if it caches size
    // Actually WaveformPass stores currentWidth_ in initialize, so we should add resize to it or just update it.
    // For now, we'll assume it doesn't strictly need it unless it recreates resources.

    // Resize history FBOs for waveforms with trails
    {
        juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
        for (auto& [key, state] : waveformStates_)
        {
            if (state.trailsEnabled)
            {
                state.resizeHistoryFBO(*context_, width, height);
            }
        }
    }

    RE_LOG("RenderEngine: Resized to " << width << "x" << height);
}

void RenderEngine::beginFrame(float deltaTime)
{
    if (!initialized_)
        return;

    stats_.update(deltaTime);

    RE_LOG_THROTTLED(60, "beginFrame: dt=" << deltaTime);

    // Clear scene FBO
    if (auto* sceneFBO = effectPipeline_->getSceneFBO())
    {
        sceneFBO->bind();
        sceneFBO->clear(backgroundColour_, false);
        sceneFBO->unbind();
    }
}

WaveformRenderState* RenderEngine::resolveWaveformState(int waveformId)
{
    // Called from renderWaveform(), which is always invoked under the
    // WaveformGLRenderer::engineLock_ WriteLock.  No concurrent mutation
    // of waveformStates_ is possible, so the SpinLock is not needed here
    // and returning a raw pointer is safe for the duration of the render pass.

    auto it = waveformStates_.find(waveformId);
    if (it == waveformStates_.end())
    {
        WaveformRenderState newState;
        newState.waveformId = waveformId;
        waveformStates_[waveformId] = std::move(newState);
        RE_LOG("RenderEngine: Registered waveform " << waveformId);
        it = waveformStates_.find(waveformId);
    }

    return (it != waveformStates_.end()) ? &it->second : nullptr;
}

void RenderEngine::renderWaveformLayer(const WaveformRenderData& data, WaveformRenderState& state)
{
    const auto& config = state.visualConfig;
    const bool hasWaveformGeometry = data.channel1.size() >= 2;

    auto* pool = effectPipeline_->getFramebufferPool();
    auto* waveformFBO = pool ? pool->getWaveformFBO() : nullptr;
    if (!waveformFBO)
        return;

    waveformFBO->bind();
    waveformFBO->clear(juce::Colours::transparentBlack, true);

    if (hasWaveformGeometry)
        waveformPass_->renderWaveformGeometry(data, config, stats_.getTime());

    waveformFBO->unbind();

    Framebuffer* processedFBO = waveformFBO;
    if (config.hasPostProcessing())
    {
        processedFBO = effectPipeline_->applyPostProcessing(processedFBO, state, *context_, stats_.getDeltaTime(),
                                                            bootstrapper_->getCompositeShader(),
                                                            bootstrapper_->getCompositeTextureLoc());
        if (!processedFBO)
            processedFBO = waveformFBO;
    }

    executeComposite(processedFBO, config);
}

void RenderEngine::renderWaveform(const WaveformRenderData& data)
{
    if (!initialized_ || !data.visible || data.bounds.isEmpty())
        return;

    auto* statePtr = resolveWaveformState(data.id);
    if (statePtr == nullptr)
        return;

    WaveformRenderState& state = *statePtr;

    waveformPass_->prepareRender(data, state, stats_.getDeltaTime());

    if (data.gridConfig.enabled)
    {
        auto* sceneFBO = effectPipeline_->getSceneFBO();
        if (sceneFBO && sceneFBO->isValid())
        {
            sceneFBO->bind();
            waveformPass_->renderGrid(data);
            sceneFBO->unbind();
        }
    }

    if (data.channel1.size() >= 2)
        renderWaveformLayer(data, state);
}

void RenderEngine::endFrame()
{
    if (!initialized_)
        return;

    // Blit scene to screen
    blitToScreen();
}

void RenderEngine::executeComposite(Framebuffer* source, const VisualConfiguration& config)
{
    if (!source || !source->isValid())
        return;

    auto* sceneFBO = effectPipeline_->getSceneFBO();
    if (!sceneFBO)
        return;

    sceneFBO->bind();

    // Restore full viewport for compositing to the scene FBO
    // This is necessary because renderWaveformGeometry/Particles sets the viewport to the pane size
    if (currentWidth_ > 0 && currentHeight_ > 0)
    {
        glViewport(0, 0, currentWidth_, currentHeight_);
    }

    glEnable(GL_BLEND);

    auto* shader = bootstrapper_->getCompositeShader();
    if (shader)
    {
        shader->use();
        context_->extensions.glUniform1i(bootstrapper_->getCompositeTextureLoc(), 0);

        // Set blend mode based on config
        switch (config.compositeBlendMode)
        {
            case BlendMode::Alpha:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Additive:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::Multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            case BlendMode::Screen:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
        }

        source->bindTexture(0);
        effectPipeline_->getFramebufferPool()->renderFullscreenQuad();
    }

    glDisable(GL_BLEND);
    sceneFBO->unbind();
}

// Waveform management methods (register, unregister, config, sync, quality)
// are in RenderEngineWaveforms.cpp

void RenderEngine::blitToScreen()
{
    // Check if context is still active/valid
    if (!context_ || !context_->isActive())
        return;

    // Bind default framebuffer
    context_->extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto* targetComponent = context_->getTargetComponent();
    if (!targetComponent)
        return;

    float desktopScale = static_cast<float>(context_->getRenderingScale());
    auto width = static_cast<GLsizei>(static_cast<float>(targetComponent->getWidth()) * desktopScale);
    auto height = static_cast<GLsizei>(static_cast<float>(targetComponent->getHeight()) * desktopScale);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    auto* shader = bootstrapper_->getBlitShader();
    auto* sceneFBO = effectPipeline_->getSceneFBO();

    if (shader && sceneFBO && sceneFBO->isValid())
    {
        shader->use();
        sceneFBO->bindTexture(0);
        context_->extensions.glUniform1i(bootstrapper_->getBlitTextureLoc(), 0);

        effectPipeline_->getFramebufferPool()->renderFullscreenQuad();
    }

    // Restore GL state to prevent leakage into subsequent operations
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace oscil
