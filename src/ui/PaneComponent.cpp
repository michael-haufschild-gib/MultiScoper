/*
    Oscil - Pane Component Implementation
*/

#include "ui/PaneComponent.h"
#include "ui/ThemeManager.h"
#include "core/PluginProcessor.h"
#include "core/InstanceRegistry.h"

namespace oscil
{

PaneComponent::PaneComponent(OscilPluginProcessor& processor, const PaneId& paneId)
    : processor_(processor)
    , paneId_(paneId)
{
    setOpaque(true);
}

void PaneComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Draw pane background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);

    // Draw border - highlight if drag target
    if (isDragOver_)
    {
        g.setColour(theme.controlActive);
        g.drawRect(bounds, 3);
    }
    else
    {
        g.setColour(theme.controlBorder);
        g.drawRect(bounds, 1);
    }

    // Draw header
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);

    // Highlight header if drag over
    if (isDragOver_)
    {
        g.setColour(theme.controlActive.withAlpha(0.3f));
    }
    else
    {
        g.setColour(theme.backgroundSecondary);
    }
    g.fillRect(headerBounds);

    // Draw drag handle indicator (three horizontal lines)
    auto handleBounds = headerBounds.removeFromLeft(20);
    g.setColour(theme.textSecondary.withAlpha(0.5f));
    int lineY = handleBounds.getCentreY() - 4;
    for (int i = 0; i < 3; ++i)
    {
        g.fillRect(handleBounds.getX() + 4, lineY + i * 4, 10, 2);
    }

    // Draw "Processing:" label and colored mode indicator per design
    if (!waveforms_.empty())
    {
        auto contentBounds = headerBounds.reduced(PADDING, 0);

        // "Processing:" label
        g.setColour(theme.textSecondary);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("Processing:", contentBounds.removeFromLeft(70), juce::Justification::centredLeft);

        // Colored mode indicator for first oscillator (primary display)
        const auto& primaryOsc = waveforms_[0].oscillator;
        auto mode = primaryOsc.getProcessingMode();
        juce::String modeText = processingModeToString(mode);

        // Get mode-specific color
        juce::Colour modeColor = primaryOsc.getColour();

        // Draw colored mode badge
        auto badgeBounds = contentBounds.removeFromLeft(100).toFloat();
        g.setColour(modeColor.withAlpha(0.2f));
        g.fillRoundedRectangle(badgeBounds.reduced(2, 4), 10.0f);

        g.setColour(modeColor);
        g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        g.drawText(modeText, badgeBounds.toNearestInt(), juce::Justification::centred);

        // Show additional oscillators count if more than one
        if (waveforms_.size() > 1)
        {
            contentBounds.removeFromLeft(8);
            g.setColour(theme.textSecondary);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText("+" + juce::String(waveforms_.size() - 1) + " more",
                       contentBounds, juce::Justification::centredLeft);
        }
    }
    else
    {
        g.setColour(theme.textSecondary);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Empty Pane", headerBounds.reduced(PADDING, 0), juce::Justification::centredLeft);
    }

    // Draw separator line
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(HEADER_HEIGHT - 1, 0.0f, static_cast<float>(getWidth()));
}

void PaneComponent::resized()
{
    updateLayout();
}

void PaneComponent::mouseDown(const juce::MouseEvent& event)
{
    // Record drag start position
    dragStartPos_ = event.getPosition();
}

void PaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Only start drag if in header area and moved beyond threshold
    if (!isInDragZone(dragStartPos_))
        return;

    auto distance = event.getPosition().getDistanceFrom(dragStartPos_);
    if (distance > DRAG_THRESHOLD)
    {
        // Find DragAndDropContainer
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            // Create drag description with pane ID
            juce::var dragDescription;
            dragDescription = juce::var(paneId_.id);

            // Create a snapshot image for dragging
            auto snapshot = createComponentSnapshot(getLocalBounds(), true, 1.0f);

            // Make semi-transparent
            juce::Image dragImage(juce::Image::ARGB, snapshot.getWidth(), snapshot.getHeight(), true);
            juce::Graphics dragG(dragImage);
            dragG.setOpacity(0.7f);
            dragG.drawImageAt(snapshot, 0, 0);

            container->startDragging(dragDescription, this, dragImage, true, nullptr, &event.source);
        }
    }
}

bool PaneComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Accept drags from other PaneComponents
    if (auto* sourcePane = dynamic_cast<PaneComponent*>(dragSourceDetails.sourceComponent.get()))
    {
        return sourcePane->getPaneId().id != paneId_.id;
    }
    return false;
}

void PaneComponent::itemDragEnter(const SourceDetails& /*dragSourceDetails*/)
{
    isDragOver_ = true;
    repaint();
}

void PaneComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
    isDragOver_ = false;
    repaint();
}

void PaneComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragOver_ = false;
    repaint();

    // Get the dragged pane ID from the description
    if (dragSourceDetails.description.isString())
    {
        PaneId movedPaneId;
        movedPaneId.id = dragSourceDetails.description.toString();

        // Notify callback
        if (paneReorderedCallback_)
        {
            paneReorderedCallback_(movedPaneId, paneId_);
        }
    }
}

bool PaneComponent::isInDragZone(const juce::Point<int>& position) const
{
    // Header area is the drag zone
    return position.y < HEADER_HEIGHT;
}

void PaneComponent::addOscillator(const Oscillator& oscillator)
{
    OscillatorEntry entry;
    entry.oscillator = oscillator;
    entry.waveform = std::make_unique<WaveformComponent>();

    // Configure waveform
    entry.waveform->setProcessingMode(oscillator.getProcessingMode());
    entry.waveform->setColour(oscillator.getColour());
    entry.waveform->setOpacity(oscillator.getOpacity());
    entry.waveform->setVisible(oscillator.isVisible());

    // Get capture buffer - always try processor's buffer first since it's always available
    // The registry lookup might fail if prepareToPlay() hasn't been called yet
    auto buffer = processor_.getCaptureBuffer();

    // If we have a valid source ID, try to get its specific buffer
    if (oscillator.getSourceId().isValid())
    {
        auto sourceBuffer = InstanceRegistry::getInstance().getCaptureBuffer(oscillator.getSourceId());
        if (sourceBuffer)
        {
            buffer = sourceBuffer;
        }
    }

    if (buffer)
    {
        entry.waveform->setCaptureBuffer(buffer);
    }

    addAndMakeVisible(*entry.waveform);
    waveforms_.push_back(std::move(entry));

    updateLayout();
}

void PaneComponent::removeOscillator(const OscillatorId& oscillatorId)
{
    waveforms_.erase(
        std::remove_if(waveforms_.begin(), waveforms_.end(),
            [&oscillatorId](const OscillatorEntry& entry) {
                return entry.oscillator.getId() == oscillatorId;
            }),
        waveforms_.end());

    updateLayout();
}

void PaneComponent::clearOscillators()
{
    waveforms_.clear();
}

void PaneComponent::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    for (auto& entry : waveforms_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            // Update the oscillator's source ID
            entry.oscillator.setSourceId(newSourceId);

            // Get the new buffer from registry
            std::shared_ptr<SharedCaptureBuffer> newBuffer;

            if (newSourceId.isValid())
            {
                newBuffer = InstanceRegistry::getInstance().getCaptureBuffer(newSourceId);
            }

            // Fall back to processor's buffer if source not found
            if (!newBuffer)
            {
                newBuffer = processor_.getCaptureBuffer();
            }

            if (newBuffer)
            {
                entry.waveform->setCaptureBuffer(newBuffer);
            }

            break;
        }
    }
}

void PaneComponent::updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible)
{
    for (auto& entry : waveforms_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            // Update the oscillator data
            entry.oscillator.setProcessingMode(mode);
            entry.oscillator.setVisible(visible);

            // Update the waveform component
            entry.waveform->setProcessingMode(mode);
            entry.waveform->setVisible(visible);

            // Repaint to reflect changes
            repaint();
            break;
        }
    }
}

void PaneComponent::updateLayout()
{
    if (waveforms_.empty())
        return;

    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);
    bounds = bounds.reduced(PADDING);

    int numWaveforms = static_cast<int>(waveforms_.size());
    int waveformHeight = std::max(100, bounds.getHeight() / std::max(1, numWaveforms));

    for (size_t i = 0; i < waveforms_.size(); ++i)
    {
        auto waveformBounds = bounds.removeFromTop(waveformHeight);
        waveforms_[i].waveform->setBounds(waveformBounds);
    }
}

void PaneComponent::setShowGrid(bool enabled)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setShowGrid(enabled);
    }
}

void PaneComponent::setAutoScale(bool enabled)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setAutoScale(enabled);
    }
}

void PaneComponent::setHoldDisplay(bool enabled)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setHoldDisplay(enabled);
    }
}

void PaneComponent::setGainDb(float dB)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setGainDb(dB);
    }
}

void PaneComponent::highlightOscillator(const OscillatorId& oscillatorId)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
        {
            bool shouldHighlight = entry.oscillator.getId() == oscillatorId;
            entry.waveform->setHighlighted(shouldHighlight);
        }
    }
}

} // namespace oscil
