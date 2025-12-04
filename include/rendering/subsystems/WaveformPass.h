/*
    Oscil - Waveform Pass
    Handles rendering of individual waveforms (geometry, particles, grid).
*/

#pragma once

#include "rendering/Camera3D.h"
#include "rendering/GridRenderer.h"
#include "rendering/ParticleRenderer.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformGLRenderer.h"
#include "rendering/WaveformRenderState.h"
#include "rendering/WaveformShader.h"
#include "rendering/materials/EnvironmentMapManager.h"
#include "rendering/materials/TextureManager.h"
#include "rendering/subsystems/EffectPipeline.h"

#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <unordered_map>

namespace oscil
{

class WaveformPass
{
public:
    WaveformPass();
    ~WaveformPass();

    bool initialize(juce::OpenGLContext& context, int width, int height);
    void shutdown(juce::OpenGLContext& context);

    Framebuffer* renderWaveform(const WaveformRenderData& data, WaveformRenderState& state,
                                EffectPipeline& effectPipeline, float deltaTime, float accumulatedTime,
                                juce::OpenGLShaderProgram* compositeShader, GLint compositeLoc);

    // Subsystem accessors
    ParticleSystem* getParticleSystem() { return particleSystem_.get(); }
    Camera3D* getCamera() { return camera_.get(); }
    EnvironmentMapManager* getEnvironmentMapManager() { return envMapManager_.get(); }
    TextureManager* getTextureManager() { return textureManager_.get(); }

private:
    void renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config, float accumulatedTime);
    void renderWaveformParticles(const WaveformRenderData& data, WaveformRenderState& state, float deltaTime);
    void renderGrid(const WaveformRenderData& data);
    void setupCamera2D();
    void setupCamera3D(const Settings3D& settings, float deltaTime);
    void createDefaultEnvironmentMaps();

    // Subsystems
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<ParticleRenderer> particleRenderer_;
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<Camera3D> camera_;
    std::unique_ptr<EnvironmentMapManager> envMapManager_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<ShaderRegistry> registry_;

    // Compiled per-instance shaders
    std::unordered_map<std::string, std::unique_ptr<WaveformShader>> compiledShaders_;

    juce::OpenGLContext* context_ = nullptr;
    int currentWidth_ = 0;
    int currentHeight_ = 0;
};

} // namespace oscil
