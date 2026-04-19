/*
    MultiScoper - Plugin Editor Sidebar and Settings Handlers
*/

#include "ui/controllers/GpuRenderCoordinator.h"
#include "ui/controllers/OscillatorPanelController.h"
#include "ui/managers/DialogManager.h"
#include "ui/managers/DisplaySettingsManager.h"
#include "ui/panels/StatusBarComponent.h"

#include "plugin/PluginEditor.h"

#include <set>

namespace multiscoper
{
namespace
{
using SourceIdSet = std::set<juce::String>;

SourceIdSet collectAvailableSourceIds(const std::vector<SourceInfo>& sources)
{
    SourceIdSet ids;
    for (const auto& source : sources)
        ids.insert(source.sourceId.id);
    return ids;
}

bool sourceIdExists(const SourceIdSet& ids, const SourceId& sourceId) { return ids.contains(sourceId.id); }
} // namespace

void MultiScoperPluginEditor::refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators)
{
    if (sidebar_ != nullptr)
        sidebar_->refreshOscillatorList(oscillators);
}

void MultiScoperPluginEditor::onSourcesChanged()
{
    const auto sources = processor_.getInstanceRegistry().getAllSources();
    const auto availableSourceIds = collectAvailableSourceIds(sources);

    const auto ownSourceId = processor_.getSourceId();
    const bool ownSourceAvailable = ownSourceId.isValid() && sourceIdExists(availableSourceIds, ownSourceId);

    if (sidebar_ != nullptr)
        sidebar_->refreshSourceList(sources);

    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    bool updated = false;

    // Adopt own source for oscillators that have no source yet, but do NOT
    // clear persisted-but-currently-unresolved bindings: a sibling plugin's
    // registration may arrive moments later (async deferRegistration, or
    // setStateInformation/prepareToPlay racing), and wiping the binding here
    // would permanently destroy it. Targeted clearing happens only via
    // onSourceRemoved when the registry explicitly reports a removal.
    for (auto& osc : oscillators)
    {
        const auto currentSourceId = osc.getSourceId();

        if (currentSourceId.isNoSource())
            continue;

        if (!currentSourceId.isValid() && ownSourceAvailable)
        {
            osc.setSourceId(ownSourceId);
            state.updateOscillator(osc);
            updated = true;
        }
    }

    if (updated && oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

void MultiScoperPluginEditor::onSourceRemoved(const SourceId& sourceId)
{
    auto& state = processor_.getState();
    auto oscillators = state.getOscillators();
    bool updated = false;

    for (auto& osc : oscillators)
    {
        if (osc.getSourceId() == sourceId)
        {
            osc.clearSource();
            state.updateOscillator(osc);
            updated = true;
        }
    }

    if (updated && oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

void MultiScoperPluginEditor::onThemeChanged(const ColorTheme& newTheme)
{
    // Push the new theme into the project-wide LookAndFeel so raw JUCE
    // widgets (Label/ListBox/AlertWindow/PopupMenu/etc.) update their
    // colour IDs in lockstep with MultiScoper-themed components.
    lookAndFeel_.applyTheme(newTheme);

    repaint();

    if (statusBar_ != nullptr)
        statusBar_->repaint();

    if (sidebar_ != nullptr)
        sidebar_->repaint();

    if (oscillatorPanelController_ != nullptr)
    {
        for (auto& pane : oscillatorPanelController_->getPaneComponents())
        {
            if (pane != nullptr)
                pane->repaint();
        }
    }

    // In GPU mode, PaneComponent/PaneBody/WaveformComponent deliberately skip
    // their own background fills so the OpenGL layer (which JUCE composites
    // *beneath* the component tree) shows through.  That means the visible
    // pane background comes from the GL clear colour — which defaults to
    // transparent-black.  Push the theme's pane colour into the GL renderer
    // so light themes don't leave a dark clear behind dark text.
    if (renderCoordinator_ != nullptr)
    {
        renderCoordinator_->setBackgroundColour(newTheme.backgroundPane);
        // Continuous GL repainting is disabled; if audio is silent the GL
        // context would otherwise skip repainting and the waveforms would
        // render with the previous theme's colours until audio resumes.
        renderCoordinator_->forceRepaint();
    }
}

void MultiScoperPluginEditor::onLayoutChanged()
{
    if (editorLayout_ != nullptr)
        resized();
}

void MultiScoperPluginEditor::toggleSidebar()
{
    if (sidebar_ != nullptr)
        sidebar_->toggleCollapsed();
}

void MultiScoperPluginEditor::onSidebarWidthChanged(int newWidth)
{
    windowLayout_.setSidebarWidth(newWidth);
    processor_.getState().setSidebarWidth(newWidth);
    resized();
}

void MultiScoperPluginEditor::onSidebarCollapsedStateChanged(bool collapsed)
{
    windowLayout_.setSidebarCollapsed(collapsed);
    processor_.getState().setSidebarCollapsed(collapsed);
    resized();
}

void MultiScoperPluginEditor::sidebarWidthChanged(int newWidth) { onSidebarWidthChanged(newWidth); }

void MultiScoperPluginEditor::sidebarCollapsedStateChanged(bool collapsed)
{
    onSidebarCollapsedStateChanged(collapsed);
}

void MultiScoperPluginEditor::timingModeChanged(TimingMode mode) { processor_.getTimingEngine().setTimingMode(mode); }

void MultiScoperPluginEditor::noteIntervalChanged(NoteInterval interval)
{
    processor_.getTimingEngine().setNoteIntervalFromEntity(interval);
}

void MultiScoperPluginEditor::timeIntervalChanged(float ms) { processor_.getTimingEngine().setTimeIntervalMs(ms); }

void MultiScoperPluginEditor::hostSyncChanged(bool enabled)
{
    processor_.getTimingEngine().setHostSyncEnabled(enabled);
}

void MultiScoperPluginEditor::waveformModeChanged(WaveformMode mode)
{
    auto& timingEngine = processor_.getTimingEngine();

    switch (mode)
    {
        case WaveformMode::FreeRunning:
            timingEngine.setWaveformTriggerMode(WaveformTriggerMode::None);
            timingEngine.setSyncToPlayhead(false);
            break;
        case WaveformMode::RestartOnPlay:
            timingEngine.setWaveformTriggerMode(WaveformTriggerMode::None);
            timingEngine.setSyncToPlayhead(true);
            break;
        case WaveformMode::RestartOnNote:
            timingEngine.setWaveformTriggerMode(WaveformTriggerMode::Midi);
            timingEngine.setSyncToPlayhead(false);
            break;
    }
}

void MultiScoperPluginEditor::bpmChanged(float bpm) { processor_.getTimingEngine().setInternalBPM(bpm); }

void MultiScoperPluginEditor::gainChanged(float dB)
{
    processor_.getState().setGainDb(dB);
    setGainDbForAllPanes(dB);
}

void MultiScoperPluginEditor::showGridChanged(bool enabled)
{
    processor_.getState().setShowGridEnabled(enabled);
    setShowGridForAllPanes(enabled);
    // Grid visibility toggle is a pure visual change; silent audio would
    // leave it unrendered until audio resumes.
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->forceRepaint();
}

void MultiScoperPluginEditor::autoScaleChanged(bool enabled)
{
    processor_.getState().setAutoScaleEnabled(enabled);
    setAutoScaleForAllPanes(enabled);
    // Autoscale changes the vertical mapping — must repaint even if no new
    // samples arrive for a while.
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->forceRepaint();
}

void MultiScoperPluginEditor::layoutChanged(int columnCount)
{
    const int normalizedColumns = juce::jlimit(1, 3, columnCount);
    processor_.getState().setColumnLayout(static_cast<ColumnLayout>(normalizedColumns));
    resized();
}

void MultiScoperPluginEditor::themeChanged(const juce::String& themeName)
{
    processor_.getState().setThemeName(themeName);
    processor_.getThemeService().setCurrentTheme(themeName);
}

void MultiScoperPluginEditor::gpuRenderingChanged(bool enabled) { setGpuRenderingEnabled(enabled); }

void MultiScoperPluginEditor::qualityPresetChanged(QualityPreset preset)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.qualityPreset == preset)
        return;

    config.qualityPreset = preset;
    processor_.getState().setCaptureQualityConfig(config);
}

void MultiScoperPluginEditor::bufferDurationChanged(BufferDuration duration)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.bufferDuration == duration)
        return;

    config.bufferDuration = duration;
    processor_.getState().setCaptureQualityConfig(config);
}

void MultiScoperPluginEditor::autoAdjustQualityChanged(bool enabled)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.autoAdjustQuality == enabled)
        return;

    config.autoAdjustQuality = enabled;
    processor_.getState().setCaptureQualityConfig(config);
}

void MultiScoperPluginEditor::onConfigPopupClosed()
{
    if (dialogManager_ != nullptr)
        dialogManager_->closeConfigPopup();
}

void MultiScoperPluginEditor::updateTimingSidebarMode(TimingMode mode)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setTimingMode(mode);
}

void MultiScoperPluginEditor::updateTimingSidebarHostSyncEnabled(bool enabled)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setHostSyncEnabled(enabled);
}

void MultiScoperPluginEditor::updateTimingSidebarHostBpm(float bpm)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setHostBPM(bpm);
}

void MultiScoperPluginEditor::setShowGridForAllPanes(bool enabled)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setShowGridForAll(enabled);
}

void MultiScoperPluginEditor::setGridConfigForAllPanes(const GridConfiguration& config)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setGridConfigForAll(config);
    // Timing/grid changes affect the time axis and grid line placement
    // irrespective of audio activity; force one repaint so the new config
    // is visible immediately even when input is silent.
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->forceRepaint();
}

void MultiScoperPluginEditor::setAutoScaleForAllPanes(bool enabled)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setAutoScaleForAll(enabled);
}

void MultiScoperPluginEditor::setGainDbForAllPanes(float dB)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setGainDbForAll(dB);
}

void MultiScoperPluginEditor::setDisplaySamplesForAllPanes(int samples)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setDisplaySamplesForAll(samples);
    // Time-interval / sync changes scale the visible window — repaint so
    // the new display length is rendered even before audio resumes.
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->forceRepaint();
}

void MultiScoperPluginEditor::setSampleRateForAllPanes(int sampleRate)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setSampleRateForAll(sampleRate);
}

void MultiScoperPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->setGpuRenderingEnabled(enabled);

    if (sidebar_ != nullptr && sidebar_->getOptionsSection() != nullptr)
        sidebar_->getOptionsSection()->setGpuRenderingEnabled(enabled);

    processor_.getState().setGpuRenderingEnabled(enabled);

    if (oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

const std::vector<std::unique_ptr<PaneComponent>>& MultiScoperPluginEditor::getPaneComponents() const
{
    static const std::vector<std::unique_ptr<PaneComponent>> empty;
    if (oscillatorPanelController_ == nullptr)
        return empty;
    return oscillatorPanelController_->getPaneComponents();
}

void MultiScoperPluginEditor::refreshPanels()
{
    // OscillatorPanelController::refreshPanels() is the authoritative path
    // and already forces a GL repaint at its tail — no need to duplicate
    // the forceRepaint() here.
    if (oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

} // namespace multiscoper
