/*
    Oscil - Pane Action Bar Implementation
*/

#include "ui/layout/pane/PaneActionBar.h"

#include "ui/components/ListItemIcons.h"

namespace oscil
{

namespace
{
constexpr float kHoldIconSize = 14.0f;

void applyHoldButtonVisuals(OscilButton& button, IThemeService& themeService, bool toggled)
{
    if (toggled)
    {
        button.setIconPath(ListItemIcons::createPlayIcon(kHoldIconSize));
        button.setBorder(themeService.getCurrentTheme().controlActive, 1.0f);
    }
    else
    {
        button.setIconPath(ListItemIcons::createPauseIcon(kHoldIconSize));
        button.setBorder(juce::Colours::transparentBlack, 0.0f);
    }
}
} // namespace

PaneActionBar::PaneActionBar(IThemeService& themeService) : themeService_(themeService) { setupButtons(); }

void PaneActionBar::setupButtons()
{
    // Hold button
    holdButton_ = std::make_unique<OscilButton>(themeService_, "", "pane_holdBtn");
    holdButton_->setVariant(ButtonVariant::Icon);
    holdButton_->setIconPath(ListItemIcons::createPauseIcon(14.0f));
    holdButton_->setTooltip("Hold/Pause display for this pane");
    holdButton_->setToggleable(true);
    applyHoldButtonVisuals(*holdButton_, themeService_, holdButton_->isToggled());
    holdButton_->onToggleStateChanged = [this](bool toggled) {
        applyHoldButtonVisuals(*holdButton_, themeService_, toggled);

        if (onActionTriggered)
            onActionTriggered(PaneAction::ToggleHold, toggled);
    };
    addAndMakeVisible(*holdButton_);

    // Layout buttons
    statsButton_ = std::make_unique<OscilButton>(themeService_, "", "pane_statsBtn");
    statsButton_->setVariant(ButtonVariant::Icon);
    statsButton_->setIconPath(ListItemIcons::createStatsIcon(static_cast<float>(BUTTON_SIZE)));
    statsButton_->setTooltip("Toggle Statistics");
    statsButton_->setToggleable(true);
    statsButton_->onClick = [this]() {
        if (onActionTriggered)
            onActionTriggered(PaneAction::ToggleStats, statsButton_->isToggled());
    };
    addAndMakeVisible(*statsButton_);
}

void PaneActionBar::resized()
{
    auto bounds = getLocalBounds();
    int x = 0;

    int const yPos = juce::jmax(0, (bounds.getHeight() - BUTTON_SIZE) / 2);

    if (holdButton_)
    {
        holdButton_->setBounds(x, yPos, BUTTON_SIZE, BUTTON_SIZE);
        x += BUTTON_SIZE + BUTTON_SPACING;
    }

    if (statsButton_)
    {
        statsButton_->setBounds(x, yPos, BUTTON_SIZE, BUTTON_SIZE);
        x += BUTTON_SIZE + BUTTON_SPACING;
    }

    // Future buttons would be positioned here
}

bool PaneActionBar::isActionToggled(PaneAction action) const
{
    switch (action)
    {
        case PaneAction::ToggleStats:
            return statsButton_ && statsButton_->isToggled();
        case PaneAction::ToggleHold:
            return holdButton_ && holdButton_->isToggled();
        default:
            return false;
    }
}

void PaneActionBar::setActionToggled(PaneAction action, bool toggled)
{
    switch (action)
    {
        case PaneAction::ToggleStats:
            if (statsButton_)
                statsButton_->setToggled(toggled, false);
            break;
        case PaneAction::ToggleHold:
            if (holdButton_)
            {
                holdButton_->setToggled(toggled, false);
                applyHoldButtonVisuals(*holdButton_, themeService_, toggled);
            }
            break;
        default:
            break;
    }
}

void PaneActionBar::setActionEnabled(PaneAction action, bool enabled)
{
    switch (action)
    {
        case PaneAction::ToggleStats:
            if (statsButton_)
                statsButton_->setEnabled(enabled);
            break;
        case PaneAction::ToggleHold:
            if (holdButton_)
                holdButton_->setEnabled(enabled);
            break;
        default:
            break;
    }
}

int PaneActionBar::getPreferredWidth() const
{
    int width = 0;
    int numButtons = 0;

    if (holdButton_)
    {
        width += BUTTON_SIZE;
        numButtons++;
    }

    if (statsButton_)
    {
        width += BUTTON_SIZE;
        numButtons++;
    }

    // Add spacing between buttons (not after last)
    if (numButtons > 1)
        width += (numButtons - 1) * BUTTON_SPACING;

    return width;
}

} // namespace oscil
