/*
    Oscil - Plugin Editor Header
    Main plugin GUI
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#if OSCIL_ENABLE_OPENGL
    #include <juce_opengl/juce_opengl.h>
#endif
#if OSCIL_ENABLE_INSPECTOR
    #include <melatonin_inspector/melatonin_inspector.h>
#endif
#include "ui/components/OscilModal.h"
#include "ui/components/TestId.h"
#include "ui/dialogs/AddOscillatorDialog.h"
#include "ui/dialogs/OscillatorColorDialog.h"
#include "ui/dialogs/SelectPaneDialog.h"
#include "ui/layout/PaneContainerComponent.h"
#include "ui/layout/PluginEditorLayout.h"
#include "ui/layout/WindowLayout.h"
#include "ui/managers/DialogManager.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "ui/managers/PerformanceMetricsController.h"
#include "ui/dialogs/OscillatorConfigDialog.h"
#include "ui/panels/StatusBarComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "ui/theme/ThemeManager.h"
#include "ui/controllers/OscillatorPanelController.h"

#include "plugin/PluginProcessor.h"
#include "ui/controllers/GpuRenderCoordinator.h"
#include "core/ServiceContext.h"

namespace oscil
{

// Forward declarations
class PaneComponent;
class StatusBarComponent;
class SidebarComponent;
class SourceCoordinator;
class ThemeCoordinator;
class LayoutCoordinator;
class PluginTestServer;

// Forward declaration for OpenGLLifecycleManager
class OpenGLLifecycleManager;

class PluginEditorLayout;
class PerformanceMetricsController;
class GpuRenderCoordinator;

/**
 * Adapter class to bridge SidebarComponent::Listener to OscilPluginEditor
 * This avoids multiple inheritance with conflicting method signatures
 */
// SidebarListenerAdapter removed - OscillatorPanelController listens directly

/**
 * Adapter class to bridge OscillatorConfigDialog::Listener to OscilPluginEditor
 */
class ConfigPopupListenerAdapter;

/**
 * Adapter class to bridge TimingEngine::Listener to OscilPluginEditor
 */
class TimingEngineListenerAdapter;

/**
 * Main plugin editor component.
 * Uses coordinator classes to handle listener callbacks, reducing direct
 * coupling and improving testability.
 */
class OscilPluginEditor : public juce::AudioProcessorEditor
    , public juce::DragAndDropContainer
    , public SidebarComponent::Listener
    , public TestIdSupport
    , private juce::Timer
{
public:
    /// Construct the editor, wiring all coordinators and UI subsystems.
    explicit OscilPluginEditor(OscilPluginProcessor& processor);
    ~OscilPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;

    /// Toggle sidebar visibility (collapse/expand).
    void toggleSidebar();

    /// Handle sidebar width drag events from layout coordinator.
    void onSidebarWidthChanged(int newWidth);
    /// Handle sidebar collapsed state toggle.
    void onSidebarCollapsedStateChanged(bool collapsed);

    /// Sync timing sidebar display when timing mode changes.
    void updateTimingSidebarMode(TimingMode mode);
    /// Sync timing sidebar host-sync toggle display.
    void updateTimingSidebarHostSyncEnabled(bool enabled);
    /// Sync timing sidebar BPM display from host.
    void updateTimingSidebarHostBpm(float bpm);

    /// Called when oscillator config popup closes (refresh panels).
    void onConfigPopupClosed();

    /// Access the underlying processor (for adapter classes).
    OscilPluginProcessor& getProcessor() { return processor_; }

    /// Apply grid visibility setting to all panes.
    void setShowGridForAllPanes(bool enabled);
    /// Apply grid configuration to all panes.
    void setGridConfigForAllPanes(const GridConfiguration& config);
    /// Apply auto-scale setting to all panes.
    void setAutoScaleForAllPanes(bool enabled);
    /// Apply gain (dB) setting to all panes.
    void setGainDbForAllPanes(float dB);
    /// Set display sample count for all panes.
    void setDisplaySamplesForAllPanes(int samples);
    /// Set display sample rate for all panes.
    void setSampleRateForAllPanes(int sampleRate);

    /// Switch between GPU (OpenGL) and software rendering.
    void setGpuRenderingEnabled(bool enabled);

    /// Get pane components (test access only).
    const std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() const;
    /// Force-refresh all oscillator panels from current state.
    void refreshPanels();

    /// Get the oscillator panel controller (for test server and adapters).
    OscillatorPanelController* getOscillatorPanelController() const { return oscillatorPanelController_.get(); }

    /// Refresh sidebar oscillator list with current oscillators.
    void refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators);

private:
    void timerCallback() override;
    
    // Coordinator callbacks
    void onSourcesChanged();
    void onThemeChanged(const ColorTheme& newTheme);
    void onLayoutChanged();

    // SidebarComponent::Listener (non-oscillator events)
    void sidebarWidthChanged(int newWidth) override;
    void sidebarCollapsedStateChanged(bool collapsed) override;
    void timingModeChanged(TimingMode mode) override;
    void noteIntervalChanged(NoteInterval interval) override;
    void timeIntervalChanged(float ms) override;
    void hostSyncChanged(bool enabled) override;
    void waveformModeChanged(WaveformMode mode) override;
    void bpmChanged(float bpm) override;
    void gainChanged(float dB) override;
    void showGridChanged(bool enabled) override;
    void autoScaleChanged(bool enabled) override;
    void layoutChanged(int columnCount) override;
    void themeChanged(const juce::String& themeName) override;
    void gpuRenderingChanged(bool enabled) override;
    void qualityPresetChanged(QualityPreset preset) override;
    void bufferDurationChanged(BufferDuration duration) override;
    void autoAdjustQualityChanged(bool enabled) override;

    OscilPluginProcessor& processor_;
    ServiceContext serviceContext_;
    WindowLayout windowLayout_;

    // Coordinators (manage listener registrations)
    std::unique_ptr<SourceCoordinator> sourceCoordinator_;
    std::unique_ptr<ThemeCoordinator> themeCoordinator_;
    std::unique_ptr<LayoutCoordinator> layoutCoordinator_;

    // UI Components
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<PaneContainerComponent> contentComponent_;
    
    // Controller now manages pane components
    std::unique_ptr<OscillatorPanelController> oscillatorPanelController_;

    std::unique_ptr<StatusBarComponent> statusBar_;
    std::unique_ptr<SidebarComponent> sidebar_;

    std::unique_ptr<TimingEngineListenerAdapter> timingEngineAdapter_;

    // Dialog Manager
    std::unique_ptr<DialogManager> dialogManager_;
    std::unique_ptr<ConfigPopupListenerAdapter> configPopupAdapter_;

    // Display Settings Manager
    std::unique_ptr<DisplaySettingsManager> displaySettingsManager_;

    // Test server (for automated UI testing)
    std::unique_ptr<PluginTestServer> testServer_;

    // State
    // Metrics handled by PerformanceMetricsController

    // Render Coordinator (handles OpenGL lifecycle)
    std::unique_ptr<GpuRenderCoordinator> renderCoordinator_;

    // Layout Engine
    std::unique_ptr<PluginEditorLayout> editorLayout_;

    // Metrics Controller
    std::unique_ptr<PerformanceMetricsController> metricsController_;

#if OSCIL_ENABLE_INSPECTOR
    // Melatonin Inspector for UI debugging (toggle with Cmd+I / Ctrl+I)
    melatonin::Inspector inspector_{*this};
#endif

    static constexpr int STATUS_BAR_HEIGHT = 24;
    static constexpr int DEFAULT_WIDTH = 1200;
    static constexpr int DEFAULT_HEIGHT = 800;
    static constexpr int TIMER_REFRESH_RATE_HZ = 60;
    static constexpr int TEST_SERVER_PORT = 9876;
    static constexpr int MIN_CONTENT_WIDTH = 400;
    static constexpr int MIN_CONTENT_HEIGHT = 300;
    static constexpr int MIN_PANE_HEIGHT = 200;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilPluginEditor)
};

} // namespace oscil
