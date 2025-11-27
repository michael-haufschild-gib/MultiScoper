/*
    Oscil - Display Options Sidebar Section Implementation
    Display toggles for grid, auto-scale, and hold
*/

#include "ui/sections/DisplayOptionsSection.h"

namespace oscil
{

DisplayOptionsSection::DisplayOptionsSection()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

DisplayOptionsSection::~DisplayOptionsSection()
{
    ThemeManager::getInstance().removeListener(this);
}

void DisplayOptionsSection::setupComponents()
{
    // Section header
    sectionLabel_ = std::make_unique<juce::Label>();
    sectionLabel_->setText("DISPLAY OPTIONS", juce::dontSendNotification);
    sectionLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*sectionLabel_);

    // Show Grid toggle
    showGridToggle_ = std::make_unique<OscilToggle>("Show Grid");
    showGridToggle_->setValue(showGridEnabled_, false);
    showGridToggle_->onValueChanged = [this](bool value)
    {
        showGridEnabled_ = value;
        notifyShowGridChanged();
    };
    addAndMakeVisible(*showGridToggle_);

    // Auto-Scale toggle
    autoScaleToggle_ = std::make_unique<OscilToggle>("Auto-Scale");
    autoScaleToggle_->setValue(autoScaleEnabled_, false);
    autoScaleToggle_->onValueChanged = [this](bool value)
    {
        autoScaleEnabled_ = value;
        notifyAutoScaleChanged();
    };
    addAndMakeVisible(*autoScaleToggle_);

    // Hold Display toggle
    holdDisplayToggle_ = std::make_unique<OscilToggle>("Hold");
    holdDisplayToggle_->setValue(holdDisplayEnabled_, false);
    holdDisplayToggle_->onValueChanged = [this](bool value)
    {
        holdDisplayEnabled_ = value;
        notifyHoldDisplayChanged();
    };
    addAndMakeVisible(*holdDisplayToggle_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void DisplayOptionsSection::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds();

    // Background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);

    // Top separator line
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
}

void DisplayOptionsSection::resized()
{
    using namespace SectionLayout;

    auto bounds = getLocalBounds().reduced(SECTION_PADDING);
    int y = 0;

    // Section header
    sectionLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_LARGE;

    // Toggles
    showGridToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    autoScaleToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    holdDisplayToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
}

void DisplayOptionsSection::themeChanged(const ColorTheme& newTheme)
{
    // OscilToggle components handle their own theming automatically
    // via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    repaint();
}

void DisplayOptionsSection::setShowGrid(bool enabled)
{
    showGridEnabled_ = enabled;
    showGridToggle_->setValue(enabled, false);
}

void DisplayOptionsSection::setAutoScale(bool enabled)
{
    autoScaleEnabled_ = enabled;
    autoScaleToggle_->setValue(enabled, false);
}

void DisplayOptionsSection::setHoldDisplay(bool enabled)
{
    holdDisplayEnabled_ = enabled;
    holdDisplayToggle_->setValue(enabled, false);
}

void DisplayOptionsSection::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void DisplayOptionsSection::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

int DisplayOptionsSection::getPreferredHeight() const
{
    using namespace SectionLayout;

    int height = SECTION_PADDING * 2;              // Top and bottom padding
    height += LABEL_HEIGHT + SPACING_LARGE;        // Section header
    height += ROW_HEIGHT + SPACING_MEDIUM;         // Show Grid
    height += ROW_HEIGHT + SPACING_MEDIUM;         // Auto-Scale
    height += ROW_HEIGHT;                          // Hold
    return height;
}

void DisplayOptionsSection::notifyShowGridChanged()
{
    listeners_.call([this](Listener& l) { l.showGridChanged(showGridEnabled_); });
}

void DisplayOptionsSection::notifyAutoScaleChanged()
{
    listeners_.call([this](Listener& l) { l.autoScaleChanged(autoScaleEnabled_); });
}

void DisplayOptionsSection::notifyHoldDisplayChanged()
{
    listeners_.call([this](Listener& l) { l.holdDisplayChanged(holdDisplayEnabled_); });
}

} // namespace oscil
