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


void OscillatorListItemComponent::setupLabels()
{
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
    trackLabel_->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(*trackLabel_);
}

void OscillatorListItemComponent::setupActionButtons(const juce::String& suffix)
{
    deleteButton_ = std::make_unique<OscilButton>(themeService_, "");
    deleteButton_->setVariant(ButtonVariant::Icon);
    deleteButton_->setIconPath(ListItemIcons::createTrashIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    deleteButton_->setTooltip("Delete Oscillator (Delete/Backspace)");
    if (suffix.isNotEmpty()) deleteButton_->setTestId(getTestId() + "_delete");
    deleteButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); }); };
    addChildComponent(*deleteButton_);

    settingsButton_ = std::make_unique<OscilButton>(themeService_, "");
    settingsButton_->setVariant(ButtonVariant::Icon);
    settingsButton_->setIconPath(ListItemIcons::createGearIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    settingsButton_->setTooltip("Configure Oscillator (Enter)");
    if (suffix.isNotEmpty()) settingsButton_->setTestId(getTestId() + "_settings");
    settingsButton_->onClick = [this]() { listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); }); };
    addChildComponent(*settingsButton_);

    visibilityButton_ = std::make_unique<OscilButton>(themeService_, "");
    visibilityButton_->setVariant(ButtonVariant::Icon);
    if (suffix.isNotEmpty()) visibilityButton_->setTestId(getTestId() + "_vis_btn");
    visibilityButton_->onClick = [this]() {
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
}

void OscillatorListItemComponent::setupModeButtons(const juce::String& suffix)
{
    modeButtons_ = std::make_unique<SegmentedButtonBar>(themeService_);
    modeButtons_->setMinButtonWidth(36);
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
        OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, getTestId() + "_mode");
    modeButtons_->onSelectionChanged = [this](int id) {
        processingMode_ = static_cast<ProcessingMode>(id);
        listeners_.call([this](Listener& l) { l.oscillatorModeChanged(oscillatorId_, processingMode_); });
    };
    addChildComponent(*modeButtons_);
}

void OscillatorListItemComponent::setupComponents(int orderIndex)
{
    juce::String suffix = juce::String(orderIndex);
    setupLabels();
    setupActionButtons(suffix);
    setupModeButtons(suffix);
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

// paint is in OscillatorListItemPainting.cpp

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
    nameLabel_->setBounds(textX, selected_ ? 0 : 0, textW, static_cast<int>(topH * 0.55f));
    
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

// mouseEnter, mouseExit, mouseMove, mouseDown, mouseDrag, mouseDoubleClick,
// mouseUp, keyPressed, focusGained, focusLost are in OscillatorListItemPainting.cpp

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

    // Re-register test IDs if order index changed (list rebuild after deletion)
    int newOrder = oscillator.getOrderIndex();
    juce::String newTestId = "sidebar_oscillators_item_" + juce::String(newOrder);
    juce::Logger::writeToLog("[ListItem] updateFromOscillator: name=" + displayName_
        + " oldTestId=" + getTestId() + " newTestId=" + newTestId
        + " orderIndex=" + juce::String(newOrder));
    if (newTestId != getTestId())
    {
        juce::Logger::writeToLog("[ListItem] Re-registering test IDs: " + getTestId() + " -> " + newTestId);
        setTestId(newTestId);
        juce::String suffix = juce::String(newOrder);
        deleteButton_->setTestId(newTestId + "_delete");
        juce::Logger::writeToLog("[ListItem] deleteButton testId now: " + deleteButton_->getTestId());
        settingsButton_->setTestId(newTestId + "_settings");
        visibilityButton_->setTestId(newTestId + "_vis_btn");
        OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, newTestId + "_mode");
        nameLabel_->setTestId(newTestId + "_name");
    }

    updateVisibility();
}


void OscillatorListItemComponent::setListIndex(int index)
{
    juce::String newTestId = "sidebar_oscillators_item_" + juce::String(index);
    if (newTestId != getTestId())
    {
        setTestId(newTestId);
        deleteButton_->setTestId(newTestId + "_delete");
        settingsButton_->setTestId(newTestId + "_settings");
        visibilityButton_->setTestId(newTestId + "_vis_btn");
        OSCIL_REGISTER_CHILD_TEST_ID(*modeButtons_, newTestId + "_mode");
        nameLabel_->setTestId(newTestId + "_name");
    }
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
