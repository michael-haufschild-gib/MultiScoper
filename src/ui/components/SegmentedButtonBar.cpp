/*
    Oscil - Segmented Button Bar Implementation
    Reusable segmented control using OscilButton for consistent styling
*/

#include "ui/components/SegmentedButtonBar.h"

namespace oscil
{

SegmentedButtonBar::SegmentedButtonBar(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
}



SegmentedButtonBar::~SegmentedButtonBar()
{
}

void SegmentedButtonBar::paint(juce::Graphics& g)
{
    // Background is handled by individual OscilButtons
    juce::ignoreUnused(g);
}

void SegmentedButtonBar::resized()
{
    if (buttons_.empty())
        return;

    auto bounds = getLocalBounds();
    int numButtons = static_cast<int>(buttons_.size());

    // Calculate button width - either equal distribution or minimum width
    int buttonWidth = std::max(minButtonWidth_, bounds.getWidth() / numButtons);
    int totalWidth = buttonWidth * numButtons;

    // Center the buttons if they don't fill the width
    int startX = (bounds.getWidth() - totalWidth) / 2;
    if (startX < 0) startX = 0;

    for (size_t i = 0; i < static_cast<size_t>(numButtons); ++i)
    {
        buttons_[i]->setBounds(startX + static_cast<int>(i) * buttonWidth, 0, buttonWidth, bounds.getHeight());
    }
}

void SegmentedButtonBar::addButton(const juce::String& label, int id, const juce::String& testId,
                                    const juce::String& tooltip)
{
    auto button = std::make_unique<OscilButton>(getThemeService(), label, testId);

    // Configure as a toggleable segment button
    button->setToggleable(true);
    button->setVariant(ButtonVariant::Secondary);
    button->setButtonId(id);

    // Set tooltip if provided
    if (tooltip.isNotEmpty())
        button->setTooltip(tooltip);

    // Set up click handler - use the button ID from getButtonId()
    button->onClick = [this, id]() { handleButtonClick(id); };

    addAndMakeVisible(*button);
    buttons_.push_back(std::move(button));

    // Update segment positions for all buttons
    updateButtonStates();

    // If this is the first button and nothing is selected, select it
    if (buttons_.size() == 1 && selectedId_ == -1)
    {
        setSelectedId(id);
    }

    resized();
}

void SegmentedButtonBar::addButtonWithPath(const juce::Path& iconPath, int id, const juce::String& testId,
                                            const juce::String& tooltip)
{
    auto button = std::make_unique<OscilButton>(getThemeService(), juce::String{}, testId);

    // Configure as a toggleable segment button with path icon
    button->setToggleable(true);
    button->setVariant(ButtonVariant::Secondary);
    button->setButtonId(id);
    button->setIconPath(iconPath);

    // Set tooltip if provided
    if (tooltip.isNotEmpty())
        button->setTooltip(tooltip);

    // Set up click handler
    button->onClick = [this, id]() { handleButtonClick(id); };

    addAndMakeVisible(*button);
    buttons_.push_back(std::move(button));

    // Update segment positions for all buttons
    updateButtonStates();

    // If this is the first button and nothing is selected, select it
    if (buttons_.size() == 1 && selectedId_ == -1)
    {
        setSelectedId(id);
    }

    resized();
}

void SegmentedButtonBar::setButtonTooltip(int id, const juce::String& tooltip)
{
    for (auto& button : buttons_)
    {
        if (button->getButtonId() == id)
        {
            button->setTooltip(tooltip);
            break;
        }
    }
}

void SegmentedButtonBar::clearButtons()
{
    buttons_.clear();
    selectedId_ = -1;
}

void SegmentedButtonBar::setSelectedId(int id)
{
    if (selectedId_ != id)
    {
        selectedId_ = id;

        for (auto& button : buttons_)
        {
            // Use setToggled with notify=false to avoid triggering callbacks
            button->setToggled(button->getButtonId() == id, false);
        }

        if (onSelectionChanged)
        {
            onSelectionChanged(id);
        }
    }
}

void SegmentedButtonBar::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setAlpha(enabled ? 1.0f : 0.5f);

        for (auto& button : buttons_)
        {
            button->setEnabled(enabled);
        }
    }
}

void SegmentedButtonBar::handleButtonClick(int id)
{
    if (enabled_)
    {
        setSelectedId(id);
    }
}

void SegmentedButtonBar::updateButtonStates()
{
    size_t numButtons = buttons_.size();

    for (size_t i = 0; i < numButtons; ++i)
    {
        SegmentPosition position;

        if (numButtons == 1)
        {
            position = SegmentPosition::Only;
        }
        else if (i == 0)
        {
            position = SegmentPosition::First;
        }
        else if (i == numButtons - 1)
        {
            position = SegmentPosition::Last;
        }
        else
        {
            position = SegmentPosition::Middle;
        }

        buttons_[i]->setSegmentPosition(position);
    }
}

int SegmentedButtonBar::getSelectedIndex() const
{
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (buttons_[i]->getButtonId() == selectedId_)
            return static_cast<int>(i);
    }
    return -1;
}

bool SegmentedButtonBar::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_ || buttons_.empty())
        return false;

    int currentIndex = getSelectedIndex();
    int numButtons = static_cast<int>(buttons_.size());

    if (key == juce::KeyPress::leftKey)
    {
        // Move to previous segment (wrap around)
        int newIndex = (currentIndex <= 0) ? numButtons - 1 : currentIndex - 1;
        setSelectedId(buttons_[static_cast<size_t>(newIndex)]->getButtonId());
        return true;
    }
    else if (key == juce::KeyPress::rightKey)
    {
        // Move to next segment (wrap around)
        int newIndex = (currentIndex >= numButtons - 1) ? 0 : currentIndex + 1;
        setSelectedId(buttons_[static_cast<size_t>(newIndex)]->getButtonId());
        return true;
    }

    return false;
}

} // namespace oscil
