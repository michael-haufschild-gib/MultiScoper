/*
    Oscil - Plugin Editor Sidebar and Settings Handlers
*/

#include "plugin/PluginEditor.h"

#include <set>

namespace oscil
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

bool sourceIdExists(const SourceIdSet& ids, const SourceId& sourceId) { return ids.find(sourceId.id) != ids.end(); }
} // namespace

void OscilPluginEditor::refreshSidebarOscillatorList(const std::vector<Oscillator>& oscillators)
{
    if (sidebar_ != nullptr)
        sidebar_->refreshOscillatorList(oscillators);
}

void OscilPluginEditor::onSourcesChanged()
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

    for (auto& osc : oscillators)
    {
        const auto currentSourceId = osc.getSourceId();

        if (currentSourceId.isNoSource())
            continue;

        if (!currentSourceId.isValid())
        {
            if (ownSourceAvailable)
            {
                osc.setSourceId(ownSourceId);
                state.updateOscillator(osc);
                updated = true;
            }
            continue;
        }

        if (!sourceIdExists(availableSourceIds, currentSourceId))
        {
            osc.clearSource();
            state.updateOscillator(osc);
            updated = true;
        }
    }

    if (updated && oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

void OscilPluginEditor::onThemeChanged(const ColorTheme& /*newTheme*/)
{
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
}

void OscilPluginEditor::onLayoutChanged()
{
    if (editorLayout_ != nullptr)
        resized();
}

void OscilPluginEditor::toggleSidebar()
{
    if (sidebar_ != nullptr)
        sidebar_->toggleCollapsed();
}

void OscilPluginEditor::onSidebarWidthChanged(int newWidth)
{
    windowLayout_.setSidebarWidth(newWidth);
    processor_.getState().setSidebarWidth(newWidth);
    resized();
}

void OscilPluginEditor::onSidebarCollapsedStateChanged(bool collapsed)
{
    windowLayout_.setSidebarCollapsed(collapsed);
    processor_.getState().setSidebarCollapsed(collapsed);
    resized();
}

void OscilPluginEditor::sidebarWidthChanged(int newWidth) { onSidebarWidthChanged(newWidth); }

void OscilPluginEditor::sidebarCollapsedStateChanged(bool collapsed) { onSidebarCollapsedStateChanged(collapsed); }

void OscilPluginEditor::timingModeChanged(TimingMode mode) { processor_.getTimingEngine().setTimingMode(mode); }

void OscilPluginEditor::noteIntervalChanged(NoteInterval interval)
{
    processor_.getTimingEngine().setNoteIntervalFromEntity(interval);
}

void OscilPluginEditor::timeIntervalChanged(float ms) { processor_.getTimingEngine().setTimeIntervalMs(ms); }

void OscilPluginEditor::hostSyncChanged(bool enabled) { processor_.getTimingEngine().setHostSyncEnabled(enabled); }

void OscilPluginEditor::waveformModeChanged(WaveformMode mode)
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

void OscilPluginEditor::bpmChanged(float bpm) { processor_.getTimingEngine().setInternalBPM(bpm); }

void OscilPluginEditor::gainChanged(float dB)
{
    processor_.getState().setGainDb(dB);
    setGainDbForAllPanes(dB);
}

void OscilPluginEditor::showGridChanged(bool enabled)
{
    processor_.getState().setShowGridEnabled(enabled);
    setShowGridForAllPanes(enabled);
}

void OscilPluginEditor::autoScaleChanged(bool enabled)
{
    processor_.getState().setAutoScaleEnabled(enabled);
    setAutoScaleForAllPanes(enabled);
}

void OscilPluginEditor::layoutChanged(int columnCount)
{
    const int normalizedColumns = juce::jlimit(1, 3, columnCount);
    processor_.getState().setColumnLayout(static_cast<ColumnLayout>(normalizedColumns));
    resized();
}

void OscilPluginEditor::themeChanged(const juce::String& themeName)
{
    processor_.getState().setThemeName(themeName);
    processor_.getThemeService().setCurrentTheme(themeName);
}

void OscilPluginEditor::gpuRenderingChanged(bool enabled) { setGpuRenderingEnabled(enabled); }

void OscilPluginEditor::qualityPresetChanged(QualityPreset preset)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.qualityPreset == preset)
        return;

    config.qualityPreset = preset;
    processor_.getState().setCaptureQualityConfig(config);
}

void OscilPluginEditor::bufferDurationChanged(BufferDuration duration)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.bufferDuration == duration)
        return;

    config.bufferDuration = duration;
    processor_.getState().setCaptureQualityConfig(config);
}

void OscilPluginEditor::autoAdjustQualityChanged(bool enabled)
{
    auto config = processor_.getState().getCaptureQualityConfig();
    if (config.autoAdjustQuality == enabled)
        return;

    config.autoAdjustQuality = enabled;
    processor_.getState().setCaptureQualityConfig(config);
}

void OscilPluginEditor::onConfigPopupClosed()
{
    if (dialogManager_ != nullptr)
        dialogManager_->closeConfigPopup();
}

void OscilPluginEditor::updateTimingSidebarMode(TimingMode mode)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setTimingMode(mode);
}

void OscilPluginEditor::updateTimingSidebarHostSyncEnabled(bool enabled)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setHostSyncEnabled(enabled);
}

void OscilPluginEditor::updateTimingSidebarHostBpm(float bpm)
{
    if (sidebar_ == nullptr)
        return;

    if (auto* timingSection = sidebar_->getTimingSection())
        timingSection->setHostBPM(bpm);
}

void OscilPluginEditor::setShowGridForAllPanes(bool enabled)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setShowGridForAll(enabled);
}

void OscilPluginEditor::setGridConfigForAllPanes(const GridConfiguration& config)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setGridConfigForAll(config);
}

void OscilPluginEditor::setAutoScaleForAllPanes(bool enabled)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setAutoScaleForAll(enabled);
}

void OscilPluginEditor::setGainDbForAllPanes(float dB)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setGainDbForAll(dB);
}

void OscilPluginEditor::setDisplaySamplesForAllPanes(int samples)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setDisplaySamplesForAll(samples);
}

void OscilPluginEditor::setSampleRateForAllPanes(int sampleRate)
{
    if (displaySettingsManager_ != nullptr)
        displaySettingsManager_->setSampleRateForAll(sampleRate);
}

void OscilPluginEditor::setGpuRenderingEnabled(bool enabled)
{
    if (renderCoordinator_ != nullptr)
        renderCoordinator_->setGpuRenderingEnabled(enabled);

    if (sidebar_ != nullptr && sidebar_->getOptionsSection() != nullptr)
        sidebar_->getOptionsSection()->setGpuRenderingEnabled(enabled);

    processor_.getState().setGpuRenderingEnabled(enabled);

    if (oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

const std::vector<std::unique_ptr<PaneComponent>>& OscilPluginEditor::getPaneComponents() const
{
    return oscillatorPanelController_->getPaneComponents();
}

void OscilPluginEditor::refreshPanels()
{
    if (oscillatorPanelController_ != nullptr)
        oscillatorPanelController_->refreshPanels();
}

} // namespace oscil
