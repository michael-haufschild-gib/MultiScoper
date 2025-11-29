/*
    Oscil - Oscillator List Item Implementation
    Compact list item that expands when selected to show controls
*/

#include "ui/OscillatorListItem.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/components/ListItemIcons.h"
#include "core/InstanceRegistry.h"

namespace oscil
{

OscillatorListItemComponent::OscillatorListItemComponent(const Oscillator& oscillator)
    : oscillatorId_(oscillator.getId())
    , displayName_(oscillator.getName())
    , colour_(oscillator.getColour())
    , processingMode_(oscillator.getProcessingMode())
    , isVisible_(oscillator.isVisible())
{
    // Register test ID with oscillator index
    juce::String testId = "sidebar_oscillators_item_" + juce::String(oscillator.getOrderIndex());
    setTestId(testId);

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

    setupComponents(oscillator.getOrderIndex());

    // Enable keyboard focus for accessibility
    setWantsKeyboardFocus(true);
}

void OscillatorListItemComponent::setupComponents(int orderIndex)
{
    juce::String suffix = juce::String(orderIndex);

    // Delete button with trash icon
    deleteButton_ = std::make_unique<OscilButton>("");
    deleteButton_->setVariant(ButtonVariant::Icon);
    deleteButton_->setIconPath(ListItemIcons::createTrashIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    deleteButton_->setTooltip("Delete Oscillator (Delete/Backspace)");
    if (suffix.isNotEmpty()) deleteButton_->setTestId(getTestId() + "_delete");
    deleteButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); }); };
    addChildComponent(*deleteButton_);

    // Settings button with gear icon
    settingsButton_ = std::make_unique<OscilButton>("");
    settingsButton_->setVariant(ButtonVariant::Icon);
    settingsButton_->setIconPath(ListItemIcons::createGearIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    settingsButton_->setTooltip("Configure Oscillator (Enter)");
    if (suffix.isNotEmpty()) settingsButton_->setTestId(getTestId() + "_settings");
    settingsButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); }); };
    addChildComponent(*settingsButton_);

    // Visibility button (Compact mode) - eye icon
    visibilityButton_ = std::make_unique<OscilButton>("");
    visibilityButton_->setVariant(ButtonVariant::Icon);
    visibilityButton_->setIconPath(isVisible_ ?
        ListItemIcons::createEyeOpenIcon(static_cast<float>(ICON_BUTTON_SIZE)) :
        ListItemIcons::createEyeClosedIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    if (suffix.isNotEmpty()) visibilityButton_->setTestId(getTestId() + "_vis_btn");
    visibilityButton_->onClick = [this]() {
        isVisible_ = !isVisible_;
        updateVisibility();
        listeners_.call([this](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId_, isVisible_); });
    };
    addChildComponent(*visibilityButton_);

    // Visibility toggle (Expanded mode)
    visibilityToggle_ = std::make_unique<OscilToggle>("Visible");
    if (suffix.isNotEmpty()) visibilityToggle_->setTestId(getTestId() + "_vis_toggle");
    visibilityToggle_->onValueChanged = [this](bool visible) {
        isVisible_ = visible;
        updateVisibility();
        listeners_.call([this](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId_, isVisible_); });
    };
    addChildComponent(*visibilityToggle_);

    // Mode buttons - using icons for Stereo/Mono, short text for others
    modeButtons_ = std::make_unique<SegmentedButtonBar>();
    modeButtons_->setMinButtonWidth(36);  // Compact buttons for 6 modes
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(14), static_cast<int>(ProcessingMode::FullStereo));
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(14), static_cast<int>(ProcessingMode::Mono));
    modeButtons_->addButton("M", static_cast<int>(ProcessingMode::Mid));
    modeButtons_->addButton("S", static_cast<int>(ProcessingMode::Side));
    modeButtons_->addButton("L", static_cast<int>(ProcessingMode::Left));
    modeButtons_->addButton("R", static_cast<int>(ProcessingMode::Right));
    if (suffix.isNotEmpty()) 
    {
        // SegmentedButtonBar doesn't easily support setting IDs for internal buttons via this API
        // But we can register the bar itself
        OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, getTestId() + "_mode");
    }
    modeButtons_->onSelectionChanged = [this](int id) {
        processingMode_ = static_cast<ProcessingMode>(id);
        listeners_.call([this](Listener& l) { l.oscillatorModeChanged(oscillatorId_, processingMode_); });
    };
    addChildComponent(*modeButtons_);

    updateVisibility();
}

void OscillatorListItemComponent::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscillatorListItemComponent::~OscillatorListItemComponent()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscillatorListItemComponent::updateVisibility()
{
    // Always show action buttons for accessibility, but dim when not hovered/selected
    // This ensures touch users and keyboard users can discover and use them
    deleteButton_->setVisible(true);
    settingsButton_->setVisible(true);

    // Use alpha to indicate interactivity without hiding
    float buttonAlpha = (selected_ || isHovered_) ? 1.0f : 0.4f;
    deleteButton_->setAlpha(buttonAlpha);
    settingsButton_->setAlpha(buttonAlpha);

    // Visibility controls
    if (selected_)
    {
        visibilityButton_->setVisible(false);
        visibilityToggle_->setVisible(true);
        visibilityToggle_->setValue(isVisible_, false);
        modeButtons_->setVisible(true);
        modeButtons_->setSelectedId(static_cast<int>(processingMode_));
    }
    else
    {
        // In compact mode, always show visibility button so users can toggle visibility
        // Update button icon and tooltip based on current state
        visibilityButton_->setIconPath(isVisible_ ?
            ListItemIcons::createEyeOpenIcon(static_cast<float>(ICON_BUTTON_SIZE)) :
            ListItemIcons::createEyeClosedIcon(static_cast<float>(ICON_BUTTON_SIZE)));
        visibilityButton_->setTooltip(isVisible_ ? "Hide Oscillator (V)" : "Show Oscillator (V)");
        visibilityButton_->setVisible(true);
        visibilityToggle_->setVisible(false);
        modeButtons_->setVisible(false);
    }
    
    repaint();
    resized();
}

void OscillatorListItemComponent::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();
    float alpha = isVisible_ ? 1.0f : 0.5f;

    // Background
    if (selected_)
    {
        g.setColour(theme.backgroundSecondary.brighter(0.05f));
        g.fillRoundedRectangle(bounds.reduced(2), 8.0f);
        
        g.setColour(hasFocus_ ? theme.controlActive.withAlpha(0.8f) : theme.controlBorder.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(2), 8.0f, hasFocus_ ? 2.0f : 1.0f);
    }
    else
    {
        if (isHovered_)
        {
            g.setColour(theme.controlHighlight.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds.reduced(2), 6.0f);
        }
        
        if (hasFocus_)
        {
            g.setColour(theme.controlActive.withAlpha(0.6f));
            g.drawRoundedRectangle(bounds.reduced(2), 6.0f, 2.0f);
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
            g.fillEllipse(startX + col * dotSpacing - dotSize/2, startY + row * dotSpacing - dotSize/2, dotSize, dotSize);

    // Color indicator
    bounds.removeFromLeft(4);
    float colorY = selected_ ? (COMPACT_HEIGHT / 2.0f - COLOR_INDICATOR_SIZE / 2.0f) : (bounds.getCentreY() - COLOR_INDICATOR_SIZE / 2.0f);
    g.setColour(colour_.withAlpha(alpha));
    g.fillEllipse(bounds.getX(), colorY, static_cast<float>(COLOR_INDICATOR_SIZE), static_cast<float>(COLOR_INDICATOR_SIZE));
    
    // Text
    float textX = bounds.getX() + COLOR_INDICATOR_SIZE + 10;
    float textW = bounds.getWidth() - (ICON_BUTTON_SIZE * 2 + 8) - (COLOR_INDICATOR_SIZE + 10); // Approx space
    float topH = selected_ ? COMPACT_HEIGHT : bounds.getHeight();
    
    auto nameBounds = juce::Rectangle<float>(textX, selected_ ? 0 : 0, textW, topH * 0.55f);
    auto trackBounds = juce::Rectangle<float>(textX, nameBounds.getBottom(), textW, topH * 0.45f);
    
    g.setColour((selected_ ? theme.textHighlight : theme.textPrimary).withAlpha(alpha));
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(displayName_, nameBounds, juce::Justification::bottomLeft, true);

    g.setColour(theme.textSecondary.withAlpha(alpha));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(trackName_, trackBounds, juce::Justification::topLeft, true);
}

void OscillatorListItemComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Right side buttons
    int topRowY = 0;
    int buttonY = topRowY + (COMPACT_HEIGHT - ICON_BUTTON_SIZE) / 2;
    
    bounds.removeFromRight(4); // Margin
    
    deleteButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));
    bounds.removeFromRight(4);
    
    settingsButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));
    bounds.removeFromRight(4);
    
    if (visibilityButton_->isVisible())
    {
        visibilityButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));
    }
    
    if (selected_)
    {
        auto bottomRow = getLocalBounds().withTop(COMPACT_HEIGHT).reduced(DRAG_HANDLE_WIDTH + 4, 4);
        
        // Mode buttons (6 buttons x ~40px each)
        modeButtons_->setBounds(bottomRow.removeFromLeft(240).withHeight(28));
        
        // Toggle
        bottomRow.removeFromLeft(16);
        visibilityToggle_->setBounds(bottomRow.removeFromLeft(80).withHeight(28));
    }
}

// ... (Remove custom paint methods: paintCompact, paintExpanded, paintModeButton, paintVisibilityToggle, paintIconButton)
// ... (Remove hit test methods: isInSettingsButton, isInDeleteButton, isInVisibilityToggle, isInModeButton)

// Update mouseUp to remove custom hit testing logic, now handled by child components
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

void OscillatorListItemComponent::mouseDown(const juce::MouseEvent& e)
{
    dragStartPos_ = e.getPosition();
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

void OscillatorListItemComponent::themeChanged(const ColorTheme&)
{
    repaint();
}

void OscillatorListItemComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging_ = false;
    auto pos = e.getPosition();

    // Drag logic remains
    
    // Select item if not in drag zone and not clicking child components (child components consume click usually, but if they are ignored?)
    // JUCE handles child clicks before parent mouseUp if children intercept.
    // So if we are here, we clicked background.
    if (!isInDragZone(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
    }
}

void OscillatorListItemComponent::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        updateVisibility(); // Triggers repaint and resize
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
    
    updateVisibility();
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
        isVisible_ = !isVisible_;
        updateVisibility();  // Must call updateVisibility to sync child components
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

void OscillatorListItemComponent::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OscillatorListItemComponent::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

} // namespace oscil
