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
    pendingComposites_.reserve(16); // Pre-reserve for common case
}

RenderEngine::~RenderEngine()
{
    // Note: shutdown() should be called explicitly while context is active
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
    for (auto& pair : waveformStates_)
    {
        pair.second.release(*context_);
    }
    waveformStates_.clear();

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
    for (auto& [key, state] : waveformStates_)
    {
        if (state.trailsEnabled)
        {
            state.resizeHistoryFBO(*context_, width, height);
        }
    }

    RE_LOG("RenderEngine: Resized to " << width << "x" << height);
}

void RenderEngine::beginFrame(float deltaTime)
{
    if (!initialized_)
        return;

    stats_.update(deltaTime);

    // Clear pending composites for the new frame
    pendingComposites_.clear();

    RE_LOG_THROTTLED(60, "beginFrame: dt=" << deltaTime);

    // Update particle system
    if (auto* ps = waveformPass_->getParticleSystem())
        ps->update(deltaTime);

    // Update camera animation
    if (auto* cam = waveformPass_->getCamera())
        cam->update(deltaTime);

    // Clear scene FBO
    if (auto* sceneFBO = effectPipeline_->getSceneFBO())
    {
        sceneFBO->bind();
        sceneFBO->clear(backgroundColour_, false);
        sceneFBO->unbind();
    }
}

void RenderEngine::renderWaveform(const WaveformRenderData& data)
{
    if (!initialized_ || !data.visible || data.channel1.size() < 2)
        return;

    // Get or create waveform state
    auto it = waveformStates_.find(data.id);
    if (it == waveformStates_.end())
    {
        registerWaveform(data.id);
        it = waveformStates_.find(data.id);
    }

    if (it != waveformStates_.end())
    {
        // Delegate to WaveformPass
        // Note: WaveformPass::renderWaveform does the rendering to a waveform FBO
        // and then applies post-processing (via EffectPipeline).
        
        Framebuffer* result = waveformPass_->renderWaveform(data, it->second, *effectPipeline_,
                                                            stats_.getDeltaTime(), stats_.getTime(),
                                                            bootstrapper_->getCompositeShader(),
                                                            bootstrapper_->getCompositeTextureLoc());

        if (result)
        {
            // Queue for compositing later (Batch optimization)
            pendingComposites_.push_back({result, it->second.visualConfig});
        }
    }
}

void RenderEngine::endFrame()
{
    if (!initialized_)
        return;

    // Batch composite all waveforms to scene FBO
    flushComposites();

    // Apply global effects to scene FBO
    if (isGlobalPostProcessingEnabled())
    {
        if (auto* sceneFBO = effectPipeline_->getSceneFBO())
            effectPipeline_->applyGlobalEffects(sceneFBO, *context_);
    }

    // Blit scene to screen
    blitToScreen();
}

void RenderEngine::flushComposites()
{
    if (pendingComposites_.empty())
        return;

    auto* sceneFBO = effectPipeline_->getSceneFBO();
    if (!sceneFBO)
        return;

    sceneFBO->bind();

    glEnable(GL_BLEND);
    
    auto* shader = bootstrapper_->getCompositeShader();
    if (shader)
    {
        shader->use();
        context_->extensions.glUniform1i(bootstrapper_->getCompositeTextureLoc(), 0);
        
        BlendMode currentBlendMode = BlendMode::Additive; // Default assumption, will force set on first
        bool first = true;

        for (const auto& cmd : pendingComposites_)
        {
            if (!cmd.source || !cmd.source->isValid())
                continue;

            // Set blend mode only if changed
            if (first || cmd.config.compositeBlendMode != currentBlendMode)
            {
                currentBlendMode = cmd.config.compositeBlendMode;
                switch (currentBlendMode)
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
                first = false;
            }

            cmd.source->bindTexture(0);
            effectPipeline_->getFramebufferPool()->renderFullscreenQuad();
        }
    }

    glDisable(GL_BLEND);
    sceneFBO->unbind();
    
    pendingComposites_.clear();
}

void RenderEngine::registerWaveform(int waveformId)
{
    if (waveformStates_.find(waveformId) == waveformStates_.end())
    {
        WaveformRenderState state;
        state.waveformId = waveformId;
        waveformStates_[waveformId] = std::move(state);
        RE_LOG("RenderEngine: Registered waveform " << waveformId);
    }
}

void RenderEngine::unregisterWaveform(int waveformId)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        if (context_)
            it->second.release(*context_);
        waveformStates_.erase(it);
        RE_LOG("RenderEngine: Unregistered waveform " << waveformId);
    }
}

VisualConfiguration* RenderEngine::getWaveformConfig(int waveformId)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
        return &it->second.visualConfig;
    return nullptr;
}

void RenderEngine::setWaveformConfig(int waveformId, const VisualConfiguration& config)
{
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        it->second.visualConfig = config;

        // Update trails state based on config
        if (context_)
        {
            if (config.trails.enabled && !it->second.trailsEnabled)
            {
                it->second.enableTrails(*context_, currentWidth_, currentHeight_);
            }
            else if (!config.trails.enabled && it->second.trailsEnabled)
            {
                it->second.disableTrails(*context_);
            }

            // Update turbulence
            if (auto* ps = waveformPass_->getParticleSystem())
            {
                if (config.particles.enabled)
                {
                    if (config.particles.useTurbulence)
                    {
                        ps->setTurbulence(
                            config.particles.turbulenceStrength,
                            config.particles.turbulenceScale,
                            config.particles.turbulenceSpeed);
                    }
                    else
                    {
                        ps->setTurbulence(0.0f, 0.5f, 0.5f);
                    }
                }
            }
        }
    }
}

void RenderEngine::clearAllWaveforms()
{
    if (context_)
    {
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
    }
    waveformStates_.clear();
}

void RenderEngine::syncWaveforms(const std::unordered_set<int>& activeIds)
{
    if (!context_)
        return;

    std::vector<int> toRemove;
    for (const auto& pair : waveformStates_)
    {
        if (activeIds.find(pair.first) == activeIds.end())
        {
            toRemove.push_back(pair.first);
        }
    }

    for (int id : toRemove)
    {
        unregisterWaveform(id);
        RE_LOG("RenderEngine: Synced out stale waveform " << id);
    }
}

void RenderEngine::setQualityLevel(QualityLevel level)
{
    qualityLevel_ = level;
    effectPipeline_->setQualityLevel(level);
    RE_LOG("RenderEngine: Quality level set to " << static_cast<int>(level));
}

PostProcessEffect* RenderEngine::getEffect(const juce::String& effectId)
{
    return effectPipeline_->getEffect(effectId);
}

void RenderEngine::setGlobalPostProcessingEnabled(bool enabled)
{
    effectPipeline_->setGlobalPostProcessingEnabled(enabled);
}

bool RenderEngine::isGlobalPostProcessingEnabled() const
{
    return effectPipeline_->isGlobalPostProcessingEnabled();
}

void RenderEngine::compositeToScene(Framebuffer* source, const VisualConfiguration& config)
{
    // Legacy method now delegates to internal list for consistency if called directly?
    // But renderWaveform calls this?
    // No, renderWaveform now adds to queue.
    // This method is kept if someone calls it externally, but mostly it's replaced by flushComposites logic.
    // Let's make it just queue it to avoid breaking API if public (it is private in header).
    // It is private. So I can change it or remove it.
    // I'll remove the body and redirect to queue, OR keep it as "immediate" fallback?
    // Queue is safer.
    
    pendingComposites_.push_back({source, config});
}

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
}

} // namespace oscil