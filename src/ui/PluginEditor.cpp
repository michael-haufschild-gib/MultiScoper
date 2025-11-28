/*
    Oscil - Plugin Editor Implementation
    Main plugin GUI
*/

#include "ui/PluginEditor.h"
#include "ui/WaveformComponent.h"
#include "ui/PaneComponent.h"
#include "ui/StatusBarComponent.h"
#include "ui/SidebarComponent.h"
#include "ui/OscillatorConfigPopup.h"
#include "ui/ThemeManager.h"
#include "ui/PluginTestServer.h"
#include "ui/coordinators/SourceCoordinator.h"
#include "ui/coordinators/ThemeCoordinator.h"
#include "ui/coordinators/LayoutCoordinator.h"
#include "core/InstanceRegistry.h"
#include <cmath>

namespace oscil
{

/**
 * Custom content component that acts as a DragAndDropTarget proxy.
 * When drops occur, it forwards them to the appropriate child PaneComponent.
 */
class PaneContainerComponent : public juce::Component, public juce::DragAndDropTarget
{
public:
    using PaneDropCallback = std::function<void(const PaneId& movedPaneId, const PaneId& targetPaneId)>;

    PaneContainerComponent() = default;

    void setPaneDropCallback(PaneDropCallback callback)
    {
        paneDropCallback_ = std::move(callback);
    }

    // DragAndDropTarget interface
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
    {
        // Accept drags from PaneComponents
        if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
        {
            juce::ignoreUnused(sourcePane);
            return true;
        }
        return false;
    }

    void itemDragEnter(const SourceDetails& dragSourceDetails) override
    {
        updateDropTarget(dragSourceDetails);
    }

    void itemDragMove(const SourceDetails& dragSourceDetails) override
    {
        updateDropTarget(dragSourceDetails);
    }

    void itemDragExit(const SourceDetails& /*dragSourceDetails*/) override
    {
        clearDropHighlight();
    }

    void itemDropped(const SourceDetails& dragSourceDetails) override
    {
        clearDropHighlight();

        if (!dragSourceDetails.description.isString())
            return;

        PaneId movedPaneId;
        movedPaneId.id = dragSourceDetails.description.toString();

        // Find which PaneComponent we dropped on
        PaneComponent* targetPane = findPaneAt(dragSourceDetails.localPosition);
        if (targetPane && targetPane->getPaneId().id != movedPaneId.id)
        {
            if (paneDropCallback_)
            {
                paneDropCallback_(movedPaneId, targetPane->getPaneId());
            }
        }
    }

private:
    PaneComponent* findPaneAt(juce::Point<int> position)
    {
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (auto* pane = dynamic_cast<PaneComponent*>(getChildComponent(i)))
            {
                if (pane->getBounds().contains(position))
                {
                    return pane;
                }
            }
        }
        return nullptr;
    }

    void updateDropTarget(const SourceDetails& dragSourceDetails)
    {
        // Find target pane and highlight it
        if (auto* targetPane = findPaneAt(dragSourceDetails.localPosition))
        {
            if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
            {
                if (targetPane->getPaneId().id != sourcePane->getPaneId().id)
                {
                    // Would trigger itemDragEnter on the target pane
                    // For now just let the visual feedback come from the target itself
                }
            }
        }
    }

    void clearDropHighlight()
    {
        // Clear any drop highlighting
    }

    PaneDropCallback paneDropCallback_;
};

/**
 * Adapter class to forward SidebarComponent::Listener events to OscilPluginEditor
 */
class SidebarListenerAdapter : public SidebarComponent::Listener
{
public:
    explicit SidebarListenerAdapter(OscilPluginEditor& editor) : editor_(editor) {}

    void sidebarWidthChanged(int newWidth) override
    {
        editor_.onSidebarWidthChanged(newWidth);
    }

    void sidebarCollapsedStateChanged(bool collapsed) override
    {
        editor_.onSidebarCollapsedStateChanged(collapsed);
    }

    void oscillatorSelected(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorSelected(oscillatorId);
    }

    void oscillatorConfigRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorConfigRequested(oscillatorId);
    }

    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorDeleteRequested(oscillatorId);
    }

    void oscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode) override
    {
        editor_.onOscillatorModeChanged(oscillatorId, mode);
    }

    void oscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible) override
    {
        editor_.onOscillatorVisibilityChanged(oscillatorId, visible);
    }

    void addSourceToPane(const SourceId& sourceId, const PaneId& paneId) override
    {
        editor_.onAddSourceToPane(sourceId, paneId);
    }

    // Timing section events - forward to processor's TimingEngine
    void timingModeChanged(TimingMode mode) override
    {
        editor_.getProcessor().getTimingEngine().setTimingMode(mode);
    }

    void noteIntervalChanged(NoteInterval interval) override
    {
        editor_.getProcessor().getTimingEngine().setNoteIntervalFromEntity(interval);
    }

    void timeIntervalChanged(int ms) override
    {
        editor_.getProcessor().getTimingEngine().setTimeIntervalMs(ms);
    }

    void hostSyncChanged(bool enabled) override
    {
        editor_.getProcessor().getTimingEngine().setHostSyncEnabled(enabled);
    }

    void resetOnPlayChanged(bool enabled) override
    {
        editor_.getProcessor().getTimingEngine().setSyncToPlayhead(enabled);
    }

    // Master controls events
    void timebaseChanged(float ms) override
    {
        // Timebase maps to time interval in milliseconds
        editor_.getProcessor().getTimingEngine().setTimeIntervalMs(static_cast<int>(ms));
    }

    void gainChanged(float dB) override
    {
        editor_.setGainDbForAllPanes(dB);
        editor_.getProcessor().getState().setGainDb(dB);
    }

    // Trigger settings events
    void triggerSourceChanged(const juce::String& /*sourceName*/) override
    {
        // Trigger source is informational - the triggering happens based on the audio buffer
    }

    void triggerModeChanged(TriggerMode mode) override
    {
        auto& engine = editor_.getProcessor().getTimingEngine();
        // Map TriggerMode to WaveformTriggerMode
        switch (mode)
        {
            case TriggerMode::FREE_RUNNING:
                engine.setWaveformTriggerMode(WaveformTriggerMode::None);
                break;
            case TriggerMode::HOST_SYNC:
                // Host sync uses timing engine's host sync, not waveform triggering
                engine.setWaveformTriggerMode(WaveformTriggerMode::None);
                engine.setHostSyncEnabled(true);
                break;
            case TriggerMode::TRIGGERED:
                // Will be set based on triggerEdgeChanged
                engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
                break;
        }
    }

    void triggerThresholdChanged(float dBFS) override
    {
        // Convert dBFS to linear (0.0 - 1.0)
        float linear = std::pow(10.0f, dBFS / 20.0f);
        editor_.getProcessor().getTimingEngine().setTriggerThreshold(linear);
    }

    void triggerEdgeChanged(TriggerEdge edge) override
    {
        auto& engine = editor_.getProcessor().getTimingEngine();
        switch (edge)
        {
            case TriggerEdge::Rising:
                engine.setWaveformTriggerMode(WaveformTriggerMode::RisingEdge);
                break;
            case TriggerEdge::Falling:
                engine.setWaveformTriggerMode(WaveformTriggerMode::FallingEdge);
                break;
            case TriggerEdge::Both:
                engine.setWaveformTriggerMode(WaveformTriggerMode::Level);
                break;
        }
    }

    // Display options events
    void showGridChanged(bool enabled) override
    {
        editor_.setShowGridForAllPanes(enabled);
        editor_.getProcessor().getState().setShowGridEnabled(enabled);
    }

    void autoScaleChanged(bool enabled) override
    {
        editor_.setAutoScaleForAllPanes(enabled);
        editor_.getProcessor().getState().setAutoScaleEnabled(enabled);
    }

    void holdDisplayChanged(bool enabled) override
    {
        editor_.setHoldDisplayForAllPanes(enabled);
        editor_.getProcessor().getState().setHoldDisplayEnabled(enabled);
    }

    void oscillatorsReordered(int fromIndex, int toIndex) override
    {
        // Reorder oscillators in state
        editor_.getProcessor().getState().reorderOscillators(fromIndex, toIndex);

        // Refresh the UI to show the new order
        auto oscillators = editor_.getProcessor().getState().getOscillators();

        // Sort by orderIndex to get the display order
        std::sort(oscillators.begin(), oscillators.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });

        // Refresh the sidebar with updated oscillator list
        editor_.refreshPanels();
    }

private:
    OscilPluginEditor& editor_;
};

/**
 * Adapter class to forward OscillatorConfigPopup::Listener events to OscilPluginEditor
 */
class ConfigPopupListenerAdapter : public OscillatorConfigPopup::Listener
{
public:
    explicit ConfigPopupListenerAdapter(OscilPluginEditor& editor) : editor_(editor) {}

    void oscillatorConfigChanged(const OscillatorId& oscillatorId, const Oscillator& updated) override
    {
        editor_.onOscillatorConfigChanged(oscillatorId, updated);
    }

    void oscillatorDeleteRequested(const OscillatorId& oscillatorId) override
    {
        editor_.onOscillatorDeleteRequested(oscillatorId);
    }

    void configPopupClosed() override
    {
        editor_.onConfigPopupClosed();
    }

private:
    OscilPluginEditor& editor_;
};

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

    // Create toolbar components
    columnSelector_ = std::make_unique<juce::ComboBox>("columns");
    columnSelector_->addItem("1 Column", 1);
    columnSelector_->addItem("2 Columns", 2);
    columnSelector_->addItem("3 Columns", 3);
    columnSelector_->setSelectedId(static_cast<int>(processor_.getState().getColumnLayout()));
    columnSelector_->onChange = [this]()
    {
        auto layout = static_cast<ColumnLayout>(columnSelector_->getSelectedId());
        processor_.getState().setColumnLayout(layout);
        refreshOscillatorPanels();  // Full refresh needed to properly apply column layout
    };
    addAndMakeVisible(*columnSelector_);

    addOscillatorButton_ = std::make_unique<juce::TextButton>("+ Add");
    addOscillatorButton_->onClick = [this]() { createDefaultOscillator(); };
    addAndMakeVisible(*addOscillatorButton_);

    themeSelector_ = std::make_unique<juce::ComboBox>("theme");
    auto themes = processor_.getThemeService().getAvailableThemes();
    for (size_t i = 0; i < themes.size(); ++i)
    {
        themeSelector_->addItem(themes[i], static_cast<int>(i) + 1);
    }
    // Load saved theme name and APPLY it (not just display)
    auto savedThemeName = processor_.getState().getThemeName();
    themeSelector_->setText(savedThemeName);
    processor_.getThemeService().setCurrentTheme(savedThemeName);

    themeSelector_->onChange = [this]()
    {
        auto themeName = themeSelector_->getText();
        processor_.getThemeService().setCurrentTheme(themeName);
        processor_.getState().setThemeName(themeName);
    };
    addAndMakeVisible(*themeSelector_);

    // Sidebar toggle button
    sidebarToggleButton_ = std::make_unique<juce::TextButton>("Sidebar");
    sidebarToggleButton_->onClick = [this]() { toggleSidebar(); };
    addAndMakeVisible(*sidebarToggleButton_);

    // Create viewport and content
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>();
    contentComponent_->setPaneDropCallback([this](const PaneId& movedPaneId, const PaneId& targetPaneId)
    {
        handlePaneReordered(movedPaneId, targetPaneId);
    });
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);

    // Create sidebar
    sidebar_ = std::make_unique<SidebarComponent>(processor_);
    sidebarAdapter_ = std::make_unique<SidebarListenerAdapter>(*this);
    sidebar_->addListener(sidebarAdapter_.get());
    addAndMakeVisible(*sidebar_);

    // Create oscillator config popup (modal overlay)
    configPopup_ = std::make_unique<OscillatorConfigPopup>();
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    configPopup_->addListener(configPopupAdapter_.get());
    configPopup_->setVisible(false);
    addChildComponent(*configPopup_);

    // Create status bar
    statusBar_ = std::make_unique<StatusBarComponent>();
    addAndMakeVisible(*statusBar_);

    // Source coordinator already initialized available sources in its constructor

    // Refresh UI
    refreshOscillatorPanels();

    // Apply saved display options
    auto& state = processor_.getState();
    setShowGridForAllPanes(state.isShowGridEnabled());
    setAutoScaleForAllPanes(state.isAutoScaleEnabled());
    setHoldDisplayForAllPanes(state.isHoldDisplayEnabled());
    setGainDbForAllPanes(state.getGainDb());

    // Set size AFTER all components are created to avoid null pointer crash in resized()
    setResizable(true, true);
    setResizeLimits(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT,
                    WindowLayout::MAX_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_HEIGHT);
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    // Start timer for UI updates (60fps target)
    startTimerHz(60);

    // Start test server for automated UI testing (standalone only)
    #if JUCE_STANDALONE_APPLICATION
    testServer_ = std::make_unique<PluginTestServer>(*this);
    testServer_->start(9876);
    DBG("Test server started on port 9876");
    #endif

    // Attach OpenGL context for GPU-accelerated rendering
    #if OSCIL_ENABLE_OPENGL
    openGLContext_.attachTo(*this);
    DBG("OpenGL context attached for GPU rendering");
    #endif
}

OscilPluginEditor::~OscilPluginEditor()
{
    // Stop timer FIRST to prevent repaint callbacks during destruction
    // This must happen before OpenGL detach to avoid render-while-detaching
    stopTimer();

    // Detach OpenGL context after timer is stopped
    #if OSCIL_ENABLE_OPENGL
    openGLContext_.detach();
    #endif

    // Stop test server
    #if JUCE_STANDALONE_APPLICATION
    if (testServer_)
        testServer_->stop();
    #endif

    // Remove sidebar listener
    if (sidebar_ && sidebarAdapter_)
        sidebar_->removeListener(sidebarAdapter_.get());

    // Coordinators handle their own listener cleanup in their destructors
    // They are destroyed in reverse order of construction
}

void OscilPluginEditor::paint(juce::Graphics& g)
{
    const auto& theme = themeCoordinator_->getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    // Draw toolbar background
    g.setColour(theme.backgroundSecondary);
    g.fillRect(0, 0, getWidth(), TOOLBAR_HEIGHT);

    // Draw separator line
    g.setColour(theme.gridMajor);
    g.drawHorizontalLine(TOOLBAR_HEIGHT - 1, 0.0f, static_cast<float>(getWidth()));
}

void OscilPluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Toolbar
    auto toolbarBounds = bounds.removeFromTop(TOOLBAR_HEIGHT);
    const int buttonWidth = 80;
    const int comboWidth = 100;
    const int padding = 10;

    auto toolbarLeft = toolbarBounds.reduced(padding, 5);

    columnSelector_->setBounds(toolbarLeft.removeFromLeft(comboWidth));
    toolbarLeft.removeFromLeft(padding);

    addOscillatorButton_->setBounds(toolbarLeft.removeFromLeft(buttonWidth));
    toolbarLeft.removeFromLeft(padding);

    themeSelector_->setBounds(toolbarLeft.removeFromLeft(comboWidth + 50));
    toolbarLeft.removeFromLeft(padding);

    sidebarToggleButton_->setBounds(toolbarLeft.removeFromLeft(buttonWidth));

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
        }
    }

    // Trigger repaint of waveform components
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->repaint();
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
    int availableWidth = std::max(400, availableArea.getWidth());
    int availableHeight = std::max(300, availableArea.getHeight());
    availableArea = juce::Rectangle<int>(0, 0, availableWidth, availableHeight);

    // Calculate content size based on number of panes
    int numPanes = static_cast<int>(paneComponents_.size());
    if (numPanes == 0)
    {
        contentComponent_->setSize(availableArea.getWidth(), availableArea.getHeight());
        return;
    }

    int numCols = layoutManager.getColumnCount();
    int numRows = (numPanes + numCols - 1) / numCols;
    int paneHeight = std::max(200, availableArea.getHeight() / std::max(1, numRows));

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
        ~TimerGuard() { timer.startTimerHz(60); }
    } timerGuard(*this);

    // Clear existing panes
    paneComponents_.clear();

    // Get oscillators from state
    auto oscillators = processor_.getState().getOscillators();
    auto& layoutManager = processor_.getState().getLayoutManager();

    // Group oscillators by pane
    std::map<juce::String, std::vector<Oscillator>> oscillatorsByPane;
    for (const auto& osc : oscillators)
    {
        oscillatorsByPane[osc.getPaneId().id].push_back(osc);
    }

    // Create pane components
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

        contentComponent_->addAndMakeVisible(*paneComponent);
        paneComponents_.push_back(std::move(paneComponent));
    }

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

        // Refresh pane list for source dropdowns
        sidebar_->refreshPaneList(layoutManager.getPanes());

        // Sort oscillators by orderIndex for display
        std::sort(oscillators.begin(), oscillators.end(),
                  [](const Oscillator& a, const Oscillator& b) {
                      return a.getOrderIndex() < b.getOrderIndex();
                  });

        // Refresh oscillator list
        sidebar_->refreshOscillatorList(oscillators);
    }

    updateLayout();
    // Timer restart is handled by RAII TimerGuard destructor
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

    refreshOscillatorPanels();
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

void OscilPluginEditor::onAddSourceToPane(const SourceId& sourceId, const PaneId& paneId)
{
    auto& state = processor_.getState();
    auto& layoutManager = state.getLayoutManager();

    // Determine target pane - create new if paneId is empty
    PaneId targetPaneId = paneId;
    if (!targetPaneId.isValid())
    {
        // Create a new pane
        Pane newPane;
        newPane.setName("Pane " + juce::String(layoutManager.getPaneCount() + 1));
        newPane.setOrderIndex(static_cast<int>(layoutManager.getPaneCount()));
        layoutManager.addPane(newPane);
        targetPaneId = newPane.getId();
    }

    // Check if an oscillator already exists for this source
    auto oscillators = state.getOscillators();
    for (auto& osc : oscillators)
    {
        if (osc.getSourceId() == sourceId)
        {
            // Update existing oscillator's pane assignment
            osc.setPaneId(targetPaneId);
            state.updateOscillator(osc);
            refreshOscillatorPanels();
            return;
        }
    }

    // No existing oscillator for this source - create a new one
    Oscillator osc;
    osc.setSourceId(sourceId);
    osc.setPaneId(targetPaneId);
    osc.setProcessingMode(ProcessingMode::FullStereo);
    osc.setColour(themeCoordinator_->getCurrentTheme().getWaveformColor(
        static_cast<int>(state.getOscillators().size())));

    // Get source name for oscillator name
    juce::String oscName = "Oscillator " + juce::String(state.getOscillators().size() + 1);
    auto sourceInfoOpt = processor_.getInstanceRegistry().getSource(sourceId);
    if (sourceInfoOpt.has_value() && sourceInfoOpt->name.isNotEmpty())
    {
        oscName = sourceInfoOpt->name;
    }
    osc.setName(oscName);

    state.addOscillator(osc);

    // Refresh the UI
    refreshOscillatorPanels();
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

void OscilPluginEditor::setShowGridForAllPanes(bool enabled)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->setShowGrid(enabled);
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

void OscilPluginEditor::highlightOscillator(const OscillatorId& oscillatorId)
{
    for (auto& pane : paneComponents_)
    {
        if (pane)
            pane->highlightOscillator(oscillatorId);
    }
}

} // namespace oscil
