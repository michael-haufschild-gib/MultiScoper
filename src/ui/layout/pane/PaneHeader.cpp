/*
    Oscil - Pane Header Implementation
*/

#include "ui/layout/pane/PaneHeader.h"

#include "ui/components/ListItemIcons.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

PaneHeader::PaneHeader(IThemeService& themeService) : themeService_(themeService) { setupComponents(); }

void PaneHeader::setupComponents()
{
    // Name label (inline editable)
    nameLabel_ = std::make_unique<InlineEditLabel>(themeService_, "pane_nameLabel");
    nameLabel_->setText("Pane", false);
    nameLabel_->setPlaceholder("Pane name...");
    nameLabel_->setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    nameLabel_->setTextJustification(juce::Justification::centredLeft);
    nameLabel_->onTextChanged = [this](const juce::String& newName) {
        if (onNameChanged)
            onNameChanged(newName);
    };
    addAndMakeVisible(*nameLabel_);

    // Action bar
    actionBar_ = std::make_unique<PaneActionBar>(themeService_);
    actionBar_->onActionTriggered = [this](PaneAction action, bool state) {
        if (onActionTriggered)
            onActionTriggered(action, state);
    };
    addAndMakeVisible(*actionBar_);

    // Close button
    closeButton_ = std::make_unique<OscilButton>(themeService_, "", "pane_closeBtn");
    closeButton_->setVariant(ButtonVariant::Icon);
    closeButton_->setIconPath(ListItemIcons::createCloseIcon(14.0f));
    closeButton_->setTooltip("Close pane");
    closeButton_->onClick = [this]() {
        if (onCloseRequested)
            onCloseRequested();
    };
    addAndMakeVisible(*closeButton_);
}

void PaneHeader::paintDragHandle(juce::Graphics& g, const juce::Rectangle<int>& handleBounds)
{
    int totalIconHeight = (3 * DRAG_HANDLE_LINE_HEIGHT) + (2 * DRAG_HANDLE_LINE_SPACING);
    int startY = handleBounds.getCentreY() - (totalIconHeight / 2);

    for (int i = 0; i < 3; ++i)
    {
        g.fillRect(handleBounds.getX() + DRAG_HANDLE_LEFT_MARGIN,
                   startY + i * (DRAG_HANDLE_LINE_HEIGHT + DRAG_HANDLE_LINE_SPACING), DRAG_HANDLE_LINE_WIDTH,
                   DRAG_HANDLE_LINE_HEIGHT);
    }
}

void PaneHeader::paintOscillatorBadge(juce::Graphics& g, juce::Rectangle<int>& bounds, const ColorTheme& theme)
{
    if (!primaryOscillator_.has_value())
        return;

    auto mode = primaryOscillator_->getProcessingMode();
    juce::Colour modeColor = primaryOscillator_->getColour();

    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Processing:", bounds.removeFromLeft(70), juce::Justification::centredLeft);

    auto badgeBounds = bounds.removeFromLeft(BADGE_WIDTH).toFloat();
    g.setColour(modeColor.withAlpha(0.2f));
    g.fillRoundedRectangle(badgeBounds.reduced(2, 4), 10.0f);

    g.setColour(modeColor);
    g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    g.drawText(processingModeToString(mode), badgeBounds.toNearestInt(), juce::Justification::centred);

    if (oscillatorCount_ > 1)
    {
        bounds.removeFromLeft(8);
        g.setColour(theme.textSecondary);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("+" + juce::String(oscillatorCount_ - 1) + " more", bounds, juce::Justification::centredLeft);
    }
}

void PaneHeader::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    g.setColour(theme.backgroundSecondary);
    g.fillRect(bounds);

    auto handleBounds = bounds.removeFromLeft(DRAG_HANDLE_WIDTH);
    g.setColour(theme.textSecondary.withAlpha(0.5f));
    paintDragHandle(g, handleBounds);

    bounds.removeFromLeft(NAME_LABEL_WIDTH + PADDING);
    paintOscillatorBadge(g, bounds, theme);

    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void PaneHeader::resized()
{
    // Close button (right side)
    closeButton_->setBounds(getWidth() - CLOSE_BUTTON_SIZE - 2, (HEIGHT - CLOSE_BUTTON_SIZE) / 2, CLOSE_BUTTON_SIZE,
                            CLOSE_BUTTON_SIZE);

    // Action bar (before close button)
    int actionBarWidth = actionBar_->getPreferredWidth();
    actionBar_->setBounds(getWidth() - CLOSE_BUTTON_SIZE - 2 - PADDING - actionBarWidth, 0, actionBarWidth, HEIGHT);

    // Name label (after drag handle)
    int labelX = DRAG_HANDLE_WIDTH + PADDING;
    int labelWidth =
        juce::jmin(NAME_LABEL_WIDTH, getWidth() - labelX - actionBarWidth - CLOSE_BUTTON_SIZE - PADDING * 3);
    labelWidth = juce::jmax(0, labelWidth); // Ensure non-negative
    nameLabel_->setBounds(labelX, (HEIGHT - nameLabel_->getPreferredHeight()) / 2, labelWidth,
                          nameLabel_->getPreferredHeight());
}

void PaneHeader::mouseDown(const juce::MouseEvent& event)
{
    dragStartPos_ = event.getPosition();
    dragStartedForGesture_ = false;
}

void PaneHeader::mouseDrag(const juce::MouseEvent& event)
{
    if (dragStartedForGesture_ || !isInDragZone(dragStartPos_))
        return;

    const auto distance = event.getPosition().getDistanceFrom(dragStartPos_);
    if (distance <= DRAG_START_THRESHOLD_PX)
        return;

    dragStartedForGesture_ = true;
    if (onDragStarted)
        onDragStarted(event);
}

void PaneHeader::mouseMove(const juce::MouseEvent& event)
{
    if (isInDragZone(event.getPosition()))
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PaneHeader::setPaneName(const juce::String& name)
{
    if (nameLabel_)
        nameLabel_->setText(name, false);
}

juce::String PaneHeader::getPaneName() const { return nameLabel_ ? nameLabel_->getText() : juce::String(); }

void PaneHeader::setPrimaryOscillator(const Oscillator* oscillator)
{
    if (oscillator)
        primaryOscillator_ = *oscillator; // Copy by value
    else
        primaryOscillator_.reset();
    repaint();
}

void PaneHeader::setOscillatorCount(int count)
{
    oscillatorCount_ = count;
    repaint();
}

bool PaneHeader::isInDragZone(juce::Point<int> pos) const
{
    // Drag zone is ONLY the drag handle area (left side)
    // Excludes the name label to allow clicking it for editing
    return pos.x >= 0 && pos.x < DRAG_HANDLE_WIDTH;
}

} // namespace oscil
