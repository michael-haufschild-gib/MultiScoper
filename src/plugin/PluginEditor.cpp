/*
    Oscil - Plugin Editor Implementation
    Main plugin GUI
*/

#include "plugin/PluginEditor.h"

#include "core/InstanceRegistry.h"
#include "ui/components/SurfaceStyle.h"
#include "ui/controllers/GpuRenderCoordinator.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/layout/LayoutCoordinator.h"
#include "ui/layout/PaneComponent.h"
#include "ui/layout/PaneContainerComponent.h"
#include "ui/layout/PluginEditorLayout.h"
#include "ui/layout/SourceCoordinator.h"
#include "ui/managers/DialogManager.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "ui/managers/PerformanceMetricsController.h"
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

    const long double scaled = (static_cast<long double>(timestamp) * static_cast<long double>(captureRate)) /
                               static_cast<long double>(sourceRate);
    if (!std::isfinite(static_cast<double>(scaled)))
        return timestamp;
    if (scaled <= 0.0L)
        return 0;

    constexpr auto maxValue = static_cast<long double>(std::numeric_limits<int64_t>::max());
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
    , serviceContext_{.instanceRegistry = processor_.getInstanceRegistry(),
                      .themeService = processor_.getThemeService(),
                      .shaderRegistry = processor_.getShaderRegistry(),
                      .presetManager = processor_.getPresetManager()}
{
    // Create coordinators
    sourceCoordinator_ =
        std::make_unique<SourceCoordinator>(processor_.getInstanceRegistry(), [this]() { onSourcesChanged(); });
    themeCoordinator_ = std::make_unique<ThemeCoordinator>(processor_.getThemeService(),
                                                           [this](const ColorTheme& theme) { onThemeChanged(theme); });
    layoutCoordinator_ = std::make_unique<LayoutCoordinator>(windowLayout_, [this]() { onLayoutChanged(); });

    processor_.getThemeService().setCurrentTheme(processor_.getState().getThemeName());

    // Install the project-wide LookAndFeel BEFORE any child components are
    // constructed so their colour lookups (called in ctor) see themed values.
    // Seed it with the active theme so first paint is already themed.
    //
    // Only the editor subtree gets lookAndFeel_ — the process-wide
    // juce::LookAndFeel::setDefaultLookAndFeel() is deliberately NOT touched.
    // Writing the process-global default from a plugin is a supply-chain
    // hazard:
    //   (a) non-Oscil plugins loaded in the same DAW process would have
    //       their theme overridden until Oscil is unloaded, and
    //   (b) multi-instance destruction cannot reliably restore a known
    //       prior default because other instances write the same global
    //       concurrently.
    //
    // Trade-off: juce::AlertWindow::showMessageBoxAsync(...) with no
    // associated component uses the global default LookAndFeel, so those
    // (rare) alerts render with JUCE's stock LookAndFeel_V4 chrome rather
    // than Oscil's dark theme. All other popups (PopupMenu, TooltipWindow,
    // modal AlertWindow shown from a themed parent) inherit this editor's
    // LookAndFeel through the component hierarchy. If future UX work needs
    // themed async alerts, replace the showMessageBoxAsync call sites with
    // a custom AlertWindow that explicitly setLookAndFeel(&lookAndFeel_).
    lookAndFeel_.applyTheme(processor_.getThemeService().getCurrentTheme());
    setLookAndFeel(&lookAndFeel_);

    initUIComponents();
    initManagers();
    initControllerAndSettings();
    initTimingEngine();

    oscillatorPanelController_->createDefaultOscillatorIfNeeded();
    onSourcesChanged();
    oscillatorPanelController_->refreshPanels();

    setResizable(true, true);
    setResizeLimits(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT, WindowLayout::MAX_WINDOW_WIDTH,
                    WindowLayout::MAX_WINDOW_HEIGHT);
    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    startTimerHz(TIMER_REFRESH_RATE_HZ);

    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
    {
        testServer_ = std::make_unique<PluginTestServer>(*this);
        testServer_->start(TEST_SERVER_PORT);
        DBG("Test server started on port " << TEST_SERVER_PORT);
    }
}

void OscilPluginEditor::initUIComponents()
{
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<PaneContainerComponent>(processor_.getThemeService());
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);

    sidebar_ = std::make_unique<SidebarComponent>(serviceContext_);
    addAndMakeVisible(*sidebar_);

    statusBar_ = std::make_unique<StatusBarComponent>(processor_.getThemeService());
    addAndMakeVisible(*statusBar_);
}

void OscilPluginEditor::initManagers()
{
    dialogManager_ =
        std::make_unique<DialogManager>(*this, processor_.getThemeService(), processor_.getInstanceRegistry());
    configPopupAdapter_ = std::make_unique<ConfigPopupListenerAdapter>(*this);
    dialogManager_->addConfigPopupListener(configPopupAdapter_.get());

    metricsController_ =
        std::make_unique<PerformanceMetricsController>(processor_, processor_.getInstanceRegistry(), *statusBar_);
    editorLayout_ =
        std::make_unique<PluginEditorLayout>(*this, *viewport_, *contentComponent_, *sidebar_, *statusBar_, processor_);
    renderCoordinator_ = std::make_unique<GpuRenderCoordinator>(*this, *statusBar_);

    bool const gpuRenderingEnabled = processor_.getState().isGpuRenderingEnabled();
    renderCoordinator_->setGpuRenderingEnabled(gpuRenderingEnabled);
    if (auto* optionsSection = sidebar_->getOptionsSection())
    {
        optionsSection->setGpuRenderingEnabled(gpuRenderingEnabled);
        auto qualityConfig = processor_.getState().getCaptureQualityConfig();
        optionsSection->setQualityPreset(qualityConfig.qualityPreset);
        optionsSection->setBufferDuration(qualityConfig.bufferDuration);
        optionsSection->setAutoAdjustQuality(qualityConfig.autoAdjustQuality);
    }
}

void OscilPluginEditor::initControllerAndSettings()
{
    // Two-phase initialization to resolve circular dependency:
    // Controller owns the pane vector, Manager needs to reference it,
    // but Controller needs Manager for applying settings.
    oscillatorPanelController_ = std::make_unique<OscillatorPanelController>(processor_, serviceContext_,
                                                                             *contentComponent_, *renderCoordinator_);

    // Snapshot callback prevents iterator invalidation if pane vector changes during settings updates
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
    sidebar_->addListener(oscillatorPanelController_.get());
    sidebar_->addListener(this);

    oscillatorPanelController_->setLayoutNeededCallback([this]() {
        if (editorLayout_)
            editorLayout_->updateLayout(oscillatorPanelController_->getPaneComponents());
    });
}

void OscilPluginEditor::initTimingEngine()
{
    timingEngineAdapter_ = std::make_unique<TimingEngineListenerAdapter>(*this);
    processor_.getTimingEngine().addListener(timingEngineAdapter_.get());

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
        timingSection->setTimeIntervalMs(timingConfig.timeIntervalMs);
        timingSection->setNoteInterval(timingConfig.noteInterval);
        timingSection->setHostSyncEnabled(timingConfig.hostSyncEnabled);
        timingSection->setHostBPM(timingConfig.hostBPM);
        timingSection->setWaveformMode(waveformMode);
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

    // Detach LookAndFeel before lookAndFeel_ is destroyed. The process-wide
    // default is never modified by Oscil (see ctor comment), so no restore
    // is needed here.
    setLookAndFeel(nullptr);
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

        // Solid base
        g.fillAll(theme.backgroundPrimary);

        // Subtle accent-tinted radial gradient overlay
        auto glass = SurfaceStyle::fromTheme(theme);

        auto bounds = getLocalBounds().toFloat();
        auto accentLow = glass.accent.withAlpha(0.08f);
        auto transparent = glass.accent.withAlpha(0.0f);

        // Top-left radial accent wash
        const juce::ColourGradient topLeft(accentLow, bounds.getWidth() * 0.2f, bounds.getHeight() * 0.1f, transparent,
                                           bounds.getWidth() * 0.7f, bounds.getHeight() * 0.6f, true);
        g.setGradientFill(topLeft);
        g.fillRect(bounds);

        // Bottom-right radial accent wash (dimmer)
        const juce::ColourGradient bottomRight(glass.accent.withAlpha(0.05f), bounds.getWidth() * 0.8f,
                                               bounds.getHeight() * 0.9f, transparent, bounds.getWidth() * 0.3f,
                                               bounds.getHeight() * 0.4f, true);
        g.setGradientFill(bottomRight);
        g.fillRect(bounds);
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
    // Resize changes the scene even when audio is silent — request a paint.
    if (renderCoordinator_)
        renderCoordinator_->forceRepaint();
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
            timingConfig.syncToPlayhead || timingConfig.triggerMode == WaveformTriggerMode::Midi;

        if (restartModeActive && timingEngine.checkAndClearTrigger())
        {
            auto triggerTimestamp = static_cast<int64_t>(std::llround(timingConfig.lastSyncTimestamp));
            if (triggerTimestamp <= 0)
            {
                auto hostTimestamp = timingEngine.getHostInfo().timeInSamples;
                if (hostTimestamp > 0)
                    triggerTimestamp = hostTimestamp;
            }

            const int sourceRate = juce::jmax(1, static_cast<int>(std::llround(processor_.getSampleRate())));
            const int captureRate = juce::jmax(1, processor_.getCaptureRate());
            triggerTimestamp = convertTimelineTimestampToCaptureDomain(triggerTimestamp, sourceRate, captureRate);

            displaySettingsManager_->requestWaveformRestartAtTimestampForAll(juce::jmax<int64_t>(0, triggerTimestamp));
        }
    }

    if (renderCoordinator_)
        renderCoordinator_->updateRendering(oscillatorPanelController_->getPaneComponents());
}

} // namespace oscil
