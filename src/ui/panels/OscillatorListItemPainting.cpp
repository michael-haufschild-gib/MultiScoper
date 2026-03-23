/*
    Oscil - Oscillator List Item Painting & Events
    (Core setup and state management are in OscillatorListItem.cpp)
*/

#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/panels/OscillatorListItem.h"

namespace oscil
{

void OscillatorListItemComponent::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();
    float alpha = isVisible_ ? 1.0f : 0.5f;

    // Background
    if (selected_)
    {
        g.setColour(theme.backgroundSecondary.brighter(0.05f));
        g.fillRoundedRectangle(bounds.reduced(2), ComponentLayout::RADIUS_LG);

        g.setColour(hasFocus_ ? theme.controlActive.withAlpha(0.8f) : theme.controlBorder.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(2), ComponentLayout::RADIUS_LG, hasFocus_ ? 2.0f : 1.0f);
    }
    else
    {
        if (isHovered_)
        {
            g.setColour(theme.controlHighlight.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds.reduced(2), ComponentLayout::RADIUS_MD);
        }

        if (hasFocus_)
        {
            g.setColour(theme.controlActive.withAlpha(0.6f));
            g.drawRoundedRectangle(bounds.reduced(2), ComponentLayout::RADIUS_MD, 2.0f);
        }
    }

    // Drag handle
    auto dragArea = bounds.removeFromLeft(DRAG_HANDLE_WIDTH);
    float dotAlpha = dragHandleHovered_ ? 0.8f : 0.4f;
    g.setColour(theme.textSecondary.withAlpha(dotAlpha * alpha));
    float dotSize = 3.0f;
    float dotSpacing = 5.0f;
    float startX = dragArea.getCentreX() - dotSpacing / 2;
    float startY = dragArea.getCentreY() - dotSpacing;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 2; ++col)
            g.fillEllipse(startX + col * dotSpacing - dotSize / 2, startY + row * dotSpacing - dotSize / 2, dotSize,
                          dotSize);

    // Color indicator
    bounds.removeFromLeft(4);
    float colorY = selected_ ? (COMPACT_HEIGHT / 2.0f - COLOR_INDICATOR_SIZE / 2.0f)
                             : (bounds.getCentreY() - COLOR_INDICATOR_SIZE / 2.0f);
    g.setColour(colour_.withAlpha(alpha));
    g.fillEllipse(bounds.getX(), colorY, static_cast<float>(COLOR_INDICATOR_SIZE),
                  static_cast<float>(COLOR_INDICATOR_SIZE));
}

void OscillatorListItemComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    updateVisibility();
}

void OscillatorListItemComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    dragHandleHovered_ = false;
    updateVisibility();
}

void OscillatorListItemComponent::mouseMove(const juce::MouseEvent& e)
{
    bool newDragHandleHovered = isInDragZone(e.getPosition());
    if (newDragHandleHovered != dragHandleHovered_)
    {
        dragHandleHovered_ = newDragHandleHovered;
        repaint();
    }
}

void OscillatorListItemComponent::mouseDown(const juce::MouseEvent& e) { dragStartPos_ = e.getPosition(); }

void OscillatorListItemComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isInDragZone(dragStartPos_) && !isDragging_)
    {
        auto distance = e.getPosition().getDistanceFrom(dragStartPos_);
        if (distance > 5)
        {
            isDragging_ = true;
            listeners_.call([this](Listener& l) { l.oscillatorDragStarted(oscillatorId_); });
        }
    }
}

void OscillatorListItemComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Check if double click is on color indicator
    // Logic mirrors paint() positioning
    auto pos = e.getPosition();

    // Drag handle is on left (width 24)
    // Then 4px margin
    // Then color indicator (size 14)

    int indicatorX = DRAG_HANDLE_WIDTH + 4;
    int indicatorY = selected_ ? (COMPACT_HEIGHT - COLOR_INDICATOR_SIZE) / 2 : (getHeight() - COLOR_INDICATOR_SIZE) / 2;

    auto indicatorBounds = juce::Rectangle<int>(indicatorX, indicatorY, COLOR_INDICATOR_SIZE, COLOR_INDICATOR_SIZE);

    // Expand hit target slightly for better usability
    if (indicatorBounds.expanded(4).contains(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorColorConfigRequested(oscillatorId_); });
        return;
    }

    // Otherwise treat as selection (already handled by mouseDown/Up but double click might need specific handling if we
    // wanted to open config) For now, just select
    listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); });
}

void OscillatorListItemComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging_ = false;
    auto pos = e.getPosition();

    // Drag logic remains

    // Select item if not in drag zone and not clicking child components
    // Note: Labels are set to NOT intercept clicks, so clicks on text fall through to here.
    // Buttons DO intercept clicks, so we won't get here for button clicks.
    if (!isInDragZone(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
    }
}

bool OscillatorListItemComponent::keyPressed(const juce::KeyPress& key)
{
    // Space/Enter to toggle selection
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
    {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
        return true;
    }

    // Delete key to delete oscillator
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); });
        return true;
    }

    // V key to toggle visibility
    if (key.getTextCharacter() == 'v' || key.getTextCharacter() == 'V')
    {
        // If trying to make visible but no valid pane, request pane selection
        if (!isVisible_ && !paneId_.isValid())
        {
            listeners_.call([this](Listener& l) { l.oscillatorPaneSelectionRequested(oscillatorId_); });
            return true;
        }
        isVisible_ = !isVisible_;
        updateVisibility(); // Must call updateVisibility to sync child components
        listeners_.call([this](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId_, isVisible_); });
        return true;
    }

    // Cmd/Ctrl+Up/Down to reorder oscillators (keyboard alternative to drag-drop)
    if (key.getModifiers().isCommandDown())
    {
        if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            listeners_.call([this](Listener& l) { l.oscillatorMoveRequested(oscillatorId_, -1); });
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            listeners_.call([this](Listener& l) { l.oscillatorMoveRequested(oscillatorId_, 1); });
            return true;
        }
    }

    return false;
}

void OscillatorListItemComponent::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscillatorListItemComponent::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

} // namespace oscil
