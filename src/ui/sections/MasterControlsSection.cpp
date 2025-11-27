/*
    Oscil - Master Controls Sidebar Section Implementation
    Timebase and Gain controls for the sidebar
*/

#include "ui/sections/MasterControlsSection.h"

namespace oscil
{

MasterControlsSection::MasterControlsSection()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

MasterControlsSection::~MasterControlsSection()
{
    ThemeManager::getInstance().removeListener(this);
}

void MasterControlsSection::setupComponents()
{
    // Section header
    sectionLabel_ = std::make_unique<juce::Label>();
    sectionLabel_->setText("MASTER CONTROLS", juce::dontSendNotification);
    sectionLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*sectionLabel_);

    // Timebase controls
    timebaseLabel_ = std::make_unique<juce::Label>();
    timebaseLabel_->setText("Timebase", juce::dontSendNotification);
    timebaseLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*timebaseLabel_);

    timebaseSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    timebaseSlider_->setRange(MIN_TIMEBASE_MS, MAX_TIMEBASE_MS, 1.0);
    timebaseSlider_->setValue(currentTimebaseMs_);
    timebaseSlider_->setSkewFactorFromMidPoint(100.0); // Log-like behavior around 100ms
    timebaseSlider_->onValueChange = [this]()
    {
        currentTimebaseMs_ = static_cast<float>(timebaseSlider_->getValue());
        updateTimebaseDisplay();
        notifyTimebaseChanged();
    };
    addAndMakeVisible(*timebaseSlider_);

    timebaseValueLabel_ = std::make_unique<juce::Label>();
    timebaseValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*timebaseValueLabel_);
    updateTimebaseDisplay();

    // Gain controls
    gainLabel_ = std::make_unique<juce::Label>();
    gainLabel_->setText("Gain", juce::dontSendNotification);
    gainLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*gainLabel_);

    gainSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    gainSlider_->setRange(MIN_GAIN_DB, MAX_GAIN_DB, 0.1);
    gainSlider_->setValue(currentGainDb_);
    gainSlider_->onValueChange = [this]()
    {
        currentGainDb_ = static_cast<float>(gainSlider_->getValue());
        updateGainDisplay();
        notifyGainChanged();
    };
    addAndMakeVisible(*gainSlider_);

    gainValueLabel_ = std::make_unique<juce::Label>();
    gainValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*gainValueLabel_);
    updateGainDisplay();

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void MasterControlsSection::paint(juce::Graphics& g)
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

void MasterControlsSection::resized()
{
    using namespace SectionLayout;

    auto bounds = getLocalBounds().reduced(SECTION_PADDING);
    int y = 0;

    // Section header
    sectionLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_LARGE;

    // Timebase controls
    int labelWidth = 60;
    int valueWidth = 70;

    timebaseLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
    timebaseValueLabel_->setBounds(bounds.getRight() - valueWidth, y, valueWidth, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_SMALL;

    timebaseSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // Gain controls
    gainLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
    gainValueLabel_->setBounds(bounds.getRight() - valueWidth, y, valueWidth, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_SMALL;

    gainSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
}

void MasterControlsSection::themeChanged(const ColorTheme& newTheme)
{
    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Timebase
    timebaseLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    timebaseValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);
    timebaseSlider_->setColour(juce::Slider::thumbColourId, newTheme.controlActive);
    timebaseSlider_->setColour(juce::Slider::trackColourId, newTheme.controlBackground);
    timebaseSlider_->setColour(juce::Slider::backgroundColourId, newTheme.controlBackground);

    // Gain
    gainLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    gainValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);
    gainSlider_->setColour(juce::Slider::thumbColourId, newTheme.controlActive);
    gainSlider_->setColour(juce::Slider::trackColourId, newTheme.controlBackground);
    gainSlider_->setColour(juce::Slider::backgroundColourId, newTheme.controlBackground);

    repaint();
}

void MasterControlsSection::updateTimebaseDisplay()
{
    juce::String text;
    if (currentTimebaseMs_ >= 1000.0f)
    {
        text = juce::String(currentTimebaseMs_ / 1000.0f, 2) + " s";
    }
    else
    {
        text = juce::String(static_cast<int>(currentTimebaseMs_)) + " ms";
    }
    timebaseValueLabel_->setText(text, juce::dontSendNotification);
}

void MasterControlsSection::updateGainDisplay()
{
    juce::String text;
    if (currentGainDb_ >= 0.0f)
    {
        text = "+" + juce::String(currentGainDb_, 1) + " dB";
    }
    else
    {
        text = juce::String(currentGainDb_, 1) + " dB";
    }
    gainValueLabel_->setText(text, juce::dontSendNotification);
}

void MasterControlsSection::setTimebaseMs(float ms)
{
    currentTimebaseMs_ = juce::jlimit(MIN_TIMEBASE_MS, MAX_TIMEBASE_MS, ms);
    timebaseSlider_->setValue(currentTimebaseMs_, juce::dontSendNotification);
    updateTimebaseDisplay();
}

void MasterControlsSection::setGainDb(float dB)
{
    currentGainDb_ = juce::jlimit(MIN_GAIN_DB, MAX_GAIN_DB, dB);
    gainSlider_->setValue(currentGainDb_, juce::dontSendNotification);
    updateGainDisplay();
}

void MasterControlsSection::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void MasterControlsSection::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

int MasterControlsSection::getPreferredHeight() const
{
    using namespace SectionLayout;

    int height = SECTION_PADDING * 2;                    // Top and bottom padding
    height += LABEL_HEIGHT + SPACING_LARGE;              // Section header
    height += ROW_HEIGHT + SPACING_SMALL;                // Timebase label/value row
    height += ROW_HEIGHT + SPACING_LARGE;                // Timebase slider
    height += ROW_HEIGHT + SPACING_SMALL;                // Gain label/value row
    height += ROW_HEIGHT;                                // Gain slider
    return height;
}

void MasterControlsSection::notifyTimebaseChanged()
{
    listeners_.call([this](Listener& l) { l.timebaseChanged(currentTimebaseMs_); });
}

void MasterControlsSection::notifyGainChanged()
{
    listeners_.call([this](Listener& l) { l.gainChanged(currentGainDb_); });
}

} // namespace oscil
