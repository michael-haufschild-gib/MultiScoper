/*
    Oscil - Sidebar Component
    Collapsible sidebar with oscillator list and resize handle
*/

#pragma once

#include "core/InstanceRegistry.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/ServiceContext.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/OscilAccordion.h"
#include "ui/components/OscilButton.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/TestId.h"
#include "ui/dialogs/AddOscillatorDialog.h"
#include "ui/layout/WindowLayout.h"
#include "ui/layout/sections/OptionsSection.h"
#include "ui/layout/sections/OscillatorSidebarSection.h"
#include "ui/layout/sections/TimingSidebarSection.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace oscil
{

// Forward declarations
class OscilPluginProcessor;

/**
 * Resize handle component for sidebar edge
 */
class SidebarResizeHandle
    : public juce::Component
    , public TestIdSupport
{
public:
    explicit SidebarResizeHandle(IThemeService& themeService);

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
    IThemeService& themeService_;
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
    explicit SidebarCollapseButton(IThemeService& themeService);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }

    std::function<void()> onClick;

private:
    IThemeService& themeService_;
    bool collapsed_ = false;
    bool isHovered_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarCollapseButton)
};

/**
 * Collapsible sidebar component with oscillator list
 */
class SidebarComponent
    : public juce::Component
    , public juce::Timer
    , public ThemeManagerListener
    , public OscillatorSidebarSection::Listener
    , public TimingSidebarSection::Listener
    , public OptionsSection::Listener
    , public TestIdSupport
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
        virtual void oscillatorColorConfigRequested(const OscillatorId& /*oscillatorId*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*oscillatorId*/) {}
        virtual void oscillatorModeChanged(const OscillatorId& /*oscillatorId*/, ProcessingMode /*mode*/) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*oscillatorId*/, bool /*visible*/) {}
        virtual void oscillatorsReordered(int /*fromIndex*/, int /*toIndex*/) {}
        virtual void addOscillatorRequested(const AddOscillatorDialog::Result& /*result*/) {}
        virtual void addOscillatorDialogRequested() {}
        /**
         * Called when user tries to set an oscillator visible but it has no valid pane assignment.
         * Parent should show a pane selection dialog.
         */
        virtual void oscillatorPaneSelectionRequested(const OscillatorId& /*oscillatorId*/) {}

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

        // Layout and theme events (from Options section)
        virtual void layoutChanged(int /*columnCount*/) {}
        virtual void themeChanged(const juce::String& /*themeName*/) {}

        // Rendering mode events
        virtual void gpuRenderingChanged(bool /*enabled*/) {}

        // Capture quality events
        virtual void qualityPresetChanged(QualityPreset /*preset*/) {}
        virtual void bufferDurationChanged(BufferDuration /*duration*/) {}
        virtual void autoAdjustQualityChanged(bool /*enabled*/) {}
    };

    /// Construct the sidebar with all accordion sections wired to the service context.
    explicit SidebarComponent(ServiceContext& context);
    ~SidebarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Timer
    void timerCallback() override;

    // State management
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }
    /// Toggle between collapsed (icon-only) and expanded sidebar states.
    void toggleCollapsed();

    void setSidebarWidth(int width);
    int getSidebarWidth() const { return expandedWidth_; }

    /// Rebuild the source list from the current registry snapshot.
    void refreshSourceList(const std::vector<SourceInfo>& sources);

    /// Rebuild the oscillator list from the current state snapshot.
    void refreshOscillatorList(const std::vector<Oscillator>& oscillators);
    void setSelectedOscillator(const OscillatorId& oscillatorId);

    /// Refresh pane names in add-to-pane dropdowns.
    void refreshPaneList(const std::vector<Pane>& panes);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Section accessors
    TimingSidebarSection* getTimingSection() { return timingSection_.get(); }
    OptionsSection* getOptionsSection() { return optionsSection_.get(); }

    // Get the effective width (collapsed or expanded)
    int getEffectiveWidth() const;

    /** Snap the width spring to the current expandedWidth_ immediately.
     *  Useful for test automation where animation delay is undesirable. */
    void snapWidthToTarget();

private:
    ServiceContext& context_;
    IThemeService& themeService_;
    IInstanceRegistry& instanceRegistry_;

    // Child components
    std::unique_ptr<SidebarResizeHandle> resizeHandle_;
    std::unique_ptr<SidebarCollapseButton> collapseButton_;

    // Accordion for all sections
    std::unique_ptr<juce::Viewport> accordionViewport_;
    std::unique_ptr<OscilAccordion> accordion_;

    // Section contents (owned by Sidebar, displayed by Accordion)
    std::unique_ptr<OscillatorSidebarSection> oscillatorSection_;
    std::unique_ptr<TimingSidebarSection> timingSection_;
    std::unique_ptr<OptionsSection> optionsSection_;

    // State
    bool collapsed_ = false;
    int expandedWidth_ = WindowLayout::DEFAULT_SIDEBAR_WIDTH;
    int dragStartWidth_ = WindowLayout::DEFAULT_SIDEBAR_WIDTH;
    OscillatorId selectedOscillatorId_;
    std::vector<Pane> currentPanes_; // Cached for source dropdown updates

    // Collapse animation
    SpringAnimation widthSpring_ = SpringPresets::slow();

    juce::ListenerList<Listener> listeners_;

    void notifySidebarWidthChanged();
    void notifySidebarCollapsedStateChanged();

    void setupSections();

    // OscillatorSidebarSection::Listener overrides
    void addOscillatorDialogRequested() override;
    void oscillatorSelected(const OscillatorId& id) override;
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override;
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override;
    void oscillatorConfigRequested(const OscillatorId& id) override;
    void oscillatorColorConfigRequested(const OscillatorId& id) override;
    void oscillatorDeleteRequested(const OscillatorId& id) override;
    void oscillatorsReordered(int fromIndex, int toIndex) override;
    void oscillatorPaneSelectionRequested(const OscillatorId& id) override;
    void oscillatorNameChanged(const OscillatorId& id, const juce::String& newName) override;

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
    void layoutChanged(int columnCount) override;
    void themeChanged(const juce::String& themeName) override;
    void gpuRenderingChanged(bool enabled) override;
    void qualityPresetChanged(QualityPreset preset) override;
    void bufferDurationChanged(BufferDuration duration) override;
    void autoAdjustQualityChanged(bool enabled) override;

    static constexpr int SECTION_HEADER_HEIGHT = 28;
    static constexpr int RESIZE_HANDLE_WIDTH = 6;
    static constexpr int COLLAPSE_BUTTON_SIZE = 24;
    static constexpr int PADDING_SMALL = 4;
    static constexpr int PADDING_MEDIUM = 8;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarComponent)
};

} // namespace oscil
