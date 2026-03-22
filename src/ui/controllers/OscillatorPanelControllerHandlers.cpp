/*
    Oscil - Oscillator Panel Controller Handlers
    Sidebar listener overrides, dialog handlers, and ValueTree listeners
*/

#include "ui/controllers/OscillatorPanelController.h"
#include "ui/controllers/GpuRenderCoordinator.h"
#include "rendering/VisualConfiguration.h"
#include "ui/managers/DialogManager.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/OscilState.h"

namespace oscil
{
// SidebarComponent::Listener overrides

void OscillatorPanelController::oscillatorSelected(const OscillatorId& oscillatorId)
{
    highlightOscillator(oscillatorId);
}

void OscillatorPanelController::oscillatorConfigRequested(const OscillatorId& oscillatorId)
{
    auto& state = dataProvider_.getState();
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

    auto& state = dataProvider_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            dialogManager_->showColorDialog(osc.getColour(), [this, oscillatorId](juce::Colour color) {
                auto& oscilState = dataProvider_.getState();
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
    dataProvider_.getState().removeOscillator(oscillatorId);
}

void OscillatorPanelController::oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
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

void OscillatorPanelController::oscillatorPaneSelectionRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_) return;
    pendingVisibilityOscillatorId_ = oscillatorId;
    
    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showSelectPaneDialog(panes,
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
        [this]() {
            pendingVisibilityOscillatorId_ = OscillatorId::invalid();
        });
}

void OscillatorPanelController::addOscillatorDialogRequested()
{
    if (!dialogManager_) return;
    auto sources = serviceContext_.instanceRegistry.getAllSources();
    auto& layoutManager = dataProvider_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showAddOscillatorDialog(sources, panes, [this](const AddOscillatorDialog::Result& result) {
        addOscillatorRequested(result);
    });
}

void OscillatorPanelController::addOscillatorRequested(const AddOscillatorDialog::Result& result)
{
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
    
    if (result.name.isNotEmpty()) osc.setName(result.name);
    else osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));

    // Ensure shader matches preset
    auto preset = VisualConfiguration::getPreset(result.visualPresetId);
    osc.setShaderId(shaderTypeToId(preset.shaderType));

    state.addOscillator(osc);
}

void OscillatorPanelController::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
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

void OscillatorPanelController::applyOscillatorPropertyChange(const OscillatorId& oscId, const juce::Identifier& property)
{
    auto oscillators = dataProvider_.getState().getOscillators();
    bool handled = false;

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscId)
        {
            if (sidebar_)
                sidebar_->refreshOscillatorList(oscillators);

            for (auto& pane : paneComponents_)
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
        refreshPanels();
}

void OscillatorPanelController::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(StateIds::Oscillator))
    {
        OscillatorId oscId{tree.getProperty(StateIds::Id).toString()};

        if (property == StateIds::Name || property == StateIds::Colour ||
            property == StateIds::ProcessingMode || property == StateIds::Visible ||
            property == StateIds::SourceId)
        {
            juce::MessageManager::callAsync([weakThis = juce::WeakReference<OscillatorPanelController>(this), oscId, property]() {
                if (auto* controller = weakThis.get())
                    controller->applyOscillatorPropertyChange(oscId, property);
            });
        }
        else
        {
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

} // namespace oscil
