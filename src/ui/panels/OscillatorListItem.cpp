/*
    Oscil - Oscillator List Item Implementation
    Compact list item that expands when selected to show controls
*/

#include "ui/panels/OscillatorListItem.h"

#include "core/OscilLog.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/ProcessingModeIcons.h"
#include "ui/theme/Typography.h"

namespace oscil
{

OscillatorListItemComponent::OscillatorListItemComponent(const Oscillator& oscillator,
                                                         IInstanceRegistry& instanceRegistry,
                                                         IThemeService& themeService)
    : ThemedComponent(themeService)
    , oscillatorId_(oscillator.getId())
    , instanceRegistry_(instanceRegistry)
    , displayName_(oscillator.getName())
    , colour_(oscillator.getColour())
    , processingMode_(oscillator.getProcessingMode())
    , isVisible_(oscillator.isVisible())
    , paneId_(oscillator.getPaneId())
{
    // Note: testId is intentionally NOT set here. The owning OscillatorListComponent
    // calls setListIndex(i) after construction, which is the single source of truth
    // for display-position-based test IDs. Child widgets are registered there too.

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

    setupComponents();

    // Initialize colors
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
    onThemeChanged(getTheme());

    // Enable keyboard focus for accessibility
    setWantsKeyboardFocus(true);
}

void OscillatorListItemComponent::setupLabels()
{
    // testId is assigned by setListIndex() after construction — pass empty here
    // and let setListIndex register the correct child test IDs as "<parent>_name".
    nameLabel_ = std::make_unique<InlineEditLabel>(getThemeService());
    nameLabel_->setText(displayName_, false);
    nameLabel_->setPlaceholder("Oscillator name...");
    // Tile treatment uses compact bold name + caption track label (see spec).
    nameLabel_->setFont(Typography::smallBold());
    nameLabel_->setTextJustification(juce::Justification::bottomLeft);
    nameLabel_->onTextChanged = [this](const juce::String& newName) {
        displayName_ = newName;
        listeners_.call([this, newName](Listener& l) { l.oscillatorNameChanged(oscillatorId_, newName); });
    };
    nameLabel_->onMouseDown = [this](const juce::MouseEvent&) {
        listeners_.call([this](Listener& l) { l.oscillatorSelected(oscillatorId_); });
    };
    addAndMakeVisible(*nameLabel_);

    trackLabel_ = std::make_unique<juce::Label>("track", trackName_);
    trackLabel_->setComponentID("trackLabel");
    trackLabel_->setFont(Typography::caption());
    trackLabel_->setJustificationType(juce::Justification::topLeft);
    trackLabel_->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(*trackLabel_);
}

void OscillatorListItemComponent::setupActionButtons()
{
    // testIds for action buttons are assigned by setListIndex() after construction.
    deleteButton_ = std::make_unique<OscilButton>(getThemeService(), "");
    deleteButton_->setVariant(ButtonVariant::Icon);
    deleteButton_->setIconPath(ListItemIcons::createTrashIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    deleteButton_->setTooltip("Delete Oscillator (Delete/Backspace)");
    deleteButton_->onClick = [this]() {
        listeners_.call([this](Listener& l) { l.oscillatorDeleteRequested(oscillatorId_); });
    };
    addChildComponent(*deleteButton_);

    settingsButton_ = std::make_unique<OscilButton>(getThemeService(), "");
    settingsButton_->setVariant(ButtonVariant::Icon);
    settingsButton_->setIconPath(ListItemIcons::createGearIcon(static_cast<float>(ICON_BUTTON_SIZE)));
    settingsButton_->setTooltip("Configure Oscillator (Enter)");
    settingsButton_->onClick = [this]() {
        listeners_.call([this](Listener& l) { l.oscillatorConfigRequested(oscillatorId_); });
    };
    addChildComponent(*settingsButton_);

    visibilityButton_ = std::make_unique<OscilButton>(getThemeService(), "");
    visibilityButton_->setVariant(ButtonVariant::Icon);
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

void OscillatorListItemComponent::setupModeButtons()
{
    // testId for modeButtons_ is assigned by setListIndex() after construction.
    modeButtons_ = std::make_unique<SegmentedButtonBar>(getThemeService());
    modeButtons_->setMinButtonWidth(36);
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(14),
                                    static_cast<int>(ProcessingMode::FullStereo), {},
                                    "Stereo: Display both L/R channels");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(14), static_cast<int>(ProcessingMode::Mono), {},
                                    "Mono: Mix to mono (L+R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createMidIcon(14), static_cast<int>(ProcessingMode::Mid), {},
                                    "Mid: Center content (L+R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createSideIcon(14), static_cast<int>(ProcessingMode::Side), {},
                                    "Side: Stereo difference (L-R)/2");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createLeftIcon(14), static_cast<int>(ProcessingMode::Left), {},
                                    "Left: Left channel only");
    modeButtons_->addButtonWithPath(ProcessingModeIcons::createRightIcon(14), static_cast<int>(ProcessingMode::Right),
                                    {}, "Right: Right channel only");
    modeButtons_->onSelectionChanged = [this](int id) {
        processingMode_ = static_cast<ProcessingMode>(id);
        listeners_.call([this](Listener& l) { l.oscillatorModeChanged(oscillatorId_, processingMode_); });
    };
    addChildComponent(*modeButtons_);
}

void OscillatorListItemComponent::setupComponents()
{
    setupLabels();
    setupActionButtons();
    setupModeButtons();
    updateVisibility();
}

void OscillatorListItemComponent::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

OscillatorListItemComponent::~OscillatorListItemComponent() = default;

void OscillatorListItemComponent::updateVisibility()
{
    // Always show action buttons for accessibility, but dim when not hovered/selected
    // This ensures touch users and keyboard users can discover and use them
    deleteButton_->setVisible(true);
    settingsButton_->setVisible(true);

    // Use alpha to indicate interactivity without hiding
    float const buttonAlpha = (selected_ || isHovered_) ? 1.0f : 0.4f;
    deleteButton_->setAlpha(buttonAlpha);
    settingsButton_->setAlpha(buttonAlpha);

    // Tile treatment: hidden state uses textMuted directly; the tile paint applies a 0.6
    // global opacity so no additional alpha manipulation is needed here.
    const auto& theme = getTheme();

    if (nameLabel_)
    {
        auto const nameColour = !isVisible_ ? theme.textMuted : (selected_ ? theme.textHighlight : theme.textPrimary);
        nameLabel_->setTextColour(nameColour);
    }

    if (trackLabel_)
    {
        auto const trackColour = !isVisible_ ? theme.textMuted : theme.textSecondary;
        trackLabel_->setColour(juce::Label::textColourId, trackColour);
    }

    // Always update button icon and tooltip based on current state
    visibilityButton_->setIconPath(isVisible_
                                       ? ListItemIcons::createEyeOpenIcon(static_cast<float>(ICON_BUTTON_SIZE))
                                       : ListItemIcons::createEyeClosedIcon(static_cast<float>(ICON_BUTTON_SIZE)));
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
    // resized() is safe here in normal use; it is also invoked transitively from
    // the constructor (ctor -> setupComponents -> updateVisibility), where
    // clang-analyzer flags it as a virtual call during construction. Bounds are
    // empty that early so the call is effectively a no-op; suppress the warning.
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
    resized();
}

// paint is in OscillatorListItemPainting.cpp

void OscillatorListItemComponent::resized()
{
    auto bounds = getLocalBounds();

    // Right side buttons
    int const topRowY = 0;
    int const buttonY = topRowY + ((COMPACT_HEIGHT - ICON_BUTTON_SIZE) / 2);

    bounds.removeFromRight(4); // Margin

    deleteButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));
    bounds.removeFromRight(4);

    settingsButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));
    bounds.removeFromRight(4);

    visibilityButton_->setBounds(bounds.removeFromRight(ICON_BUTTON_SIZE).withY(buttonY).withHeight(ICON_BUTTON_SIZE));

    // Text Area (Left side)
    // Bounds start from 0, but we have drag handle (24) + margin (4) + indicator (14) + margin (10)
    int const textX = DRAG_HANDLE_WIDTH + 4 + COLOR_INDICATOR_SIZE + 10;
    int const textW = bounds.getWidth() - textX; // remaining width after buttons removed from right
    auto topH = selected_ ? static_cast<float>(COMPACT_HEIGHT) : static_cast<float>(getHeight());

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

// paint(), mouseEnter/Exit/Move/Down/Up/Drag/DoubleClick, keyPressed, focusGained/Lost
// are implemented in OscillatorListItemPainting.cpp

bool OscillatorListItemComponent::isInDragZone(const juce::Point<int>& pos) const { return pos.x < DRAG_HANDLE_WIDTH; }

void OscillatorListItemComponent::onThemeChanged(const ColorTheme& newTheme)
{
    if (nameLabel_)
    {
        auto const nameColour =
            !isVisible_ ? newTheme.textMuted : (selected_ ? newTheme.textHighlight : newTheme.textPrimary);
        nameLabel_->setTextColour(nameColour);
    }

    if (trackLabel_)
    {
        auto const trackColour = !isVisible_ ? newTheme.textMuted : newTheme.textSecondary;
        trackLabel_->setColour(juce::Label::textColourId, trackColour);
    }
}

void OscillatorListItemComponent::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;

        // Update label colors for selection state. updateVisibility() handles both
        // selected and hidden colour resolution via textMuted/textHighlight/textPrimary.
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
    if (nameLabel_)
        nameLabel_->setText(displayName_, false);
    if (trackLabel_)
        trackLabel_->setText(trackName_, juce::dontSendNotification);

    // Note: testId is intentionally NOT updated here. The owning OscillatorListComponent
    // calls setListIndex() after a refresh, which is the single source of truth for
    // display-position-based test IDs.

    updateVisibility();
}

void OscillatorListItemComponent::setListIndex(int index)
{
    juce::String const newTestId = "sidebar_oscillators_item_" + juce::String(index);
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

void OscillatorListItemComponent::addListener(Listener* listener) { listeners_.add(listener); }

void OscillatorListItemComponent::removeListener(Listener* listener) { listeners_.remove(listener); }

} // namespace oscil
