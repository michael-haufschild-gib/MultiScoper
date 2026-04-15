/*
    Oscil - Oscillator Panel Controller Handlers
    Sidebar listener overrides, dialog handlers, and ValueTree listeners
*/

#include "core/OscilLog.h"
#include "core/OscilState.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/controllers/GpuRenderCoordinator.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/managers/DialogManager.h"

#include "rendering/VisualConfiguration.h"

namespace oscil
{
// SidebarComponent::Listener overrides

void OscillatorPanelController::oscillatorSelected(const OscillatorId& oscillatorId)
{
    OSCIL_LOG(CONTROLLER, "oscillatorSelected: id=" << oscillatorId.id);
    highlightOscillator(oscillatorId);
}

void OscillatorPanelController::oscillatorConfigRequested(const OscillatorId& oscillatorId)
{
    OSCIL_LOG(CONTROLLER, "oscillatorConfigRequested: id=" << oscillatorId.id);
    auto& state = dataProvider_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            auto& layoutManager = state.getLayoutManager();
            auto panes = layoutManager.getPanes();
            std::vector<std::pair<PaneId, juce::String>> paneList;
            paneList.reserve(panes.size());
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

void OscillatorPanelController::applyOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color)
{
    auto& oscilState = dataProvider_.getState();
    auto oscList = oscilState.getOscillators();
    for (auto& o : oscList)
    {
        if (o.getId() == oscillatorId)
        {
            o.setColour(color);
            oscilState.updateOscillator(o);
            return;
        }
    }
}

void OscillatorPanelController::oscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_)
        return;

    auto& state = dataProvider_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            dialogManager_->showColorDialog(osc.getColour(), [this, oscillatorId](juce::Colour color) {
                applyOscillatorColor(oscillatorId, color);
            });
            return;
        }
    }
}

void OscillatorPanelController::oscillatorDeleteRequested(const OscillatorId& oscillatorId)
{
    OSCIL_LOG(CONTROLLER, "oscillatorDeleteRequested: id=" << oscillatorId.id);
    dataProvider_.getState().removeOscillator(oscillatorId);
    refreshPanels();
}

void OscillatorPanelController::oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    OSCIL_LOG(CONTROLLER, "oscillatorModeChanged: id=" << oscillatorId.id << " mode=" << processingModeToString(mode));
    auto& state = dataProvider_.getState();
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
    OSCIL_LOG(CONTROLLER,
              "oscillatorVisibilityChanged: id=" << oscillatorId.id << " visible=" << (visible ? "true" : "false"));
    auto& state = dataProvider_.getState();
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

void OscillatorPanelController::oscillatorNameChanged(const OscillatorId& oscillatorId, const juce::String& newName)
{
    OSCIL_LOG(CONTROLLER, "oscillatorNameChanged: id=" << oscillatorId.id << " newName=" << newName);
    // Reject empty names — preserves the existing name on blank input.
    if (!Oscillator::isValidName(newName))
        return;

    auto& state = dataProvider_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            if (osc.getName() == newName)
                return; // No change — skip redundant state update.
            osc.setName(newName);
            state.updateOscillator(osc);
            return;
        }
    }
}

void OscillatorPanelController::oscillatorsReordered(int fromIndex, int toIndex)
{
    OSCIL_LOG(CONTROLLER, "oscillatorsReordered: from=" << fromIndex << " to=" << toIndex);
    if (fromIndex == toIndex || fromIndex < 0 || toIndex < 0)
        return;

    auto& state = dataProvider_.getState();
    auto const oscCount = static_cast<int>(state.getOscillators().size());
    if (fromIndex >= oscCount || toIndex >= oscCount)
        return;

    // Persist the new ordering to state. The ValueTree listener chain triggers
    // applyOscillatorPropertyChange for each affected oscillator and refreshes
    // the sidebar/panes accordingly.
    //
    // Note: fromIndex and toIndex are positions in the currently displayed
    // sidebar list. When no filter is active this matches the global orderIndex
    // space one-to-one, which is what state.reorderOscillators expects. Drag
    // reorder while a non-All filter is applied is intentionally not supported
    // yet — the UI disables drag while a filter is active to avoid index
    // mismatch (filter indices would need per-ID translation).
    state.reorderOscillators(fromIndex, toIndex);
}

void OscillatorPanelController::oscillatorPaneSelectionRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_)
        return;
    pendingVisibilityOscillatorId_ = oscillatorId;

    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showSelectPaneDialog(
        panes,
        // onComplete callback
        [this](const SelectPaneDialog::Result& result) {
            auto& stateRef = dataProvider_.getState();
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
        [this]() { pendingVisibilityOscillatorId_ = OscillatorId::invalid(); });
}

void OscillatorPanelController::addOscillatorDialogRequested()
{
    OSCIL_LOG(CONTROLLER, "addOscillatorDialogRequested");
    if (!dialogManager_)
        return;
    auto sources = serviceContext_.instanceRegistry.getAllSources();
    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showAddOscillatorDialog(
        sources, panes, [this](const AddOscillatorDialog::Result& result) { addOscillatorRequested(result); });
}

void OscillatorPanelController::addOscillatorRequested(const AddOscillatorDialog::Result& result)
{
    OSCIL_LOG(CONTROLLER, "addOscillatorRequested: sourceId=" << result.sourceId.id << " paneId=" << result.paneId.id
                                                              << " name=" << result.name << " createNewPane="
                                                              << (result.createNewPane ? "true" : "false")
                                                              << " preset=" << result.visualPresetId);
    auto& state = dataProvider_.getState();
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

    if (result.name.isNotEmpty())
        osc.setName(result.name);
    else
        osc.setName("Oscillator " + juce::String(state.getOscillatorCount() + 1));

    // Ensure shader matches preset
    auto preset = VisualConfiguration::getPreset(result.visualPresetId);
    osc.setShaderId(shaderTypeToId(preset.shaderType));

    state.addOscillator(osc);
}

void OscillatorPanelController::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    OSCIL_LOG(CONTROLLER, "updateOscillatorSource: oscId=" << oscillatorId.id << " newSourceId=" << newSourceId.id);
    auto& state = dataProvider_.getState();
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

bool OscillatorPanelController::dispatchOscillatorPropertyToPane(const Oscillator& osc,
                                                                 const juce::Identifier& property)
{
    const auto& oscId = osc.getId();
    for (auto& pane : paneComponents_)
    {
        if (!pane || pane->getPaneId() != osc.getPaneId())
            continue;

        if (property == StateIds::Name)
            pane->updateOscillatorName(oscId, osc.getName());
        else if (property == StateIds::Colour)
            pane->updateOscillatorColor(oscId, osc.getColour());
        else if (property == StateIds::ProcessingMode || property == StateIds::Visible)
            pane->updateOscillator(oscId, osc.getProcessingMode(), osc.isVisible());
        else if (property == StateIds::SourceId)
            pane->updateOscillatorSource(oscId, osc.getSourceId());

        return true;
    }
    return false;
}

void OscillatorPanelController::applyOscillatorPropertyChange(const OscillatorId& oscId,
                                                              const juce::Identifier& property)
{
    OSCIL_LOG(CONTROLLER, "applyOscillatorPropertyChange: oscId=" << oscId.id << " property=" << property.toString());
    auto oscillators = dataProvider_.getState().getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() != oscId)
            continue;

        if (sidebar_)
            sidebar_->refreshOscillatorList(oscillators);

        if (dispatchOscillatorPropertyToPane(osc, property))
            return;
        break;
    }

    triggerAsyncUpdate();
}

void OscillatorPanelController::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(StateIds::Oscillator))
    {
        OSCIL_LOG(CONTROLLER, "valueTreePropertyChanged: Oscillator property="
                                  << property.toString() << " oscId=" << tree.getProperty(StateIds::Id).toString());
        OscillatorId const oscId{tree.getProperty(StateIds::Id).toString()};

        if (property == StateIds::Name || property == StateIds::Colour || property == StateIds::ProcessingMode ||
            property == StateIds::Visible || property == StateIds::SourceId)
        {
            juce::MessageManager::callAsync(
                [weakThis = juce::WeakReference<OscillatorPanelController>(this), oscId, property]() {
                    if (auto* controller = weakThis.get())
                        controller->applyOscillatorPropertyChange(oscId, property);
                });
        }
        else
        {
            triggerAsyncUpdate();
        }
    }
    else if (tree.hasType(StateIds::Pane))
    {
        OSCIL_LOG(CONTROLLER, "valueTreePropertyChanged: Pane property=" << property.toString());
        triggerAsyncUpdate();
    }
}

void OscillatorPanelController::valueTreeChildAdded(juce::ValueTree& /*parentTree*/, juce::ValueTree& child)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        OSCIL_LOG(CONTROLLER, "valueTreeChildAdded: type=" << child.getType().toString()
                                                           << " id=" << child.getProperty(StateIds::Id).toString());
        triggerAsyncUpdate();
    }
}

void OscillatorPanelController::valueTreeChildRemoved(juce::ValueTree& /*parentTree*/, juce::ValueTree& child,
                                                      int /*indexFromWhichChildWasRemoved*/)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        OSCIL_LOG(CONTROLLER, "valueTreeChildRemoved: type=" << child.getType().toString()
                                                             << " id=" << child.getProperty(StateIds::Id).toString());
        // Close dialog if open
        if (dialogManager_ && child.hasType(StateIds::Oscillator))
        {
            OscillatorId const oid{child.getProperty(StateIds::Id).toString()};
            if (dialogManager_->isConfigPopupVisibleFor(oid))
            {
                dialogManager_->closeConfigPopup();
            }
        }

        triggerAsyncUpdate();
    }
}

void OscillatorPanelController::valueTreeChildOrderChanged(juce::ValueTree& parent, int /*oldIndex*/, int /*newIndex*/)
{
    if (parent.hasType(StateIds::Oscillators) || parent.hasType(StateIds::Panes))
    {
        triggerAsyncUpdate();
    }
}

void OscillatorPanelController::valueTreeParentChanged(juce::ValueTree& /*treeWhoseParentHasChanged*/) {}

} // namespace oscil
