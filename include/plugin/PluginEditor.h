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
#include "ui/animation/OscilAnimationService.h"

#include "plugin/PluginProcessor.h"
#include "ui/managers/GpuRenderCoordinator.h"
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
 * Adapter class to bridge SidebarComponent::Listener timing events to TimingEngine
 */
class TimingSidebarListenerAdapter;

/**
 * Adapter class to bridge SidebarComponent::Listener options events to Editor/Processor
 */
class OptionsSidebarListenerAdapter;

/**
 * Main plugin editor component.
 * Uses coordinator classes to handle listener callbacks, reducing direct
 * coupling and improving testability.
 */
class OscilPluginEditor : public juce::AudioProcessorEditor
    , public juce::DragAndDropContainer
    , public TestIdSupport
    , private juce::Timer
{
public:
    explicit OscilPluginEditor(OscilPluginProcessor& processor);
    ~OscilPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // Sidebar control
    void toggleSidebar();

    // Sidebar event handlers (called from adapter)
    void onSidebarWidthChanged(int newWidth);
    void onSidebarCollapsedStateChanged(bool collapsed);

    // Dialog Listeners
    void onConfigPopupClosed();

    // Accessor for adapter classes to reach processor
    OscilPluginProcessor& getProcessor() { return processor_; }

    // Display options - apply to all panes
    void setShowGridForAllPanes(bool enabled);
    void setGridConfigForAllPanes(const GridConfiguration& config);
    void setAutoScaleForAllPanes(bool enabled);
    void setGainDbForAllPanes(float dB);
    void setDisplaySamplesForAllPanes(int samples);
    void setSampleRateForAllPanes(int sampleRate);

    // Rendering mode control
    void setGpuRenderingEnabled(bool enabled);

    // Test access - for automated testing only
    // Delegated to controller
    const std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() const;
    void refreshPanels();
    
    /**
     * Request a capture of the rendered frame (OpenGL or Software).
     * @param callback Function to call with the result.
     */
    void requestFrameCapture(std::function<void(juce::Image)> callback);

    // Accessor for test server and adapters
    OscillatorPanelController* getOscillatorPanelController() const { return oscillatorPanelController_.get(); }
    
    // Accessor for sidebar (used by TimingEngineListenerAdapter for bidirectional sync)
    SidebarComponent* getSidebar() const { return sidebar_.get(); }
    
    // Accessor for animation service
    OscilAnimationService& getAnimationService() { return *animationService_; }

    // Public refresh methods for adapters
    void refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators);

private:
    void timerCallback() override;
    
    // Coordinator callbacks
    void onSourcesChanged();
    void onThemeChanged(const ColorTheme& newTheme);
    void onLayoutChanged();

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
    std::unique_ptr<TimingSidebarListenerAdapter> timingSidebarAdapter_;
    std::unique_ptr<OptionsSidebarListenerAdapter> optionsSidebarAdapter_;

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
    
    // Animation Service
    std::unique_ptr<OscilAnimationService> animationService_;

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