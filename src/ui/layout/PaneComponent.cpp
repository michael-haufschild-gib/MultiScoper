/*
    Oscil - Pane Component Implementation
*/

#include "ui/layout/PaneComponent.h"

#include "core/OscilState.h"
#include "core/interfaces/IAudioDataProvider.h"
#include "ui/theme/ThemeManager.h"

#include <utility>

namespace oscil
{

PaneComponent::PaneComponent(IAudioDataProvider& dataProvider, ServiceContext& context, PaneId paneId)
    : dataProvider_(dataProvider)
    , themeService_(context.themeService)
    , shaderRegistry_(context.shaderRegistry)
    , paneId_(std::move(paneId))
    , paneName_("Pane")
{
    setOpaque(true);

    // Create header
    header_ = std::make_unique<PaneHeader>(themeService_);
    header_->setPaneName(paneName_);
    header_->onNameChanged = [this](const juce::String& newName) {
        paneName_ = newName;
        if (paneNameChangedCallback_)
            paneNameChangedCallback_(paneId_, newName);
    };
    header_->onCloseRequested = [this]() {
        if (paneCloseCallback_)
            paneCloseCallback_(paneId_);
    };
    header_->onActionTriggered = [this](PaneAction action, bool state) { handleHeaderAction(action, state); };
    header_->onDragStarted = [this](const juce::MouseEvent& event) { handleDragStarted(event); };
    addAndMakeVisible(*header_);

    // Create body
    body_ = std::make_unique<PaneBody>(dataProvider_, context.instanceRegistry, themeService_, shaderRegistry_);
    addAndMakeVisible(*body_);
}

void PaneComponent::setPaneIndex(int index)
{
    paneIndex_ = index;

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    // Register pane with index-based testId
    auto testId = juce::String("pane_") + juce::String(index);
    OSCIL_REGISTER_TEST_ID(testId.toRawUTF8());
#endif
}

void PaneComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto& theme = themeService_.getCurrentTheme();

    // FIX: In GPU mode, the OpenGL renderer handles the background.
    // Only fill the pane background in software rendering mode.
#if OSCIL_ENABLE_OPENGL
    bool const gpuEnabled = dataProvider_.getState().isGpuRenderingEnabled();
    if (!gpuEnabled)
#endif
    {
        g.setColour(theme.backgroundPane);
        g.fillRect(bounds);
    }

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
}

void PaneComponent::resized()
{
    auto bounds = getLocalBounds();

    // Header at top
    if (header_)
        header_->setBounds(bounds.removeFromTop(PaneHeader::HEIGHT));

    // Body fills the rest
    if (body_)
        body_->setBounds(bounds);
}

void PaneComponent::handleHeaderAction(PaneAction action, bool state)
{
    switch (action)
    {
        case PaneAction::ToggleStats:
            if (body_)
                body_->setStatsVisible(state);
            break;

        case PaneAction::ToggleHold:
            setHoldDisplay(state);
            break;

        case PaneAction::Screenshot:
            // Future: Implement screenshot capture
            break;

        case PaneAction::ReorderWaveforms:
            // Future: Implement waveform reordering
            break;
    }
}

void PaneComponent::handleDragStarted(const juce::MouseEvent& event)
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
        juce::Image const dragImage(juce::Image::ARGB, snapshot.getWidth(), snapshot.getHeight(), true);
        juce::Graphics dragG(dragImage);
        dragG.setOpacity(0.7f);
        dragG.drawImageAt(snapshot, 0, 0);

        const auto eventRelativeToPane = event.getEventRelativeTo(this);
        auto mouseDownInPane = eventRelativeToPane.getMouseDownPosition();
        mouseDownInPane.x = juce::jlimit(0, juce::jmax(0, dragImage.getWidth() - 1), mouseDownInPane.x);
        mouseDownInPane.y = juce::jlimit(0, juce::jmax(0, dragImage.getHeight() - 1), mouseDownInPane.y);
        juce::Point<int> const dragImageOffset(-mouseDownInPane.x, -mouseDownInPane.y);

        container->startDragging(dragDescription, this, juce::ScaledImage(dragImage), true, &dragImageOffset,
                                 &event.source);
    }
}

void PaneComponent::updateHeaderBadge()
{
    if (!header_ || !body_)
        return;

    auto count = body_->getOscillatorCount();
    header_->setOscillatorCount(static_cast<int>(count));

    // Set primary oscillator for badge display
    // Note: We don't store the pointer - just pass it directly to header which copies by value.
    // This avoids potential dangling pointer issues if oscillators are removed.
    if (count > 0 && body_->getWaveformStack())
    {
        header_->setPrimaryOscillator(body_->getWaveformStack()->getOscillatorAt(0));
    }
    else
    {
        header_->setPrimaryOscillator(nullptr);
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

// Oscillator management - delegates to body

void PaneComponent::addOscillator(const Oscillator& oscillator)
{
    if (body_)
    {
        body_->addOscillator(oscillator);
        updateHeaderBadge();
    }
}

void PaneComponent::removeOscillator(const OscillatorId& oscillatorId)
{
    if (body_)
    {
        body_->removeOscillator(oscillatorId);
        updateHeaderBadge();
    }
}

void PaneComponent::clearOscillators()
{
    if (body_)
    {
        body_->clearOscillators();
        updateHeaderBadge();
    }
}

void PaneComponent::updateOscillatorSource(const OscillatorId& oscillatorId, const SourceId& newSourceId)
{
    if (body_)
        body_->updateOscillatorSource(oscillatorId, newSourceId);
}

void PaneComponent::updateOscillator(const OscillatorId& oscillatorId, ProcessingMode mode, bool visible)
{
    if (body_)
    {
        body_->updateOscillator(oscillatorId, mode, visible);
        updateHeaderBadge(); // Mode change affects header badge
    }
}

void PaneComponent::updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name)
{
    if (body_)
        body_->updateOscillatorName(oscillatorId, name);
}

void PaneComponent::updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color)
{
    if (body_)
    {
        body_->updateOscillatorColor(oscillatorId, color);
        updateHeaderBadge(); // Color change affects header badge
    }
}

void PaneComponent::updateOscillatorFull(const Oscillator& oscillator)
{
    if (body_)
    {
        body_->updateOscillatorFull(oscillator);
        updateHeaderBadge();
    }
}

size_t PaneComponent::getOscillatorCount() const { return body_ ? body_->getOscillatorCount() : 0; }

// Display settings - delegates to body

void PaneComponent::setShowGrid(bool enabled)
{
    if (body_)
        body_->setShowGrid(enabled);
}

void PaneComponent::setGridConfig(const GridConfiguration& config)
{
    if (body_)
        body_->setGridConfig(config);
}

void PaneComponent::setAutoScale(bool enabled)
{
    if (body_)
        body_->setAutoScale(enabled);
}

void PaneComponent::toggleHoldDisplay() { setHoldDisplay(!isHeld_); }

void PaneComponent::setHoldDisplay(bool enabled)
{
    isHeld_ = enabled;
    if (body_)
        body_->setHoldDisplay(enabled);

    // Update header button state if this was called programmatically
    if (header_ && header_->getActionBar())
    {
        // Only update if different to avoid loop (though setActionToggled handles it)
        if (header_->getActionBar()->isActionToggled(PaneAction::ToggleHold) != enabled)
        {
            header_->getActionBar()->setActionToggled(PaneAction::ToggleHold, enabled);
        }
    }
}

void PaneComponent::setGainDb(float dB)
{
    if (body_)
        body_->setGainDb(dB);
}

void PaneComponent::setDisplaySamples(int samples)
{
    if (body_)
        body_->setDisplaySamples(samples);
}

void PaneComponent::setSampleRate(int sampleRate)
{
    if (body_)
        body_->setSampleRate(sampleRate);
}

void PaneComponent::requestWaveformRestartAtTimestamp(int64_t timelineSampleTimestamp)
{
    if (body_)
        body_->requestWaveformRestartAtTimestamp(timelineSampleTimestamp);
}

void PaneComponent::highlightOscillator(const OscillatorId& oscillatorId)
{
    if (body_)
        body_->highlightOscillator(oscillatorId);
}

void PaneComponent::setPaneName(const juce::String& name)
{
    paneName_ = name;
    if (header_)
        header_->setPaneName(name);
}

juce::String PaneComponent::getPaneName() const { return paneName_; }

// Test access

WaveformComponent* PaneComponent::getWaveformAt(size_t index) const
{
    if (body_ && body_->getWaveformStack())
        return body_->getWaveformStack()->getWaveformAt(index);
    return nullptr;
}

const Oscillator* PaneComponent::getOscillatorAt(size_t index) const
{
    if (body_ && body_->getWaveformStack())
        return body_->getWaveformStack()->getOscillatorAt(index);
    return nullptr;
}

} // namespace oscil
