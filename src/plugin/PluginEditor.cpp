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

#include "tools/PluginEditor_Adapters.h"
#include "tools/test_server/PluginTestServer.h"

#include <cmath>

namespace oscil
{

OscilPluginEditor::OscilPluginEditor(OscilPluginProcessor& p)
    : AudioProcessorEditor(&p)
    , processor_(p)
    , serviceContext_{ processor_.getInstanceRegistry(), processor_.getThemeService() }
{
    // Create coordinators with callbacks
    sourceCoordinator_ = std::make_unique<SourceCoordinator>(
        processor_.getInstanceRegistry(),
        [this]() { onSourcesChanged(); });

    themeCoordinator_ = std::make_unique<ThemeCoordinator>(
        processor_.getThemeService(),
        [this](const ColorTheme& theme) { onThemeChanged(theme); });

    layoutCoordinator_ = std::make_unique<LayoutCoordinator>(
        windowLayout_,
        [this]() { onLayoutChanged(); });

    // Load and apply saved theme
    auto savedThemeName = processor_.getState().getThemeName();
    processor_.getThemeService().setCurrentTheme(savedThemeName);

    // Create dialog manager
    dialogManager_ = std::make_unique<DialogManager>(*this, processor_.getThemeService(), processor_.getInstanceRegistry());

    // Create config popup adapter and register with manager
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    dialogManager_->addConfigPopupListener(configPopupAdapter_.get());

    // Create viewport and content
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>();
    contentComponent_->setPaneDropCallback([this](const PaneId& movedPaneId, const PaneId& targetPaneId) {
        handlePaneReordered(movedPaneId, targetPaneId);
    });
    contentComponent_->setEmptyColumnDropCallback([this](const PaneId& movedPaneId, int targetColumn) {
        handleEmptyColumnDrop(movedPaneId, targetColumn);
    });
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);

    // Create sidebar
    sidebar_ = std::make_unique<SidebarComponent>(serviceContext_);
    sidebarAdapter_ = std::make_unique<SidebarListenerAdapter>(*this);
    sidebar_->addListener(sidebarAdapter_.get());
    addAndMakeVisible(*sidebar_);

    // Create timing engine listener for automatic updates on host timing changes
    timingEngineAdapter_ = std::make_unique<TimingEngineListenerAdapter>(*this);
    processor_.getTimingEngine().addListener(timingEngineAdapter_.get());

    // Sync timing settings from engine to UI
    auto timingConfig = processor_.getTimingEngine().toEntityConfig();
    if (auto* timingSection = sidebar_->getTimingSection())
    {
        timingSection->setTimingMode(timingConfig.timingMode);
        timingSection->setTimeIntervalMs(static_cast<int>(timingConfig.timeIntervalMs));
        timingSection->setNoteInterval(timingConfig.noteInterval);
        timingSection->setHostSyncEnabled(timingConfig.hostSyncEnabled);
        timingSection->setHostBPM(timingConfig.hostBPM);
    }

    // Initialize display samples from timing config
    // This must be done after syncing timing settings since setTimeIntervalMs
    // doesn't trigger notifications to avoid circular updates
    // Use actualIntervalMs which is computed correctly for both TIME and MELODIC modes
    double sampleRate = processor_.getSampleRate();
    if (sampleRate > 0)
    {
        int displaySamples = static_cast<int>(sampleRate * (static_cast<double>(timingConfig.actualIntervalMs) / 1000.0));
        setDisplaySamplesForAllPanes(displaySamples);
    }

    // Create status bar
    statusBar_ = std::make_unique<StatusBarComponent>(processor_.getThemeService());
    addAndMakeVisible(*statusBar_);

    // Create sub-components
    metricsController_ = std::make_unique<PerformanceMetricsController>(processor_, *statusBar_);
    editorLayout_ = std::make_unique<PluginEditorLayout>(*this, *viewport_, *contentComponent_, *sidebar_, *statusBar_, processor_);
    renderCoordinator_ = std::make_unique<GpuRenderCoordinator>(*this, *statusBar_);

    // Listen to state changes for external updates (API, automation)
    processor_.getState().addListener(this);

    // Source coordinator already initialized available sources in its constructor

    // Initialize default state if needed (e.g. first run)
    // We do this BEFORE refreshing panels so the default state is reflected immediately.
    // We check for empty layout AND empty oscillators to ensure we don't override a loaded preset.
    auto& state = processor_.getState();
    if (state.getLayoutManager().getPaneCount() == 0 && state.getOscillators().empty())
    {
        const auto& availableSources = sourceCoordinator_->getAvailableSources();
        if (!availableSources.empty())
        {
            createDefaultOscillator();
        }
    }

    // CRITICAL: Initialize GPU rendering state from saved preference BEFORE building panels
    // This ensures WaveformComponents get correct GPU state when created in refreshOscillatorPanels()
    // Note: This is now handled by local variable 'gpuRenderingEnabled' and passed to glManager later

    // Refresh UI - now WaveformComponents will be created with correct GPU state
    refreshOscillatorPanels();
    DBG("Panels refreshed");

    // Create display settings manager after panes are created
    displaySettingsManager_ = std::make_unique<DisplaySettingsManager>(paneComponents_);

    // Apply saved display options
    setShowGridForAllPanes(state.isShowGridEnabled());
    setAutoScaleForAllPanes(state.isAutoScaleEnabled());
    setHoldDisplayForAllPanes(state.isHoldDisplayEnabled());
    setGainDbForAllPanes(state.getGainDb());
    DBG("Display options applied");

    // Apply saved GPU rendering preference
    bool gpuRenderingEnabled = state.isGpuRenderingEnabled();
    if (auto* optionsSection = sidebar_->getOptionsSection())
    {
        optionsSection->setGpuRenderingEnabled(gpuRenderingEnabled);

        // Apply saved capture quality settings
        auto qualityConfig = state.getCaptureQualityConfig();
        optionsSection->setQualityPreset(qualityConfig.qualityPreset);
        optionsSection->setBufferDuration(qualityConfig.bufferDuration);
        optionsSection->setAutoAdjustQuality(qualityConfig.autoAdjustQuality);
    }

    // Set size AFTER all components are created to avoid null pointer crash in resized()
    setResizable(true, true);
    setResizeLimits(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT,
                    WindowLayout::MAX_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_HEIGHT);
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    // Start timer for UI updates (60fps target)
    startTimerHz(TIMER_REFRESH_RATE_HZ);

    // Start test server for automated UI testing (standalone only)
    // Use runtime check because compile-time JUCE_STANDALONE_APPLICATION is 1
    // for shared code library even when linked into VST3/AU targets
    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
    {
        testServer_ = std::make_unique<PluginTestServer>(*this);
        testServer_->start(TEST_SERVER_PORT);
        DBG("Test server started on port " << TEST_SERVER_PORT);
    }

    // Initialize GPU Rendering
    renderCoordinator_->setGpuRenderingEnabled(gpuRenderingEnabled);

    // CRITICAL: Propagate GPU rendering state to all waveform components
    // This must happen AFTER OpenGL context setup and AFTER panes are built
    // Otherwise WaveformComponents won't know they should render via GPU
    renderCoordinator_->propagateGpuStateToPanes(paneComponents_);
}

OscilPluginEditor::~OscilPluginEditor()
{
    // Stop timer FIRST to prevent repaint callbacks during destruction
    // This must happen before OpenGL detach to avoid render-while-detaching
    stopTimer();

    // Detach OpenGL context after timer is stopped
    if (renderCoordinator_)
        renderCoordinator_->detach();

    // Stop test server (if it was started)
    if (testServer_)
        testServer_->stop();

    // Remove sidebar listener
    if (sidebar_ && sidebarAdapter_)
        sidebar_->removeListener(sidebarAdapter_.get());

    // Remove config popup listener (via manager)
    if (dialogManager_ && configPopupAdapter_)
        dialogManager_->removeConfigPopupListener(configPopupAdapter_.get());

    // Remove timing engine listener
    if (timingEngineAdapter_)
        processor_.getTimingEngine().removeListener(timingEngineAdapter_.get());

    // Remove state listener
    processor_.getState().removeListener(this);

    // Coordinators handle their own listener cleanup in their destructors
    // They are destroyed in reverse order of construction
}

void OscilPluginEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();

    // Detach OpenGL context early when component is being removed from hierarchy
    if (getParentComponent() == nullptr && renderCoordinator_)
    {
        stopTimer(); // Stop timer first to prevent render callbacks
        renderCoordinator_->detach();
        DBG("OpenGL context detached early (parentHierarchyChanged)");
    }
}

void OscilPluginEditor::paint(juce::Graphics& g)
{
    // FIX: In GPU mode, the OpenGL renderer clears the background.
    // Only fill the background in software rendering mode to avoid
    // overwriting the GPU-rendered waveforms.
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
        editorLayout_->updateLayout(paneComponents_);
    }
}

void OscilPluginEditor::timerCallback()
{
    // Dispatch any pending timing engine updates (BPM changes, sync state, etc.)
    // This replaces the previous async callback mechanism for real-time safety
    processor_.getTimingEngine().dispatchPendingUpdates();

    if (metricsController_)
        metricsController_->update();

    if (renderCoordinator_)
        renderCoordinator_->updateRendering(paneComponents_);
}

#include <iostream>

void OscilPluginEditor::refreshOscillatorPanels()
{
    // RAII guard to ensure timer restarts even if an exception is thrown
    // This prevents UI freeze if component creation fails
    struct TimerGuard
    {
        juce::Timer& timer;
        explicit TimerGuard(juce::Timer& t)
            : timer(t) { timer.stopTimer(); }
        ~TimerGuard() { timer.startTimerHz(TIMER_REFRESH_RATE_HZ); }
    } timerGuard(*this);

    // FIX: Clear all waveforms from GL renderer BEFORE clearing pane components.
    // This prevents stale waveform entries (with old colors) from persisting
    // and being rendered alongside new waveforms when panels are recreated.
    if (renderCoordinator_)
    {
        renderCoordinator_->clearWaveforms();
    }

    // Clear existing panes
    paneComponents_.clear();

    // Get oscillators from state
    auto oscillators = processor_.getState().getOscillators();
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Create pane components
    createPaneComponents(oscillators, layoutManager);

    // Refresh sidebar
    if (sidebar_)
    {
        // Refresh source list from registry
        auto sources = processor_.getInstanceRegistry().getAllSources();
        sidebar_->refreshSourceList(sources);

        // Refresh pane list for source dropdowns (deferred to avoid modifying
        // dropdowns during their callback execution - prevents reentrancy crash)
        auto panes = layoutManager.getPanes();
        juce::MessageManager::callAsync([this, panes]() {
            if (sidebar_)
                sidebar_->refreshPaneList(panes);
        });

        // Sort oscillators by orderIndex for display
        std::sort(oscillators.begin(), oscillators.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });

        // Refresh oscillator list
        sidebar_->refreshOscillatorList(oscillators);
    }

    if (editorLayout_)
        editorLayout_->updateLayout(paneComponents_);

    // Re-apply settings
    reapplyGlobalSettings();

    // Re-propagate GPU rendering state to newly created pane components
    // This is critical for oscillators added after initial setup - without this,
    // new WaveformComponents won't have gpuRenderingEnabled_ set and won't render
    if (renderCoordinator_)
        renderCoordinator_->propagateGpuStateToPanes(paneComponents_);

    // Timer restart is handled by RAII TimerGuard destructor
}

void OscilPluginEditor::createPaneComponents(const std::vector<Oscillator>& oscillators, const PaneLayoutManager& layoutManager)
{
    // Group oscillators by pane
    std::map<juce::String, std::vector<Oscillator>> oscillatorsByPane;
    for (const auto& osc : oscillators)
    {
        oscillatorsByPane[osc.getPaneId().id].push_back(osc);
    }

    int paneIndex = 0;
    for (const auto& pane : layoutManager.getPanes())
    {
        auto paneComponent = std::make_unique<PaneComponent>(processor_, serviceContext_, pane.getId());
        paneComponent->setPaneIndex(paneIndex++);

        // Set up drag-and-drop reorder callback
        paneComponent->onPaneReordered([this](const PaneId& movedPaneId, const PaneId& targetPaneId) {
            handlePaneReordered(movedPaneId, targetPaneId);
        });

        // Set up pane close callback
        paneComponent->onPaneCloseRequested([this](const PaneId& paneIdToClose) {
            handlePaneClose(paneIdToClose);
        });

        // Add oscillators to pane
        auto it = oscillatorsByPane.find(pane.getId().id);
        if (it != oscillatorsByPane.end())
        {
            for (const auto& osc : it->second)
            {
                paneComponent->addOscillator(osc);
            }
        }

        // CRITICAL: Set GPU rendering state on all waveforms IMMEDIATELY after creation
        // This must happen before addAndMakeVisible triggers any paint calls
        bool gpuEnabled = renderCoordinator_ && renderCoordinator_->isGpuRenderingEnabled();
        for (size_t i = 0; i < paneComponent->getOscillatorCount(); ++i)
        {
            if (auto* waveform = paneComponent->getWaveformAt(i))
            {
                waveform->setGpuRenderingEnabled(gpuEnabled);
            }
        }

        contentComponent_->addAndMakeVisible(*paneComponent);
        paneComponents_.push_back(std::move(paneComponent));
    }
}

void OscilPluginEditor::reapplyGlobalSettings()
{
    // Re-apply timing settings to newly created pane components
    // This ensures displaySamples is preserved when panes are recreated
    // Use actualIntervalMs which is computed correctly for both TIME and MELODIC modes
    auto timingConfig = processor_.getTimingEngine().toEntityConfig();
    double sampleRate = processor_.getSampleRate();
    if (sampleRate > 0)
    {
        int displaySamples = static_cast<int>(sampleRate * (static_cast<double>(timingConfig.actualIntervalMs) / 1000.0));
        setDisplaySamplesForAllPanes(displaySamples);
    }

    // Re-apply display options to newly created pane components
    auto& state = processor_.getState();
    setShowGridForAllPanes(state.isShowGridEnabled());
    setAutoScaleForAllPanes(state.isAutoScaleEnabled());
    setHoldDisplayForAllPanes(state.isHoldDisplayEnabled());
    setGainDbForAllPanes(state.getGainDb());
}

void OscilPluginEditor::createDefaultOscillator()
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();
    const auto& availableSources = sourceCoordinator_->getAvailableSources();

    // Create pane if needed
    if (layoutManager.getPaneCount() == 0)
    {
        Pane defaultPane;
        defaultPane.setName("Pane 1");
        defaultPane.setOrderIndex(0);
        layoutManager.addPane(defaultPane);
    }

    // Get the first pane
    PaneId targetPaneId;
    if (layoutManager.getPaneCount() > 0)
    {
        targetPaneId = layoutManager.getPanes()[0].getId();
    }

    // Create oscillator - always use processor's source ID first (even if not yet registered)
    // The PaneComponent will fall back to processor's capture buffer anyway
    Oscillator osc;

    // Try to get a valid source ID
    if (processor_.getSourceId().isValid())
    {
        osc.setSourceId(processor_.getSourceId());
    }
    else if (!availableSources.empty())
    {
        osc.setSourceId(availableSources[0]);
    }
    // No source available yet - that's OK, PaneComponent will use processor's buffer

    osc.setPaneId(targetPaneId);
    osc.setProcessingMode(ProcessingMode::FullStereo);
    osc.setColour(themeCoordinator_->getCurrentTheme().getWaveformColor(
        static_cast<int>(state.getOscillators().size())));
    osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));

    state.addOscillator(osc);

    // No need to call refreshOscillatorPanels() directly as state listener will trigger it
}

void OscilPluginEditor::handlePaneReordered(const PaneId& movedPaneId, const PaneId& targetPaneId)
{
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Get source and target panes to check columns
    const Pane* sourcePanePtr = layoutManager.getPane(movedPaneId);
    const Pane* targetPanePtr = layoutManager.getPane(targetPaneId);

    if (!sourcePanePtr || !targetPanePtr)
        return;

    int sourceColumn = sourcePanePtr->getColumnIndex();
    int targetColumn = targetPanePtr->getColumnIndex();

    if (sourceColumn != targetColumn)
    {
        // Cross-column move - use movePaneToColumn
        // Calculate position within target column based on target pane's position
        auto targetColumnPanes = layoutManager.getPanesInColumn(targetColumn);
        int positionInColumn = 0;
        for (size_t i = 0; i < targetColumnPanes.size(); ++i)
        {
            if (targetColumnPanes[i]->getId().id == targetPaneId.id)
            {
                positionInColumn = static_cast<int>(i);
                break;
            }
        }

        layoutManager.movePaneToColumn(movedPaneId, targetColumn, positionInColumn);
    }
    else
    {
        // Same column - use existing movePane logic for reordering
        int targetIndex = -1;
        const auto& panes = layoutManager.getPanes();
        for (size_t i = 0; i < panes.size(); ++i)
        {
            if (panes[i].getId().id == targetPaneId.id)
            {
                targetIndex = static_cast<int>(i);
                break;
            }
        }

        if (targetIndex >= 0)
        {
            layoutManager.movePane(movedPaneId, targetIndex);
        }
    }

    // Refresh the UI to reflect the new order
    refreshOscillatorPanels();
}

void OscilPluginEditor::handleEmptyColumnDrop(const PaneId& movedPaneId, int targetColumn)
{
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Move pane to the empty column (position 0 since column is empty)
    layoutManager.movePaneToColumn(movedPaneId, targetColumn, 0);

    // Refresh the UI to reflect the new order
    refreshOscillatorPanels();
}

void OscilPluginEditor::handlePaneClose(const PaneId& paneId)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();

    // Update all oscillators that were using this pane:
    // Set their paneId to invalid and make them invisible
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getPaneId() == paneId)
        {
            osc.setPaneId(PaneId::invalid());
            osc.setVisible(false);
            state.updateOscillator(osc);
        }
    }

    // Remove the pane from the layout manager
    layoutManager.removePane(paneId);

    // Refresh the UI
    refreshOscillatorPanels();
    if (sidebar_)
    {
        sidebar_->refreshOscillatorList(state.getOscillators());
    }
}

// Sidebar control

void OscilPluginEditor::toggleSidebar()
{
    if (sidebar_)
    {
        sidebar_->toggleCollapsed();
    }
}

// Sidebar event handlers (called from adapter)

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
    // Find the oscillator
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            // Update available panes for the selector
            auto& layoutManager = state.getLayoutManager();
            auto panes = layoutManager.getPanes();
            std::vector<std::pair<PaneId, juce::String>> paneList;
            for (const auto& pane : panes)
            {
                paneList.emplace_back(pane.getId(), pane.getName());
            }

            if (dialogManager_)
            {
                // Show the popup via manager
                dialogManager_->showConfigPopup(osc, paneList, nullptr);
            }
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_)
        return;

    // Find the oscillator to get current color
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            // Show color dialog via manager
            dialogManager_->showColorDialog(osc.getColour(), [this, oscillatorId](juce::Colour color) {
                // Update oscillator
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

void OscilPluginEditor::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    // 1. Update the oscillator in OscilState
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            osc.setSourceId(newSourceId);
            state.updateOscillator(osc);

            // 2. Find the PaneComponent that contains this oscillator and update buffer
            for (auto& pane : paneComponents_)
            {
                if (pane && osc.getPaneId() == pane->getPaneId())
                {
                    pane->updateOscillatorSource(oscillatorId, newSourceId);
                    break;
                }
            }

            // 3. Refresh sidebar to reflect updated oscillator
            if (sidebar_)
            {
                sidebar_->refreshOscillatorList(state.getOscillators());
            }

            break;
        }
    }
}

void OscilPluginEditor::onOscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated)
{
    auto& state = processor_.getState();

    // Check if pane changed - need to move oscillator
    auto currentOscillators = state.getOscillators();
    for (const auto& existing : currentOscillators)
    {
        if (existing.getId() == oscillatorId)
        {
            // Check if source changed
            if (existing.getSourceId() != updated.getSourceId())
            {
                updateOscillatorSource(oscillatorId, updated.getSourceId());
            }
            break;
        }
    }

    // Update the oscillator in state
    state.updateOscillator(updated);

    // Refresh UI
    refreshOscillatorPanels();
    if (sidebar_)
    {
        sidebar_->refreshOscillatorList(state.getOscillators());
    }
}

void OscilPluginEditor::onOscillatorDeleteRequested(const OscillatorId& oscillatorId)
{
    auto& state = processor_.getState();
    state.removeOscillator(oscillatorId);

    // Refresh UI
    refreshOscillatorPanels();
    if (sidebar_)
    {
        sidebar_->refreshOscillatorList(state.getOscillators());
    }
}

void OscilPluginEditor::onConfigPopupClosed()
{
    if (dialogManager_)
        dialogManager_->closeConfigPopup();
}

void OscilPluginEditor::onAddOscillatorDialogRequested()
{
    if (!dialogManager_)
        return;

    // Gather sources from registry
    auto sources = processor_.getInstanceRegistry().getAllSources();

    // Gather panes from layout manager
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    // Show dialog via manager
    dialogManager_->showAddOscillatorDialog(sources, panes, [this](const AddOscillatorDialog::Result& result) {
        onAddOscillatorResult(result);
    });
}

void OscilPluginEditor::onAddOscillatorResult(const AddOscillatorDialog::Result& result)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();

    // Determine target pane
    PaneId targetPaneId = result.paneId;

    if (result.createNewPane)
    {
        // Create new pane
        Pane newPane;
        newPane.setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
        newPane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
        layoutManager.addPane(newPane);
        targetPaneId = newPane.getId();
    }

    // Create oscillator
    Oscillator osc;
    osc.setSourceId(result.sourceId);
    osc.setPaneId(targetPaneId);
    osc.setProcessingMode(ProcessingMode::FullStereo);
    osc.setColour(result.color);
    osc.setVisualPresetId(result.visualPresetId);

    // Set name (use result name, or generate default)
    if (result.name.isNotEmpty())
    {
        osc.setName(result.name);
    }
    else
    {
        osc.setName("Oscillator " + juce::String(state.getOscillators().size() + 1));
    }

    state.addOscillator(osc);

    // UI refresh is handled by valueTreeChildAdded listener
}

void OscilPluginEditor::onOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            // Update the oscillator in state - this will trigger valueTreePropertyChanged
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
            // Update the oscillator in state - this will trigger valueTreePropertyChanged
            osc.setVisible(visible);
            state.updateOscillator(osc);
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorPaneSelectionRequested(const OscillatorId& oscillatorId)
{
    if (!dialogManager_)
        return;

    // Store the oscillator ID for when the dialog completes
    pendingVisibilityOscillatorId_ = oscillatorId;

    // Gather panes from layout manager
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    // Show dialog via manager
    dialogManager_->showSelectPaneDialog(panes, [this](const SelectPaneDialog::Result& result) {
        auto& stateRef = processor_.getState();
        auto& layoutMgr = stateRef.getLayoutManager();

        // Determine target pane
        PaneId targetPaneId = result.paneId;

        if (result.createNewPane)
        {
            // Create new pane
            Pane newPane;
            newPane.setName("Pane " + juce::String(layoutMgr.getPaneCount() + 1));
            newPane.setOrderIndex(static_cast<int>(layoutMgr.getPaneCount()));
            layoutMgr.addPane(newPane);
            targetPaneId = newPane.getId();
        }

        // Find and update the oscillator
        auto oscList = stateRef.getOscillators();
        for (auto& osc : oscList)
        {
            if (osc.getId() == pendingVisibilityOscillatorId_)
            {
                // Assign pane and set visible
                osc.setPaneId(targetPaneId);
                osc.setVisible(true);
                stateRef.updateOscillator(osc);
                break;
            }
        }

        // Clear pending oscillator
        pendingVisibilityOscillatorId_ = OscillatorId::invalid();

        // UI refresh handled by state listener
    });
}

// ValueTree::Listener overrides
void OscilPluginEditor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Handle property changes
    if (tree.hasType(StateIds::Oscillator))
    {
        // Optimize: For simple properties, update existing components instead of full rebuild
        if (property == StateIds::Name ||
            property == StateIds::Colour ||
            property == StateIds::ProcessingMode ||
            property == StateIds::Visible ||
            property == StateIds::SourceId) // Source change might be simple enough to handle in-place
        {
            OscillatorId oscId{tree.getProperty(StateIds::Id).toString()};

            // Find the oscillator object to get full state
            auto oscillators = processor_.getState().getOscillators();
            for (const auto& osc : oscillators)
            {
                if (osc.getId() == oscId)
                {
                    // Update Sidebar list item
                    if (sidebar_)
                    {
                        // We can't easily update just one item in the list without exposing more internal logic,
                        // but we can try to implement a specific update method or just refresh the list (cheaper than panes)
                        // sidebar_->updateOscillator(osc); // Not implemented yet
                        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<OscilPluginEditor>(this)]() {
                            if (safeThis != nullptr && safeThis->sidebar_)
                                safeThis->sidebar_->refreshOscillatorList(safeThis->processor_.getState().getOscillators());
                        });
                    }

                    // Update PaneComponent
                    for (auto& pane : paneComponents_)
                    {
                        if (pane && pane->getPaneId() == osc.getPaneId())
                        {
                            // Found the pane containing this oscillator
                            if (property == StateIds::Name)
                            {
                                pane->updateOscillatorName(oscId, osc.getName());
                            }
                            else if (property == StateIds::Colour)
                            {
                                pane->updateOscillatorColor(oscId, osc.getColour());
                            }
                            else if (property == StateIds::ProcessingMode || property == StateIds::Visible)
                            {
                                pane->updateOscillator(oscId, osc.getProcessingMode(), osc.isVisible());
                            }
                            else if (property == StateIds::SourceId)
                            {
                                pane->updateOscillatorSource(oscId, osc.getSourceId());
                            }
                            return; // Done, no full rebuild needed
                        }
                    }

                    // If we didn't find the pane (e.g. it was just assigned), we might need a rebuild
                    // But if it's just property change on existing, we should have found it.
                    // If the PaneId changed, that's a different property key, so it will fall through to full rebuild.
                    break;
                }
            }
        }

        // Fallback for structural changes (PaneId, Order) or if optimization failed
        auto safeThis = juce::Component::SafePointer<OscilPluginEditor>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
                safeThis->refreshOscillatorPanels();
        });
    }
    else if (tree.hasType(StateIds::Pane))
    {
        auto safeThis = juce::Component::SafePointer<OscilPluginEditor>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
                safeThis->refreshOscillatorPanels();
        });
    }
}

void OscilPluginEditor::valueTreeChildAdded(juce::ValueTree& /*parentTree*/, juce::ValueTree& child)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        auto safeThis = juce::Component::SafePointer<OscilPluginEditor>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
                safeThis->refreshOscillatorPanels();
        });
    }
}

void OscilPluginEditor::valueTreeChildRemoved(juce::ValueTree& /*parentTree*/, juce::ValueTree& child, int)
{
    if (child.hasType(StateIds::Oscillator) || child.hasType(StateIds::Pane))
    {
        // If an oscillator was removed, check if it's currently being edited in the popup
        if (child.hasType(StateIds::Oscillator) && dialogManager_ && dialogManager_->isConfigPopupVisibleFor(OscillatorId{child.getProperty(StateIds::Id).toString()}))
        {
            // Close popup immediately to prevent editing a non-existent oscillator
            dialogManager_->closeConfigPopup();
        }

        auto safeThis = juce::Component::SafePointer<OscilPluginEditor>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
                safeThis->refreshOscillatorPanels();
        });
    }
}

void OscilPluginEditor::valueTreeChildOrderChanged(juce::ValueTree& parent, int, int)
{
    if (parent.hasType(StateIds::Oscillators) || parent.hasType(StateIds::Panes))
    {
        auto safeThis = juce::Component::SafePointer<OscilPluginEditor>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (safeThis != nullptr)
                safeThis->refreshOscillatorPanels();
        });
    }
}

void OscilPluginEditor::valueTreeParentChanged(juce::ValueTree&) {}

void OscilPluginEditor::setShowGridForAllPanes(bool enabled)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setShowGridForAll(enabled);
}

void OscilPluginEditor::setGridConfigForAllPanes(const GridConfiguration& config)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setGridConfigForAll(config);
}

void OscilPluginEditor::setAutoScaleForAllPanes(bool enabled)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setAutoScaleForAll(enabled);
}

void OscilPluginEditor::setHoldDisplayForAllPanes(bool enabled)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setHoldDisplayForAll(enabled);
}

void OscilPluginEditor::setGainDbForAllPanes(float dB)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setGainDbForAll(dB);
}

void OscilPluginEditor::setDisplaySamplesForAllPanes(int samples)
{
    if (displaySettingsManager_)
        displaySettingsManager_->setDisplaySamplesForAll(samples);
}

void OscilPluginEditor::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (displaySettingsManager_)
        displaySettingsManager_->highlightOscillator(oscillatorId);
}

void OscilPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    if (renderCoordinator_)
        renderCoordinator_->setGpuRenderingEnabled(enabled);

    // Update all waveform components' GPU mode
    for (auto& pane : paneComponents_)
    {
        for (size_t i = 0; i < pane->getOscillatorCount(); ++i)
        {
            if (auto* waveform = pane->getWaveformAt(i))
            {
                waveform->setGpuRenderingEnabled(enabled);
            }
        }
    }

    // Update sidebar toggle to reflect current state
    if (sidebar_ && sidebar_->getOptionsSection())
    {
        sidebar_->getOptionsSection()->setGpuRenderingEnabled(enabled);
    }

    // Save state
    processor_.getState().setGpuRenderingEnabled(enabled);

    // Force repaint of all panes
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->repaint();
    }
    repaint();
}

void OscilPluginEditor::refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators)
{
    if (sidebar_)
    {
        sidebar_->refreshOscillatorList(oscillators);
    }
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

    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->repaint();
    }
}

void OscilPluginEditor::onLayoutChanged()
{
    resized();
}

} // namespace oscil
