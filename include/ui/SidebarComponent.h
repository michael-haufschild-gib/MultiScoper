/*
    Oscil - Sidebar Component
    Collapsible sidebar with oscillator list and resize handle
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include "core/WindowLayout.h"
#include "core/Oscillator.h"
#include "core/InstanceRegistry.h"
#include "core/Pane.h"
#include "ui/OscillatorListItem.h"
#include "ui/OscillatorListToolbar.h"
#include "ui/AddOscillatorDialog.h"
#include "ui/sections/TimingSidebarSection.h"
#include "ui/sections/OptionsSection.h"
#include "ui/components/OscilAccordion.h"
#include "ui/components/OscilButton.h"
#include "ui/components/TestId.h"
#include <vector>

namespace oscil
{

// Forward declarations
class OscilPluginProcessor;
class SourceItemComponent;

/**
 * Resize handle component for sidebar edge
 */
class SidebarResizeHandle : public juce::Component,
                            public TestIdSupport
{
public:
    SidebarResizeHandle();

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void(int)> onResizeDrag;
    std::function<void()> onResizeStart;
    std::function<void()> onResizeEnd;

private:
    bool isHovered_ = false;
    bool isDragging_ = false;
    int dragStartX_ = 0;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarResizeHandle)
};

/**
 * Collapse toggle button for sidebar
 */
class SidebarCollapseButton : public juce::Component
{
public:
    SidebarCollapseButton();

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }

    std::function<void()> onClick;

private:
    bool collapsed_ = false;
    bool isHovered_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarCollapseButton)
};

/**
 * Collapsible sidebar component with oscillator list
 * Note: juce::Component already inherits from MouseListener, so we use that
 * for global mouse tracking during drag operations
 */
class SidebarComponent : public juce::Component,
                         public ThemeManagerListener,
                         public OscillatorListItemComponent::Listener,
                         public OscillatorListToolbar::Listener,
                         public TimingSidebarSection::Listener,
                         public OptionsSection::Listener,
                         public TestIdSupport
{
public:
    /**
     * Listener interface for sidebar events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void sidebarWidthChanged(int /*newWidth*/) {}
        virtual void sidebarCollapsedStateChanged(bool /*collapsed*/) {}
        virtual void oscillatorSelected(const OscillatorId& /*oscillatorId*/) {}
        virtual void oscillatorConfigRequested(const OscillatorId& /*oscillatorId*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*oscillatorId*/) {}
        virtual void oscillatorModeChanged(const OscillatorId& /*oscillatorId*/, ProcessingMode /*mode*/) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*oscillatorId*/, bool /*visible*/) {}
        virtual void oscillatorsReordered(int /*fromIndex*/, int /*toIndex*/) {}
        virtual void addOscillatorRequested(const AddOscillatorDialog::Result& /*result*/) {}
        virtual void addOscillatorDialogRequested() {}

        // Timing section events
        virtual void timingModeChanged(TimingMode /*mode*/) {}
        virtual void noteIntervalChanged(NoteInterval /*interval*/) {}
        virtual void timeIntervalChanged(float /*ms*/) {}
        virtual void hostSyncChanged(bool /*enabled*/) {}
        virtual void waveformModeChanged(WaveformMode /*mode*/) {}
        virtual void bpmChanged(float /*bpm*/) {}

        // Master controls events
        virtual void gainChanged(float /*dB*/) {}

        // Display options events
        virtual void showGridChanged(bool /*enabled*/) {}
        virtual void autoScaleChanged(bool /*enabled*/) {}
        virtual void holdDisplayChanged(bool /*enabled*/) {}

        // Layout and theme events (from Options section)
        virtual void layoutChanged(int /*columnCount*/) {}
        virtual void themeChanged(const juce::String& /*themeName*/) {}

        // Rendering mode events
        virtual void gpuRenderingChanged(bool /*enabled*/) {}
    };

    explicit SidebarComponent(OscilPluginProcessor& processor);
    ~SidebarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // MouseListener (for drag-drop)
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // State management
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }
    void toggleCollapsed();

    void setSidebarWidth(int width);
    int getSidebarWidth() const { return expandedWidth_; }

    // Source list management
    void refreshSourceList(const std::vector<SourceInfo>& sources);

    // Oscillator list management
    void refreshOscillatorList(const std::vector<Oscillator>& oscillators);
    void setSelectedOscillator(const OscillatorId& oscillatorId);

    // Pane list management (for add-to-pane dropdowns)
    void refreshPaneList(const std::vector<Pane>& panes);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Section accessors
    TimingSidebarSection* getTimingSection() { return timingSection_.get(); }
    OptionsSection* getOptionsSection() { return optionsSection_.get(); }

    // Get the effective width (collapsed or expanded)
    int getEffectiveWidth() const;

private:
    OscilPluginProcessor& processor_;

    // Child components
    std::unique_ptr<SidebarResizeHandle> resizeHandle_;
    std::unique_ptr<SidebarCollapseButton> collapseButton_;

    // Add Oscillator button
    std::unique_ptr<OscilButton> addOscillatorButton_;

    // Oscillators section with toolbar
    std::unique_ptr<OscillatorListToolbar> oscillatorToolbar_;
    std::unique_ptr<juce::Viewport> listViewport_;
    std::unique_ptr<juce::Component> listContainer_;
    std::unique_ptr<juce::Label> emptyStateLabel_;  // Shown when no oscillators
    std::vector<std::unique_ptr<OscillatorListItemComponent>> oscillatorItems_;

    // Accordion for control sections
    std::unique_ptr<juce::Viewport> accordionViewport_;
    std::unique_ptr<OscilAccordion> accordion_;
    
    // Section contents (owned by Sidebar, displayed by Accordion)
    std::unique_ptr<TimingSidebarSection> timingSection_;
    std::unique_ptr<OptionsSection> optionsSection_;

    // State
    bool collapsed_ = false;
    int expandedWidth_ = WindowLayout::DEFAULT_SIDEBAR_WIDTH;
    OscillatorId selectedOscillatorId_;
    std::vector<Pane> currentPanes_;  // Cached for source dropdown updates
    OscillatorFilterMode currentFilterMode_ = OscillatorFilterMode::All;
    std::vector<Oscillator> allOscillators_;  // Cached for filtering

    // Drag-drop state
    bool isDraggingItem_ = false;
    int dragSourceIndex_ = -1;
    int dragTargetIndex_ = -1;
    OscillatorId draggedOscillatorId_;
    juce::Image dragImage_;               // Snapshot of dragged item for visual feedback
    juce::Point<int> dragMousePosition_;  // Current mouse position for drawing ghost

    juce::ListenerList<Listener> listeners_;

    void notifySidebarWidthChanged();
    void notifySidebarCollapsedStateChanged();
    void notifyOscillatorSelected(const OscillatorId& oscillatorId);
    void notifyOscillatorConfigRequested(const OscillatorId& oscillatorId);
    void notifyOscillatorDeleteRequested(const OscillatorId& oscillatorId);
    void notifyOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode);
    void notifyOscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible);
    void notifyOscillatorsReordered(int fromIndex, int toIndex);
    void notifyAddOscillatorRequested(const AddOscillatorDialog::Result& result);

    // Drag-drop helpers
    int getItemIndexAtY(int y) const;
    void updateDragIndicator(int targetIndex);
    void finishDrag(int fromIndex, int toIndex);

    void setupSections();
    void updateOscillatorCounts();

    // OscillatorListItemComponent::Listener overrides
    void oscillatorSelected(const OscillatorId& id) override;
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override;
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override;
    void oscillatorConfigRequested(const OscillatorId& id) override;
    void oscillatorDeleteRequested(const OscillatorId& id) override;
    void oscillatorDragStarted(const OscillatorId& id) override;
    void oscillatorMoveRequested(const OscillatorId& id, int direction) override;

    // OscillatorListToolbar::Listener overrides
    void filterModeChanged(OscillatorFilterMode mode) override;

    // TimingSidebarSection::Listener overrides
    void timingModeChanged(TimingMode mode) override;
    void noteIntervalChanged(NoteInterval interval) override;
    void timeIntervalChanged(float ms) override;
    void hostSyncChanged(bool enabled) override;
    void waveformModeChanged(WaveformMode mode) override;
    void bpmChanged(float bpm) override;

    // OptionsSection::Listener overrides
    void gainChanged(float dB) override;
    void showGridChanged(bool enabled) override;
    void autoScaleChanged(bool enabled) override;
    void holdDisplayChanged(bool enabled) override;
    void layoutChanged(int columnCount) override;
    void themeChanged(const juce::String& themeName) override;
    void gpuRenderingChanged(bool enabled) override;

    static constexpr int SECTION_HEADER_HEIGHT = 28;
    static constexpr int ITEM_HEIGHT = OscillatorListItemComponent::PREFERRED_HEIGHT;
    static constexpr int RESIZE_HANDLE_WIDTH = 6;
    static constexpr int ADD_BUTTON_HEIGHT = 36;
    static constexpr int OSCILLATOR_TOOLBAR_HEIGHT = OscillatorListToolbar::PREFERRED_HEIGHT;
    static constexpr int COLLAPSE_BUTTON_SIZE = 24;
    static constexpr int PADDING_SMALL = 4;
    static constexpr int PADDING_MEDIUM = 8;
    static constexpr int MIN_LIST_HEIGHT = 100;
    static constexpr int MAX_LIST_HEIGHT = 300;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarComponent)
};

} // namespace oscil
