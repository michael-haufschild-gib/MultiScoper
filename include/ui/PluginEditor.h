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
#include "core/PluginProcessor.h"
#include "core/WindowLayout.h"
#include "ui/components/TestId.h"
#include "ui/AddOscillatorDialog.h"

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

// Forward declaration for OpenGLLifecycleManager
class OpenGLLifecycleManager;

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
                          public juce::ValueTree::Listener,
                          public TestIdSupport,
                          private juce::Timer
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
    void onOscillatorSelected(const OscillatorId& oscillatorId);
    void onOscillatorConfigRequested(const OscillatorId& oscillatorId);

    // Config popup event handlers
    void onOscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated);
    void onOscillatorDeleteRequested(const OscillatorId& oscillatorId);
    void onConfigPopupClosed();

    // Add oscillator dialog handlers
    void onAddOscillatorDialogRequested();
    void onAddOscillatorResult(const AddOscillatorDialog::Result& result);

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
    void setDisplaySamplesForAllPanes(int samples);
    void highlightOscillator(const OscillatorId& oscillatorId);

    // Rendering mode control
    void setGpuRenderingEnabled(bool enabled);

    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int moveFromIndex) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override;

    // Test access - for automated testing only
    const std::vector<std::unique_ptr<PaneComponent>>& getPaneComponents() const { return paneComponents_; }
    void refreshPanels() { refreshOscillatorPanels(); }

private:
    void timerCallback() override;
    void updateLayout();
    void refreshOscillatorPanels();
    void createDefaultOscillator();
    void handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId);
    void handleEmptyColumnDrop(const PaneId& movedPaneId, int targetColumn);

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
    std::unique_ptr<AddOscillatorDialog> addOscillatorDialog_;


    // Test server (for automated UI testing)
    std::unique_ptr<PluginTestServer> testServer_;

    // State
    int frameCount_ = 0;
    double lastFrameTime_ = 0.0;
    float currentFps_ = 0.0f;

    // OpenGL Lifecycle Manager (handles context and renderer)
    std::unique_ptr<OpenGLLifecycleManager> glManager_;

#if OSCIL_ENABLE_INSPECTOR
    // Melatonin Inspector for UI debugging (toggle with Cmd+I / Ctrl+I)
    melatonin::Inspector inspector_ { *this };
#endif

    static constexpr int STATUS_BAR_HEIGHT = 24;
    static constexpr int DEFAULT_WIDTH = 1200;
    static constexpr int DEFAULT_HEIGHT = 800;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilPluginEditor)
};

} // namespace oscil
