/*
    Oscil - Sidebar Component Handlers
    Section listener callbacks forwarded to parent listeners
*/

#include "ui/layout/SidebarComponent.h"

namespace oscil
{
void SidebarComponent::addOscillatorDialogRequested()
{
    listeners_.call([](Listener& l) { l.addOscillatorDialogRequested(); });
}

void SidebarComponent::oscillatorSelected(const OscillatorId& id)
{
    selectedOscillatorId_ = id;
    listeners_.call([id](Listener& l) { l.oscillatorSelected(id); });
}

void SidebarComponent::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    listeners_.call([id, visible](Listener& l) { l.oscillatorVisibilityChanged(id, visible); });
}

void SidebarComponent::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    listeners_.call([id, mode](Listener& l) { l.oscillatorModeChanged(id, mode); });
}

void SidebarComponent::oscillatorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorConfigRequested(id); });
}

void SidebarComponent::oscillatorColorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorColorConfigRequested(id); });
}

void SidebarComponent::oscillatorDeleteRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
}

void SidebarComponent::oscillatorsReordered(int fromIndex, int toIndex)
{
    listeners_.call([fromIndex, toIndex](Listener& l) { l.oscillatorsReordered(fromIndex, toIndex); });
}

void SidebarComponent::oscillatorPaneSelectionRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorPaneSelectionRequested(id); });
}

void SidebarComponent::oscillatorNameChanged(const OscillatorId& id, const juce::String& newName)
{
    // Name changes are handled internally by OscillatorListComponent
    juce::ignoreUnused(id, newName);
}

// TimingSidebarSection::Listener implementation
void SidebarComponent::timingModeChanged(TimingMode mode)
{
    listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
}

void SidebarComponent::noteIntervalChanged(NoteInterval interval)
{
    listeners_.call([interval](Listener& l) { l.noteIntervalChanged(interval); });
}

void SidebarComponent::timeIntervalChanged(float ms)
{
    listeners_.call([ms](Listener& l) { l.timeIntervalChanged(ms); });
}

void SidebarComponent::hostSyncChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.hostSyncChanged(enabled); });
}

void SidebarComponent::waveformModeChanged(WaveformMode mode)
{
    listeners_.call([mode](Listener& l) { l.waveformModeChanged(mode); });
}

void SidebarComponent::bpmChanged(float bpm)
{
    listeners_.call([bpm](Listener& l) { l.bpmChanged(bpm); });
}

// OptionsSection::Listener implementation
void SidebarComponent::gainChanged(float dB)
{
    listeners_.call([dB](Listener& l) { l.gainChanged(dB); });
}

void SidebarComponent::showGridChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.showGridChanged(enabled); });
}

void SidebarComponent::autoScaleChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.autoScaleChanged(enabled); });
}

void SidebarComponent::layoutChanged(int columnCount)
{
    listeners_.call([columnCount](Listener& l) { l.layoutChanged(columnCount); });
}

void SidebarComponent::themeChanged(const juce::String& themeName)
{
    listeners_.call([&themeName](Listener& l) { l.themeChanged(themeName); });
}

void SidebarComponent::gpuRenderingChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.gpuRenderingChanged(enabled); });
}

void SidebarComponent::qualityPresetChanged(QualityPreset preset)
{
    listeners_.call([preset](Listener& l) { l.qualityPresetChanged(preset); });
}

void SidebarComponent::bufferDurationChanged(BufferDuration duration)
{
    listeners_.call([duration](Listener& l) { l.bufferDurationChanged(duration); });
}

void SidebarComponent::autoAdjustQualityChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.autoAdjustQualityChanged(enabled); });
}

} // namespace oscil
