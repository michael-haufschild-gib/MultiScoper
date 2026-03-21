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
#include <limits>

namespace
{
int64_t convertTimelineTimestampToCaptureDomain(int64_t timestamp, int sourceRate, int captureRate)
{
    if (timestamp <= 0)
        return 0;

    if (sourceRate <= 0 || captureRate <= 0 || sourceRate == captureRate)
        return timestamp;

    const long double scaled = (static_cast<long double>(timestamp) * static_cast<long double>(captureRate))
                             / static_cast<long double>(sourceRate);
    if (!std::isfinite(static_cast<double>(scaled)))
        return timestamp;
    if (scaled <= 0.0L)
        return 0;

    constexpr long double maxValue = static_cast<long double>(std::numeric_limits<int64_t>::max());
    if (scaled >= maxValue)
        return std::numeric_limits<int64_t>::max();

    return static_cast<int64_t>(std::llround(scaled));
}
} // namespace

namespace oscil
{

OscilPluginEditor::OscilPluginEditor(OscilPluginProcessor& p)
    : AudioProcessorEditor(&p)
    , processor_(p)
    , serviceContext_{ processor_.getInstanceRegistry(), processor_.getThemeService(), processor_.getShaderRegistry(), processor_.getPresetManager() }
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
    addAndMakeVisible(*sidebar_);

    statusBar_ = std::make_unique<StatusBarComponent>(processor_.getThemeService());
    addAndMakeVisible(*statusBar_);

    // Create managers and coordinators
    dialogManager_ = std::make_unique<DialogManager>(*this, processor_.getThemeService(), processor_.getInstanceRegistry());
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    dialogManager_->addConfigPopupListener(configPopupAdapter_.get());

    metricsController_ = std::make_unique<PerformanceMetricsController>(processor_, processor_.getInstanceRegistry(), *statusBar_);
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
    sidebar_->addListener(this);

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
    auto engineTimingConfig = processor_.getTimingEngine().getConfig();
    if (auto* timingSection = sidebar_->getTimingSection())
    {
        WaveformMode waveformMode = WaveformMode::FreeRunning;
        if (engineTimingConfig.triggerMode == WaveformTriggerMode::Midi)
            waveformMode = WaveformMode::RestartOnNote;
        else if (engineTimingConfig.syncToPlayhead)
            waveformMode = WaveformMode::RestartOnPlay;

        timingSection->setTimingMode(timingConfig.timingMode);
        timingSection->setTimeIntervalMs(static_cast<int>(timingConfig.timeIntervalMs));
        timingSection->setNoteInterval(timingConfig.noteInterval);
        timingSection->setHostSyncEnabled(timingConfig.hostSyncEnabled);
        timingSection->setHostBPM(timingConfig.hostBPM);
        timingSection->setWaveformMode(waveformMode);
    }

    // Create default oscillator if needed
    oscillatorPanelController_->createDefaultOscillatorIfNeeded();

    // Perform initial source reconciliation now that UI/controller are ready.
    // This catches stale source IDs and legacy uninitialized IDs on editor open.
    onSourcesChanged();

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

    if (sidebar_ && oscillatorPanelController_)
        sidebar_->removeListener(oscillatorPanelController_.get());

    if (sidebar_)
        sidebar_->removeListener(this);

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

    if (displaySettingsManager_)
    {
        auto& timingEngine = processor_.getTimingEngine();
        const auto timingConfig = timingEngine.getConfig();
        const bool restartModeActive =
            timingConfig.syncToPlayhead ||
            timingConfig.triggerMode == WaveformTriggerMode::Midi;

        if (restartModeActive && timingEngine.checkAndClearTrigger())
        {
            int64_t triggerTimestamp = static_cast<int64_t>(
                std::llround(timingConfig.lastSyncTimestamp));
            if (triggerTimestamp <= 0)
            {
                auto hostTimestamp = timingEngine.getHostInfo().timeInSamples;
                if (hostTimestamp > 0)
                    triggerTimestamp = hostTimestamp;
            }

            const int sourceRate = juce::jmax(1, static_cast<int>(
                std::llround(processor_.getSampleRate())));
            const int captureRate = juce::jmax(1, processor_.getCaptureRate());
            triggerTimestamp = convertTimelineTimestampToCaptureDomain(triggerTimestamp,
                                                                       sourceRate,
                                                                       captureRate);

            displaySettingsManager_->requestWaveformRestartAtTimestampForAll(
                juce::jmax<int64_t>(0, triggerTimestamp));
        }
    }

    if (renderCoordinator_)
        renderCoordinator_->updateRendering(oscillatorPanelController_->getPaneComponents());
}

} // namespace oscil
