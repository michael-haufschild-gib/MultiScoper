/*
    Oscil - Waveform Stack Implementation
*/

#include "ui/layout/pane/WaveformStack.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/interfaces/IAudioBuffer.h"
#include "core/DebugLogger.h"

namespace oscil
{

WaveformStack::WaveformStack(OscilPluginProcessor& processor,
                              IThemeService& themeService,
                              ShaderRegistry& shaderRegistry)
    : processor_(processor)
    , themeService_(themeService)
    , shaderRegistry_(shaderRegistry)
{
    setOpaque(false);
    // Allow mouse events to pass through to parent (for crosshair)
    setInterceptsMouseClicks(false, true);
}

void WaveformStack::resized()
{
    updateLayout();
}

void WaveformStack::addOscillator(const Oscillator& oscillator)
{
    OSCIL_LOG("WAVEFORM_STACK", "addOscillator: oscId=" + juce::String(oscillator.getId().id)
              + ", name=" + oscillator.getName()
              + ", sourceId=" + juce::String(oscillator.getSourceId().id)
              + ", sourceName=" + oscillator.getSourceName()
              + ", sourceUUID=" + oscillator.getSourceInstanceUUID()
              + ", sourceValid=" + juce::String(oscillator.getSourceId().isValid() ? "YES" : "NO"));

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

    // Get capture buffer - resolve source using 4-step resolution:
    // Step 0: sourceInstanceUUID (most reliable, persists across sessions)
    // Step 1: sourceId (fast runtime path)
    // Step 2: sourceName (migration fallback)
    // Step 3: Local buffer (last resort)
    std::shared_ptr<IAudioBuffer> buffer;
    SourceId resolvedSourceId = oscillator.getSourceId();
    juce::String resolvedInstanceUUID = oscillator.getSourceInstanceUUID();

    // Step 0: Try sourceInstanceUUID lookup (most reliable - persists across DAW sessions)
    if (!buffer && oscillator.getSourceInstanceUUID().isNotEmpty())
    {
        OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Trying sourceInstanceUUID lookup: '"
                  + oscillator.getSourceInstanceUUID() + "'");

        auto sourceInfo = processor_.getInstanceRegistry().getSourceByInstanceUUID(oscillator.getSourceInstanceUUID());
        if (sourceInfo.has_value())
        {
            resolvedSourceId = sourceInfo->sourceId;
            resolvedInstanceUUID = sourceInfo->instanceUUID;
            buffer = processor_.getInstanceRegistry().getCaptureBuffer(resolvedSourceId);

            if (buffer)
            {
                OSCIL_LOG("WAVEFORM_STACK", "addOscillator: RESOLVED sourceInstanceUUID='"
                          + oscillator.getSourceInstanceUUID() + "' to sourceId="
                          + juce::String(resolvedSourceId.id));

                // Update entry with resolved source info
                entry.oscillator.setSourceIdNameAndUUID(resolvedSourceId, sourceInfo->name, resolvedInstanceUUID);

                // Persist to OscilState for session save (critical for cross-session reconnection)
                processor_.getState().updateOscillator(entry.oscillator);
            }
        }
        else
        {
            OSCIL_LOG("WAVEFORM_STACK", "addOscillator: sourceInstanceUUID '"
                      + oscillator.getSourceInstanceUUID() + "' not found in registry");
        }
    }

    // Step 1: Try direct sourceId lookup (fast runtime path)
    if (!buffer && oscillator.getSourceId().isValid())
    {
        OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Trying direct sourceId lookup: "
                  + juce::String(oscillator.getSourceId().id));
        buffer = processor_.getInstanceRegistry().getCaptureBuffer(oscillator.getSourceId());

        // If found, cache the instanceUUID for future sessions
        if (buffer)
        {
            auto sourceInfo = processor_.getInstanceRegistry().getSource(oscillator.getSourceId());
            if (sourceInfo.has_value() && sourceInfo->instanceUUID.isNotEmpty())
            {
                resolvedInstanceUUID = sourceInfo->instanceUUID;
                // Use consistent API to set all source fields
                entry.oscillator.setSourceIdNameAndUUID(oscillator.getSourceId(), sourceInfo->name, resolvedInstanceUUID);

                // Persist to OscilState for session save (critical for cross-session reconnection)
                processor_.getState().updateOscillator(entry.oscillator);

                OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Cached instanceUUID='" + resolvedInstanceUUID
                          + "' for sourceId=" + juce::String(oscillator.getSourceId().id));
            }
        }
    }

    // Step 2: If direct lookup failed and we have a sourceName, try to resolve by name
    // This handles migration from older versions without instanceUUID
    if (!buffer && oscillator.getSourceName().isNotEmpty())
    {
        OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Trying sourceName resolution: '"
                  + oscillator.getSourceName() + "'");

        auto sourceInfo = processor_.getInstanceRegistry().getSourceByName(oscillator.getSourceName());
        if (sourceInfo.has_value())
        {
            resolvedSourceId = sourceInfo->sourceId;
            resolvedInstanceUUID = sourceInfo->instanceUUID;
            buffer = processor_.getInstanceRegistry().getCaptureBuffer(resolvedSourceId);

            if (buffer)
            {
                OSCIL_LOG("WAVEFORM_STACK", "addOscillator: RESOLVED sourceName='" + oscillator.getSourceName()
                          + "' to sourceId=" + juce::String(resolvedSourceId.id)
                          + ", caching instanceUUID='" + resolvedInstanceUUID + "'");

                // Update entry with resolved source info (including UUID for future sessions)
                entry.oscillator.setSourceIdNameAndUUID(resolvedSourceId, oscillator.getSourceName(), resolvedInstanceUUID);

                // Persist to OscilState for session save (critical for migration to UUID-based lookup)
                processor_.getState().updateOscillator(entry.oscillator);
            }
        }
        else
        {
            OSCIL_LOG("WAVEFORM_STACK", "addOscillator: sourceName '" + oscillator.getSourceName()
                      + "' not found in registry");
        }
    }

    // Step 3: Fallback to local processor buffer
    if (!buffer)
    {
        buffer = processor_.getCaptureBuffer();
        OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Using LOCAL buffer fallback, valid="
                  + juce::String(buffer != nullptr ? "YES" : "NO"));
    }

    if (buffer)
    {
        entry.waveform->setCaptureBuffer(buffer);
        OSCIL_LOG("WAVEFORM_STACK", "addOscillator: Buffer SET on WaveformComponent, availableSamples="
                  + juce::String(static_cast<int>(buffer->getAvailableSamples())));
    }
    else
    {
        OSCIL_LOG_ERROR("WaveformStack::addOscillator", "NO BUFFER available for oscillator "
                       + juce::String(oscillator.getId().id));
    }

    // Add component first, then set visibility (JUCE requires this order)
    addChildComponent(*entry.waveform);
    entry.waveform->setVisible(oscillator.isVisible());
    entries_.push_back(std::move(entry));

    OSCIL_LOG("WAVEFORM_STACK", "addOscillator: COMPLETE, total entries=" + juce::String(static_cast<int>(entries_.size())));

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
    OSCIL_LOG("WAVEFORM_STACK", "updateOscillatorSource: oscId=" + juce::String(oscillatorId.id)
              + ", newSourceId=" + juce::String(newSourceId.id)
              + ", sourceValid=" + juce::String(newSourceId.isValid() ? "YES" : "NO"));

    for (auto& entry : entries_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            // CRITICAL FIX: Use setSourceIdNameAndUUID to persist all source info for cross-session reconnection
            std::shared_ptr<IAudioBuffer> newBuffer;
            juce::String sourceName = "Unknown Source";
            juce::String sourceInstanceUUID;

            if (newSourceId.isValid())
            {
                OSCIL_LOG("WAVEFORM_STACK", "updateOscillatorSource: Attempting external buffer for sourceId="
                          + juce::String(newSourceId.id));

                // Look up source info to get name and UUID
                auto sourceInfo = processor_.getInstanceRegistry().getSource(newSourceId);
                if (sourceInfo.has_value())
                {
                    sourceName = sourceInfo->name;
                    sourceInstanceUUID = sourceInfo->instanceUUID;
                }

                newBuffer = processor_.getInstanceRegistry().getCaptureBuffer(newSourceId);
                if (!newBuffer)
                {
                    OSCIL_LOG_ERROR("WaveformStack::updateOscillatorSource",
                                   "FAILED to get external buffer for sourceId=" + juce::String(newSourceId.id)
                                   + " - falling back to local buffer");
                }
                else
                {
                    OSCIL_LOG("WAVEFORM_STACK", "updateOscillatorSource: SUCCESS - got external buffer, name='"
                              + sourceName + "', uuid='" + sourceInstanceUUID + "'");
                }
            }

            if (!newBuffer)
            {
                newBuffer = processor_.getCaptureBuffer();
                OSCIL_LOG("WAVEFORM_STACK", "updateOscillatorSource: Using LOCAL buffer, valid="
                          + juce::String(newBuffer != nullptr ? "YES" : "NO"));
            }

            // Set all source fields for proper persistence
            entry.oscillator.setSourceIdNameAndUUID(newSourceId, sourceName, sourceInstanceUUID);

            if (newBuffer)
            {
                entry.waveform->setCaptureBuffer(newBuffer);
                OSCIL_LOG("WAVEFORM_STACK", "updateOscillatorSource: Buffer SET, availableSamples="
                          + juce::String(static_cast<int>(newBuffer->getAvailableSamples())));
            }
            else
            {
                OSCIL_LOG_ERROR("WaveformStack::updateOscillatorSource", "NO BUFFER available!");
            }

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
