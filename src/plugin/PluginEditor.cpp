/*
    Oscil - Plugin Editor Implementation
    Main plugin GUI
*/

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"

#include "core/InstanceRegistry.h"
#include "ui/animation/OscilAnimationService.h"
#include "ui/layout/LayoutCoordinator.h"
#include "ui/layout/PaneComponent.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/layout/SourceCoordinator.h"
#include "ui/managers/DialogManager.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "ui/panels/StatusBarComponent.h"
#include "ui/panels/WaveformComponent.h"
#include "ui/theme/ThemeCoordinator.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/VisualConfiguration.h"

#include "tools/PluginEditor_Adapters.h"
#include "tools/test_server/PluginTestServer.h"

#include <cmath>

namespace oscil
{

OscilPluginEditor::OscilPluginEditor(OscilPluginProcessor& p)
    : AudioProcessorEditor(&p)
    , processor_(p)
    , serviceContext_{ processor_.getInstanceRegistry(), processor_.getThemeService(), processor_.getShaderRegistry() }
{
    // Create coordinators
    sourceCoordinator_ = std::make_unique<SourceCoordinator>(
        processor_.getInstanceRegistry(),
        [this]() { onSourcesChanged(); });

    themeCoordinator_ = std::make_unique<ThemeCoordinator>(
        processor_.getThemeService(),
        [this](const ColorTheme& theme) { onThemeChanged(theme); });

    layoutCoordinator_ = std::make_unique<LayoutCoordinator>(
        windowLayout_,
        [this]() { onLayoutChanged(); });

    // Apply theme
    processor_.getThemeService().setCurrentTheme(processor_.getState().getThemeName());

    // H50 FIX: Create basic UI components using addChildComponent (not visible yet)
    // Components are made visible at the end after all setup is complete
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>();
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addChildComponent(*viewport_);  // H50: Added but not visible yet

    sidebar_ = std::make_unique<SidebarComponent>(serviceContext_);
    addChildComponent(*sidebar_);  // H50: Added but not visible yet

    statusBar_ = std::make_unique<StatusBarComponent>(processor_.getThemeService());
    addChildComponent(*statusBar_);  // H50: Added but not visible yet

    // Create managers and coordinators
    dialogManager_ = std::make_unique<DialogManager>(
        *this, 
        processor_.getThemeService(), 
        processor_.getInstanceRegistry(),
        PluginFactory::getInstance().getVisualPresetManager());
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    dialogManager_->addConfigPopupListener(configPopupAdapter_.get());

    metricsController_ = std::make_unique<PerformanceMetricsController>(processor_, *statusBar_);
    editorLayout_ = std::make_unique<PluginEditorLayout>(*this, *viewport_, *contentComponent_, *sidebar_, *statusBar_, processor_);
    renderCoordinator_ = std::make_unique<GpuRenderCoordinator>(*this, *statusBar_);
    animationService_ = std::make_unique<OscilAnimationService>(this);
    
    // Register animation service in properties so child components can find it
    getProperties().set("oscilAnimationService", reinterpret_cast<int64_t>(animationService_.get()));

    // Initialize GPU Rendering
    bool gpuRenderingEnabled = processor_.getState().isGpuRenderingEnabled();
    renderCoordinator_->setGpuRenderingEnabled(gpuRenderingEnabled);
    
    // Set initial background color for GPU renderer
    const auto& theme = processor_.getThemeService().getCurrentTheme();
    renderCoordinator_->setBackgroundColour(theme.backgroundPane);
    
    if (auto* optionsSection = sidebar_->getOptionsSection())
    {
        optionsSection->setGpuRenderingEnabled(gpuRenderingEnabled);
        
        // Sync visual settings from state
        auto& state = processor_.getState();
        optionsSection->setAutoScale(state.isAutoScaleEnabled());
        optionsSection->setShowGrid(state.isShowGridEnabled());
        optionsSection->setGainDb(state.getGainDb());
        
        auto qualityConfig = state.getCaptureQualityConfig();
        optionsSection->setQualityPreset(qualityConfig.qualityPreset);
        optionsSection->setBufferDuration(qualityConfig.bufferDuration);
        optionsSection->setAutoAdjustQuality(qualityConfig.autoAdjustQuality);
    }

    // Initialize Controller and DisplaySettingsManager with two-phase initialization to resolve
    // circular dependency: Controller owns the pane vector, Manager needs to reference it,
    // but Controller needs Manager for applying settings. Solution: Controller constructed first,
    // Manager constructed with Controller's vector reference, then Manager passed to initialize().
    oscillatorPanelController_ = std::make_unique<OscillatorPanelController>(
        processor_, serviceContext_, *contentComponent_, *renderCoordinator_
    );
    
    // Create DisplaySettingsManager with a callback that returns snapshot of current panes.
    // This prevents iterator invalidation if pane vector changes during settings updates.
    displaySettingsManager_ = std::make_unique<DisplaySettingsManager>([this]() {
        std::vector<PaneComponent*> snapshot;
        for (auto& pane : oscillatorPanelController_->getPaneComponents())
        {
            if (pane)
                snapshot.push_back(pane.get());
        }
        return snapshot;
    });
    
    oscillatorPanelController_->initialize(sidebar_.get(), dialogManager_.get(), displaySettingsManager_.get());

    // Register Controller as Sidebar Listener (Handles Oscillator Events)
    sidebar_->addListener(oscillatorPanelController_.get());
    
    // Register Timing Adapter as Sidebar Listener (Handles Timing Events)
    timingSidebarAdapter_ = std::make_unique<TimingSidebarListenerAdapter>(*this);
    sidebar_->addListener(timingSidebarAdapter_.get());

    // Register Options Adapter as Sidebar Listener (Handles Global Options)
    optionsSidebarAdapter_ = std::make_unique<OptionsSidebarListenerAdapter>(*this);
    sidebar_->addListener(optionsSidebarAdapter_.get());

    // CRITICAL: Wire layout callback so panes get positioned after async refreshPanels()
    oscillatorPanelController_->setLayoutNeededCallback([this]() {
        if (editorLayout_)
            editorLayout_->updateLayout(oscillatorPanelController_->getPaneComponents());
    });

    // ... Rest of initialization ...

    // Timing Engine
    timingEngineAdapter_ = std::make_unique<TimingEngineListenerAdapter>(*this);
    processor_.getTimingEngine().addListener(timingEngineAdapter_.get());
    
    // Sync timing UI
    auto timingConfig = processor_.getTimingEngine().toEntityConfig();
    if (auto* timingSection = sidebar_->getTimingSection())
    {
        timingSection->setTimingMode(timingConfig.timingMode);
        timingSection->setTimeIntervalMs(static_cast<int>(timingConfig.timeIntervalMs));
        timingSection->setNoteInterval(timingConfig.noteInterval);
        timingSection->setHostSyncEnabled(timingConfig.hostSyncEnabled);
        timingSection->setHostBPM(timingConfig.hostBPM);
    }

    // Create default oscillator if needed
    oscillatorPanelController_->createDefaultOscillatorIfNeeded();

    // Refresh panels
    oscillatorPanelController_->refreshPanels();

    // Window setup
    setResizable(true, true);
    setResizeLimits(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT,
                    WindowLayout::MAX_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_HEIGHT);
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    // H50 FIX: Make child components visible AFTER all setup is complete
    // This ensures proper initialization order and prevents issues with
    // components accessing uninitialized state during visibility callbacks
    viewport_->setVisible(true);
    sidebar_->setVisible(true);
    statusBar_->setVisible(true);

    startTimerHz(TIMER_REFRESH_RATE_HZ);

    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
    {
        testServer_ = std::make_unique<PluginTestServer>(*this);
        testServer_->start(TEST_SERVER_PORT);
        DBG("Test server started on port " << TEST_SERVER_PORT);
    }
}

OscilPluginEditor::~OscilPluginEditor()
{
    stopTimer();

    // H47 FIX: Clear animation service property BEFORE destroying the service
    // This prevents child components from accessing a dangling pointer
    getProperties().remove("oscilAnimationService");

    // H47 FIX: Stop and clear animation service properly
    if (animationService_)
    {
        animationService_->stopAll();
        animationService_.reset();
    }

    if (renderCoordinator_)
        renderCoordinator_->detach();

    if (testServer_)
        testServer_->stop();

    if (sidebar_ && oscillatorPanelController_)
        sidebar_->removeListener(oscillatorPanelController_.get());

    if (sidebar_ && timingSidebarAdapter_)
        sidebar_->removeListener(timingSidebarAdapter_.get());

    if (sidebar_ && optionsSidebarAdapter_)
        sidebar_->removeListener(optionsSidebarAdapter_.get());

    if (dialogManager_ && configPopupAdapter_)
        dialogManager_->removeConfigPopupListener(configPopupAdapter_.get());

    if (timingEngineAdapter_)
        processor_.getTimingEngine().removeListener(timingEngineAdapter_.get());
}

void OscilPluginEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();
    if (getParentComponent() == nullptr && renderCoordinator_)
    {
        stopTimer();
        renderCoordinator_->detach();
        DBG("OpenGL context detached early");
    }
}

void OscilPluginEditor::paint(juce::Graphics& g)
{
    if (!renderCoordinator_ || !renderCoordinator_->isGpuRenderingEnabled())
    {
        const auto& theme = themeCoordinator_->getCurrentTheme();
        g.fillAll(theme.backgroundPrimary);
    }
}

void OscilPluginEditor::resized()
{
    if (editorLayout_)
    {
        editorLayout_->resized();
        // Layout engine expects a vector of unique_ptr<PaneComponent>
        // We pass the one from the controller
        editorLayout_->updateLayout(oscillatorPanelController_->getPaneComponents());
    }
}

void OscilPluginEditor::timerCallback()
{
    processor_.getTimingEngine().dispatchPendingUpdates();

    if (metricsController_)
        metricsController_->update();

    if (renderCoordinator_)
        renderCoordinator_->updateRendering(oscillatorPanelController_->getPaneComponents());
}

// Delegated methods

void OscilPluginEditor::refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators)
{
    if (sidebar_) sidebar_->refreshOscillatorList(oscillators);
}

void OscilPluginEditor::onSourcesChanged()
{
    auto sources = processor_.getInstanceRegistry().getAllSources();

    if (sidebar_)
    {
        sidebar_->refreshSourceList(sources);
    }

    // Update oscillators that have invalid or stale sourceIds
    // This handles:
    // 1. Oscillators created before sources were registered (invalid sourceId)
    // 2. Oscillators with stale sourceIds from previous sessions that no longer exist
    if (!sources.empty())
    {
        auto& state = processor_.getState();
        auto oscillators = state.getOscillators();
        bool updated = false;
        auto& registry = processor_.getInstanceRegistry();

        for (auto& osc : oscillators)
        {
            bool needsSourceAssignment = false;

            // Check if sourceId is invalid (empty)
            if (!osc.getSourceId().isValid())
            {
                needsSourceAssignment = true;
            }
            // Check if sourceId is stale (valid format but doesn't exist in current registry)
            else if (!registry.getSource(osc.getSourceId()).has_value())
            {
                // Try to resolve by instanceUUID first (most reliable for cross-session)
                if (osc.getSourceInstanceUUID().isNotEmpty())
                {
                    auto resolvedSource = registry.getSourceByInstanceUUID(osc.getSourceInstanceUUID());
                    if (resolvedSource.has_value())
                    {
                        // Resolved by UUID - update with new sourceId
                        osc.setSourceIdNameAndUUID(resolvedSource->sourceId, resolvedSource->name, resolvedSource->instanceUUID);
                        state.updateOscillator(osc);
                        updated = true;
                        continue;  // Successfully resolved
                    }
                }

                // Try to resolve by sourceName (fallback)
                if (osc.getSourceName().isNotEmpty())
                {
                    auto resolvedSource = registry.getSourceByName(osc.getSourceName());
                    if (resolvedSource.has_value())
                    {
                        // Resolved by name - update with new sourceId and cache UUID
                        osc.setSourceIdNameAndUUID(resolvedSource->sourceId, resolvedSource->name, resolvedSource->instanceUUID);
                        state.updateOscillator(osc);
                        updated = true;
                        continue;  // Successfully resolved
                    }
                }

                // All resolution failed - assign default source
                needsSourceAssignment = true;
            }

            if (needsSourceAssignment)
            {
                // Assign the first available source (typically the processor's own source)
                // Prefer processor's own source if available
                SourceId sourceToAssign = sources[0].sourceId;
                juce::String sourceName = sources[0].name;
                juce::String sourceUUID = sources[0].instanceUUID;
                if (processor_.getSourceId().isValid())
                {
                    sourceToAssign = processor_.getSourceId();
                    // Look up source info from registry
                    auto sourceInfo = registry.getSource(sourceToAssign);
                    if (sourceInfo.has_value())
                    {
                        sourceName = sourceInfo->name;
                        sourceUUID = sourceInfo->instanceUUID;
                    }
                }
                // Use setSourceIdNameAndUUID to capture all identifiers for persistence
                osc.setSourceIdNameAndUUID(sourceToAssign, sourceName, sourceUUID);
                state.updateOscillator(osc);
                updated = true;
            }
        }

        // Refresh panels if any oscillators were updated
        if (updated && oscillatorPanelController_)
        {
            oscillatorPanelController_->refreshPanels();
        }
    }
}

void OscilPluginEditor::onThemeChanged(const ColorTheme& newTheme)
{
    // Update GPU renderer background color
    if (renderCoordinator_)
        renderCoordinator_->setBackgroundColour(newTheme.backgroundPane);

    repaint();

    if (statusBar_)
        statusBar_->repaint();

    if (sidebar_)
        sidebar_->repaint();

    if (oscillatorPanelController_)
    {
        for (auto& pane : oscillatorPanelController_->getPaneComponents())
        {
            if (pane)
                pane->repaint();
        }
    }
}

void OscilPluginEditor::onLayoutChanged()
{
    if (editorLayout_)
        resized();
}

// Event Handlers (Adapters)

void OscilPluginEditor::toggleSidebar()
{
    if (sidebar_) sidebar_->toggleCollapsed();
}

void OscilPluginEditor::onSidebarWidthChanged(int newWidth)
{
    windowLayout_.setSidebarWidth(newWidth);
    resized();
}

void OscilPluginEditor::onSidebarCollapsedStateChanged(bool collapsed)
{
    windowLayout_.setSidebarCollapsed(collapsed);
    resized();
}

void OscilPluginEditor::onConfigPopupClosed()
{
    if (dialogManager_) dialogManager_->closeConfigPopup();
}

// Global Settings (delegated to Manager)

void OscilPluginEditor::setShowGridForAllPanes(bool enabled)
{
    if (displaySettingsManager_) displaySettingsManager_->setShowGridForAll(enabled);
}

void OscilPluginEditor::setGridConfigForAllPanes(const GridConfiguration& config)
{
    if (displaySettingsManager_) displaySettingsManager_->setGridConfigForAll(config);
}

void OscilPluginEditor::setAutoScaleForAllPanes(bool enabled)
{
    if (displaySettingsManager_) displaySettingsManager_->setAutoScaleForAll(enabled);
}

void OscilPluginEditor::setGainDbForAllPanes(float dB)
{
    if (displaySettingsManager_) displaySettingsManager_->setGainDbForAll(dB);
}

void OscilPluginEditor::setDisplaySamplesForAllPanes(int samples)
{
    if (displaySettingsManager_) displaySettingsManager_->setDisplaySamplesForAll(samples);
}

void OscilPluginEditor::setSampleRateForAllPanes(int sampleRate)
{
    if (displaySettingsManager_) displaySettingsManager_->setSampleRateForAll(sampleRate);
}

void OscilPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    // Clear waveforms before switching modes to prevent ghost images
    if (renderCoordinator_)
    {
        renderCoordinator_->clearWaveforms();
        renderCoordinator_->setGpuRenderingEnabled(enabled);
    }
    
    // Update sidebar
    if (sidebar_ && sidebar_->getOptionsSection())
        sidebar_->getOptionsSection()->setGpuRenderingEnabled(enabled);
        
    processor_.getState().setGpuRenderingEnabled(enabled);
    
    // Refresh panels to propagate GPU state to all waveform components
    if (oscillatorPanelController_)
        oscillatorPanelController_->refreshPanels();
    
    // Force repaint of entire editor to clear any ghost images
    repaint();
}

// Test access

const std::vector<std::unique_ptr<PaneComponent>>& OscilPluginEditor::getPaneComponents() const
{
    return oscillatorPanelController_->getPaneComponents();
}

void OscilPluginEditor::refreshPanels()
{
    if (oscillatorPanelController_)
        oscillatorPanelController_->refreshPanels();
}

void OscilPluginEditor::requestFrameCapture(std::function<void(juce::Image)> callback)
{
    if (renderCoordinator_)
    {
        renderCoordinator_->requestFrameCapture(std::move(callback));
    }
    else
    {
        callback(juce::Image());
    }
}

} // namespace oscil
