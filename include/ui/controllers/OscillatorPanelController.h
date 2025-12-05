/*
    Oscil - Oscillator Panel Controller Header
    Manages the lifecycle, layout, and updates of oscillator panels.
*/

#pragma once

#include <juce_core/juce_core.h>
#include "plugin/PluginProcessor.h" // Needs full type for dependencies
#include "ui/layout/PaneContainerComponent.h"
#include "ui/layout/PaneComponent.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "rendering/GpuRenderCoordinator.h"
#include "core/ServiceContext.h"
#include <vector>
#include <memory>

namespace oscil
{

// Forward declarations
class DialogManager;

/**
 * Controller responsible for managing the dynamic grid of Oscillator Panes.
 * Extracts complex pane management logic from PluginEditor.
 */
class OscillatorPanelController : public juce::ValueTree::Listener
{
public:
    OscillatorPanelController(OscilPluginProcessor& processor,
                              ServiceContext& serviceContext,
                              PaneContainerComponent& container,
                              GpuRenderCoordinator& gpuCoordinator);

    ~OscillatorPanelController() override;

    /**
     * Initialize the controller.
     * Sets up listeners and initial state.
     * @param sidebar Optional pointer to sidebar for refreshing lists.
     * @param dialogManager Optional pointer to dialog manager for config popups.
     * @param displaySettings Pointer to display settings manager for global updates.
     */
    void initialize(SidebarComponent* sidebar, DialogManager* dialogManager, DisplaySettingsManager* displaySettings);

    /**
     * Set callback for when layout needs to be updated after panes are refreshed.
     * This is critical - without this, panes won't be positioned after async refreshes.
     */
    void setLayoutNeededCallback(std::function<void()> callback) { layoutNeededCallback_ = std::move(callback); }

    /**
     * Force a full refresh of the oscillator panels.
     * Recreates all components based on current state.
     */
    void refreshPanels();

    /**
     * Create a default oscillator configuration if the state is empty.
     */
    void createDefaultOscillatorIfNeeded();

    /**
     * Re-apply global settings (grid, gain, etc.) to all managed panes.
     */
    void reapplyGlobalSettings();

    /**
     * Handle dropped pane (reordering).
     */
    void handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId);

    /**
     * Handle pane dropped onto empty column.
     */
    void handleEmptyColumnDrop(const PaneId& movedPaneId, int targetColumn);

    /**
     * Handle request to close a pane.
     */
    void handlePaneClose(const PaneId& paneId);

    /**
     * Access managed pane components (e.g. for layout or GPU coordinator).
     */
    std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() { return paneComponents_; }

    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

private:
    void createPaneComponents(const std::vector<Oscillator>& oscillators, const PaneLayoutManager& layoutManager);
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);

    OscilPluginProcessor& processor_;
    ServiceContext& serviceContext_;
    PaneContainerComponent& container_;
    GpuRenderCoordinator& gpuCoordinator_;
    
    // Weak references
    DisplaySettingsManager* displaySettings_ = nullptr;
    SidebarComponent* sidebar_ = nullptr;
    DialogManager* dialogManager_ = nullptr;

    std::vector<std::unique_ptr<PaneComponent>> paneComponents_;
    bool isUpdating_ = false; // Guard against recursive updates
    std::function<void()> layoutNeededCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorPanelController)
    JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorPanelController)
};

} // namespace oscil
