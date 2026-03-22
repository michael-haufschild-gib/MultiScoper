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

GpuRenderCoordinator::~GpuRenderCoordinator()
{
    detach();
}

void GpuRenderCoordinator::setGpuRenderingEnabled(bool enabled)
{
    if (glManager_)
    {
        glManager_->setGpuRenderingEnabled(enabled);
        statusBar_.setRenderingMode(enabled ? RenderingMode::OpenGL : RenderingMode::Software);
    }
}

bool GpuRenderCoordinator::isGpuRenderingEnabled() const
{
    return glManager_ && glManager_->isGpuRenderingEnabled();
}

void GpuRenderCoordinator::updateRendering(const std::vector<std::unique_ptr<PaneComponent>>& panes)
{
    // Update GL renderer with waveform data (if GPU mode enabled)
    if (glManager_ && glManager_->isGpuRenderingEnabled())
    {
        glManager_->updateWaveformData(panes);
    }
    else
    {
        // Trigger repaint of waveform components (only needed for software rendering)
        for (auto& pane : panes)
        {
            if (pane)
                pane->repaint();
        }
    }
}

void GpuRenderCoordinator::propagateGpuStateToPanes(const std::vector<std::unique_ptr<PaneComponent>>& panes)
{
    bool enabled = isGpuRenderingEnabled();
    for (auto& pane : panes)
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
