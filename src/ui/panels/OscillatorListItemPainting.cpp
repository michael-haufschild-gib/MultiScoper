/*
    MultiScoper - Oscillator List Item Painting & Events
    (Core setup and state management are in OscillatorListItem.cpp)
*/

#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/panels/OscillatorListItem.h"

namespace multiscoper
{

void OscillatorListItemComponent::paint(juce::Graphics& g)
{
    const auto& theme = getTheme();
    auto const bounds = getLocalBounds().toFloat();

    // Tile background. Hover and selected both lift to backgroundRaised; rest uses
    // backgroundSecondary so the tile reads against the sidebar surface.
    auto const tileBg = (selected_ || isHovered_) ? theme.backgroundRaised : theme.backgroundSecondary;
    g.setColour(tileBg);
    g.fillRect(bounds);

    // Left colour swatch strip. Widened to 4px after consolidating with the
    // previous colour indicator ellipse — this strip is now the double-click
    // hit target for the colour picker (see mouseDoubleClick).
    constexpr float kSwatchWidth = 4.0f;
    auto const swatchColour = isVisible_ ? colour_ : colour_.withAlpha(0.3f);
    g.setColour(swatchColour);
    g.fillRect(juce::Rectangle<float>(0.0f, 0.0f, kSwatchWidth, bounds.getHeight()));

    // Selection + focus outline. Selected tiles get a 1px border in the oscillator's
    // colour. A focused tile — selected or not — also gets an accessibility ring so
    // keyboard users can always see the focus target (WCAG 2.4.7).
    if (selected_)
    {
        g.setColour(colour_);
        g.drawRect(bounds, 1.0f);

        if (hasFocus_)
        {
            // Inset ring so the coloured border stays visible beneath the focus indicator.
            g.setColour(theme.controlActive.withAlpha(0.8f));
            g.drawRect(bounds.reduced(2.0f), 1.0f);
        }
    }
    else if (hasFocus_)
    {
        g.setColour(theme.controlActive.withAlpha(0.6f));
        g.drawRect(bounds.reduced(1.0f), 2.0f);
    }

    // Drag handle dots. Tile-wide dim for hidden rows is applied in
    // paintOverChildren() — Graphics opacity set here would not affect child
    // labels/buttons that paint in their own paint() pass.
    auto const dragArea = bounds.withWidth(static_cast<float>(DRAG_HANDLE_WIDTH));
    float const dotAlpha = dragHandleHovered_ ? 0.8f : 0.4f;
    g.setColour(theme.textSecondary.withAlpha(dotAlpha));
    constexpr float dotSize = 3.0f;
    constexpr float dotSpacing = 5.0f;
    float const startX = dragArea.getCentreX() - (dotSpacing / 2.0f);
    float const startY = dragArea.getCentreY() - dotSpacing;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 2; ++col)
            g.fillEllipse(startX + (static_cast<float>(col) * dotSpacing) - (dotSize / 2.0f),
                          startY + (static_cast<float>(row) * dotSpacing) - (dotSize / 2.0f), dotSize, dotSize);
}

void OscillatorListItemComponent::paintOverChildren(juce::Graphics& g)
{
    // paintOverChildren runs after every child component has painted, so a
    // translucent overlay here dims the whole tile — background and children
    // alike. Graphics opacity in paint() cannot achieve this because each
    // child receives a fresh Graphics state.
    if (!isVisible_)
    {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(getLocalBounds().toFloat());
    }
}

void OscillatorListItemComponent::mouseEnter(const juce::MouseEvent& /*event*/)
{
    isHovered_ = true;
    updateVisibility();
}

void OscillatorListItemComponent::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered_ = false;
    dragHandleHovered_ = false;
    updateVisibility();
}

void OscillatorListItemComponent::mouseMove(const juce::MouseEvent& e)
{
    bool const newDragHandleHovered = isInDragZone(e.getPosition());
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
    // Left swatch strip is the colour-picker hit target after the ellipse
    // was removed. Hit zone = 12px from the left edge (4px strip + 8px
    // tolerance) so users can reliably double-click it without aiming
    // exactly at a 4px-wide target.
    constexpr int kSwatchHitWidth = 12;
    if (e.getPosition().getX() < kSwatchHitWidth)
    {
        listeners_.call([this](Listener& l) { l.oscillatorColorConfigRequested(oscillatorId_); });
        return;
    }

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
    // Space/return deliberately not handled — they must pass through to the
    // DAW for transport shortcuts. Selection is mouse-driven.

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

void OscillatorListItemComponent::focusGained(FocusChangeType /*cause*/)
{
    hasFocus_ = true;
    repaint();
}

void OscillatorListItemComponent::focusLost(FocusChangeType /*cause*/)
{
    hasFocus_ = false;
    repaint();
}

} // namespace multiscoper
