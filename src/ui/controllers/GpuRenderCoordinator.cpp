/*
    Oscil - GPU Render Coordinator Implementation
*/

#include "ui/controllers/GpuRenderCoordinator.h"

namespace oscil
{

GpuRenderCoordinator::GpuRenderCoordinator(juce::AudioProcessorEditor& editor, StatusBarComponent& statusBar)
    : statusBar_(statusBar)
{
    // Initialize OpenGL Lifecycle Manager
    glManager_ = std::make_unique<OpenGLLifecycleManager>(editor);
}

GpuRenderCoordinator::~GpuRenderCoordinator() { detach(); }

void GpuRenderCoordinator::setGpuRenderingEnabled(bool enabled)
{
    if (glManager_)
    {
        const bool wasEnabled = glManager_->isGpuRenderingEnabled();
        glManager_->setGpuRenderingEnabled(enabled);
        statusBar_.setRenderingMode(enabled ? RenderingMode::OpenGL : RenderingMode::Software);

        // On transition to enabled, seed an explicit repaint so the GL
        // context draws a fresh frame rather than potentially showing
        // garbage pixels until the first signal-driven tick fires. Without
        // this, toggling GPU on while audio is silent leaves a blank /
        // uninitialised framebuffer on screen indefinitely.
        if (!wasEnabled && enabled)
            forceRepaint();
    }
}

bool GpuRenderCoordinator::isGpuRenderingEnabled() const { return glManager_ && glManager_->isGpuRenderingEnabled(); }

void GpuRenderCoordinator::updateRendering(const std::vector<std::unique_ptr<PaneComponent>>& panes)
{
    // Update GL renderer with waveform data (if GPU mode enabled)
    if (glManager_ && glManager_->isGpuRenderingEnabled())
    {
        bool const hasSignal = glManager_->updateWaveformData(panes);
        // Gate repaints on actual signal activity + a short post-signal tail.
        // The tail ensures fade-outs / envelope decays still render smoothly
        // for a few frames after the source goes silent.
        if (hasSignal)
        {
            silentFrames_ = 0;
            glManager_->triggerRepaint();
        }
        else if (silentFrames_ < kPostSilenceFrames)
        {
            ++silentFrames_;
            glManager_->triggerRepaint();
        }
        // else: truly idle — skip the repaint.  No VSync wake, no GPU work.
    }
    else
    {
        // Trigger repaint of waveform components (only needed for software rendering)
        for (const auto& pane : panes)
        {
            if (pane)
                pane->repaint();
        }
    }
}

void GpuRenderCoordinator::forceRepaint()
{
    // Called when a non-signal event (UI change, theme, layout) needs to
    // refresh the scene even though audio activity alone wouldn't.
    silentFrames_ = 0;
    if (glManager_ && glManager_->isGpuRenderingEnabled())
        glManager_->triggerRepaint();
}

void GpuRenderCoordinator::propagateGpuStateToPanes(const std::vector<std::unique_ptr<PaneComponent>>& panes) const
{
    bool const enabled = isGpuRenderingEnabled();
    for (const auto& pane : panes)
    {
        if (pane)
        {
            for (size_t i = 0; i < pane->getOscillatorCount(); ++i)
            {
                if (auto* waveform = pane->getWaveformAt(i))
                {
                    waveform->setGpuRenderingEnabled(enabled);
                }
            }
        }
    }
}

void GpuRenderCoordinator::clearWaveforms()
{
    if (glManager_)
    {
        glManager_->clearAllWaveforms();
    }
}

void GpuRenderCoordinator::detach()
{
    if (glManager_)
    {
        glManager_->detach();
    }
}

} // namespace oscil
