/*
    Oscil - Waveform Stack Implementation
*/

#include "ui/layout/pane/WaveformStack.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/interfaces/IAudioBuffer.h"

namespace oscil
{

namespace
{
std::shared_ptr<IAudioBuffer> resolveCaptureBufferForSource(OscilPluginProcessor& processor, const SourceId& sourceId)
{
    if (sourceId.isNoSource())
    {
        // Explicitly disconnected from any source.
        return nullptr;
    }

    if (sourceId.isValid())
    {
        // Cross-instance binding: only use the selected source buffer.
        return processor.getInstanceRegistry().getCaptureBuffer(sourceId);
    }

    // Legacy/uninitialized source fallback during startup: use local processor source.
    return processor.getCaptureBuffer();
}
} // namespace

WaveformStack::WaveformStack(OscilPluginProcessor& processor,
                              IThemeService& themeService,
                              ShaderRegistry& shaderRegistry)
    : processor_(processor)
    , themeService_(themeService)
    , shaderRegistry_(shaderRegistry)
{
    setOpaque(false);
    // Allow mouse events to pass through to parent (for crosshair updates in PaneBody).
    setInterceptsMouseClicks(false, false);
}

void WaveformStack::resized()
{
    updateLayout();
}

void WaveformStack::addOscillator(const Oscillator& oscillator)
{
    OscillatorEntry entry;
    entry.oscillator = oscillator;
    entry.waveform = std::make_unique<WaveformComponent>(themeService_, shaderRegistry_);

    // Configure waveform with all oscillator properties
    entry.waveform->setProcessingMode(oscillator.getProcessingMode());
    entry.waveform->setColour(oscillator.getColour());
    entry.waveform->setOpacity(oscillator.getOpacity());
    entry.waveform->setLineWidth(oscillator.getLineWidth());
    entry.waveform->setShaderId(oscillator.getShaderId());
    entry.waveform->setVisualPresetId(oscillator.getVisualPresetId());
    entry.waveform->setVisualOverrides(oscillator.getVisualOverrides());

    auto buffer = resolveCaptureBufferForSource(processor_, oscillator.getSourceId());
    entry.waveform->setCaptureBuffer(buffer);

    // Add component first, then set visibility (JUCE requires this order)
    addChildComponent(*entry.waveform);
    entry.waveform->setVisible(oscillator.isVisible());
    entries_.push_back(std::move(entry));

    updateLayout();
}

void WaveformStack::removeOscillator(const OscillatorId& oscillatorId)
{
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [&oscillatorId](const OscillatorEntry& entry) {
            return entry.oscillator.getId() == oscillatorId;
        });

    if (it != entries_.end())
    {
        // Remove component from parent before destroying entry
        if (it->waveform)
            removeChildComponent(it->waveform.get());

        entries_.erase(it);
    }

    updateLayout();
}

void WaveformStack::clearOscillators()
{
    // Remove all child components before clearing
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            removeChildComponent(entry.waveform.get());
    }
    entries_.clear();
}

void WaveformStack::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setSourceId(newSourceId);

            auto newBuffer = resolveCaptureBufferForSource(processor_, newSourceId);
            entry.waveform->setCaptureBuffer(newBuffer);

            break;
        }
    }
}

void WaveformStack::updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible)
{
    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setProcessingMode(mode);
            entry.oscillator.setVisible(visible);
            entry.waveform->setProcessingMode(mode);
            entry.waveform->setVisible(visible);
            break;
        }
    }
}

void WaveformStack::updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name)
{
    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setName(name);
            break;
        }
    }
}

void WaveformStack::updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color)
{
    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setColour(color);
            entry.waveform->setColour(color);
            break;
        }
    }
}

void WaveformStack::updateOscillatorFull(const Oscillator& oscillator)
{
    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillator.getId())
        {
            entry.oscillator = oscillator;
            entry.waveform->setProcessingMode(oscillator.getProcessingMode());
            entry.waveform->setColour(oscillator.getColour());
            entry.waveform->setOpacity(oscillator.getOpacity());
            entry.waveform->setLineWidth(oscillator.getLineWidth());
            entry.waveform->setVisible(oscillator.isVisible());
            entry.waveform->setShaderId(oscillator.getShaderId());
            entry.waveform->setVisualPresetId(oscillator.getVisualPresetId());
            entry.waveform->setVisualOverrides(oscillator.getVisualOverrides());
            break;
        }
    }
}

void WaveformStack::setLayoutMode(LayoutMode mode)
{
    if (layoutMode_ != mode)
    {
        layoutMode_ = mode;
        updateLayout();
    }
}

void WaveformStack::setShowGrid(bool enabled)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setShowGrid(enabled);
    }
}

void WaveformStack::setGridConfig(const GridConfiguration& config)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setGridConfig(config);
    }
}

void WaveformStack::setAutoScale(bool enabled)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setAutoScale(enabled);
    }
}

void WaveformStack::setHoldDisplay(bool enabled)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setHoldDisplay(enabled);
    }
}

void WaveformStack::setGainDb(float dB)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setGainDb(dB);
    }
}

void WaveformStack::setDisplaySamples(int samples)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->setDisplaySamples(samples);
    }
}

void WaveformStack::requestRestartAtTimestamp(int64_t timelineSampleTimestamp)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
            entry.waveform->requestRestartAtTimestamp(timelineSampleTimestamp);
    }
}

void WaveformStack::highlightOscillator(const OscillatorId& oscillatorId)
{
    for (auto& entry : entries_)
    {
        if (entry.waveform)
        {
            bool shouldHighlight = entry.oscillator.getId() == oscillatorId;
            entry.waveform->setHighlighted(shouldHighlight);
        }
    }
}

void WaveformStack::updateLayout()
{
    if (entries_.empty())
        return;

    auto bounds = getLocalBounds();

    if (layoutMode_ == LayoutMode::Stacked)
    {
        int numWaveforms = static_cast<int>(entries_.size());
        // Divide available height equally among waveforms
        int waveformHeight = bounds.getHeight() / std::max(1, numWaveforms);

        for (size_t i = 0; i < entries_.size(); ++i)
        {
            // For the last item, take the remaining space to handle rounding errors
            int height = (i == entries_.size() - 1) ? bounds.getHeight() : waveformHeight;
            
            auto waveformBounds = bounds.removeFromTop(height);
            entries_[i].waveform->setBounds(waveformBounds);
        }
    }
    else // Overlapped
    {
        // All waveforms share the same bounds
        for (auto& entry : entries_)
        {
            entry.waveform->setBounds(bounds);
        }
    }
}

} // namespace oscil
