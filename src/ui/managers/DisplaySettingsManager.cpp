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

DisplaySettingsManager::DisplaySettingsManager(std::vector<std::unique_ptr<PaneComponent>>& panes)
    : panes_(panes)
{
}

void DisplaySettingsManager::setShowGridForAll(bool show)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->setShowGrid(show);
    }
}

void DisplaySettingsManager::setGridConfigForAll(const GridConfiguration& config)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->setGridConfig(config);
    }
}

void DisplaySettingsManager::setAutoScaleForAll(bool autoScale)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->setAutoScale(autoScale);
    }
}

void DisplaySettingsManager::setGainDbForAll(float gainDb)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->setGainDb(gainDb);
    }
}

void DisplaySettingsManager::setDisplaySamplesForAll(int samples)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->setDisplaySamples(samples);
    }
}

void DisplaySettingsManager::highlightOscillator(const OscillatorId& id)
{
    for (auto& pane : panes_)
    {
        if (pane)
            pane->highlightOscillator(id);
    }
}

} // namespace oscil
