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
#include "core/PluginProcessor.h"
#include "core/WindowLayout.h"

namespace oscil
{

// Forward declarations
class WaveformComponent;
class OscillatorPanel;
class PaneComponent;
class StatusBarComponent;
class SidebarComponent;
class OscillatorConfigPopup;
class SourceCoordinator;
class ThemeCoordinator;
class LayoutCoordinator;
class PluginTestServer;
struct ColorTheme;

/**
 * Adapter class to bridge SidebarComponent::Listener to OscilPluginEditor
 * This avoids multiple inheritance with conflicting method signatures
 */
class SidebarListenerAdapter;

/**
 * Adapter class to bridge OscillatorConfigPopup::Listener to OscilPluginEditor
 */
class ConfigPopupListenerAdapter;

// Forward declaration
class PaneContainerComponent;

/**
 * Main plugin editor component.
 * Uses coordinator classes to handle listener callbacks, reducing direct
 * coupling and improving testability.
 */
class OscilPluginEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer,
                          private juce::Timer
{
public:
    explicit OscilPluginEditor(OscilPluginProcessor& processor);
    ~OscilPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Sidebar control
    void toggleSidebar();

    // Sidebar event handlers (called from adapter)
    void onSidebarWidthChanged(int newWidth);
    void onSidebarCollapsedStateChanged(bool collapsed);
    void onOscillatorSelected(const OscillatorId& oscillatorId);
    void onOscillatorConfigRequested(const OscillatorId& oscillatorId);
    void onAddSourceToPane(const SourceId& sourceId, const PaneId& paneId);

    // Config popup event handlers
    void onOscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated);
    void onOscillatorDeleteRequested(const OscillatorId& oscillatorId);
    void onConfigPopupClosed();

    // Oscillator property change handlers (from sidebar)
    void onOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode);
    void onOscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible);

    // Accessor for adapter classes to reach processor
    OscilPluginProcessor& getProcessor() { return processor_; }

    /**
     * Update an oscillator's source and refresh the waveform buffer binding
     * This is the central coordination point for source changes from any UI
     */
    void updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId);

    // Display options - apply to all panes
    void setShowGridForAllPanes(bool enabled);
    void setAutoScaleForAllPanes(bool enabled);
    void setHoldDisplayForAllPanes(bool enabled);
    void setGainDbForAllPanes(float dB);
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Test access - for automated testing only
    const std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() const { return paneComponents_; }
    void refreshPanels() { refreshOscillatorPanels(); }

private:
    void timerCallback() override;
    void updateLayout();
    void refreshOscillatorPanels();
    void createDefaultOscillator();
    void handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId);

    // Coordinator callbacks
    void onSourcesChanged();
    void onThemeChanged(const ColorTheme& newTheme);
    void onLayoutChanged();

    OscilPluginProcessor& processor_;
    WindowLayout windowLayout_;

    // Coordinators (manage listener registrations)
    std::unique_ptr<SourceCoordinator> sourceCoordinator_;
    std::unique_ptr<ThemeCoordinator> themeCoordinator_;
    std::unique_ptr<LayoutCoordinator> layoutCoordinator_;

    // UI Components
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<PaneContainerComponent> contentComponent_;
    std::vector<std::unique_ptr<PaneComponent>> paneComponents_;
    std::unique_ptr<StatusBarComponent> statusBar_;
    std::unique_ptr<SidebarComponent> sidebar_;
    std::unique_ptr<SidebarListenerAdapter> sidebarAdapter_;
    std::unique_ptr<OscillatorConfigPopup> configPopup_;
    std::unique_ptr<ConfigPopupListenerAdapter> configPopupAdapter_;

    // Toolbar components
    std::unique_ptr<juce::ComboBox> columnSelector_;
    std::unique_ptr<juce::TextButton> addOscillatorButton_;
    std::unique_ptr<juce::ComboBox> themeSelector_;
    std::unique_ptr<juce::TextButton> sidebarToggleButton_;

    // Test server (for automated UI testing)
    std::unique_ptr<PluginTestServer> testServer_;

    // State
    int frameCount_ = 0;
    double lastFrameTime_ = 0.0;
    float currentFps_ = 0.0f;

#if OSCIL_ENABLE_OPENGL
    // OpenGL context for GPU-accelerated rendering
    juce::OpenGLContext openGLContext_;
#endif

    static constexpr int TOOLBAR_HEIGHT = 40;
    static constexpr int STATUS_BAR_HEIGHT = 24;
    static constexpr int DEFAULT_WIDTH = 1200;
    static constexpr int DEFAULT_HEIGHT = 800;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilPluginEditor)
};

} // namespace oscil
