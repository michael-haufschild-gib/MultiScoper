/*
    Oscil - OpenGL Lifecycle Manager
    Manages OpenGL context attachment and renderer lifecycle
*/

#pragma once

#include "rendering/WaveformGLRenderer.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

namespace oscil
{

class PaneComponent;

class OpenGLLifecycleManager
{
public:
    /// Create a lifecycle manager and allocate an OpenGL context for the editor.
    explicit OpenGLLifecycleManager(juce::AudioProcessorEditor& editor);
    ~OpenGLLifecycleManager();

    void setGpuRenderingEnabled(bool enabled);
    bool isGpuRenderingEnabled() const { return gpuRenderingEnabled_; }

    /// Push latest waveform sample data to the GL renderer for all panes.
    void updateWaveformData(const std::vector<std::unique_ptr<PaneComponent>>& paneComponents);

    WaveformGLRenderer* getRenderer() { return renderer_.get(); }
    juce::OpenGLContext& getContext() { return context_; }

    /// Detach the OpenGL context from the editor component.
    void detach();
    /// Remove all registered waveforms from the renderer.
    void clearAllWaveforms();

private:
    juce::AudioProcessorEditor& editor_;
    std::unique_ptr<WaveformGLRenderer> renderer_;
    juce::OpenGLContext context_;
    bool gpuRenderingEnabled_ = false;
    bool isDetached_ = true;
};

} // namespace oscil
