/*
    Oscil - Oscillator List Toolbar Implementation
    Filter and count controls for the oscillator list
*/

#include "ui/OscillatorListToolbar.h"

namespace oscil
{

OscillatorListToolbar::OscillatorListToolbar()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

OscillatorListToolbar::~OscillatorListToolbar()
{
    ThemeManager::getInstance().removeListener(this);
}

void OscillatorListToolbar::setupComponents()
{
    // Filter tabs
    filterTabs_ = std::make_unique<SegmentedButtonBar>();
    filterTabs_->addButton("All", static_cast<int>(OscillatorFilterMode::All));
    filterTabs_->addButton("Visible", static_cast<int>(OscillatorFilterMode::Visible));
    filterTabs_->addButton("Hidden", static_cast<int>(OscillatorFilterMode::Hidden));
    filterTabs_->setSelectedId(static_cast<int>(currentFilterMode_));
    filterTabs_->onSelectionChanged = [this](int id)
    {
        currentFilterMode_ = static_cast<OscillatorFilterMode>(id);
        listeners_.call([this](Listener& l) { l.filterModeChanged(currentFilterMode_); });
    };
    addAndMakeVisible(*filterTabs_);

    // Count label (hidden - we draw custom badge in paint)
    countLabel_ = std::make_unique<juce::Label>();
    countLabel_->setJustificationType(juce::Justification::centredRight);
    countLabel_->setVisible(false);
    addAndMakeVisible(*countLabel_);
    updateCountLabel();

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void OscillatorListToolbar::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Background
    g.setColour(theme.backgroundSecondary);
    g.fillRect(getLocalBounds());

    // Bottom border
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

    // Draw cyan badge for oscillator count
    auto bounds = getLocalBounds().reduced(4);
    auto countBounds = bounds.removeFromRight(90);

    juce::String countText;
    if (currentFilterMode_ == OscillatorFilterMode::All)
    {
        countText = juce::String(totalCount_) + " Oscillators";
    }
    else if (currentFilterMode_ == OscillatorFilterMode::Visible)
    {
        countText = juce::String(visibleCount_) + " Visible";
    }
    else
    {
        countText = juce::String(totalCount_ - visibleCount_) + " Hidden";
    }

    // Cyan pill badge background
    auto pillBounds = countBounds.toFloat().reduced(2, 4);
    g.setColour(theme.controlActive.withAlpha(0.2f));
    g.fillRoundedRectangle(pillBounds, 10.0f);

    // Cyan text
    g.setColour(theme.controlActive);
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    g.drawText(countText, countBounds, juce::Justification::centred);
}

void OscillatorListToolbar::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Single row: Filter tabs and count badge
    // Count badge is drawn in paint(), so just reserve space
    bounds.removeFromRight(90);
    bounds.removeFromRight(4);

    // Filter tabs take remaining space
    filterTabs_->setBounds(bounds);
}

void OscillatorListToolbar::themeChanged(const ColorTheme& newTheme)
{
    // Count label
    countLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    countLabel_->setFont(juce::FontOptions(11.0f));

    repaint();
}

void OscillatorListToolbar::updateCountLabel()
{
    juce::String text;
    if (currentFilterMode_ == OscillatorFilterMode::All)
    {
        text = juce::String(totalCount_) + " Oscillators";
    }
    else if (currentFilterMode_ == OscillatorFilterMode::Visible)
    {
        text = juce::String(visibleCount_) + " Visible";
    }
    else
    {
        text = juce::String(totalCount_ - visibleCount_) + " Hidden";
    }
    countLabel_->setText(text, juce::dontSendNotification);
    repaint();  // Repaint to update the badge
}

void OscillatorListToolbar::setFilterMode(OscillatorFilterMode mode)
{
    if (currentFilterMode_ != mode)
    {
        currentFilterMode_ = mode;
        filterTabs_->setSelectedId(static_cast<int>(mode));
        updateCountLabel();
    }
}

void OscillatorListToolbar::setOscillatorCount(int total, int visible)
{
    totalCount_ = total;
    visibleCount_ = visible;
    updateCountLabel();
}

void OscillatorListToolbar::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OscillatorListToolbar::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

} // namespace oscil
