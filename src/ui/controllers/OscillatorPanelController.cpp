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
    sidebar_ = sidebar;       // WeakReference assignment
    dialogManager_ = dialogManager;
    displaySettings_ = displaySettings;

    // Listen to state changes
    processor_.getState().addListener(this);

    // Setup container callbacks - use WeakReference to prevent dangling this capture
    juce::WeakReference<OscillatorPanelController> weakThis(this);
    container_.setPaneDropCallback([weakThis](const PaneId& moved, const PaneId& target) {
        if (auto* controller = weakThis.get())
            controller->handlePaneReordered(moved, target);
    });
    container_.setEmptyColumnDropCallback([weakThis](const PaneId& moved, int col) {
        if (auto* controller = weakThis.get())
            controller->handleEmptyColumnDrop(moved, col);
    });
}

void OscillatorPanelController::scheduleRefresh()
{
    // Coalesce multiple rapid state changes into a single refresh
    if (!refreshPending_.exchange(true))
    {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this)]() {
            if (auto* controller = weakThis.get())
            {
                controller->refreshPending_.store(false);
                controller->refreshPanels();
            }
        });
    }
}

void OscillatorPanelController::refreshPanels()
{
    // Prevent recursion using atomic compare-exchange
    bool expected = false;
    if (!isUpdating_.compare_exchange_strong(expected, true))
        return;

    // RAII guard to ensure isUpdating_ is reset even if exception occurs
    struct ScopeGuard {
        std::atomic<bool>& flag_;
        ~ScopeGuard() { flag_.store(false); }
    } guard{isUpdating_};

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
    if (auto* sidebarPtr = sidebar_.get())
    {
        // Refresh lists
        auto sources = processor_.getInstanceRegistry().getAllSources();
        sidebarPtr->refreshSourceList(sources);

        auto panes = layoutManager.getPanes();
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this), panes]() {
            if (auto* controller = weakThis.get())
                if (auto* sb = controller->sidebar_.get())
                    sb->refreshPaneList(panes);
        });

        // Sort oscillators by order
        std::vector<Oscillator> sortedOscs = oscillators;
        std::sort(sortedOscs.begin(), sortedOscs.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });
        
        sidebarPtr->refreshOscillatorList(sortedOscs);
    }

    // Apply settings
    reapplyGlobalSettings();

    // Propagate GPU state
    gpuCoordinator_.propagateGpuStateToPanes(paneComponents_);

    // CRITICAL: Notify that layout needs to be updated
    // The container has no resized() override, so we must trigger layout externally
    if (layoutNeededCallback_)
        layoutNeededCallback_();

    // Note: isUpdating_ is reset automatically by ScopeGuard destructor
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

        // Set up pane callbacks - use WeakReference to prevent dangling this capture
        juce::WeakReference<OscillatorPanelController> weakThis(this);

        // Set up drag-and-drop reorder callback - managed by Container now, but component initiates drag
        paneComponent->onPaneReordered([weakThis](const PaneId& moved, const PaneId& target) {
            if (auto* controller = weakThis.get())
                controller->handlePaneReordered(moved, target);
        });

        // Set up pane close callback
        paneComponent->onPaneCloseRequested([weakThis](const PaneId& id) {
            if (auto* controller = weakThis.get())
                controller->handlePaneClose(id);
        });

        // Set up pane name change callback
        paneComponent->onPaneNameChanged([weakThis](const PaneId& paneId, const juce::String& newName) {
            if (auto* controller = weakThis.get())
            {
                auto& mgr = controller->processor_.getState().getLayoutManager();
                if (auto* p = mgr.getPane(paneId))
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
        
        // Update PaneComponent opaque state for proper JUCE/GL interaction
        paneComponent->setGpuRenderingEnabled(gpuEnabled);
        
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
        if (auto* settings = displaySettings_.get())
        {
            settings->setDisplaySamplesForAll(displaySamples);
            settings->setSampleRateForAll(captureRate);
        }
    }

    // Re-apply visual settings
    if (auto* settings = displaySettings_.get())
    {
        settings->setShowGridForAll(state.isShowGridEnabled());
        settings->setAutoScaleForAll(state.isAutoScaleEnabled());
        settings->setGainDbForAll(state.getGainDb());
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
            // Use setSourceIdNameAndUUID to capture all identifiers for persistence
            osc.setSourceIdNameAndUUID(sources[0].sourceId, sources[0].name, sources[0].instanceUUID);
        }
        else if (processor_.getSourceId().isValid())
        {
            // Look up source info from registry for persistence
            auto sourceInfo = processor_.getInstanceRegistry().getSource(processor_.getSourceId());
            if (sourceInfo.has_value())
                osc.setSourceIdNameAndUUID(processor_.getSourceId(), sourceInfo->name, sourceInfo->instanceUUID);
            else
                osc.setSourceIdNameAndUUID(processor_.getSourceId(), "Unknown Source", ""); // Fallback with display name
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
    
    // Force repaint of container to clear any ghost images
    container_.repaint();
}

void OscillatorPanelController::handleEmptyColumnDrop(const PaneId& movedPaneId, int targetColumn)
{
    auto& layoutManager = processor_.getState().getLayoutManager();
    layoutManager.movePaneToColumn(movedPaneId, targetColumn, 0);
    refreshPanels();
    
    // Force repaint of container to clear any ghost images
    container_.repaint();
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
    if (auto* settings = displaySettings_.get())
        settings->highlightOscillator(oscillatorId);
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

            if (auto* dialog = dialogManager_.get())
                dialog->showConfigPopup(osc, paneList);
            break;
        }
    }
}

void OscillatorPanelController::oscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    auto* dialog = dialogManager_.get();
    if (!dialog) return;

    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            juce::WeakReference<OscillatorPanelController> weakThis(this);
            dialog->showColorDialog(osc.getColour(), [weakThis, oscillatorId](juce::Colour color) {
                if (auto* controller = weakThis.get())
                {
                    auto& oscilState = controller->processor_.getState();
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

void OscillatorPanelController::oscillatorsReordered(int fromIndex, int toIndex)
{
    processor_.getState().reorderOscillators(fromIndex, toIndex);
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
    auto* dialog = dialogManager_.get();
    if (!dialog) return;
    pendingVisibilityOscillatorId_ = oscillatorId;

    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    juce::WeakReference<OscillatorPanelController> weakThis(this);
    dialog->showSelectPaneDialog(panes,
        // onComplete callback
        [weakThis](const SelectPaneDialog::Result& result) {
            if (auto* controller = weakThis.get())
            {
                auto& stateRef = controller->processor_.getState();
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
                    if (osc.getId() == controller->pendingVisibilityOscillatorId_)
                    {
                        osc.setPaneId(targetPaneId);
                        osc.setVisible(true);
                        stateRef.updateOscillator(osc);
                        break;
                    }
                }
                controller->pendingVisibilityOscillatorId_ = OscillatorId::invalid();
            }
        },
        // onCancel callback - clear pending ID when user cancels
        [weakThis]() {
            if (auto* controller = weakThis.get())
                controller->pendingVisibilityOscillatorId_ = OscillatorId::invalid();
        });
}

void OscillatorPanelController::addOscillatorDialogRequested()
{
    auto* dialog = dialogManager_.get();
    if (!dialog) return;
    auto sources = processor_.getInstanceRegistry().getAllSources();
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    juce::WeakReference<OscillatorPanelController> weakThis(this);
    dialog->showAddOscillatorDialog(sources, panes, [weakThis](const AddOscillatorDialog::Result& result) {
        if (auto* controller = weakThis.get())
            controller->addOscillatorRequested(result);
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
    // Look up source info from registry for persistence
    auto sourceInfo = processor_.getInstanceRegistry().getSource(result.sourceId);
    if (sourceInfo.has_value())
        osc.setSourceIdNameAndUUID(result.sourceId, sourceInfo->name, sourceInfo->instanceUUID);
    else
        osc.setSourceIdNameAndUUID(result.sourceId, "Unknown Source", ""); // Fallback with display name
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
            // Look up source info from registry for persistence
            auto sourceInfo = processor_.getInstanceRegistry().getSource(newSourceId);
            if (sourceInfo.has_value())
                osc.setSourceIdNameAndUUID(newSourceId, sourceInfo->name, sourceInfo->instanceUUID);
            else
                osc.setSourceIdNameAndUUID(newSourceId, "Unknown Source", ""); // Fallback with display name
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
            
            if (auto* sb = sidebar_.get())
            {
                sb->refreshOscillatorList(state.getOscillators());
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
        
        // CRITICAL FIX: Handle ALL oscillator properties to prevent destructive rebuilds.
        // Properties fall into categories:
        // 1. Sidebar + specific update (Name, Colour, ProcessingMode, Visible, SourceId)
        // 2. Full visual update (Opacity, LineWidth, ShaderId, VisualPresetId)
        // 3. Persistence-only - no visual update (SourceName, SourceInstanceUUID)
        // 4. Sidebar-only (Order)
        // 5. Complex - requires refresh (PaneId - moves oscillator between panes)
        if (property == StateIds::Name || property == StateIds::Colour ||
            property == StateIds::ProcessingMode || property == StateIds::Visible ||
            property == StateIds::SourceId || property == StateIds::SourceName ||
            property == StateIds::SourceInstanceUUID || property == StateIds::Opacity ||
            property == StateIds::LineWidth || property == StateIds::ShaderId ||
            property == StateIds::VisualPresetId || property == StateIds::Order ||
            property == StateIds::TimeWindow || property == StateIds::Persistence ||
            property == StateIds::OscillatorState)
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
                        if (auto* sb = controller->sidebar_.get())
                        {
                             sb->refreshOscillatorList(oscillators);
                        }

                        for (auto& pane : controller->paneComponents_)
                        {
                            if (pane && pane->getPaneId() == osc.getPaneId())
                            {
                                // Route to specific handlers or full update based on property type
                                if (property == StateIds::Name)
                                    pane->updateOscillatorName(oscId, osc.getName());
                                else if (property == StateIds::Colour)
                                    pane->updateOscillatorColor(oscId, osc.getColour());
                                else if (property == StateIds::ProcessingMode || property == StateIds::Visible)
                                    pane->updateOscillator(oscId, osc.getProcessingMode(), osc.isVisible());
                                else if (property == StateIds::SourceId)
                                    pane->updateOscillatorSource(oscId, osc.getSourceId());
                                // Visual properties - use full update
                                else if (property == StateIds::Opacity || property == StateIds::LineWidth ||
                                         property == StateIds::ShaderId || property == StateIds::VisualPresetId ||
                                         property == StateIds::TimeWindow || property == StateIds::Persistence ||
                                         property == StateIds::OscillatorState)
                                    pane->updateOscillatorFull(osc);
                                // SourceName, SourceInstanceUUID, Order are persistence/sidebar-only - no pane update needed

                                handled = true;
                                break;
                            }
                        }

                        // Properties that don't need pane updates but are still "handled"
                        if (property == StateIds::SourceName || property == StateIds::SourceInstanceUUID ||
                            property == StateIds::Order)
                            handled = true;
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
            // Fallback for complex properties - use coalesced refresh
            scheduleRefresh();
        }
    }
    else if (tree.hasType(StateIds::Pane))
    {
        scheduleRefresh();
    }
}

void OscillatorPanelController::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree& child)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        scheduleRefresh();
    }
}

void OscillatorPanelController::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree& child, int)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        // Close dialog if open
        if (auto* dialog = dialogManager_.get())
        {
            if (child.hasType(StateIds::Oscillator))
            {
                OscillatorId oid{child.getProperty(StateIds::Id).toString()};
                if (dialog->isConfigPopupVisibleFor(oid))
                {
                    dialog->closeConfigPopup();
                }
            }
        }

        scheduleRefresh();
    }
}

void OscillatorPanelController::valueTreeChildOrderChanged(juce::ValueTree& parent, int, int)
{
    if (parent.hasType(StateIds::Oscillators) || parent.hasType(StateIds::Panes))
    {
        scheduleRefresh();
    }
}

void OscillatorPanelController::valueTreeParentChanged(juce::ValueTree&) {}

void OscillatorPanelController::gainChanged(float dB)
{
    // Update state
    processor_.getState().setGainDb(dB);
    
    // Apply to panes immediately
    if (auto* settings = displaySettings_.get())
    {
        settings->setGainDbForAll(dB);
    }
}

void OscillatorPanelController::autoScaleChanged(bool enabled)
{
    // Update state
    processor_.getState().setAutoScaleEnabled(enabled);
    
    // Apply to panes immediately
    if (auto* settings = displaySettings_.get())
    {
        settings->setAutoScaleForAll(enabled);
    }
}

void OscillatorPanelController::showGridChanged(bool enabled)
{
    // Update state
    processor_.getState().setShowGridEnabled(enabled);
    
    // Apply to panes immediately
    if (auto* settings = displaySettings_.get())
    {
        settings->setShowGridForAll(enabled);
    }
}

} // namespace oscil
