/*
    Oscil - Segmented Button Bar Implementation
    Reusable segmented control for exclusive selection
*/

#include "ui/SegmentedButtonBar.h"

namespace oscil
{

// SegmentButton implementation

SegmentButton::SegmentButton(const juce::String& label, int buttonId)
    : label_(label)
    , buttonId_(buttonId)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SegmentButton::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Determine corner radius based on position
    float cornerRadius = 4.0f;
    juce::Path buttonPath;

    if (isFirst_ && isLast_)
    {
        // Single button - round all corners
        buttonPath.addRoundedRectangle(bounds, cornerRadius);
    }
    else if (isFirst_)
    {
        // First button - round left corners only
        buttonPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        true, false, true, false);
    }
    else if (isLast_)
    {
        // Last button - round right corners only
        buttonPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        cornerRadius, cornerRadius,
                                        false, true, false, true);
    }
    else
    {
        // Middle button - no rounded corners
        buttonPath.addRectangle(bounds);
    }

    // Background
    if (selected_)
    {
        g.setColour(theme.controlActive);
    }
    else if (isHovered_)
    {
        g.setColour(theme.controlHighlight);
    }
    else
    {
        g.setColour(theme.controlBackground);
    }
    g.fillPath(buttonPath);

    // Border
    g.setColour(theme.controlBorder);
    g.strokePath(buttonPath, juce::PathStrokeType(1.0f));

    // Text
    g.setColour(selected_ ? theme.textHighlight : theme.textPrimary);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(label_, bounds, juce::Justification::centred, true);
}

void SegmentButton::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SegmentButton::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SegmentButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
    {
        onClick(buttonId_);
    }
}

void SegmentButton::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        repaint();
    }
}

// SegmentedButtonBar implementation

SegmentedButtonBar::SegmentedButtonBar()
{
    ThemeManager::getInstance().addListener(this);
}

SegmentedButtonBar::~SegmentedButtonBar()
{
    ThemeManager::getInstance().removeListener(this);
}

void SegmentedButtonBar::paint(juce::Graphics& g)
{
    // Background is handled by individual buttons
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

    for (int i = 0; i < numButtons; ++i)
    {
        buttons_[i]->setBounds(startX + i * buttonWidth, 0, buttonWidth, bounds.getHeight());
    }
}

void SegmentedButtonBar::themeChanged(const ColorTheme&)
{
    repaint();
    for (auto& button : buttons_)
    {
        button->repaint();
    }
}

void SegmentedButtonBar::addButton(const juce::String& label, int id)
{
    auto button = std::make_unique<SegmentButton>(label, id);
    button->onClick = [this](int buttonId) { handleButtonClick(buttonId); };

    addAndMakeVisible(*button);
    buttons_.push_back(std::move(button));

    // Update first/last flags
    updateButtonStates();

    // If this is the first button and nothing is selected, select it
    if (buttons_.size() == 1 && selectedId_ == -1)
    {
        setSelectedId(id);
    }

    resized();
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
            button->setSelected(button->getButtonId() == id);
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
            button->setInterceptsMouseClicks(enabled, enabled);
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
    int numButtons = static_cast<int>(buttons_.size());

    for (int i = 0; i < numButtons; ++i)
    {
        buttons_[i]->setFirst(i == 0);
        buttons_[i]->setLast(i == numButtons - 1);
    }
}

} // namespace oscil
