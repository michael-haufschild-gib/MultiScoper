/*
    Oscil - GPU Render Coordinator
    Manages OpenGL lifecycle and rendering state propagation
*/

#pragma once

#include "ui/layout/PaneComponent.h"
#include "ui/panels/StatusBarComponent.h"

#include "plugin/OpenGLLifecycleManager.h"

#include <memory>
#include <vector>

namespace oscil
{

class OscilPluginEditor;

class GpuRenderCoordinator
{
public:
    GpuRenderCoordinator(OscilPluginEditor& editor, StatusBarComponent& statusBar);
    ~GpuRenderCoordinator();

    void setGpuRenderingEnabled(bool enabled);
    bool isGpuRenderingEnabled() const;

    void updateRendering(const std::vector<std::unique_ptr<PaneComponent>>& panes);
    void propagateGpuStateToPanes(const std::vector<std::unique_ptr<PaneComponent>>& panes);
    void clearWaveforms();
    void detach();

private:
    std::unique_ptr<OpenGLLifecycleManager> glManager_;
    StatusBarComponent& statusBar_;
};

} // namespace oscil
