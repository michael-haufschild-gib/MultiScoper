/*
    Oscil - Oscillator List Item Implementation
    Compact list item that expands when selected to show controls
*/

#include "ui/panels/OscillatorListItem.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/InlineEditLabel.h"
#include "core/interfaces/IInstanceRegistry.h"

namespace oscil
{

OscillatorListItemComponent::OscillatorListItemComponent(const Oscillator& oscillator, IInstanceRegistry& instanceRegistry, IThemeService& themeService)
    : oscillatorId_(oscillator.getId())
    , instanceRegistry_(instanceRegistry)
    , themeService_(themeService)
    , displayName_(oscillator.getName())
    , colour_(oscillator.getColour())
    , processingMode_(oscillator.getProcessingMode())
    , isVisible_(oscillator.isVisible())
    , paneId_(oscillator.getPaneId())
{
    // Register test ID with oscillator index
    juce::String testId = "sidebar_oscillators_item_" + juce::String(oscillator.getOrderIndex());
    setTestId(testId);

    // Get track name from source registry
    if (oscillator.getSourceId().isValid())
    {
        auto sourceInfo = instanceRegistry_.getSource(oscillator.getSourceId());
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

    themeService_.addListener(this);

    setupComponents(oscillator.getOrderIndex());

    // Initialize colors
    themeChanged(themeService_.getCurrentTheme());

    // Enable keyboard focus for accessibility
    setWantsKeyboardFocus(true);
}


void OscillatorListItemComponent::setupComponents(int orderIndex)
{
    juce::String suffix = juce::String(orderIndex);

    // Name label (inline editable)
    nameLabel_ = std::make_unique<InlineEditLabel>(themeService_, getTestId() + "_name");
    nameLabel_->setText(displayName_, false);
    nameLabel_->setPlaceholder("Oscillator name...");
    nameLabel_->setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_DEFAULT).withStyle("Bold"));
    nameLabel_->setTextJustification(juce::Justification::bottomLeft);
    nameLabel_->onTextChanged = [this](const juce::String& newName) {
        displayName_ = newName;
        listeners_.call([this, &newName](Listener& l) { l.oscillatorNameChanged(oscillatorId_, newName); });
    };
    nameLabel_->onMouseDown = [this](const juce::MouseEvent&) {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
    };
    addAndMakeVisible(*nameLabel_);

    trackLabel_ = std::make_unique<juce::Label>("track", trackName_);
    trackLabel_->setComponentID("trackLabel");
    trackLabel_->setFont(juce::FontOptions(ComponentLayout::FONT_SIZE_SMALL));
    trackLabel_->setJustificationType(juce::Justification::topLeft);
    trackLabel_->setInterceptsMouseClicks(false, false); // Let clicks pass to item
    addAndMakeVisible(*trackLabel_);

    // Delete button with trash icon
    deleteButton_ = std::make_unique<OscilButton>(themeService_, "");
    deleteButton_->setVariant(ButtonVariant::Icon);
    deleteButton_->setIconPath(ListItemIcons::createTrashIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    deleteButton_->setTooltip("Delete Oscillator (Delete/Backspace)");
    if (suffix.isNotEmpty()) deleteButton_->setTestId(getTestId() + "_delete");
    deleteButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); }); };
    addChildComponent(*deleteButton_);

    // Settings button with gear icon
    settingsButton_ = std::make_unique<OscilButton>(themeService_, "");
    settingsButton_->setVariant(ButtonVariant::Icon);
    settingsButton_->setIconPath(ListItemIcons::createGearIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    settingsButton_->setTooltip("Configure Oscillator (Enter)");
    if (suffix.isNotEmpty()) settingsButton_->setTestId(getTestId() + "_settings");
    settingsButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); }); };
    addChildComponent(*settingsButton_);

    // Visibility button - eye icon
    visibilityButton_ = std::make_unique<OscilButton>(themeService_, "");
    visibilityButton_->setVariant(ButtonVariant::Icon);
    if (suffix.isNotEmpty()) visibilityButton_->setTestId(getTestId() + "_vis_btn");
    visibilityButton_->onClick = [this]() {
        // If trying to make visible but no valid pane, request pane selection
        if (!isVisible_ && !paneId_.isValid())
        {
            listeners_.call([this](Listener& l) { l.oscillatorPaneSelectionRequested(oscillatorId_); });
            return;
        }
        isVisible_ = !isVisible_;
        updateVisibility();
        listeners_.call([this](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId_, isVisible_); });
    };
    addChildComponent(*visibilityButton_);

    // Mode buttons - using icons for Stereo/Mono, short text for others
    // Tooltips help users understand what each processing mode does
    modeButtons_ = std::make_unique<SegmentedButtonBar>(themeService_);
    modeButtons_->setMinButtonWidth(36);  // Compact buttons for 6 modes
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(14),
        static_cast<int>(ProcessingMode::FullStereo), {}, "Stereo: Display both L/R channels");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(14),
        static_cast<int>(ProcessingMode::Mono), {}, "Mono: Mix to mono (L+R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMidIcon(14),
        static_cast<int>(ProcessingMode::Mid), {}, "Mid: Center content (L+R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createSideIcon(14),
        static_cast<int>(ProcessingMode::Side), {}, "Side: Stereo difference (L-R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createLeftIcon(14),
        static_cast<int>(ProcessingMode::Left), {}, "Left: Left channel only");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createRightIcon(14),
        static_cast<int>(ProcessingMode::Right), {}, "Right: Right channel only");
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
    themeService_.removeListener(this);
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
    
    // Update label alpha based on visibility
    const auto& theme = themeService_.getCurrentTheme();
    float alpha = isVisible_ ? 1.0f : 0.5f;

    if (nameLabel_)
    {
        nameLabel_->setTextColour((selected_ ? theme.textHighlight : theme.textPrimary).withAlpha(alpha));
    }

    if (trackLabel_)
    {
        trackLabel_->setColour(juce::Label::textColourId,
            theme.textSecondary.withAlpha(alpha));
    }

    // Always update button icon and tooltip based on current state
    visibilityButton_->setIconPath(isVisible_ ?
        ListItemIcons::createEyeOpenIcon(static_cast<float>(ICON_BUTTON_SIZE)) :
        ListItemIcons::createEyeClosedIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    visibilityButton_->setTooltip(isVisible_ ? "Hide Oscillator (V)" : "Show Oscillator (V)");
    visibilityButton_->setVisible(true);

    // Visibility controls
    if (selected_)
    {
        modeButtons_->setVisible(true);
        modeButtons_->setSelectedId(static_cast<int>(processingMode_));
    }
    else
    {
        modeButtons_->setVisible(false);
    }
    
    repaint();
    resized();
}

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
            g.fillEllipse(startX + col * dotSpacing - dotSize/2, startY + row * dotSpacing - dotSize/2, dotSize, dotSize);

    // Color indicator
    bounds.removeFromLeft(4);
    float colorY = selected_ ? (COMPACT_HEIGHT / 2.0f - COLOR_INDICATOR_SIZE / 2.0f) : (bounds.getCentreY() - COLOR_INDICATOR_SIZE / 2.0f);
    g.setColour(colour_.withAlpha(alpha));
    g.fillEllipse(bounds.getX(), colorY, static_cast<float>(COLOR_INDICATOR_SIZE), static_cast<float>(COLOR_INDICATOR_SIZE));
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
    
    visibilityButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));

    // Text Area (Left side)
    // Bounds start from 0, but we have drag handle (24) + margin (4) + indicator (14) + margin (10)
    int textX = DRAG_HANDLE_WIDTH + 4 + COLOR_INDICATOR_SIZE + 10;
    int textW = bounds.getWidth() - textX; // remaining width after buttons removed from right
    float topH = selected_ ? COMPACT_HEIGHT : getHeight();
    
    // nameLabel_ takes top 55%
    nameLabel_->setBounds(textX, 0, textW, static_cast<int>(topH * 0.55f));
    
    // trackLabel_ takes bottom 45%
    trackLabel_->setBounds(textX, nameLabel_->getBottom(), textW, static_cast<int>(topH * 0.45f));
    
    if (selected_)
    {
        auto bottomRow = getLocalBounds().withTop(COMPACT_HEIGHT).reduced(DRAG_HANDLE_WIDTH + 4, 4);
        
        // Mode buttons (6 buttons x ~40px each)
        modeButtons_->setBounds(bottomRow.removeFromLeft(240).withHeight(28));
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

void OscillatorListItemComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Check if double click is on color indicator
    // Logic mirrors paint() positioning
    auto pos = e.getPosition();
    
    // Drag handle is on left (width 24)
    // Then 4px margin
    // Then color indicator (size 14)
    
    int indicatorX = DRAG_HANDLE_WIDTH + 4;
    int indicatorY = selected_ ? (COMPACT_HEIGHT - COLOR_INDICATOR_SIZE) / 2 
                               : (getHeight() - COLOR_INDICATOR_SIZE) / 2;
    
    auto indicatorBounds = juce::Rectangle<int>(indicatorX, indicatorY, COLOR_INDICATOR_SIZE, COLOR_INDICATOR_SIZE);
    
    // Expand hit target slightly for better usability
    if (indicatorBounds.expanded(4).contains(pos))
    {
        listeners_.call([this](Listener& l) { l.oscillatorColorConfigRequested(oscillatorId_); });
        return;
    }

    // Otherwise treat as selection (already handled by mouseDown/Up but double click might need specific handling if we wanted to open config)
    // For now, just select
    listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); });
}

bool OscillatorListItemComponent::isInDragZone(const juce::Point<int>& pos) const
{
    return pos.x < DRAG_HANDLE_WIDTH;
}

void OscillatorListItemComponent::themeChanged(const ColorTheme&)
{
    const auto& theme = themeService_.getCurrentTheme();
    float alpha = isVisible_ ? 1.0f : 0.5f;

    if (nameLabel_)
    {
        nameLabel_->setTextColour((selected_ ? theme.textHighlight : theme.textPrimary).withAlpha(alpha));
    }

    if (trackLabel_)
    {
        trackLabel_->setColour(juce::Label::textColourId,
            theme.textSecondary.withAlpha(alpha));
    }

    repaint();
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

void OscillatorListItemComponent::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;

        // Update label colors for selection state
        const auto& theme = themeService_.getCurrentTheme();
        float alpha = isVisible_ ? 1.0f : 0.5f;
        if (nameLabel_)
        {
            nameLabel_->setTextColour((selected_ ? theme.textHighlight : theme.textPrimary).withAlpha(alpha));
        }

        updateVisibility(); // Triggers repaint and resize
    }
}

void OscillatorListItemComponent::updateFromOscillator(const Oscillator& oscillator)
{
    displayName_ = oscillator.getName();
    colour_ = oscillator.getColour();
    processingMode_ = oscillator.getProcessingMode();
    isVisible_ = oscillator.isVisible();
    paneId_ = oscillator.getPaneId();

    if (oscillator.getSourceId().isValid())
    {
        auto sourceInfo = instanceRegistry_.getSource(oscillator.getSourceId());
        if (sourceInfo.has_value())
        {
            trackName_ = sourceInfo->name;
        }
    }

    // Update labels
    if (nameLabel_) nameLabel_->setText(displayName_, false);
    if (trackLabel_) trackLabel_->setText(trackName_, juce::dontSendNotification);

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
        // If trying to make visible but no valid pane, request pane selection
        if (!isVisible_ && !paneId_.isValid())
        {
            listeners_.call([this](Listener& l) { l.oscillatorPaneSelectionRequested(oscillatorId_); });
            return true;
        }
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
