/*
    Oscil - Particle Renderer
    Handles configuration and rendering of waveform particles
*/

#pragma once

#include <juce_opengl/juce_opengl.h>
#include "rendering/WaveformGLRenderer.h" // For WaveformRenderData
#include "rendering/WaveformRenderState.h" // For WaveformRenderState
#include "rendering/particles/ParticleSystem.h"
#include "rendering/Framebuffer.h"

namespace oscil
{

class ParticleRenderer
{
public:
    explicit ParticleRenderer(ParticleSystem& particleSystem);
    ~ParticleRenderer() = default;

    void render(juce::OpenGLContext& context, 
                const WaveformRenderData& data, 
                WaveformRenderState& state, 
                float deltaTime,
                Framebuffer* depthBuffer = nullptr);

private:
    ParticleSystem& particleSystem_;
};

} // namespace oscil
