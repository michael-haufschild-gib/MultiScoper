/*
    Oscil - RenderEngine Waveform Management
    Waveform registration, configuration, sync, and quality control
*/

#include "rendering/RenderEngine.h"
#include "rendering/subsystems/WaveformPass.h"
#include "rendering/subsystems/EffectPipeline.h"

namespace oscil
{

void RenderEngine::registerWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    if (waveformStates_.find(waveformId) == waveformStates_.end())
    {
        WaveformRenderState state;
        state.waveformId = waveformId;
        waveformStates_[waveformId] = std::move(state);
        RE_LOG("RenderEngine: Registered waveform " << waveformId);
    }
}

void RenderEngine::unregisterWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        if (context_)
            it->second.release(*context_);
        waveformStates_.erase(it);
        RE_LOG("RenderEngine: Unregistered waveform " << waveformId);
    }
}

std::optional<VisualConfiguration> RenderEngine::getWaveformConfig(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
        return it->second.visualConfig;
    return std::nullopt;
}

bool RenderEngine::hasWaveform(int waveformId)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    return waveformStates_.find(waveformId) != waveformStates_.end();
}

void RenderEngine::setWaveformConfig(int waveformId, const VisualConfiguration& config)
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    auto it = waveformStates_.find(waveformId);
    if (it != waveformStates_.end())
    {
        it->second.visualConfig = config;

        // Update trails state based on config
        if (context_)
        {
            if (config.trails.enabled && !it->second.trailsEnabled)
            {
                it->second.enableTrails(*context_, currentWidth_, currentHeight_);
            }
            else if (!config.trails.enabled && it->second.trailsEnabled)
            {
                it->second.disableTrails(*context_);
            }

            // Update turbulence
            if (auto* ps = waveformPass_->getParticleSystem())
            {
                if (config.particles.enabled)
                {
                    if (config.particles.useTurbulence)
                    {
                        ps->setTurbulence(
                            config.particles.turbulenceStrength,
                            config.particles.turbulenceScale,
                            config.particles.turbulenceSpeed);
                    }
                    else
                    {
                        ps->setTurbulence(0.0f, 0.5f, 0.5f);
                    }
                }
            }
        }
    }
}

void RenderEngine::clearAllWaveforms()
{
    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);
    if (context_)
    {
        for (auto& pair : waveformStates_)
        {
            pair.second.release(*context_);
        }
    }
    waveformStates_.clear();
}

void RenderEngine::syncWaveforms(const std::unordered_set<int>& activeIds)
{
    if (!context_)
        return;

    juce::SpinLock::ScopedLockType lock(waveformStatesMutex_);

    std::vector<int> toRemove;
    for (const auto& pair : waveformStates_)
    {
        if (activeIds.find(pair.first) == activeIds.end())
        {
            toRemove.push_back(pair.first);
        }
    }

    for (int id : toRemove)
    {
        // Inline unregisterWaveform to avoid recursive lock
        auto it = waveformStates_.find(id);
        if (it != waveformStates_.end())
        {
            it->second.release(*context_);
            waveformStates_.erase(it);
            RE_LOG("RenderEngine: Synced out stale waveform " << id);
        }
    }
}

void RenderEngine::setQualityLevel(QualityLevel level)
{
    qualityLevel_ = level;
    effectPipeline_->setQualityLevel(level);
    RE_LOG("RenderEngine: Quality level set to " << static_cast<int>(level));
}

PostProcessEffect* RenderEngine::getEffect(const juce::String& effectId)
{
    return effectPipeline_->getEffect(effectId);
}

void RenderEngine::setGlobalPostProcessingEnabled(bool enabled)
{
    effectPipeline_->setGlobalPostProcessingEnabled(enabled);
}

bool RenderEngine::isGlobalPostProcessingEnabled() const
{
    return effectPipeline_->isGlobalPostProcessingEnabled();
}

} // namespace oscil
