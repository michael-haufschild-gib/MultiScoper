/*
    Oscil - Oscillator List Item Implementation
    Compact list item that expands when selected to show controls
*/

#include "ui/OscillatorListItem.h"
#include "core/InstanceRegistry.h"
#include <iostream>

namespace oscil
{

OscillatorListItemComponent::OscillatorListItemComponent(const Oscillator& oscillator)
    : oscillatorId_(oscillator.getId())
    , displayName_(oscillator.getName())
    , colour_(oscillator.getColour())
    , processingMode_(oscillator.getProcessingMode())
    , isVisible_(oscillator.isVisible())
{
    // Get track name from source registry
    if (oscillator.getSourceId().isValid())
    {
        auto sourceInfo = InstanceRegistry::getInstance().getSource(oscillator.getSourceId());
        if (sourceInfo.has_value())
        {
            trackName_ = sourceInfo->name;
        }
        else
        {
            trackName_ = "Track";
        }
    }
    else
    {
        trackName_ = "Self";
    }

    ThemeManager::getInstance().addListener(this);

    // Enable keyboard focus for accessibility
    setWantsKeyboardFocus(true);
}

OscillatorListItemComponent::~OscillatorListItemComponent()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscillatorListItemComponent::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    if (selected_)
    {
        paintExpanded(g, theme);
    }
    else
    {
        paintCompact(g, theme);
    }
}

void OscillatorListItemComponent::paintCompact(juce::Graphics& g, const ColorTheme& theme)
{
    auto bounds = getLocalBounds().toFloat();
    float alpha = isVisible_ ? 1.0f : 0.5f;

    // Background for hover/selection
    if (isHovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(2), 6.0f);
    }

    // Focus ring for keyboard accessibility
    if (hasFocus_)
    {
        g.setColour(theme.controlActive.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds.reduced(2), 6.0f, 2.0f);
    }

    // Drag handle (6 dots) - with hover feedback
    auto dragArea = bounds.removeFromLeft(DRAG_HANDLE_WIDTH);
    float dotAlpha = dragHandleHovered_ ? 0.8f : 0.4f;
    g.setColour(theme.textSecondary.withAlpha(dotAlpha * alpha));
    float dotSize = 3.0f;
    float dotSpacing = 5.0f;
    float startX = dragArea.getCentreX() - dotSpacing / 2;
    float startY = dragArea.getCentreY() - dotSpacing;
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            float x = startX + col * dotSpacing;
            float y = startY + row * dotSpacing;
            g.fillEllipse(x - dotSize / 2, y - dotSize / 2, dotSize, dotSize);
        }
    }

    // Color indicator
    bounds.removeFromLeft(4);
    float colorY = bounds.getCentreY() - COLOR_INDICATOR_SIZE / 2.0f;
    g.setColour(colour_.withAlpha(alpha));
    g.fillEllipse(bounds.getX(), colorY,
                  static_cast<float>(COLOR_INDICATOR_SIZE),
                  static_cast<float>(COLOR_INDICATOR_SIZE));
    bounds.removeFromLeft(COLOR_INDICATOR_SIZE + 10);

    // Right side buttons (from right to left)
    auto rightArea = bounds.removeFromRight(ICON_BUTTON_SIZE * 2 + 8);

    // Delete button
    auto deleteBounds = rightArea.removeFromRight(ICON_BUTTON_SIZE);
    paintIconButton(g, deleteBounds, "trash", deleteHovered_, theme, true);

    // Settings button
    auto settingsBounds = rightArea.removeFromRight(ICON_BUTTON_SIZE);
    paintIconButton(g, settingsBounds, "gear", settingsHovered_, theme);

    // If hidden, show eye-slash icon
    if (!isVisible_)
    {
        auto eyeBounds = rightArea.removeFromRight(ICON_BUTTON_SIZE);
        paintIconButton(g, eyeBounds, "eye-slash", false, theme);
    }

    // Text area
    bounds.removeFromRight(8);

    // Display name (bold, larger)
    g.setColour(theme.textPrimary.withAlpha(alpha));
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    auto nameBounds = bounds.removeFromTop(bounds.getHeight() * 0.55f);
    g.drawText(displayName_, nameBounds.toNearestInt(), juce::Justification::bottomLeft, true);

    // Track name (smaller, secondary)
    g.setColour(theme.textSecondary.withAlpha(alpha));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(trackName_, bounds.toNearestInt(), juce::Justification::topLeft, true);
}

void OscillatorListItemComponent::paintExpanded(juce::Graphics& g, const ColorTheme& theme)
{
    auto bounds = getLocalBounds().toFloat();

    // Selected background
    g.setColour(theme.backgroundSecondary.brighter(0.05f));
    g.fillRoundedRectangle(bounds.reduced(2), 8.0f);

    // Border (use focus ring color if focused)
    if (hasFocus_)
    {
        g.setColour(theme.controlActive.withAlpha(0.8f));
        g.drawRoundedRectangle(bounds.reduced(2), 8.0f, 2.0f);
    }
    else
    {
        g.setColour(theme.controlBorder.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(2), 8.0f, 1.0f);
    }

    // Top row (same as compact but with different background)
    auto topRow = bounds.removeFromTop(COMPACT_HEIGHT);

    // Drag handle - with hover feedback
    auto dragArea = topRow.removeFromLeft(DRAG_HANDLE_WIDTH);
    float dotAlpha = dragHandleHovered_ ? 0.8f : 0.4f;
    g.setColour(theme.textSecondary.withAlpha(dotAlpha));
    float dotSize = 3.0f;
    float dotSpacing = 5.0f;
    float startX = dragArea.getCentreX() - dotSpacing / 2;
    float startY = dragArea.getCentreY() - dotSpacing;
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            float x = startX + col * dotSpacing;
            float y = startY + row * dotSpacing;
            g.fillEllipse(x - dotSize / 2, y - dotSize / 2, dotSize, dotSize);
        }
    }

    // Color indicator
    topRow.removeFromLeft(4);
    float colorY = topRow.getCentreY() - COLOR_INDICATOR_SIZE / 2.0f;
    g.setColour(colour_);
    g.fillEllipse(topRow.getX(), colorY,
                  static_cast<float>(COLOR_INDICATOR_SIZE),
                  static_cast<float>(COLOR_INDICATOR_SIZE));
    topRow.removeFromLeft(COLOR_INDICATOR_SIZE + 10);

    // Right side buttons
    auto rightArea = topRow.removeFromRight(ICON_BUTTON_SIZE * 2 + 8);

    auto deleteBounds = rightArea.removeFromRight(ICON_BUTTON_SIZE);
    paintIconButton(g, deleteBounds, "trash", deleteHovered_, theme, true);

    auto settingsBounds = rightArea.removeFromRight(ICON_BUTTON_SIZE);
    paintIconButton(g, settingsBounds, "gear", settingsHovered_, theme);

    // Text
    topRow.removeFromRight(8);
    g.setColour(theme.textHighlight);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    auto nameBounds = topRow.removeFromTop(topRow.getHeight() * 0.55f);
    g.drawText(displayName_, nameBounds.toNearestInt(), juce::Justification::bottomLeft, true);

    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(trackName_, topRow.toNearestInt(), juce::Justification::topLeft, true);

    // Bottom row - mode buttons and visibility toggle
    auto bottomRow = bounds.reduced(DRAG_HANDLE_WIDTH + 4, 4);

    // Mode buttons (segmented control style)
    auto modeArea = bottomRow.removeFromLeft(200);
    float modeButtonWidth = 48.0f;
    float modeButtonHeight = 28.0f;
    float modeY = modeArea.getCentreY() - modeButtonHeight / 2;

    // Draw mode button group background
    auto modeGroupBounds = juce::Rectangle<float>(modeArea.getX(), modeY, modeButtonWidth * 4, modeButtonHeight);
    g.setColour(theme.controlBackground);
    g.fillRoundedRectangle(modeGroupBounds, 4.0f);

    // Draw each mode button
    paintModeButton(g, juce::Rectangle<float>(modeArea.getX(), modeY, modeButtonWidth, modeButtonHeight),
                    "Stereo", processingMode_ == ProcessingMode::FullStereo, theme);
    paintModeButton(g, juce::Rectangle<float>(modeArea.getX() + modeButtonWidth, modeY, modeButtonWidth, modeButtonHeight),
                    "Mono", processingMode_ == ProcessingMode::Mono, theme);
    paintModeButton(g, juce::Rectangle<float>(modeArea.getX() + modeButtonWidth * 2, modeY, modeButtonWidth, modeButtonHeight),
                    "Mid", processingMode_ == ProcessingMode::Mid, theme);
    paintModeButton(g, juce::Rectangle<float>(modeArea.getX() + modeButtonWidth * 3, modeY, modeButtonWidth, modeButtonHeight),
                    "Side", processingMode_ == ProcessingMode::Side, theme);

    // Visibility toggle (on right side)
    bottomRow.removeFromLeft(16);
    auto toggleArea = bottomRow.removeFromLeft(56);
    paintVisibilityToggle(g, toggleArea, theme);
}

void OscillatorListItemComponent::paintModeButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                   const juce::String& text, bool isActive, const ColorTheme& theme)
{
    if (isActive)
    {
        g.setColour(theme.controlActive);
        g.fillRoundedRectangle(bounds.reduced(1), 3.0f);
        g.setColour(theme.textHighlight);
    }
    else
    {
        g.setColour(theme.textSecondary);
    }

    g.setFont(juce::FontOptions(11.0f));
    g.drawText(text, bounds.toNearestInt(), juce::Justification::centred, false);

    // Draw separator line (except for last button)
    if (text != "Side")
    {
        g.setColour(theme.controlBorder.withAlpha(0.3f));
        g.drawVerticalLine(static_cast<int>(bounds.getRight()), bounds.getY() + 4, bounds.getBottom() - 4);
    }
}

void OscillatorListItemComponent::paintVisibilityToggle(juce::Graphics& g, juce::Rectangle<float> bounds, const ColorTheme& theme)
{
    // Draw toggle switch
    float toggleWidth = 44.0f;
    float toggleHeight = 24.0f;
    float toggleX = bounds.getCentreX() - toggleWidth / 2;
    float toggleY = bounds.getCentreY() - toggleHeight / 2;

    auto toggleBounds = juce::Rectangle<float>(toggleX, toggleY, toggleWidth, toggleHeight);

    // Background pill
    g.setColour(isVisible_ ? theme.statusActive : theme.controlBackground);
    g.fillRoundedRectangle(toggleBounds, toggleHeight / 2);

    // Knob
    float knobSize = toggleHeight - 4;
    float knobX = isVisible_ ? (toggleBounds.getRight() - knobSize - 2) : (toggleBounds.getX() + 2);
    float knobY = toggleBounds.getY() + 2;

    g.setColour(juce::Colours::white);
    g.fillEllipse(knobX, knobY, knobSize, knobSize);

    // Eye icon on knob
    g.setColour(isVisible_ ? theme.statusActive : theme.textSecondary);
    g.setFont(juce::FontOptions(11.0f));
    // Simple eye representation
    float eyeX = knobX + knobSize / 2;
    float eyeY = knobY + knobSize / 2;
    float eyeSize = 6.0f;
    g.drawEllipse(eyeX - eyeSize, eyeY - eyeSize / 2, eyeSize * 2, eyeSize, 1.5f);
    g.fillEllipse(eyeX - 2, eyeY - 2, 4, 4);
}

void OscillatorListItemComponent::paintIconButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                   const juce::String& icon, bool isHovered,
                                                   const ColorTheme& theme, bool isDanger)
{
    if (isHovered)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);
    }

    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    if (icon == "gear")
    {
        g.setColour(isHovered ? theme.textHighlight : theme.textSecondary);
        // Simple gear icon
        float size = 8.0f;
        g.drawEllipse(cx - size/2, cy - size/2, size, size, 1.5f);
        // Gear teeth
        for (int i = 0; i < 6; ++i)
        {
            float angle = i * juce::MathConstants<float>::pi / 3;
            float x1 = cx + std::cos(angle) * (size/2 - 1);
            float y1 = cy + std::sin(angle) * (size/2 - 1);
            float x2 = cx + std::cos(angle) * (size/2 + 2);
            float y2 = cy + std::sin(angle) * (size/2 + 2);
            g.drawLine(x1, y1, x2, y2, 2.0f);
        }
    }
    else if (icon == "trash")
    {
        g.setColour(isDanger && isHovered ? juce::Colours::red : (isHovered ? theme.textHighlight : theme.textSecondary));
        float s = 5.0f;
        // Trash can body
        g.drawRoundedRectangle(cx - s, cy - s + 3, s * 2, s * 2 - 2, 1.0f, 1.5f);
        // Lid
        g.drawLine(cx - s - 1, cy - s + 2, cx + s + 1, cy - s + 2, 1.5f);
        // Handle
        g.drawLine(cx - 2, cy - s, cx + 2, cy - s, 1.5f);
        g.drawLine(cx - 2, cy - s, cx - 2, cy - s + 2, 1.0f);
        g.drawLine(cx + 2, cy - s, cx + 2, cy - s + 2, 1.0f);
    }
    else if (icon == "eye-slash")
    {
        g.setColour(theme.textSecondary.withAlpha(0.6f));
        float size = 8.0f;
        g.drawEllipse(cx - size, cy - size / 2, size * 2, size, 1.5f);
        // Slash through
        g.drawLine(cx - size, cy + size/2, cx + size, cy - size/2, 1.5f);
    }
}

void OscillatorListItemComponent::resized()
{
    // No child components to resize - all painting is custom
}

void OscillatorListItemComponent::themeChanged(const ColorTheme&)
{
    repaint();
}

void OscillatorListItemComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void OscillatorListItemComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    settingsHovered_ = false;
    deleteHovered_ = false;
    visibilityHovered_ = false;
    hoveredModeButton_ = -1;
    repaint();
}

void OscillatorListItemComponent::mouseDown(const juce::MouseEvent& e)
{
    dragStartPos_ = e.getPosition();
}

void OscillatorListItemComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging_ = false;
    auto pos = e.getPosition();

    // Check button clicks
    if (isInDeleteButton(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); });
        return;
    }

    if (isInSettingsButton(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); });
        return;
    }

    // Only check expanded controls when selected
    if (selected_)
    {
        if (isInVisibilityToggle(pos))
        {
            isVisible_ = !isVisible_;
            repaint();
            listeners_.call([this](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId_, isVisible_); });
            return;
        }

        ProcessingMode clickedMode;
        if (isInModeButton(pos, clickedMode))
        {
            processingMode_ = clickedMode;
            repaint();
            listeners_.call([this, clickedMode](Listener& l) { l.oscillatorModeChanged(oscillatorId_, clickedMode); });
            return;
        }
    }

    // Select item if not in drag zone
    if (!isInDragZone(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
    }
}

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

bool OscillatorListItemComponent::isInDragZone(const juce::Point<int>& pos) const
{
    return pos.x < DRAG_HANDLE_WIDTH;
}

bool OscillatorListItemComponent::isInSettingsButton(const juce::Point<int>& pos) const
{
    int buttonX = getWidth() - ICON_BUTTON_SIZE * 2 - 4;
    int topRowBottom = selected_ ? COMPACT_HEIGHT : getHeight();
    return pos.x >= buttonX && pos.x < buttonX + ICON_BUTTON_SIZE && pos.y < topRowBottom;
}

bool OscillatorListItemComponent::isInDeleteButton(const juce::Point<int>& pos) const
{
    int buttonX = getWidth() - ICON_BUTTON_SIZE - 4;
    int topRowBottom = selected_ ? COMPACT_HEIGHT : getHeight();
    return pos.x >= buttonX && pos.x < buttonX + ICON_BUTTON_SIZE && pos.y < topRowBottom;
}

bool OscillatorListItemComponent::isInVisibilityToggle(const juce::Point<int>& pos) const
{
    if (!selected_) return false;

    // Toggle is in bottom row, after mode buttons
    int toggleX = DRAG_HANDLE_WIDTH + 4 + 200 + 16;
    int toggleY = COMPACT_HEIGHT + 4;
    int toggleWidth = 56;
    int toggleHeight = EXPANDED_HEIGHT - COMPACT_HEIGHT - 8;

    return pos.x >= toggleX && pos.x < toggleX + toggleWidth &&
           pos.y >= toggleY && pos.y < toggleY + toggleHeight;
}

bool OscillatorListItemComponent::isInModeButton(const juce::Point<int>& pos, ProcessingMode& outMode) const
{
    if (!selected_) return false;

    int modeX = DRAG_HANDLE_WIDTH + 4;
    int modeY = COMPACT_HEIGHT + 4;
    int modeHeight = EXPANDED_HEIGHT - COMPACT_HEIGHT - 8;
    float modeButtonWidth = 48.0f;

    if (pos.y < modeY || pos.y >= modeY + modeHeight) return false;
    if (pos.x < modeX || pos.x >= modeX + modeButtonWidth * 4) return false;

    int buttonIndex = static_cast<int>((pos.x - modeX) / modeButtonWidth);
    switch (buttonIndex)
    {
        case 0: outMode = ProcessingMode::FullStereo; return true;
        case 1: outMode = ProcessingMode::Mono; return true;
        case 2: outMode = ProcessingMode::Mid; return true;
        case 3: outMode = ProcessingMode::Side; return true;
        default: return false;
    }
}

void OscillatorListItemComponent::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        repaint();
    }
}

void OscillatorListItemComponent::updateFromOscillator(const Oscillator& oscillator)
{
    displayName_ = oscillator.getName();
    colour_ = oscillator.getColour();
    processingMode_ = oscillator.getProcessingMode();
    isVisible_ = oscillator.isVisible();

    if (oscillator.getSourceId().isValid())
    {
        auto sourceInfo = InstanceRegistry::getInstance().getSource(oscillator.getSourceId());
        if (sourceInfo.has_value())
        {
            trackName_ = sourceInfo->name;
        }
    }

    repaint();
}

void OscillatorListItemComponent::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OscillatorListItemComponent::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

} // namespace oscil
