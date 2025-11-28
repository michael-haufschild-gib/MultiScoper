/*
    Oscil - Oscillator List Toolbar Implementation
    Filter and count controls for the oscillator list
*/

#include "ui/OscillatorListToolbar.h"

namespace oscil
{

OscillatorListToolbar::OscillatorListToolbar()
{
    OSCIL_REGISTER_TEST_ID("sidebar_oscillators_toolbar");
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
    filterTabs_->addButton("All", static_cast<int>(OscillatorFilterMode::All), "sidebar_oscillators_toolbar_allTab");
    filterTabs_->addButton("Visible", static_cast<int>(OscillatorFilterMode::Visible), "sidebar_oscillators_toolbar_visibleTab");
    filterTabs_->addButton("Hidden", static_cast<int>(OscillatorFilterMode::Hidden), "sidebar_oscillators_toolbar_hiddenTab");
    filterTabs_->setSelectedId(static_cast<int>(currentFilterMode_));
    filterTabs_->onSelectionChanged = [this](int id)
    {
        currentFilterMode_ = static_cast<OscillatorFilterMode>(id);
        listeners_.call([this](Listener& l) { l.filterModeChanged(currentFilterMode_); });
    };
    addAndMakeVisible(*filterTabs_);
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

void OscillatorListToolbar::themeChanged(const ColorTheme& /*newTheme*/)
{
    repaint();
}

void OscillatorListToolbar::updateCountLabel()
{
    // Count badge is drawn directly in paint() using the state variables
    repaint();
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
