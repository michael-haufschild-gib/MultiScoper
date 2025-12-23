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
    // Prevent recursion
    if (isUpdating_) return;
    isUpdating_ = true;

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
        bool autoScaleEnabled = processor_.getState().isAutoScaleEnabled(); // Get auto-scale state
        
        for (size_t i = 0; i < paneComponent->getOscillatorCount(); ++i)
        {
            if (auto* waveform = paneComponent->getWaveformAt(i))
            {
                waveform->setGpuRenderingEnabled(gpuEnabled);
                waveform->setAutoScale(autoScaleEnabled); // Explicitly set auto-scale
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
        
        // Try to find a source
        auto sources = processor_.getInstanceRegistry().getAllSources();
        if (!sources.empty())
        {
            osc.setSourceId(sources[0].sourceId);
        }
        else if (processor_.getSourceId().isValid())
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

// SidebarComponent::Listener overrides

void OscillatorPanelController::oscillatorSelected(const OscillatorId& oscillatorId)
{
    highlightOscillator(oscillatorId);
}

void OscillatorPanelController::oscillatorConfigRequested(const OscillatorId& oscillatorId)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            auto& layoutManager = state.getLayoutManager();
            auto panes = layoutManager.getPanes();
            std::vector<std::pair<PaneId, juce::String>> paneList;
            for (const auto& pane : panes)
            {
                paneList.emplace_back(pane.getId(), pane.getName());
            }

            if (dialogManager_)
                dialogManager_->showConfigPopup(osc, paneList);
            break;
        }
    }
}

void OscillatorPanelController::oscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_) return;

    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            dialogManager_->showColorDialog(osc.getColour(), [this, oscillatorId](juce::Colour color) {
                auto& oscilState = processor_.getState();
                auto oscList = oscilState.getOscillators();
                for (auto& o : oscList)
                {
                    if (o.getId() == oscillatorId)
                    {
                        o.setColour(color);
                        oscilState.updateOscillator(o);
                        break;
                    }
                }
            });
            break;
        }
    }
}

void OscillatorPanelController::oscillatorDeleteRequested(const OscillatorId& oscillatorId)
{
    processor_.getState().removeOscillator(oscillatorId);
}

void OscillatorPanelController::oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setProcessingMode(mode);
            state.updateOscillator(osc);
            break;
        }
    }
}

void OscillatorPanelController::oscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setVisible(visible);
            state.updateOscillator(osc);
            break;
        }
    }
}

void OscillatorPanelController::oscillatorPaneSelectionRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_) return;
    pendingVisibilityOscillatorId_ = oscillatorId;
    
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showSelectPaneDialog(panes,
        // onComplete callback
        [this](const SelectPaneDialog::Result& result) {
            auto& stateRef = processor_.getState();
            auto& layoutMgr = stateRef.getLayoutManager();
            PaneId targetPaneId = result.paneId;

            if (result.createNewPane)
            {
                Pane newPane;
                newPane.setName("Pane " + juce::String(layoutMgr.getPaneCount() + 1));
                newPane.setOrderIndex(static_cast<int>(layoutMgr.getPaneCount()));
                layoutMgr.addPane(newPane);
                targetPaneId = newPane.getId();
            }

            auto oscList = stateRef.getOscillators();
            for (auto& osc : oscList)
            {
                if (osc.getId() == pendingVisibilityOscillatorId_)
                {
                    osc.setPaneId(targetPaneId);
                    osc.setVisible(true);
                    stateRef.updateOscillator(osc);
                    break;
                }
            }
            pendingVisibilityOscillatorId_ = OscillatorId::invalid();
        },
        // onCancel callback - clear pending ID when user cancels
        [this]() {
            pendingVisibilityOscillatorId_ = OscillatorId::invalid();
        });
}

void OscillatorPanelController::addOscillatorDialogRequested()
{
    if (!dialogManager_) return;
    auto sources = processor_.getInstanceRegistry().getAllSources();
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showAddOscillatorDialog(sources, panes, [this](const AddOscillatorDialog::Result& result) {
        addOscillatorRequested(result);
    });
}

void OscillatorPanelController::addOscillatorRequested(const AddOscillatorDialog::Result& result)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();
    PaneId targetPaneId = result.paneId;

    if (result.createNewPane)
    {
        Pane newPane;
        newPane.setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
        newPane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
        layoutManager.addPane(newPane);
        targetPaneId = newPane.getId();
    }

    Oscillator osc;
    osc.setSourceId(result.sourceId);
    osc.setPaneId(targetPaneId);
    osc.setProcessingMode(ProcessingMode::FullStereo);
    osc.setColour(result.color);
    osc.setVisualPresetId(result.visualPresetId);
    
    if (result.name.isNotEmpty()) osc.setName(result.name);
    else osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));

    // Ensure shader matches preset
    auto preset = VisualConfiguration::getPreset(result.visualPresetId);
    osc.setShaderId(shaderTypeToId(preset.shaderType));

    state.addOscillator(osc);
}

void OscillatorPanelController::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setSourceId(newSourceId);
            state.updateOscillator(osc);

            // Optimization: Update in-place if possible
            for (auto& pane : paneComponents_)
            {
                if (pane && osc.getPaneId() == pane->getPaneId())
                {
                    pane->updateOscillatorSource(oscillatorId, newSourceId);
                    break;
                }
            }
            
            if (sidebar_)
            {
                sidebar_->refreshOscillatorList(state.getOscillators());
            }
            return;
        }
    }
}

// ValueTree Listeners

void OscillatorPanelController::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(StateIds::Oscillator))
    {
        OscillatorId oscId{tree.getProperty(StateIds::Id).toString()};
        
        // Optimization for simple properties
        if (property == StateIds::Name || property == StateIds::Colour || 
            property == StateIds::ProcessingMode || property == StateIds::Visible ||
            property == StateIds::SourceId)
        {
            juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this), oscId, property]() {
                auto* controller = weakThis.get();
                if (!controller) return;

                // Find oscillator details
                auto oscillators = controller->processor_.getState().getOscillators();
                bool handled = false;
                
                for (const auto& osc : oscillators)
                {
                    if (osc.getId() == oscId)
                    {
                        if (controller->sidebar_)
                        {
                             controller->sidebar_->refreshOscillatorList(oscillators);
                        }

                        for (auto& pane : controller->paneComponents_)
                        {
                            if (pane && pane->getPaneId() == osc.getPaneId())
                            {
                                if (property == StateIds::Name) pane->updateOscillatorName(oscId, osc.getName());
                                else if (property == StateIds::Colour) pane->updateOscillatorColor(oscId, osc.getColour());
                                else if (property == StateIds::ProcessingMode || property == StateIds::Visible)
                                    pane->updateOscillator(oscId, osc.getProcessingMode(), osc.isVisible());
                                else if (property == StateIds::SourceId) pane->updateOscillatorSource(oscId, osc.getSourceId());
                                
                                handled = true;
                                break; 
                            }
                        }
                        break;
                    }
                }
                
                if (!handled)
                {
                    controller->refreshPanels();
                }
            });
        }
        else
        {
            // Fallback for complex properties
            juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() { 
                if (auto* controller = weakThis.get()) controller->refreshPanels(); 
            });
        }
    }
    else if (tree.hasType(StateIds::Pane))
    {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() { 
            if (auto* controller = weakThis.get()) controller->refreshPanels(); 
        });
    }
}

void OscillatorPanelController::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree& child)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() { 
            if (auto* controller = weakThis.get()) controller->refreshPanels(); 
        });
    }
}

void OscillatorPanelController::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree& child, int)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        // Close dialog if open
        if (dialogManager_ && child.hasType(StateIds::Oscillator))
        {
            OscillatorId oid{child.getProperty(StateIds::Id).toString()};
            if (dialogManager_->isConfigPopupVisibleFor(oid))
            {
                dialogManager_->closeConfigPopup();
            }
        }

        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() { 
            if (auto* controller = weakThis.get()) controller->refreshPanels(); 
        });
    }
}

void OscillatorPanelController::valueTreeChildOrderChanged(juce::ValueTree& parent, int, int)
{
    if (parent.hasType(StateIds::Oscillators) || parent.hasType(StateIds::Panes))
    {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() { 
            if (auto* controller = weakThis.get()) controller->refreshPanels(); 
        });
    }
}

void OscillatorPanelController::valueTreeParentChanged(juce::ValueTree&) {}

void OscillatorPanelController::gainChanged(float dB)
{
    // Update state
    processor_.getState().setGainDb(dB);
    
    // Apply to panes immediately
    if (displaySettings_)
    {
        displaySettings_->setGainDbForAll(dB);
    }
}

void OscillatorPanelController::autoScaleChanged(bool enabled)
{
    // Update state
    processor_.getState().setAutoScaleEnabled(enabled);
    
    // Apply to panes immediately
    if (displaySettings_)
    {
        displaySettings_->setAutoScaleForAll(enabled);
    }
}

void OscillatorPanelController::showGridChanged(bool enabled)
{
    // Update state
    processor_.getState().setShowGridEnabled(enabled);
    
    // Apply to panes immediately
    if (displaySettings_)
    {
        displaySettings_->setShowGridForAll(enabled);
    }
}

} // namespace oscil
