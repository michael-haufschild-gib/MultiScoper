/*
    Oscil - Particle Renderer Implementation
*/

#include "rendering/ParticleRenderer.h"
#include "rendering/VisualConfiguration.h"
#include "rendering/Framebuffer.h"

namespace oscil
{

ParticleRenderer::ParticleRenderer(ParticleSystem& particleSystem)
    : particleSystem_(particleSystem)
{
}

void ParticleRenderer::render(juce::OpenGLContext& context, 
                              const WaveformRenderData& data, 
                              WaveformRenderState& state, 
                              float deltaTime,
                              Framebuffer* depthBuffer)
{
    if (!particleSystem_.isInitialized())
        return;

    const auto& config = state.visualConfig.particles;

    if (!config.enabled)
        return;

    // Set particle physics from this waveform's configuration
    particleSystem_.setGravity(config.gravity);
    particleSystem_.setDrag(config.drag);

    // Create emitter configuration from visual config
    ParticleEmitterConfig emitterConfig;
    emitterConfig.mode = config.emissionMode;
    emitterConfig.emissionRate = config.emissionRate;
    emitterConfig.particleLifeMin = config.particleLife * 0.8f;
    emitterConfig.particleLifeMax = config.particleLife * 1.2f;
    emitterConfig.particleSizeMin = config.particleSize * 0.7f;
    emitterConfig.particleSizeMax = config.particleSize * 1.3f;
    emitterConfig.colorStart = config.particleColor;
    emitterConfig.colorEnd = config.particleColor.withAlpha(0.0f);
    emitterConfig.gravity = config.gravity;
    emitterConfig.drag = config.drag;
    emitterConfig.velocityMin = 10.0f * config.velocityScale;
    emitterConfig.velocityMax = 50.0f * config.velocityScale;
    emitterConfig.velocityAngleSpread = config.randomness * 360.0f;

    // Create emitter if this waveform needs particles but has none
    if (state.emitterIds.empty())
    {
        ParticleEmitterId emitterId = particleSystem_.createEmitter(emitterConfig);
        state.addParticleEmitter(emitterId);
    }
    else
    {
        // Update existing emitters with new configuration
        for (auto emitterId : state.emitterIds)
        {
            if (auto* emitter = particleSystem_.getEmitter(emitterId))
            {
                emitter->setConfig(emitterConfig);
            }
        }
    }

    // Update emitters with waveform data
    for (auto emitterId : state.emitterIds)
    {
        // Pass verticalScale to correct particle positioning on scaled waveforms
        particleSystem_.updateEmitter(emitterId, data.channel1, data.bounds, deltaTime, data.verticalScale);
    }

    // Render particles using LOGICAL dimensions for projection
    auto* target = context.getTargetComponent();
    if (!target) return;
    
    int logicalWidth = target->getWidth();
    int logicalHeight = target->getHeight();

    // If soft particles enabled, bind depth texture from scene FBO
    if (config.softParticles && depthBuffer && depthBuffer->hasDepth)
    {
        depthBuffer->bindDepthTexture(1); // Bind to unit 1
    }

    particleSystem_.render(context, logicalWidth, logicalHeight, state.emitterIds, config);
}

} // namespace oscil
