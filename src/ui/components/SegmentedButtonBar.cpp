/*
    Oscil - Segmented Button Bar Implementation
    Glassmorphism-styled segmented control with spring-animated sliding indicator
*/

#include "ui/components/SegmentedButtonBar.h"

#include "ui/components/AnimationSettings.h"
#include "ui/components/GlassPainter.h"

#include <algorithm>
#include <utility>

namespace oscil
{

SegmentedButtonBar::SegmentedButtonBar(IThemeService& themeService) : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
    indicatorSpring_.position = 0.0f;
}

SegmentedButtonBar::~SegmentedButtonBar() { stopTimer(); }

void SegmentedButtonBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto& glass = getGlass();

    // Container: glass input styling on the entire bar
    GlassPainter::paintGlassInput(g, bounds, glass, ComponentLayout::RADIUS_LG, false, false);

    // Sliding indicator behind the active button
    if (!buttons_.empty() && selectedId_ >= 0)
    {
        float const indicatorX = indicatorSpring_.position;
        float const indicatorH = bounds.getHeight() - 4.0f;
        float const indicatorY = bounds.getY() + 2.0f;

        auto indicatorBounds =
            juce::Rectangle<float>(indicatorX + 2.0f, indicatorY, indicatorWidth_ - 4.0f, indicatorH);

        // Fill with accentSubtle
        g.setColour(glass.accentSubtle);
        g.fillRoundedRectangle(indicatorBounds, ComponentLayout::RADIUS_MD);

        // Border with accentMuted
        g.setColour(glass.accentMuted);
        g.drawRoundedRectangle(indicatorBounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);
    }
}

void SegmentedButtonBar::resized()
{
    if (buttons_.empty())
        return;

    auto bounds = getLocalBounds();
    int const numButtons = static_cast<int>(buttons_.size());

    // Calculate button width - either equal distribution or minimum width
    int const buttonWidth = std::max(minButtonWidth_, bounds.getWidth() / numButtons);
    int const totalWidth = buttonWidth * numButtons;

    // Center the buttons if they don't fill the width
    int startX = (bounds.getWidth() - totalWidth) / 2;
    startX = std::max(startX, 0);

    for (size_t i = 0; std::cmp_less(i, numButtons); ++i)
    {
        buttons_[i]->setBounds(startX + (static_cast<int>(i) * buttonWidth), 0, buttonWidth, bounds.getHeight());
    }

    // Update indicator width and position after layout
    indicatorWidth_ = static_cast<float>(buttonWidth);
    updateIndicatorTarget(!indicatorInitialized_ ? false : true);

    if (!indicatorInitialized_ && selectedId_ >= 0)
    {
        // First layout — snap indicator to position without animation
        int const idx = getSelectedIndex();
        if (idx >= 0)
        {
            indicatorSpring_.position = static_cast<float>(buttons_[static_cast<size_t>(idx)]->getX());
            indicatorSpring_.target = indicatorSpring_.position;
            indicatorInitialized_ = true;
        }
    }
}

void SegmentedButtonBar::addButton(const juce::String& label, int id, const juce::String& testId,
                                   const juce::String& tooltip)
{
    auto button = std::make_unique<OscilButton>(getThemeService(), label, testId);

    // Configure as transparent segment button — the bar paints the indicator
    button->setToggleable(true);
    button->setVariant(ButtonVariant::Ghost);
    button->setButtonId(id);

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

void SegmentedButtonBar::addButtonWithPath(const juce::Path& iconPath, int id, const juce::String& testId,
                                           const juce::String& tooltip)
{
    auto button = std::make_unique<OscilButton>(getThemeService(), juce::String{}, testId);

    // Configure as transparent segment button with path icon
    button->setToggleable(true);
    button->setVariant(ButtonVariant::Ghost);
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
    indicatorInitialized_ = false;
    stopTimer();
}

void SegmentedButtonBar::setSelectedId(int id)
{
    if (selectedId_ != id)
    {
        selectedId_ = id;

        for (auto& button : buttons_)
        {
            button->setToggled(button->getButtonId() == id, false);
        }

        updateIndicatorTarget(true);

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
        juce::Component::setEnabled(enabled);
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
    // No per-button segment position needed — buttons use Ghost variant (transparent)
    // The sliding indicator provides the visual selection feedback
    for (auto& button : buttons_)
    {
        button->setSegmentPosition(SegmentPosition::None);
    }
}

void SegmentedButtonBar::updateIndicatorTarget(bool animate)
{
    int const idx = getSelectedIndex();
    if (idx < 0 || buttons_.empty())
        return;

    auto const targetX = static_cast<float>(buttons_[static_cast<size_t>(idx)]->getX());

    if (animate && indicatorInitialized_ && AnimationSettings::shouldUseSpringAnimations())
    {
        indicatorSpring_.setTarget(targetX);
        startTimerHz(ComponentLayout::ANIMATION_FPS);
    }
    else
    {
        indicatorSpring_.position = targetX;
        indicatorSpring_.target = targetX;
        indicatorSpring_.velocity = 0.0f;
        indicatorInitialized_ = true;
        repaint();
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

    int const currentIndex = getSelectedIndex();
    int const numButtons = static_cast<int>(buttons_.size());

    if (key == juce::KeyPress::leftKey)
    {
        // Move to previous segment (wrap around)
        int const newIndex = (currentIndex <= 0) ? numButtons - 1 : currentIndex - 1;
        setSelectedId(buttons_[static_cast<size_t>(newIndex)]->getButtonId());
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        // Move to next segment (wrap around)
        int const newIndex = (currentIndex >= numButtons - 1) ? 0 : currentIndex + 1;
        setSelectedId(buttons_[static_cast<size_t>(newIndex)]->getButtonId());
        return true;
    }

    return false;
}

void SegmentedButtonBar::timerCallback()
{
    indicatorSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);

    if (indicatorSpring_.isSettled())
        stopTimer();

    repaint();
}

} // namespace oscil
