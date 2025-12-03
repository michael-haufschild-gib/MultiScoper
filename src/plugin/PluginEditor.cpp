/*
    Oscil - Plugin Editor Implementation
    Main plugin GUI
*/

#include "plugin/PluginEditor.h"
#include "tools/PluginEditor_Adapters.h"
#include "ui/panels/WaveformComponent.h"
#include "ui/layout/PaneComponent.h"
#include "ui/panels/StatusBarComponent.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/panels/OscillatorConfigPopup.h"
#include "ui/theme/ThemeManager.h"
#include "tools/test_server/PluginTestServer.h"
#include "ui/layout/SourceCoordinator.h"
#include "ui/theme/ThemeCoordinator.h"
#include "ui/layout/LayoutCoordinator.h"
#include "core/InstanceRegistry.h"
#include "ui/theme/WaveformColorPalette.h"
#include "plugin/OpenGLLifecycleManager.h"
#include <cmath>

namespace oscil
{

OscilPluginEditor::OscilPluginEditor(OscilPluginProcessor& p)
    : AudioProcessorEditor(&p)
    , processor_(p)
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

    // Create viewport and content
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>();
    contentComponent_->setPaneDropCallback([this](const PaneId& movedPaneId, const PaneId& targetPaneId)
    {
        handlePaneReordered(movedPaneId, targetPaneId);
    });
    contentComponent_->setEmptyColumnDropCallback([this](const PaneId& movedPaneId, int targetColumn)
    {
        handleEmptyColumnDrop(movedPaneId, targetColumn);
    });
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);

    // Create sidebar
    sidebar_ = std::make_unique<SidebarComponent>(processor_.getInstanceRegistry());
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

    // Create oscillator config popup (modal overlay)
    configPopup_ = std::make_unique<OscillatorConfigPopup>(processor_.getThemeService());
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    configPopup_->addListener(configPopupAdapter_.get());
    configPopup_->setVisible(false);
    addChildComponent(*configPopup_);

    // Create add oscillator dialog (modal overlay)
    addOscillatorDialog_ = std::make_unique<AddOscillatorDialog>();
    addOscillatorDialog_->setVisible(false);
    addChildComponent(*addOscillatorDialog_);

    // Create color selection modal
    colorDialogContent_ = std::make_unique<OscillatorColorDialog>();
    colorModal_ = std::make_unique<OscilModal>("Select Color", "colorDialogModal");
    colorModal_->setContent(colorDialogContent_.get());
    // Modal manages its own visibility and attachment when shown

    // Create status bar
    statusBar_ = std::make_unique<StatusBarComponent>();
    addAndMakeVisible(*statusBar_);

    // Listen to state changes for external updates (API, automation)
    processor_.getState().addListener(this);

    // Source coordinator already initialized available sources in its constructor

    // CRITICAL: Initialize GPU rendering state from saved preference BEFORE building panels
    // This ensures WaveformComponents get correct GPU state when created in refreshOscillatorPanels()
    // Note: This is now handled by local variable 'gpuRenderingEnabled' and passed to glManager later

    // Refresh UI - now WaveformComponents will be created with correct GPU state
    refreshOscillatorPanels();

    // Apply saved display options
    auto& state = processor_.getState();
    setShowGridForAllPanes(state.isShowGridEnabled());
    setAutoScaleForAllPanes(state.isAutoScaleEnabled());
    setHoldDisplayForAllPanes(state.isHoldDisplayEnabled());
    setGainDbForAllPanes(state.getGainDb());

    // Apply saved GPU rendering preference
    bool gpuRenderingEnabled = state.isGpuRenderingEnabled();
    if (auto* optionsSection = sidebar_->getOptionsSection())
    {
        optionsSection->setGpuRenderingEnabled(gpuRenderingEnabled);
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

    // Initialize OpenGL Lifecycle Manager
    glManager_ = std::make_unique<OpenGLLifecycleManager>(*this);
    glManager_->setGpuRenderingEnabled(gpuRenderingEnabled);
    
    // CRITICAL: Propagate GPU rendering state to all waveform components
    // This must happen AFTER OpenGL context setup and AFTER panes are built
    // Otherwise WaveformComponents won't know they should render via GPU
    for (auto& pane : paneComponents_)
    {
        if (pane)
        {
            for (size_t i = 0; i < pane->getOscillatorCount(); ++i)
            {
                if (auto* waveform = pane->getWaveformAt(i))
                {
                    waveform->setGpuRenderingEnabled(gpuRenderingEnabled);
                }
            }
        }
    }

    statusBar_->setRenderingMode(gpuRenderingEnabled ? RenderingMode::OpenGL : RenderingMode::Software);
}

OscilPluginEditor::~OscilPluginEditor()
{
    // Stop timer FIRST to prevent repaint callbacks during destruction
    // This must happen before OpenGL detach to avoid render-while-detaching
    stopTimer();

    // Detach OpenGL context after timer is stopped
    if (glManager_)
        glManager_->detach();

    // Stop test server (if it was started)
    if (testServer_)
        testServer_->stop();

    // Remove sidebar listener
    if (sidebar_ && sidebarAdapter_)
        sidebar_->removeListener(sidebarAdapter_.get());

    // Remove config popup listener
    if (configPopup_ && configPopupAdapter_)
        configPopup_->removeListener(configPopupAdapter_.get());

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
    if (getParentComponent() == nullptr && glManager_)
    {
        stopTimer();  // Stop timer first to prevent render callbacks
        glManager_->detach();
        DBG("OpenGL context detached early (parentHierarchyChanged)");
    }
}

void OscilPluginEditor::paint(juce::Graphics& g)
{
    // FIX: In GPU mode, the OpenGL renderer clears the background.
    // Only fill the background in software rendering mode to avoid
    // overwriting the GPU-rendered waveforms.
    if (!glManager_ || !glManager_->isGpuRenderingEnabled())
    {
        const auto& theme = themeCoordinator_->getCurrentTheme();
        g.fillAll(theme.backgroundPrimary);
    }
}

void OscilPluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Status bar
    auto statusBarBounds = bounds.removeFromBottom(STATUS_BAR_HEIGHT);
    statusBar_->setBounds(statusBarBounds);

    // Sidebar on right
    int sidebarWidth = sidebar_->getEffectiveWidth();
    auto sidebarBounds = bounds.removeFromRight(sidebarWidth);
    sidebar_->setBounds(sidebarBounds);

    // Main content area (left of sidebar)
    viewport_->setBounds(bounds);

    updateLayout();
}

void OscilPluginEditor::timerCallback()
{
    // Dispatch any pending timing engine updates (BPM changes, sync state, etc.)
    // This replaces the previous async callback mechanism for real-time safety
    processor_.getTimingEngine().dispatchPendingUpdates();

    updatePerformanceMetrics();
    updateRendering();
}

void OscilPluginEditor::updatePerformanceMetrics()
{
    // Calculate FPS
    auto currentTime = juce::Time::getMillisecondCounterHiRes();
    frameCount_++;

    if (currentTime - lastFrameTime_ >= 1000.0)
    {
        currentFps_ = static_cast<float>(frameCount_) * 1000.0f / static_cast<float>(currentTime - lastFrameTime_);
        frameCount_ = 0;
        lastFrameTime_ = currentTime;

        // Update status bar with all metrics
        if (statusBar_)
        {
            statusBar_->setFps(currentFps_);

            // CPU usage from processor (approximation based on audio callback time)
            statusBar_->setCpuUsage(processor_.getCpuUsage());

            // Memory usage - estimate based on capture buffer and oscillators
            // Each capture buffer ~0.5MB, plus UI overhead
            size_t sourceCount = processor_.getInstanceRegistry().getSourceCount();
            size_t oscillatorCount = processor_.getState().getOscillators().size();
            float memoryMB = 0.5f * static_cast<float>(sourceCount)
                           + 0.1f * static_cast<float>(oscillatorCount)
                           + 5.0f; // Base UI overhead
            statusBar_->setMemoryUsage(memoryMB);

            // Oscillator and source counts
            statusBar_->setOscillatorCount(static_cast<int>(oscillatorCount));
            statusBar_->setSourceCount(static_cast<int>(sourceCount));

            // Trigger repaint to display updated values
            statusBar_->repaint();
        }
    }
}

void OscilPluginEditor::updateRendering()
{
    // Update GL renderer with waveform data (if GPU mode enabled)
    if (glManager_)
    {
        // Note: glManager_->updateWaveformData() triggers processAudioData() internally
        // so we don't need to call it explicitly here.
        glManager_->updateWaveformData(paneComponents_);
    }

    // Trigger repaint of waveform components (only needed for software rendering)
    if (!glManager_ || !glManager_->isGpuRenderingEnabled())
    {
        for (auto& pane : paneComponents_)
        {
            if (pane)
                pane->repaint();
        }
    }
}

// Coordinator callbacks

void OscilPluginEditor::onSourcesChanged()
{
    // Called by SourceCoordinator when sources change
    // The coordinator already manages the availableSources list
    refreshOscillatorPanels();
}

void OscilPluginEditor::onThemeChanged(const ColorTheme& /*newTheme*/)
{
    // Called by ThemeCoordinator when theme changes
    repaint();
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->repaint();
    }
}

void OscilPluginEditor::onLayoutChanged()
{
    // Called by LayoutCoordinator when layout changes
    resized();
}

void OscilPluginEditor::updateLayout()
{
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Use viewport's own bounds - this is the space available for content
    // Note: getViewArea() returns the visible portion of CONTENT, not viewport size
    auto availableArea = viewport_->getLocalBounds();

    // Ensure minimum dimensions to prevent zero-size components
    int availableWidth = std::max(MIN_CONTENT_WIDTH, availableArea.getWidth());
    int availableHeight = std::max(MIN_CONTENT_HEIGHT, availableArea.getHeight());
    availableArea = juce::Rectangle<int>(0, 0, availableWidth, availableHeight);

    // Calculate content size based on number of panes
    int numPanes = static_cast<int>(paneComponents_.size());
    if (numPanes == 0)
    {
        contentComponent_->setSize(availableArea.getWidth(), availableArea.getHeight());
        return;
    }

    int numCols = layoutManager.getColumnCount();
    contentComponent_->setColumnCount(numCols);
    int numRows = (numPanes + numCols - 1) / numCols;
    int paneHeight = std::max(MIN_PANE_HEIGHT, availableArea.getHeight() / std::max(1, numRows));

    contentComponent_->setSize(availableArea.getWidth(), paneHeight * numRows);

    // Position panes
    for (int i = 0; i < numPanes; ++i)
    {
        auto bounds = layoutManager.getPaneBounds(i,
            juce::Rectangle<int>(0, 0, contentComponent_->getWidth(), contentComponent_->getHeight()));
        paneComponents_[static_cast<size_t>(i)]->setBounds(bounds);
    }
}

void OscilPluginEditor::refreshOscillatorPanels()
{
    // RAII guard to ensure timer restarts even if an exception is thrown
    // This prevents UI freeze if component creation fails
    struct TimerGuard
    {
        juce::Timer& timer;
        explicit TimerGuard(juce::Timer& t) : timer(t) { timer.stopTimer(); }
        ~TimerGuard() { timer.startTimerHz(TIMER_REFRESH_RATE_HZ); }
    } timerGuard(*this);

    // FIX: Clear all waveforms from GL renderer BEFORE clearing pane components.
    // This prevents stale waveform entries (with old colors) from persisting
    // and being rendered alongside new waveforms when panels are recreated.
    if (glManager_)
    {
        glManager_->clearAllWaveforms();
    }

    // Clear existing panes
    paneComponents_.clear();

    // Get oscillators from state
    auto oscillators = processor_.getState().getOscillators();
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Create pane components
    createPaneComponents(oscillators, layoutManager);

    // If no panes exist, create a default one with the current source
    const auto& availableSources = sourceCoordinator_->getAvailableSources();
    if (paneComponents_.empty() && !availableSources.empty())
    {
        createDefaultOscillator();
    }

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

    updateLayout();

    // Re-apply settings
    reapplyGlobalSettings();

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
        auto paneComponent = std::make_unique<PaneComponent>(processor_, pane.getId());
        paneComponent->setPaneIndex(paneIndex++);

        // Set up drag-and-drop reorder callback
        paneComponent->onPaneReordered([this](const PaneId& movedPaneId, const PaneId& targetPaneId)
        {
            handlePaneReordered(movedPaneId, targetPaneId);
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
        bool gpuEnabled = glManager_ && glManager_->isGpuRenderingEnabled();
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
            if (configPopup_)
            {
                configPopup_->setAvailablePanes(paneList);

                // Show the popup for this oscillator
                configPopup_->showForOscillator(osc);
                configPopup_->setBounds(getLocalBounds());
                configPopup_->toFront(true);
            }
            break;
        }
    }
}

void OscilPluginEditor::onOscillatorColorConfigRequested(const OscillatorId& oscillatorId)
{
    if (!colorModal_ || !colorDialogContent_)
        return;

    // Find the oscillator to get current color
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    
    for (const auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            // Set up dialog content
            colorDialogContent_->setColors(WaveformColorPalette::getAllColors()); // Use standard palette for swatches
            colorDialogContent_->setSelectedColor(osc.getColour());
            
            // Define callbacks
            colorDialogContent_->setOnColorSelected([this, oscillatorId](juce::Colour color) {
                // Update oscillator
                auto& state = processor_.getState();
                auto oscillators = state.getOscillators();
                for (auto& o : oscillators) {
                    if (o.getId() == oscillatorId) {
                        o.setColour(color);
                        state.updateOscillator(o);
                        break;
                    }
                }
                
                // UI update is handled via state listener (onOscillatorConfigChanged)
                // But we can force specific refresh if needed
                // refreshOscillatorPanels(); // State listener handles this
                
                if (colorModal_)
                    colorModal_->hide();
            });
            
            colorDialogContent_->setOnCancel([this]() {
                if (colorModal_)
                    colorModal_->hide();
            });
            
            // Show modal
            colorModal_->show(this);
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
    if (configPopup_)
        configPopup_->setVisible(false);
}

void OscilPluginEditor::onAddOscillatorDialogRequested()
{
    if (!addOscillatorDialog_)
        return;

    // Gather sources from registry
    auto sources = processor_.getInstanceRegistry().getAllSources();

    // Gather panes from layout manager
    auto& layoutManager = processor_.getState().getLayoutManager();
    auto panes = layoutManager.getPanes();

    // Show dialog with callback
    addOscillatorDialog_->showDialog(this, sources, panes,
        [this](const AddOscillatorDialog::Result& result) {
            onAddOscillatorResult(result);
        });

    // Position dialog to cover entire editor
    addOscillatorDialog_->setBounds(getLocalBounds());
    addOscillatorDialog_->toFront(true);
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

    // Hide dialog
    if (addOscillatorDialog_)
        addOscillatorDialog_->hideDialog();

    // Refresh UI
    refreshOscillatorPanels();
}

void OscilPluginEditor::onOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();

    for (auto& osc : oscillators)
    {
        if (osc.getId() == oscillatorId)
        {
            // Update the oscillator in state
            osc.setProcessingMode(mode);
            state.updateOscillator(osc);

            // Update the PaneComponent that contains this oscillator
            for (auto& pane : paneComponents_)
            {
                if (pane && pane->getPaneId() == osc.getPaneId())
                {
                    pane->updateOscillator(oscillatorId, mode, osc.isVisible());
                    break;
                }
            }

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
            // Update the oscillator in state
            osc.setVisible(visible);
            state.updateOscillator(osc);

            // Update the PaneComponent that contains this oscillator
            for (auto& pane : paneComponents_)
            {
                if (pane && pane->getPaneId() == osc.getPaneId())
                {
                    pane->updateOscillator(oscillatorId, osc.getProcessingMode(), visible);
                    break;
                }
            }

            // Refresh sidebar to update oscillator list (counts may have changed)
            if (sidebar_)
            {
                sidebar_->refreshOscillatorList(state.getOscillators());
            }

            break;
        }
    }
}

// ValueTree::Listener overrides
void OscilPluginEditor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    // Handle property changes that require full UI refresh
    if (tree.hasType(StateIds::Oscillator) || tree.hasType(StateIds::Pane))
    {
        // Use SafePointer to prevent dangling pointer if editor is destroyed before async callback
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
        if (child.hasType(StateIds::Oscillator) && configPopup_ && configPopup_->isPopupVisible())
        {
            OscillatorId removedId{ child.getProperty(StateIds::Id).toString() };
            if (configPopup_->getOscillatorId() == removedId)
            {
                // Close popup immediately to prevent editing a non-existent oscillator
                configPopup_->setVisible(false);
            }
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
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setShowGrid(enabled);
    }
}

void OscilPluginEditor::setGridConfigForAllPanes(const GridConfiguration& config)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setGridConfig(config);
    }
}

void OscilPluginEditor::setAutoScaleForAllPanes(bool enabled)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setAutoScale(enabled);
    }
}

void OscilPluginEditor::setHoldDisplayForAllPanes(bool enabled)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setHoldDisplay(enabled);
    }
}

void OscilPluginEditor::setGainDbForAllPanes(float dB)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setGainDb(dB);
    }
}

void OscilPluginEditor::setDisplaySamplesForAllPanes(int samples)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setDisplaySamples(samples);
    }
}

void OscilPluginEditor::highlightOscillator(const OscillatorId& oscillatorId)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->highlightOscillator(oscillatorId);
    }
}

void OscilPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    if (glManager_)
        glManager_->setGpuRenderingEnabled(enabled);

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

    // Update status bar
    if (statusBar_)
    {
        statusBar_->setRenderingMode(enabled ? RenderingMode::OpenGL : RenderingMode::Software);
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

} // namespace oscil
