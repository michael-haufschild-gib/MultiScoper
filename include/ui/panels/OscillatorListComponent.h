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
#include "ui/components/TestId.h"

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
                                public TestIdSupport
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscillatorSelected(const OscillatorId& id) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) {}
        virtual void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) {}
        virtual void oscillatorConfigRequested(const OscillatorId& id) {}
        virtual void oscillatorColorConfigRequested(const OscillatorId& id) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& id) {}
        virtual void oscillatorsReordered(int fromIndex, int toIndex) {}
    };

    explicit OscillatorListComponent(IInstanceRegistry& instanceRegistry);
    ~OscillatorListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // OscillatorListToolbar::Listener
    void filterModeChanged(OscillatorFilterMode mode) override;

    // OscillatorListItemComponent::Listener
    void oscillatorSelected(const OscillatorId& id) override;
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override;
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override;
    void oscillatorConfigRequested(const OscillatorId& id) override;
    void oscillatorColorConfigRequested(const OscillatorId& id) override;
    void oscillatorDeleteRequested(const OscillatorId& id) override;
    void oscillatorDragStarted(const OscillatorId& id) override;
    void oscillatorMoveRequested(const OscillatorId& id, int direction) override;

    // MouseListener overrides for drag-and-drop
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    /**
     * Refreshes the list with the provided oscillators.
     * Applies current filter settings.
     */
    void refreshList(const std::vector<Oscillator>& oscillators);

    /**
     * Selects an oscillator by ID.
     */
    void setSelectedOscillator(const OscillatorId& oscillatorId);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Layout constants
    static constexpr int MIN_LIST_HEIGHT = 100;
    static constexpr int MAX_LIST_HEIGHT = 400;
    static constexpr int OSCILLATOR_TOOLBAR_HEIGHT = 32;

private:
    void updateOscillatorCounts();
    int getItemIndexAtY(int y) const;
    void updateDragIndicator(int targetIndex);
    void finishDrag(int fromIndex, int toIndex);

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    IInstanceRegistry& instanceRegistry_;
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
    bool isDraggingItem_ = false;
    OscillatorId draggedOscillatorId_;
    int dragSourceIndex_ = -1;
    int dragTargetIndex_ = -1;
    juce::Image dragImage_;
    juce::Point<int> dragMousePosition_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorListComponent)
};

} // namespace oscil
