/*
    Oscil - Waveform Pass
    Handles rendering of individual waveforms (geometry, grid).
*/

#pragma once

#include "rendering/GridRenderer.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformGLRenderer.h"
#include "rendering/WaveformRenderState.h"
#include "rendering/WaveformShader.h"

#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <unordered_map>

namespace oscil
{

class WaveformPass
{
public:
    /// Create an uninitialized waveform pass.
    WaveformPass();
    ~WaveformPass();

    /// Initialize grid renderer, shader registry, and compile shaders.
    bool initialize(juce::OpenGLContext& context, int width, int height);
    /// Release all GPU resources.
    void shutdown(juce::OpenGLContext& context);

    /// Update render state (trails, timing) before drawing a waveform.
    void prepareRender(const WaveformRenderData& data, WaveformRenderState& state, float deltaTime);
    /// Render the background grid for a waveform pane.
    void renderGrid(const WaveformRenderData& data);
    /// Render waveform geometry using the configured shader.
    void renderWaveformGeometry(const WaveformRenderData& data, const VisualConfiguration& config,
                                float accumulatedTime);

private:
    struct ViewportRect
    {
        int x, y, w, h;
    };
    ViewportRect computePaneViewport(const juce::Rectangle<float>& bounds) const;
    WaveformShader* resolveShader(const juce::String& shaderId);

    // Subsystems
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<ShaderRegistry> registry_;

    // Compiled per-instance shaders
    std::unordered_map<std::string, std::unique_ptr<WaveformShader>> compiledShaders_;

    juce::OpenGLContext* context_ = nullptr;
    int currentWidth_ = 0;
    int currentHeight_ = 0;
    bool initialized_ = false;
};

} // namespace oscil
