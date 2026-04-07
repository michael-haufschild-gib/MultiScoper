/*
    Oscil - Display Settings Manager Implementation
    Manages global display settings across all panes
*/

#include "ui/managers/DisplaySettingsManager.h"

#include "core/Oscillator.h"
#include "core/dsp/TimingConfig.h"
#include "ui/layout/PaneComponent.h"

namespace oscil
{

DisplaySettingsManager::DisplaySettingsManager(PaneGetter paneGetter) : paneGetter_(std::move(paneGetter)) {}

void DisplaySettingsManager::setShowGridForAll(bool show)
{
    forEachPane([show](PaneComponent& p) { p.setShowGrid(show); });
}

void DisplaySettingsManager::setGridConfigForAll(const GridConfiguration& config)
{
    forEachPane([&config](PaneComponent& p) { p.setGridConfig(config); });
}

void DisplaySettingsManager::setAutoScaleForAll(bool autoScale)
{
    forEachPane([autoScale](PaneComponent& p) { p.setAutoScale(autoScale); });
}

void DisplaySettingsManager::setGainDbForAll(float gainDb)
{
    forEachPane([gainDb](PaneComponent& p) { p.setGainDb(gainDb); });
}

void DisplaySettingsManager::setDisplaySamplesForAll(int samples)
{
    forEachPane([samples](PaneComponent& p) { p.setDisplaySamples(samples); });
}

void DisplaySettingsManager::setSampleRateForAll(int sampleRate)
{
    forEachPane([sampleRate](PaneComponent& p) { p.setSampleRate(sampleRate); });
}

void DisplaySettingsManager::requestWaveformRestartAtTimestampForAll(int64_t timelineSampleTimestamp)
{
    forEachPane(
        [timelineSampleTimestamp](PaneComponent& p) { p.requestWaveformRestartAtTimestamp(timelineSampleTimestamp); });
}

void DisplaySettingsManager::highlightOscillator(const OscillatorId& id)
{
    forEachPane([&id](PaneComponent& p) { p.highlightOscillator(id); });
}

} // namespace oscil
