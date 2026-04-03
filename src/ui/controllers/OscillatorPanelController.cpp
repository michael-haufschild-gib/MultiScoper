/*
    Oscil - Oscillator Panel Controller Implementation
*/

#include "ui/controllers/OscillatorPanelController.h"

#include "core/OscilLog.h"
#include "core/OscilState.h"
#include "core/dsp/TimingEngine.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/controllers/GpuRenderCoordinator.h"
#include "ui/managers/DialogManager.h"

namespace oscil
{

OscillatorPanelController::OscillatorPanelController(IAudioDataProvider& dataProvider, ServiceContext& serviceContext,
                                                     PaneContainerComponent& container,
                                                     GpuRenderCoordinator& gpuCoordinator)
    : dataProvider_(dataProvider)
    , serviceContext_(serviceContext)
    , container_(container)
    , gpuCoordinator_(gpuCoordinator)
{
}

OscillatorPanelController::~OscillatorPanelController() { dataProvider_.getState().removeListener(this); }

void OscillatorPanelController::initialize(SidebarComponent* sidebar, DialogManager* dialogManager,
                                           DisplaySettingsManager* displaySettings)
{
    sidebar_ = sidebar;
    dialogManager_ = dialogManager;
    displaySettings_ = displaySettings;

    // Listen to state changes
    dataProvider_.getState().addListener(this);

    // Setup container callbacks
    container_.setPaneDropCallback(
        [this](const PaneId& moved, const PaneId& target) { handlePaneReordered(moved, target); });
    container_.setEmptyColumnDropCallback([this](const PaneId& moved, int col) { handleEmptyColumnDrop(moved, col); });
}

void OscillatorPanelController::refreshSidebar(const std::vector<Oscillator>& oscillators,
                                               const PaneLayoutManager& layoutManager)
{
    if (!sidebar_)
        return;

    sidebar_->refreshSourceList(serviceContext_.instanceRegistry.getAllSources());
    sidebar_->refreshPaneList(layoutManager.getPanes());

    std::vector<Oscillator> sortedOscs = oscillators;
    std::ranges::sort(sortedOscs,
                      [](const Oscillator& a, const Oscillator& b) { return a.getOrderIndex() < b.getOrderIndex(); });
    sidebar_->refreshOscillatorList(sortedOscs);
}

void OscillatorPanelController::handleAsyncUpdate() { refreshPanels(); }

void OscillatorPanelController::refreshPanels()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // A synchronous refresh supersedes any pending async one.
    cancelPendingUpdate();

    if (isUpdating_)
    {
        OSCIL_LOG(CONTROLLER, "refreshPanels: DEFERRED (already updating)");
        pendingRefresh_ = true;
        return;
    }

    static constexpr int kMaxRefreshIterations = 10;

    for (int iteration = 0; iteration < kMaxRefreshIterations; ++iteration)
    {
        isUpdating_ = true;
        pendingRefresh_ = false;

        gpuCoordinator_.clearWaveforms();
        paneComponents_.clear();

        auto oscillators = dataProvider_.getState().getOscillators();
        auto& layoutManager = dataProvider_.getState().getLayoutManager();

        OSCIL_LOG(CONTROLLER, "refreshPanels: "
                                  << oscillators.size() << " oscillators, " << layoutManager.getPaneCount() << " panes"
                                  << (iteration > 0 ? " (iteration " + juce::String(iteration) + ")" : ""));

        createPaneComponents(oscillators, layoutManager);
        refreshSidebar(oscillators, layoutManager);
        reapplyGlobalSettings();
        gpuCoordinator_.propagateGpuStateToPanes(paneComponents_);

        if (layoutNeededCallback_)
            layoutNeededCallback_();

        isUpdating_ = false;

        if (!pendingRefresh_)
            return;

        OSCIL_LOG(CONTROLLER, "refreshPanels: processing queued refresh");
    }

    jassertfalse; // unexpected refresh reentrancy loop
    OSCIL_LOG(CONTROLLER, "refreshPanels: WARNING — hit max iteration limit (" << kMaxRefreshIterations << ")");
    triggerAsyncUpdate(); // preserve the final queued refresh for the next async cycle
}

void OscillatorPanelController::createPaneComponents(const std::vector<Oscillator>& oscillators,
                                                     const PaneLayoutManager& layoutManager)
{
    OSCIL_LOG(CONTROLLER, "createPaneComponents: " << layoutManager.getPaneCount() << " panes, " << oscillators.size()
                                                   << " oscillators");
    // Group oscillators by pane
    std::map<juce::String, std::vector<Oscillator>> oscillatorsByPane;
    for (const auto& osc : oscillators)
    {
        oscillatorsByPane[osc.getPaneId().id].push_back(osc);
    }

    int paneIndex = 0;
    for (const auto& pane : layoutManager.getPanes())
    {
        auto paneComponent = std::make_unique<PaneComponent>(dataProvider_, serviceContext_, pane.getId());
        paneComponent->setTestId("pane_" + pane.getId().id);
        paneComponent->setPaneIndex(paneIndex++);

        // Set up drag-and-drop reorder callback - managed by Container now, but component initiates drag
        paneComponent->onPaneReordered(
            [this](const PaneId& moved, const PaneId& target) { handlePaneReordered(moved, target); });

        // Set up pane close callback
        paneComponent->onPaneCloseRequested([this](const PaneId& id) { handlePaneClose(id); });

        // Set up pane name change callback
        paneComponent->onPaneNameChanged([this](const PaneId& paneId, const juce::String& newName) {
            auto& mgr = dataProvider_.getState().getLayoutManager();
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

        // GPU state is propagated after all panes are created via
        // gpuCoordinator_.propagateGpuStateToPanes() in refreshPanels()

        container_.addAndMakeVisible(*paneComponent);
        paneComponents_.push_back(std::move(paneComponent));
    }
}

void OscillatorPanelController::reapplyGlobalSettings()
{
    auto& state = dataProvider_.getState();

    // Re-apply timing settings (display samples)
    // IMPORTANT: Use capture rate (decimated), not source rate, since display buffers are decimated
    auto timingConfig = dataProvider_.getTimingEngine().toEntityConfig();
    int const captureRate = dataProvider_.getCaptureRate();
    juce::Logger::writeToLog("[Controller] reapplyGlobalSettings: captureRate=" + juce::String(captureRate) +
                             " actualIntervalMs=" + juce::String(timingConfig.actualIntervalMs) +
                             " hostBPM=" + juce::String(timingConfig.hostBPM) +
                             " timingMode=" + juce::String(static_cast<int>(timingConfig.timingMode)));
    if (captureRate > 0)
    {
        int const displaySamples = static_cast<int>(static_cast<double>(captureRate) *
                                                    (static_cast<double>(timingConfig.actualIntervalMs) / 1000.0));
        juce::Logger::writeToLog("[Controller] displaySamples=" + juce::String(displaySamples));
        if (displaySettings_)
        {
            displaySettings_->setDisplaySamplesForAll(displaySamples);
            displaySettings_->setSampleRateForAll(captureRate);
        }
    }

    // Re-apply visual settings
    if (displaySettings_)
    {
        auto& timingEngine = dataProvider_.getTimingEngine();
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
    auto& state = dataProvider_.getState();
    auto& layoutManager = state.getLayoutManager();

    if (layoutManager.getPaneCount() == 0 && state.getOscillators().empty())
    {
        OSCIL_LOG(CONTROLLER, "createDefaultOscillatorIfNeeded: creating default pane + oscillator"
                                  << " sourceId=" << dataProvider_.getSourceId().id);
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
        if (dataProvider_.getSourceId().isValid())
        {
            osc.setSourceId(dataProvider_.getSourceId());
        }

        state.addOscillator(osc);
    }
}

void OscillatorPanelController::handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId)
{
    OSCIL_LOG(CONTROLLER, "handlePaneReordered: moved=" << movedPaneId.id << " target=" << targetPaneId.id);
    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    const Pane* sourcePanePtr = layoutManager.getPane(movedPaneId);
    const Pane* targetPanePtr = layoutManager.getPane(targetPaneId);

    if (!sourcePanePtr || !targetPanePtr)
        return;

    int const sourceColumn = sourcePanePtr->getColumnIndex();
    int const targetColumn = targetPanePtr->getColumnIndex();

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
    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    layoutManager.movePaneToColumn(movedPaneId, targetColumn, 0);
    refreshPanels();
}

void OscillatorPanelController::handlePaneClose(const PaneId& paneId)
{
    OSCIL_LOG(CONTROLLER, "handlePaneClose: paneId=" << paneId.id);
    auto& state = dataProvider_.getState();
    auto& layoutManager = state.getLayoutManager();

    // Hide oscillators that were on the closed pane and clear their pane assignment
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getPaneId() == paneId)
        {
            osc.setVisible(false);
            osc.setPaneId(PaneId::invalid());
            state.updateOscillator(osc);
        }
    }

    layoutManager.removePane(paneId);
    refreshPanels();
}

void OscillatorPanelController::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (displaySettings_)
        displaySettings_->highlightOscillator(oscillatorId);
}

// Sidebar listener overrides, dialog handlers, and ValueTree listeners
// are in OscillatorPanelControllerHandlers.cpp

} // namespace oscil
