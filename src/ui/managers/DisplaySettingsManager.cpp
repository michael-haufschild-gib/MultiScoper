/*
    Oscil - Display Settings Manager Implementation
    Manages global display settings across all panes
*/

#include "ui/managers/DisplaySettingsManager.h"
#include "ui/layout/PaneComponent.h"
#include "core/dsp/TimingConfig.h"
#include "core/Oscillator.h"

namespace oscil
{

DisplaySettingsManager::DisplaySettingsManager(PaneGetter paneGetter)
    : paneGetter_(std::move(paneGetter))
{
}

void DisplaySettingsManager::setShowGridForAll(bool show)
{
    // Get snapshot of panes to prevent iterator invalidation during callbacks
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setShowGrid(show);
    }
}

void DisplaySettingsManager::setGridConfigForAll(const GridConfiguration& config)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setGridConfig(config);
    }
}

void DisplaySettingsManager::setAutoScaleForAll(bool autoScale)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setAutoScale(autoScale);
    }
}

void DisplaySettingsManager::setGainDbForAll(float gainDb)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setGainDb(gainDb);
    }
}

void DisplaySettingsManager::setDisplaySamplesForAll(int samples)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setDisplaySamples(samples);
    }
}

void DisplaySettingsManager::setSampleRateForAll(int sampleRate)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->setSampleRate(sampleRate);
    }
}

void DisplaySettingsManager::highlightOscillator(const OscillatorId& id)
{
    for (auto* pane : paneGetter_())
    {
        if (pane)
            pane->highlightOscillator(id);
    }
}

} // namespace oscil
