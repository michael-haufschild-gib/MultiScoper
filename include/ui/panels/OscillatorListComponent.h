/*
    Oscil - Oscillator List Component
    Manages the list of oscillators, including filtering, selection, and reordering
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/panels/OscillatorListItem.h"
#include "ui/panels/OscillatorListToolbar.h"
#include "core/Oscillator.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "core/ServiceContext.h"
#include "ui/components/TestId.h"
#include <unordered_map>

namespace oscil
{

class IInstanceRegistry;

/**
 * Component that displays and manages the list of oscillators.
 * Handles drag-and-drop reordering, filtering, and selection.
 */
class OscillatorListComponent : public juce::Component,
                                public ThemeManagerListener,
                                public OscillatorListItemComponent::Listener,
                                public OscillatorListToolbar::Listener,
                                public TestIdSupport,
                                public juce::DragAndDropContainer,
                                public juce::DragAndDropTarget
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        /// Called when user clicks/selects an oscillator in the list.
        virtual void oscillatorSelected(const OscillatorId& /*id*/) {}
        /// Called when user toggles oscillator visibility (eye icon).
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*id*/, bool /*visible*/) {}
        /// Called when user changes oscillator processing mode.
        virtual void oscillatorModeChanged(const OscillatorId& /*id*/, ProcessingMode /*mode*/) {}
        /// Called when user requests oscillator configuration dialog.
        virtual void oscillatorConfigRequested(const OscillatorId& /*id*/) {}
        /// Called when user requests oscillator color picker.
        virtual void oscillatorColorConfigRequested(const OscillatorId& /*id*/) {}
        /// Called when user requests oscillator deletion.
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        /// Called when user drag-drops to reorder oscillators.
        virtual void oscillatorsReordered(int /*fromIndex*/, int /*toIndex*/) {}
        /**
         * Called when user tries to set an oscillator visible but it has no valid pane assignment.
         * Parent should show a pane selection dialog.
         */
        virtual void oscillatorPaneSelectionRequested(const OscillatorId& /*id*/) {}
        /**
         * Called when user edits the oscillator name inline.
         */
        virtual void oscillatorNameChanged(const OscillatorId& /*id*/, const juce::String& /*newName*/) {}
    };

    /// Construct with full service context.
    explicit OscillatorListComponent(ServiceContext& context);
    /// Construct with individual service dependencies.
    OscillatorListComponent(IThemeService& themeService, IInstanceRegistry& instanceRegistry);
    ~OscillatorListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// React to theme changes.
    void themeChanged(const ColorTheme& newTheme) override;
    /// React to filter mode changes from toolbar.
    void filterModeChanged(OscillatorFilterMode mode) override;

    /// Forward list item events to listeners.
    void oscillatorSelected(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorVisibilityChanged
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorModeChanged
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorConfigRequested
    void oscillatorConfigRequested(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorColorConfigRequested
    void oscillatorColorConfigRequested(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorDeleteRequested
    void oscillatorDeleteRequested(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorDragStarted
    void oscillatorDragStarted(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorMoveRequested
    void oscillatorMoveRequested(const OscillatorId& id, int direction) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorPaneSelectionRequested
    void oscillatorPaneSelectionRequested(const OscillatorId& id) override;
    /// @copydoc OscillatorListItemComponent::Listener::oscillatorNameChanged
    void oscillatorNameChanged(const OscillatorId& id, const juce::String& newName) override;

    /// Check if drag source is an oscillator list item.
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    /// Handle drag enter event for reordering.
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    /// Update drop indicator position during drag.
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    /// Clean up drag state when exiting.
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    /// Execute oscillator reorder on drop.
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    /**
     * Refreshes the list with the provided oscillators.
     * Applies current filter settings.
     */
    void refreshList(const std::vector<Oscillator>& oscillators);

    /**
     * Selects an oscillator by ID.
     */
    void setSelectedOscillator(const OscillatorId& oscillatorId);

    /// Register a listener for oscillator list events.
    void addListener(Listener* listener);
    /// Remove a previously registered listener.
    void removeListener(Listener* listener);

    // Layout constants
    static constexpr int MIN_LIST_HEIGHT = 100;
    static constexpr int MAX_LIST_HEIGHT = 400;
    static constexpr int OSCILLATOR_TOOLBAR_HEIGHT = 32;

private:
    std::vector<Oscillator> filterOscillators(const std::vector<Oscillator>& oscillators) const;
    void rebuildItems(const std::vector<Oscillator>& filtered,
                      std::unordered_map<juce::String, std::unique_ptr<OscillatorListItemComponent>>& reusedItems);
    void syncContainerChildren();
    void updateOscillatorCounts();
    int getItemIndexAtY(int y) const;
    void updateDragIndicator(int targetIndex);

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    IInstanceRegistry& instanceRegistry_;
    IThemeService& themeService_;
    juce::ListenerList<Listener> listeners_;

    std::unique_ptr<OscillatorListToolbar> toolbar_;
    std::unique_ptr<juce::Component> container_;
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<juce::Label> emptyStateLabel_;

    std::vector<std::unique_ptr<OscillatorListItemComponent>> items_;
    std::vector<Oscillator> allOscillators_;
    OscillatorFilterMode currentFilterMode_ = OscillatorFilterMode::All;
    OscillatorId selectedOscillatorId_;

    // Drag and drop state
    int dragTargetIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorListComponent)
};

} // namespace oscil
