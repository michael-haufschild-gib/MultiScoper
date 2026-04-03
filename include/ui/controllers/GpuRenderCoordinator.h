/*
    Oscil - GPU Render Coordinator
    Manages OpenGL lifecycle and rendering state propagation
*/

#pragma once

#include "ui/controllers/OpenGLLifecycleManager.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/StatusBarComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>
#include <vector>

namespace oscil
{

class GpuRenderCoordinator
{
public:
    /// Create a GPU render coordinator attached to the given editor and status bar.
    GpuRenderCoordinator(juce::AudioProcessorEditor& editor, StatusBarComponent& statusBar);
    ~GpuRenderCoordinator();

    void setGpuRenderingEnabled(bool enabled);
    bool isGpuRenderingEnabled() const;

    /// Push latest waveform data to the GPU renderer for all visible panes.
    void updateRendering(const std::vector<std::unique_ptr<PaneComponent>>& panes);
    /// Propagate GPU rendering state (enabled/disabled, context) to each pane.
    void propagateGpuStateToPanes(const std::vector<std::unique_ptr<PaneComponent>>& panes) const;
    /// Clear all registered waveforms from the GPU renderer.
    void clearWaveforms();
    /// Detach the OpenGL context from the editor.
    void detach();

private:
    std::unique_ptr<OpenGLLifecycleManager> glManager_;
    StatusBarComponent& statusBar_;
};

} // namespace oscil
