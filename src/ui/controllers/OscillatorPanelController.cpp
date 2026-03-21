/*
    Oscil - Oscillator Panel Controller Implementation
*/

#include "ui/controllers/OscillatorPanelController.h"
#include "ui/managers/DialogManager.h"
#include "core/OscilState.h"
#include "core/InstanceRegistry.h"

namespace oscil
{

OscillatorPanelController::OscillatorPanelController(OscilPluginProcessor& processor,
                                                     ServiceContext& serviceContext,
                                                     PaneContainerComponent& container,
                                                     GpuRenderCoordinator& gpuCoordinator)
    : processor_(processor)
    , serviceContext_(serviceContext)
    , container_(container)
    , gpuCoordinator_(gpuCoordinator)
{
}

OscillatorPanelController::~OscillatorPanelController()
{
    processor_.getState().removeListener(this);
}

void OscillatorPanelController::initialize(SidebarComponent* sidebar, DialogManager* dialogManager, DisplaySettingsManager* displaySettings)
{
    sidebar_ = sidebar;
    dialogManager_ = dialogManager;
    displaySettings_ = displaySettings;

    // Listen to state changes
    processor_.getState().addListener(this);

    // Setup container callbacks
    container_.setPaneDropCallback([this](const PaneId& moved, const PaneId& target) {
        handlePaneReordered(moved, target);
    });
    container_.setEmptyColumnDropCallback([this](const PaneId& moved, int col) {
        handleEmptyColumnDrop(moved, col);
    });
}

void OscillatorPanelController::refreshPanels()
{
    // Prevent recursion; queue one trailing refresh pass if needed.
    if (isUpdating_)
    {
        pendingRefresh_ = true;
        return;
    }

    isUpdating_ = true;
    pendingRefresh_ = false;

    // Clear GL renderer waveforms first to prevent stale data
    gpuCoordinator_.clearWaveforms();

    // Clear existing components
    paneComponents_.clear();
    
    // Also clear from DisplaySettingsManager BEFORE recreating
    // (DisplaySettingsManager holds raw pointers to components we just deleted)
    // We need a way to tell DisplaySettingsManager to clear its list, 
    // or simply re-register later. The manager likely rebuilds on demand or we pass the new list.
    // Since DisplaySettingsManager::registerPanes isn't a method, we rely on
    // the fact that it usually stores a reference to the vector or we pass it in methods.
    // Looking at existing code: DisplaySettingsManager takes the vector reference in constructor.
    // So clearing `paneComponents_` updates the manager's view automatically (as long as it doesn't cache).

    // Get state
    auto oscillators = processor_.getState().getOscillators();
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Recreate components
    createPaneComponents(oscillators, layoutManager);

    // Update Sidebar
    if (sidebar_)
    {
        // Refresh lists
        auto sources = processor_.getInstanceRegistry().getAllSources();
        sidebar_->refreshSourceList(sources);

        auto panes = layoutManager.getPanes();
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this), panes]() {
            if (auto* controller = weakThis.get())
                if (controller->sidebar_) controller->sidebar_->refreshPaneList(panes);
        });

        // Sort oscillators by order
        std::vector<Oscillator> sortedOscs = oscillators;
        std::sort(sortedOscs.begin(), sortedOscs.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });
        
        sidebar_->refreshOscillatorList(sortedOscs);
    }

    // Apply settings
    reapplyGlobalSettings();

    // Propagate GPU state
    gpuCoordinator_.propagateGpuStateToPanes(paneComponents_);

    // CRITICAL: Notify that layout needs to be updated
    // The container has no resized() override, so we must trigger layout externally
    if (layoutNeededCallback_)
        layoutNeededCallback_();

    isUpdating_ = false;

    // Process any refresh request that arrived while we were rebuilding.
    if (pendingRefresh_)
    {
        pendingRefresh_ = false;
        refreshPanels();
    }
}

void OscillatorPanelController::createPaneComponents(const std::vector<Oscillator>& oscillators, const PaneLayoutManager& layoutManager)
{
    // Group oscillators by pane
    std::map<juce::String, std::vector<Oscillator>> oscillatorsByPane;
    for (const auto& osc : oscillators)
    {
        oscillatorsByPane[osc.getPaneId().id].push_back(osc);
    }

    int paneIndex = 0;
    for (const auto& pane : layoutManager.getPanes())
    {
        auto paneComponent = std::make_unique<PaneComponent>(processor_, serviceContext_, pane.getId());
        paneComponent->setPaneIndex(paneIndex++);

        // Set up drag-and-drop reorder callback - managed by Container now, but component initiates drag
        paneComponent->onPaneReordered([this](const PaneId& moved, const PaneId& target) {
            handlePaneReordered(moved, target);
        });

        // Set up pane close callback
        paneComponent->onPaneCloseRequested([this](const PaneId& id) {
            handlePaneClose(id);
        });

        // Set up pane name change callback
        paneComponent->onPaneNameChanged([this](const PaneId& paneId, const juce::String& newName) {
            auto& mgr = processor_.getState().getLayoutManager();
            if (auto* p = mgr.getPane(paneId))
            {
                p->setName(newName);
            }
        });

        // Add oscillators to pane
        auto it = oscillatorsByPane.find(pane.getId().id);
        if (it != oscillatorsByPane.end())
        {
            for (const auto& osc : it->second)
            {
                paneComponent->addOscillator(osc);
            }
        }

        // Set initial GPU state
        bool gpuEnabled = gpuCoordinator_.isGpuRenderingEnabled();
        for (size_t i = 0; i < paneComponent->getOscillatorCount(); ++i)
        {
            if (auto* waveform = paneComponent->getWaveformAt(i))
            {
                waveform->setGpuRenderingEnabled(gpuEnabled);
            }
        }

        container_.addAndMakeVisible(*paneComponent);
        paneComponents_.push_back(std::move(paneComponent));
    }
}

void OscillatorPanelController::reapplyGlobalSettings()
{
    auto& state = processor_.getState();
    
    // Re-apply timing settings (display samples)
    // IMPORTANT: Use capture rate (decimated), not source rate, since display buffers are decimated
    auto timingConfig = processor_.getTimingEngine().toEntityConfig();
    int captureRate = processor_.getCaptureRate();
    if (captureRate > 0)
    {
        int displaySamples = static_cast<int>(static_cast<double>(captureRate) * (static_cast<double>(timingConfig.actualIntervalMs) / 1000.0));
        if (displaySettings_)
        {
            displaySettings_->setDisplaySamplesForAll(displaySamples);
            displaySettings_->setSampleRateForAll(captureRate);
        }
    }

    // Re-apply visual settings
    if (displaySettings_)
    {
        auto& timingEngine = processor_.getTimingEngine();
        auto hostInfo = timingEngine.getHostInfo();
        auto engineConfig = timingEngine.getConfig();

        GridConfiguration gridConfig;
        gridConfig.enabled = state.isShowGridEnabled();
        gridConfig.timingMode = engineConfig.timingMode;
        gridConfig.visibleDurationMs = timingEngine.getActualIntervalMs();
        gridConfig.noteInterval = timingEngine.getNoteIntervalAsEntity();
        gridConfig.bpm = static_cast<float>(hostInfo.bpm);
        gridConfig.timeSigNumerator = hostInfo.timeSigNumerator;
        gridConfig.timeSigDenominator = hostInfo.timeSigDenominator;

        displaySettings_->setGridConfigForAll(gridConfig);
        displaySettings_->setShowGridForAll(state.isShowGridEnabled());
        displaySettings_->setAutoScaleForAll(state.isAutoScaleEnabled());
        displaySettings_->setGainDbForAll(state.getGainDb());
    }
}

void OscillatorPanelController::createDefaultOscillatorIfNeeded()
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();

    if (layoutManager.getPaneCount() == 0 && state.getOscillators().empty())
    {
        // Need sources to create meaningful default
        // Just create placeholder if no sources yet, or use first source
        
        // Create default pane
        Pane defaultPane;
        defaultPane.setName("Pane 1");
        defaultPane.setOrderIndex(0);
        layoutManager.addPane(defaultPane);

        Oscillator osc;
        osc.setPaneId(defaultPane.getId());
        osc.setProcessingMode(ProcessingMode::FullStereo);
        osc.setName("Oscillator 1");
        osc.setVisualPresetId("default");
        
        // Only auto-bind to this processor's own source.
        // If registration is not ready yet, keep NO_SOURCE so later source
        // notifications can bind correctly instead of latching onto a foreign source.
        if (processor_.getSourceId().isValid())
        {
            osc.setSourceId(processor_.getSourceId());
        }

        state.addOscillator(osc);
    }
}

void OscillatorPanelController::handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId)
{
    auto& layoutManager = processor_.getState().getLayoutManager();
    const Pane* sourcePanePtr = layoutManager.getPane(movedPaneId);
    const Pane* targetPanePtr = layoutManager.getPane(targetPaneId);

    if (!sourcePanePtr || !targetPanePtr) return;

    int sourceColumn = sourcePanePtr->getColumnIndex();
    int targetColumn = targetPanePtr->getColumnIndex();

    if (sourceColumn != targetColumn)
    {
        // Cross-column move
        auto targetColumnPanes = layoutManager.getPanesInColumn(targetColumn);
        int positionInColumn = 0;
        for (size_t i = 0; i < targetColumnPanes.size(); ++i)
        {
            if (targetColumnPanes[i]->getId() == targetPaneId)
            {
                positionInColumn = static_cast<int>(i);
                break;
            }
        }
        layoutManager.movePaneToColumn(movedPaneId, targetColumn, positionInColumn);
    }
    else
    {
        // Same column move
        int targetIndex = -1;
        const auto& panes = layoutManager.getPanes();
        for (size_t i = 0; i < panes.size(); ++i)
        {
            if (panes[i].getId() == targetPaneId)
            {
                targetIndex = static_cast<int>(i);
                break;
            }
        }
        if (targetIndex >= 0)
        {
            layoutManager.movePane(movedPaneId, targetIndex);
        }
    }

    refreshPanels();
}

void OscillatorPanelController::handleEmptyColumnDrop(const PaneId& movedPaneId, int targetColumn)
{
    auto& layoutManager = processor_.getState().getLayoutManager();
    layoutManager.movePaneToColumn(movedPaneId, targetColumn, 0);
    refreshPanels();
}

void OscillatorPanelController::handlePaneClose(const PaneId& paneId)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();

    // Hide oscillators in this pane
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getPaneId() == paneId)
        {
            osc.setPaneId(PaneId::invalid());
            osc.setVisible(false);
            state.updateOscillator(osc);
        }
    }

    layoutManager.removePane(paneId);
    refreshPanels();
}

void OscillatorPanelController::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (displaySettings_) displaySettings_->highlightOscillator(oscillatorId);
}


// Sidebar listener overrides, dialog handlers, and ValueTree listeners
// are in OscillatorPanelControllerHandlers.cpp

} // namespace oscil
