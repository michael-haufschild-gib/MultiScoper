/*
    Oscil - Pane Component Implementation
*/

#include "ui/layout/PaneComponent.h"
#include "ui/theme/ThemeManager.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/interfaces/IAudioBuffer.h"
#include "ui/components/TestId.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/InlineEditLabel.h"

namespace oscil
{

PaneComponent::PaneComponent(OscilPluginProcessor& processor, ServiceContext& context, const PaneId& paneId)
    : processor_(processor)
    , themeService_(context.themeService)
    , paneId_(paneId)
    , paneName_("Pane")
{
    setOpaque(true);

    // Create pane name label (inline editable)
    nameLabel_ = std::make_unique<InlineEditLabel>(themeService_, "pane_nameLabel");
    nameLabel_->setText(paneName_, false);
    nameLabel_->setPlaceholder("Pane name...");
    nameLabel_->setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    nameLabel_->setTextJustification(juce::Justification::centredLeft);
    nameLabel_->onTextChanged = [this](const juce::String& newName) {
        paneName_ = newName;
        if (paneNameChangedCallback_)
            paneNameChangedCallback_(paneId_, newName);
    };
    addAndMakeVisible(*nameLabel_);

    // Create close button for header
    closeButton_ = std::make_unique<OscilButton>(themeService_, "", "pane_closeBtn");
    closeButton_->setVariant(ButtonVariant::Icon);
    closeButton_->setIconPath(ListItemIcons::createCloseIcon(14.0f));
    closeButton_->setTooltip("Close pane");
    closeButton_->onClick = [this]() {
        if (paneCloseCallback_)
            paneCloseCallback_(paneId_);
    };
    addAndMakeVisible(*closeButton_);
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
    bool gpuEnabled = processor_.getState().isGpuRenderingEnabled();
    if (!gpuEnabled)
#endif
    {
        // Draw pane background
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

    // Reserve space for close button on the right
    static constexpr int CLOSE_BUTTON_AREA = 24;
    headerBounds.removeFromRight(CLOSE_BUTTON_AREA);

    // Draw drag handle indicator (three horizontal lines)
    auto handleBounds = headerBounds.removeFromLeft(DRAG_HANDLE_WIDTH);

    // Skip space for name label (InlineEditLabel handles its own rendering)
    static constexpr int NAME_LABEL_AREA = 124; // width + padding
    headerBounds.removeFromLeft(NAME_LABEL_AREA);
    g.setColour(theme.textSecondary.withAlpha(0.5f));
    
    // Center vertically based on total height of 3 lines and spacing
    int totalIconHeight = (3 * DRAG_HANDLE_LINE_HEIGHT) + (2 * DRAG_HANDLE_LINE_SPACING);
    int startY = handleBounds.getCentreY() - (totalIconHeight / 2);
    
    for (int i = 0; i < 3; ++i)
    {
        g.fillRect(
            handleBounds.getX() + DRAG_HANDLE_LEFT_MARGIN, 
            startY + i * (DRAG_HANDLE_LINE_HEIGHT + DRAG_HANDLE_LINE_SPACING), 
            DRAG_HANDLE_LINE_WIDTH, 
            DRAG_HANDLE_LINE_HEIGHT
        );
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
    // Note: Empty pane state is now handled by nameLabel_ (InlineEditLabel)

    // Draw separator line
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(HEADER_HEIGHT - 1, 0.0f, static_cast<float>(getWidth()));

    // Draw hover crosshair
    if (isMouseHovering_)
    {
        g.setColour(theme.crosshairLine);
        
        // Draw vertical line (constrained to body area)
        if (mouseY_ >= HEADER_HEIGHT)
        {
            g.drawVerticalLine(mouseX_, (float)HEADER_HEIGHT, (float)getHeight());
            g.drawHorizontalLine(mouseY_, 0.0f, (float)getWidth());
        }
    }
}

void PaneComponent::resized()
{
    static constexpr int CLOSE_BUTTON_SIZE = 20;
    static constexpr int CLOSE_BUTTON_MARGIN = 2;
    static constexpr int NAME_LABEL_WIDTH = 120;

    // Position close button in header (right side)
    if (closeButton_)
    {
        int buttonX = getWidth() - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN;
        int buttonY = (HEADER_HEIGHT - CLOSE_BUTTON_SIZE) / 2;
        closeButton_->setBounds(buttonX, buttonY, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
    }

    // Position name label in header (after drag handle)
    if (nameLabel_)
    {
        int labelX = DRAG_HANDLE_WIDTH + PADDING;
        int labelY = (HEADER_HEIGHT - nameLabel_->getPreferredHeight()) / 2;
        int labelWidth = juce::jmin(NAME_LABEL_WIDTH, getWidth() - labelX - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN - PADDING);
        nameLabel_->setBounds(labelX, labelY, labelWidth, nameLabel_->getPreferredHeight());
    }

    updateLayout();
}

void PaneComponent::mouseMove(const juce::MouseEvent& event)
{
    // Show drag cursor when hovering over header (drag zone)
    bool inHeader = isInDragZone(event.getPosition());
    if (inHeader)
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        isMouseHovering_ = false; // Don't show crosshair in header
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        isMouseHovering_ = true;
        mouseX_ = event.getPosition().x;
        mouseY_ = event.getPosition().y;
    }
    repaint();
}

void PaneComponent::mouseEnter(const juce::MouseEvent& event)
{
    if (!isInDragZone(event.getPosition()))
    {
        isMouseHovering_ = true;
        mouseX_ = event.getPosition().x;
        mouseY_ = event.getPosition().y;
        repaint();
    }
}

void PaneComponent::mouseExit(const juce::MouseEvent& /*event*/)
{
    // Reset cursor when mouse leaves component
    setMouseCursor(juce::MouseCursor::NormalCursor);
    
    // Clear hover state
    isMouseHovering_ = false;
    repaint();
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

            container->startDragging(dragDescription, this, juce::ScaledImage(dragImage), true, nullptr, &event.source);
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
    entry.waveform = std::make_unique<WaveformComponent>(themeService_);

    // Configure waveform with all oscillator properties
    entry.waveform->setProcessingMode(oscillator.getProcessingMode());
    entry.waveform->setColour(oscillator.getColour());
    entry.waveform->setOpacity(oscillator.getOpacity());
    entry.waveform->setLineWidth(oscillator.getLineWidth());
    entry.waveform->setVisible(oscillator.isVisible());
    entry.waveform->setShaderId(oscillator.getShaderId());
    entry.waveform->setVisualPresetId(oscillator.getVisualPresetId());
    entry.waveform->setVisualOverrides(oscillator.getVisualOverrides());

    // Get capture buffer - always try processor's buffer first since it's always available
    // The registry lookup might fail if prepareToPlay() hasn't been called yet
    std::shared_ptr<IAudioBuffer> buffer = processor_.getCaptureBuffer();

    // If we have a valid source ID, try to get its specific buffer
    if (oscillator.getSourceId().isValid())
    {
        auto sourceBuffer = processor_.getInstanceRegistry().getCaptureBuffer(oscillator.getSourceId());
        if (sourceBuffer)
        {
            buffer = sourceBuffer;
        }
    }

    if (buffer)
    {
        entry.waveform->setCaptureBuffer(buffer);
    }

    // Use addChildComponent (not addAndMakeVisible) to preserve the visibility
    // state set at line 241 from oscillator.isVisible()
    addChildComponent(*entry.waveform);
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
            std::shared_ptr<IAudioBuffer> newBuffer;

            if (newSourceId.isValid())
            {
                newBuffer = processor_.getInstanceRegistry().getCaptureBuffer(newSourceId);
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

void PaneComponent::updateOscillatorName(const OscillatorId& oscillatorId, const juce::String& name)
{
    for (auto& entry : waveforms_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setName(name);
            // Name might be displayed in header if it's the primary oscillator, or tooltip
            repaint();
            break;
        }
    }
}

void PaneComponent::updateOscillatorColor(const OscillatorId& oscillatorId, juce::Colour color)
{
    for (auto& entry : waveforms_)
    {
        if (entry.oscillator.getId() == oscillatorId)
        {
            entry.oscillator.setColour(color);
            entry.waveform->setColour(color);
            repaint(); // Update badge in header
            break;
        }
    }
}

void PaneComponent::updateOscillatorFull(const Oscillator& oscillator)
{
    for (auto& entry : waveforms_)
    {
        if (entry.oscillator.getId() == oscillator.getId())
        {
            // Update all oscillator data
            entry.oscillator = oscillator;

            // Update all waveform component properties
            entry.waveform->setProcessingMode(oscillator.getProcessingMode());
            entry.waveform->setColour(oscillator.getColour());
            entry.waveform->setOpacity(oscillator.getOpacity());
            entry.waveform->setLineWidth(oscillator.getLineWidth());
            entry.waveform->setVisible(oscillator.isVisible());
            entry.waveform->setShaderId(oscillator.getShaderId());
            entry.waveform->setVisualPresetId(oscillator.getVisualPresetId());
            entry.waveform->setVisualOverrides(oscillator.getVisualOverrides());

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

void PaneComponent::setGridConfig(const GridConfiguration& config)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setGridConfig(config);
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

void PaneComponent::setDisplaySamples(int samples)
{
    for (auto& entry : waveforms_)
    {
        if (entry.waveform)
            entry.waveform->setDisplaySamples(samples);
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

void PaneComponent::setPaneName(const juce::String& name)
{
    paneName_ = name;
    if (nameLabel_)
        nameLabel_->setText(name, false);
}

juce::String PaneComponent::getPaneName() const
{
    return paneName_;
}

} // namespace oscil
