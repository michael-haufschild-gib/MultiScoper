/*
    Oscil - Plugin Editor Implementation
    Main plugin GUI
*/

#include "plugin/PluginEditor.h"

#include "core/InstanceRegistry.h"
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

    // Create basic UI components
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>();
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);

    sidebar_ = std::make_unique<SidebarComponent>(serviceContext_);
    sidebarAdapter_ = std::make_unique<SidebarListenerAdapter>(*this);
    sidebar_->addListener(sidebarAdapter_.get());
    addAndMakeVisible(*sidebar_);

    statusBar_ = std::make_unique<StatusBarComponent>(processor_.getThemeService());
    addAndMakeVisible(*statusBar_);

    // Create managers and coordinators
    dialogManager_ = std::make_unique<DialogManager>(*this, processor_.getThemeService(), processor_.getInstanceRegistry());
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    dialogManager_->addConfigPopupListener(configPopupAdapter_.get());

    metricsController_ = std::make_unique<PerformanceMetricsController>(processor_, *statusBar_);
    editorLayout_ = std::make_unique<PluginEditorLayout>(*this, *viewport_, *contentComponent_, *sidebar_, *statusBar_, processor_);
    renderCoordinator_ = std::make_unique<GpuRenderCoordinator>(*this, *statusBar_);

    // Initialize GPU Rendering
    bool gpuRenderingEnabled = processor_.getState().isGpuRenderingEnabled();
    renderCoordinator_->setGpuRenderingEnabled(gpuRenderingEnabled);
    if (auto* optionsSection = sidebar_->getOptionsSection())
    {
        optionsSection->setGpuRenderingEnabled(gpuRenderingEnabled);
        auto qualityConfig = processor_.getState().getCaptureQualityConfig();
        optionsSection->setQualityPreset(qualityConfig.qualityPreset);
        optionsSection->setBufferDuration(qualityConfig.bufferDuration);
        optionsSection->setAutoAdjustQuality(qualityConfig.autoAdjustQuality);
    }

    // Initialize Display Settings Manager (Controller will manage the components list)
    // We pass a reference to the controller's vector, which is initially empty
    // The controller populates it, and DisplaySettingsManager sees the updates
    // NOTE: DisplaySettingsManager holds a reference to the vector. We must ensure
    // controller is created before passing its vector.
    
    // Create Controller
    // We need to create it first to get reference to its vector, but it needs dependencies
    // Circular dependency: Controller needs DisplaySettingsManager, Manager needs Controller's vector.
    // Solution: Create Manager with temporary/empty vector (or update Manager to accept vector later)
    // Current Manager takes `const std::vector<std::unique_ptr<PaneComponent>>&` in constructor.
    // We will construct the controller, then construct the manager using controller's vector.
    // BUT Controller needs Manager for global settings.
    // We'll construct Manager first with a dummy/local vector, or refactor Manager.
    // Refactoring Manager is out of scope for this step.
    // Wait, `OscillatorPanelController` stores the vector.
    // Let's initialize `oscillatorPanelController_` in the member initializer list if possible,
    // or use `std::unique_ptr` and construct in body.
    
    // Correct order:
    // 1. Construct Controller (owns the vector)
    // 2. Construct Manager (references the vector)
    // 3. Pass Manager to Controller (via setter or init method)
    
    // But Controller needs Manager in constructor? 
    // My `OscillatorPanelController` constructor takes `DisplaySettingsManager&`.
    // This implies Manager must exist first.
    // But Manager needs the vector from Controller.
    
    // Workaround: `PluginEditor` owns the vector *conceptually* but delegates management?
    // No, Controller owns it.
    // We can make `DisplaySettingsManager` take a function or interface to get components.
    // Or we create `DisplaySettingsManager` with a reference to a vector we declare in `PluginEditor` *before* transferring ownership?
    // No, `OscillatorPanelController` has `std::vector<...> paneComponents_`.
    
    // Let's look at `DisplaySettingsManager.h`. It likely stores `const std::vector...& panes_`.
    // We can't change that easily without editing it.
    // We will add `getPaneComponents()` to `OscillatorPanelController`.
    // We will delay `OscillatorPanelController::initialize()` until after Manager is created.
    // But we can't construct Controller without Manager reference if it's in constructor.
    
    // Let's create a placeholder vector in PluginEditor that effectively is unused, 
    // or realize that `DisplaySettingsManager` might need to be owned by `OscillatorPanelController` too?
    // No, DisplaySettingsManager handles global settings for *all* panes.
    
    // Simplest fix: Controller takes Manager in `initialize()`, not constructor.
    // I defined `OscillatorPanelController(..., DisplaySettingsManager&)` in the previous step.
    // I should change that to be passed in `initialize` or `reapplyGlobalSettings`.
    // Actually, let's just pass it in constructor.
    // BUT Manager needs vector.
    
    // Okay, `PluginEditor` will *not* own the vector anymore. `OscillatorPanelController` does.
    // So we can't pass a reference to a member of `PluginEditor` to `DisplaySettingsManager`.
    // Unless... `OscillatorPanelController` exposes the vector.
    // We create `oscillatorPanelController_` first.
    // Then create `displaySettingsManager_` passing `oscillatorPanelController_->getPaneComponents()`.
    // Then call `oscillatorPanelController_->initialize(...)` passing `*displaySettingsManager_`.
    
    // This requires `OscillatorPanelController` NOT to take `DisplaySettingsManager` in constructor.
    // I will update `OscillatorPanelController` to take it in `initialize`.
    
    oscillatorPanelController_ = std::make_unique<OscillatorPanelController>(
        processor_, serviceContext_, *contentComponent_, *renderCoordinator_
    );
    
    displaySettingsManager_ = std::make_unique<DisplaySettingsManager>(oscillatorPanelController_->getPaneComponents());
    
    oscillatorPanelController_->initialize(sidebar_.get(), dialogManager_.get(), displaySettingsManager_.get());

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

    // Listen to state
    processor_.getState().addListener(this);

    // Create default oscillator if needed
    oscillatorPanelController_->createDefaultOscillatorIfNeeded();

    // Refresh panels
    oscillatorPanelController_->refreshPanels();

    // Window setup
    setResizable(true, true);
    setResizeLimits(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT,
                    WindowLayout::MAX_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_HEIGHT);
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

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

    if (renderCoordinator_)
        renderCoordinator_->detach();

    if (testServer_)
        testServer_->stop();

    if (sidebar_ && sidebarAdapter_)
        sidebar_->removeListener(sidebarAdapter_.get());

    if (dialogManager_ && configPopupAdapter_)
        dialogManager_->removeConfigPopupListener(configPopupAdapter_.get());

    if (timingEngineAdapter_)
        processor_.getTimingEngine().removeListener(timingEngineAdapter_.get());

    processor_.getState().removeListener(this);
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
    if (sidebar_)
    {
        auto sources = processor_.getInstanceRegistry().getAllSources();
        sidebar_->refreshSourceList(sources);
    }
}

void OscilPluginEditor::onThemeChanged(const ColorTheme& /*newTheme*/)
{
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

void OscilPluginEditor::onOscillatorSelected(const OscillatorId& oscillatorId)
{
    highlightOscillator(oscillatorId);
}

void OscilPluginEditor::onOscillatorConfigRequested(const OscillatorId& oscillatorId)
{
    // Logic still mostly here for now, but could be moved to controller
    // The DialogManager needs the editor as parent, so we keep this wiring here
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            auto& layoutManager = state.getLayoutManager();
            auto panes = layoutManager.getPanes();
            std::vector<std::pair<PaneId, juce::String>> paneList;
            for (const auto& pane : panes)
            {
                paneList.emplace_back(pane.getId(), pane.getName());
            }

            if (dialogManager_)
                dialogManager_->showConfigPopup(osc, paneList, nullptr);
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_) return;

    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            dialogManager_->showColorDialog(osc.getColour(), [this, oscillatorId](juce::Colour color) {
                auto& oscilState = processor_.getState();
                auto oscList = oscilState.getOscillators();
                for (auto& o : oscList)
                {
                    if (o.getId() == oscillatorId)
                    {
                        o.setColour(color);
                        oscilState.updateOscillator(o);
                        break;
                    }
                }
            });
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated)
{
    // This is complex logic involving source changes
    // We can delegate source update to controller
    // But moving pane/updating state is also controller logic
    // Ideally Controller handles ALL of this.
    
    // For now, we'll update state directly and let Controller listener handle refresh
    // BUT if source changed, we need specific handling
    
    auto& state = processor_.getState();
    auto currentOscillators = state.getOscillators();
    for (const auto& existing : currentOscillators)
    {
        if (existing.getId() == oscillatorId)
        {
            if (existing.getSourceId() != updated.getSourceId())
            {
                updateOscillatorSource(oscillatorId, updated.getSourceId());
            }
            break;
        }
    }
    
    state.updateOscillator(updated);
    // Controller listener handles UI refresh
}

void OscilPluginEditor::onOscillatorDeleteRequested(const OscillatorId& oscillatorId)
{
    processor_.getState().removeOscillator(oscillatorId);
}

void OscilPluginEditor::onConfigPopupClosed()
{
    if (dialogManager_) dialogManager_->closeConfigPopup();
}

void OscilPluginEditor::onAddOscillatorDialogRequested()
{
    if (!dialogManager_) return;
    auto sources = processor_.getInstanceRegistry().getAllSources();
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showAddOscillatorDialog(sources, panes, [this](const AddOscillatorDialog::Result& result) {
        onAddOscillatorResult(result);
    });
}

void OscilPluginEditor::onAddOscillatorResult(const AddOscillatorDialog::Result& result)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();
    PaneId targetPaneId = result.paneId;

    if (result.createNewPane)
    {
        Pane newPane;
        newPane.setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
        newPane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
        layoutManager.addPane(newPane);
        targetPaneId = newPane.getId();
    }

    Oscillator osc;
    osc.setSourceId(result.sourceId);
    osc.setPaneId(targetPaneId);
    osc.setProcessingMode(ProcessingMode::FullStereo);
    osc.setColour(result.color);
    osc.setVisualPresetId(result.visualPresetId);
    
    if (result.name.isNotEmpty()) osc.setName(result.name);
    else osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));

    // Ensure shader matches preset
    auto preset = VisualConfiguration::getPreset(result.visualPresetId);
    osc.setShaderId(shaderTypeToId(preset.shaderType));

    state.addOscillator(osc);
}

void OscilPluginEditor::onOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setProcessingMode(mode);
            state.updateOscillator(osc);
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setVisible(visible);
            state.updateOscillator(osc);
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorPaneSelectionRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_) return;
    pendingVisibilityOscillatorId_ = oscillatorId;
    
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    dialogManager_->showSelectPaneDialog(panes, [this](const SelectPaneDialog::Result& result) {
        auto& stateRef = processor_.getState();
        auto& layoutMgr = stateRef.getLayoutManager();
        PaneId targetPaneId = result.paneId;

        if (result.createNewPane)
        {
            Pane newPane;
            newPane.setName("Pane " + juce::String(layoutMgr.getPaneCount() + 1));
            newPane.setOrderIndex(static_cast<int>(layoutMgr.getPaneCount()));
            layoutMgr.addPane(newPane);
            targetPaneId = newPane.getId();
        }

        auto oscList = stateRef.getOscillators();
        for (auto& osc : oscList)
        {
            if (osc.getId() == pendingVisibilityOscillatorId_)
            {
                osc.setPaneId(targetPaneId);
                osc.setVisible(true);
                stateRef.updateOscillator(osc);
                break;
            }
        }
        pendingVisibilityOscillatorId_ = OscillatorId::invalid();
    });
}

void OscilPluginEditor::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    // This needs to be explicit because it might involve buffer swapping not just state change
    // Delegate to controller?
    // Controller's updateOscillatorSource is private.
    // But changing state triggers Listener in controller.
    // So we just update state here.
    
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setSourceId(newSourceId);
            state.updateOscillator(osc);
            break;
        }
    }
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

void OscilPluginEditor::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (displaySettingsManager_) displaySettingsManager_->highlightOscillator(oscillatorId);
}

void OscilPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    if (renderCoordinator_) renderCoordinator_->setGpuRenderingEnabled(enabled);
    
    // Update sidebar
    if (sidebar_ && sidebar_->getOptionsSection())
        sidebar_->getOptionsSection()->setGpuRenderingEnabled(enabled);
        
    processor_.getState().setGpuRenderingEnabled(enabled);
    
    // Controller listener will pick up state change? 
    // GpuRenderingEnabled is a property of root or Options state?
    // It's in OscilState.
    // Controller doesn't listen to GPU state specifically in `valueTreePropertyChanged` in my implementation.
    // We should manually propagate or update controller.
    
    if (oscillatorPanelController_)
        oscillatorPanelController_->refreshPanels(); // Re-propagates GPU state
}

// ValueTree Listeners (Just pass-through or handled by Controller)
// Actually, PluginEditor is still a listener.
// But Controller is ALSO a listener.
// We should remove redundant logic.

void OscilPluginEditor::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {}
void OscilPluginEditor::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) {}
void OscilPluginEditor::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) {}
void OscilPluginEditor::valueTreeChildOrderChanged(juce::ValueTree&, int, int) {}
void OscilPluginEditor::valueTreeParentChanged(juce::ValueTree&) {}

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



} // namespace oscil
