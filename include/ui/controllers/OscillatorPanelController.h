/*
    Oscil - Oscillator Panel Controller Header
    Manages the lifecycle, layout, and updates of oscillator panels.
*/

#pragma once

#include "core/ServiceContext.h"
#include "ui/layout/PaneComponent.h"
#include "ui/layout/PaneContainerComponent.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/managers/DisplaySettingsManager.h"

#include <juce_core/juce_core.h>

#include <memory>
#include <vector>

namespace oscil
{

// Forward declarations
class IAudioDataProvider;
class DialogManager;
class GpuRenderCoordinator;

/**
 * Controller responsible for managing the dynamic grid of Oscillator Panes.
 * Extracts complex pane management logic from PluginEditor.
 */
class OscillatorPanelController
    : public juce::ValueTree::Listener
    , public SidebarComponent::Listener
{
public:
    OscillatorPanelController(IAudioDataProvider& dataProvider, ServiceContext& serviceContext,
                              PaneContainerComponent& container, GpuRenderCoordinator& gpuCoordinator);

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

    /** Update sidebar with current oscillators, sources, and pane list. */
    void refreshSidebar(const std::vector<Oscillator>& oscillators, const PaneLayoutManager& layoutManager);

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
     * Highlight an oscillator in the grid (e.g., flash border).
     */
    void highlightOscillator(const OscillatorId& oscillatorId);

    /**
     * Access managed pane components (e.g. for layout or GPU coordinator).
     */
    std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() { return paneComponents_; }

    // SidebarComponent::Listener overrides (Oscillator logic moved from PluginEditor)
    void oscillatorSelected(const OscillatorId& oscillatorId) override;
    void oscillatorConfigRequested(const OscillatorId& oscillatorId) override;
    void oscillatorColorConfigRequested(const OscillatorId& oscillatorId) override;
    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override;
    void oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode) override;
    void oscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible) override;
    void oscillatorPaneSelectionRequested(const OscillatorId& oscillatorId) override;
    void addOscillatorDialogRequested() override;
    void addOscillatorRequested(const AddOscillatorDialog::Result& result) override;

    // Note: Other Sidebar events (Timing, Options) are handled by Editor or other controllers

    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    // Publicly exposed for specific edge cases (e.g. dialog results that change source)
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);

private:
    void createPaneComponents(const std::vector<Oscillator>& oscillators, const PaneLayoutManager& layoutManager);
    void applyOscillatorPropertyChange(const OscillatorId& oscId, const juce::Identifier& property);

    IAudioDataProvider& dataProvider_;
    ServiceContext& serviceContext_;
    PaneContainerComponent& container_;
    GpuRenderCoordinator& gpuCoordinator_;

    // Weak references
    DisplaySettingsManager* displaySettings_ = nullptr;
    SidebarComponent* sidebar_ = nullptr;
    DialogManager* dialogManager_ = nullptr;

    std::vector<std::unique_ptr<PaneComponent>> paneComponents_;
    bool isUpdating_ = false;     // Guard against recursive updates
    bool pendingRefresh_ = false; // Coalesce refresh requests that arrive mid-update
    std::function<void()> layoutNeededCallback_;

    // Track oscillator pending visibility change (for pane selection dialog)
    OscillatorId pendingVisibilityOscillatorId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorPanelController)
    JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorPanelController)
};

} // namespace oscil
