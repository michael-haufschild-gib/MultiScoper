/*
    Oscil - OpenGL Lifecycle Manager
    Manages OpenGL context attachment and renderer lifecycle
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "rendering/WaveformGLRenderer.h"
#include "ui/layout/PaneComponent.h"

namespace oscil
{

class OpenGLLifecycleManager
{
public:
    explicit OpenGLLifecycleManager(juce::AudioProcessorEditor& editor);
    ~OpenGLLifecycleManager();

    void setGpuRenderingEnabled(bool enabled);
    bool isGpuRenderingEnabled() const { return gpuRenderingEnabled_; }

    // Call this from timer callback to update renderer data
    void updateWaveformData(const std::vector<std::unique_ptr<PaneComponent>>& paneComponents);
    
    WaveformGLRenderer* getRenderer() { return renderer_.get(); }
    juce::OpenGLContext& getContext() { return context_; }

    void detach();
    void clearAllWaveforms();

private:
    juce::AudioProcessorEditor& editor_;
    std::unique_ptr<WaveformGLRenderer> renderer_;
    juce::OpenGLContext context_;
    bool gpuRenderingEnabled_ = false;
    bool isDetached_ = true;
};

} // namespace oscil
