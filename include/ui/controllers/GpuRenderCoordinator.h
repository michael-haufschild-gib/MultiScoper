/*
    MultiScoper - GPU Render Coordinator
    Manages OpenGL lifecycle and rendering state propagation
*/

#pragma once

#include "ui/controllers/OpenGLLifecycleManager.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/StatusBarComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>
#include <vector>

namespace multiscoper
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

    /// Force a one-off repaint even when the waveform is silent.
    /// Callers: anyone changing scene state that isn't driven by audio
    /// (theme change, pane reorder, oscillator add/remove, resize).
    void forceRepaint();

    /// Push the theme background colour into the GL renderer so the visible
    /// clear matches the active theme.  Components that skip their background
    /// fill in GPU mode rely on this to avoid showing transparent-black in
    /// light themes, which otherwise makes dark text unreadable.
    void setBackgroundColour(juce::Colour colour);

private:
    std::unique_ptr<OpenGLLifecycleManager> glManager_;
    StatusBarComponent& statusBar_;

    // Post-silence tail — after the last non-silent frame we still repaint
    // for a few more ticks so fades/decays don't freeze mid-animation.
    // 60Hz timer × 30 frames = ~0.5s.
    static constexpr int kPostSilenceFrames = 30;
    int silentFrames_ = kPostSilenceFrames;
};

} // namespace multiscoper
