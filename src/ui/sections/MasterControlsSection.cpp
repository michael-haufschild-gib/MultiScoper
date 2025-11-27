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
    timebaseSlider_ = std::make_unique<OscilSlider>();
    timebaseSlider_->setLabel("Timebase");
    timebaseSlider_->setRange(MIN_TIMEBASE_MS, MAX_TIMEBASE_MS);
    timebaseSlider_->setStep(1.0);
    timebaseSlider_->setValue(currentTimebaseMs_, false);
    timebaseSlider_->setSkewFactor(0.3);  // Log-like behavior (lower value = more log-like)
    timebaseSlider_->onValueChanged = [this](double value)
    {
        currentTimebaseMs_ = static_cast<float>(value);
        // Update suffix dynamically based on value
        if (currentTimebaseMs_ >= 1000.0f)
        {
            timebaseSlider_->setSuffix(" s");
            timebaseSlider_->setValue(currentTimebaseMs_ / 1000.0f, false);
        }
        else
        {
            timebaseSlider_->setSuffix(" ms");
        }
        notifyTimebaseChanged();
    };
    // Set initial suffix
    if (currentTimebaseMs_ >= 1000.0f)
        timebaseSlider_->setSuffix(" s");
    else
        timebaseSlider_->setSuffix(" ms");
    addAndMakeVisible(*timebaseSlider_);

    // Gain controls
    gainSlider_ = std::make_unique<OscilSlider>();
    gainSlider_->setLabel("Gain");
    gainSlider_->setRange(MIN_GAIN_DB, MAX_GAIN_DB);
    gainSlider_->setStep(0.1);
    gainSlider_->setValue(currentGainDb_, false);
    gainSlider_->setSuffix(" dB");
    gainSlider_->onValueChanged = [this](double value)
    {
        currentGainDb_ = static_cast<float>(value);
        notifyGainChanged();
    };
    addAndMakeVisible(*gainSlider_);

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

    // OscilSlider components have integrated labels - use full row height (40px)
    timebaseSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), 40);
    y += 40 + SPACING_LARGE;

    gainSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), 40);
}

void MasterControlsSection::themeChanged(const ColorTheme& newTheme)
{
    // OscilSlider components handle their own theming automatically
    // via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    repaint();
}

void MasterControlsSection::setTimebaseMs(float ms)
{
    currentTimebaseMs_ = juce::jlimit(MIN_TIMEBASE_MS, MAX_TIMEBASE_MS, ms);

    // Update slider value and suffix based on magnitude
    if (currentTimebaseMs_ >= 1000.0f)
    {
        timebaseSlider_->setSuffix(" s");
        timebaseSlider_->setValue(currentTimebaseMs_ / 1000.0f, false);
    }
    else
    {
        timebaseSlider_->setSuffix(" ms");
        timebaseSlider_->setValue(currentTimebaseMs_, false);
    }
}

void MasterControlsSection::setGainDb(float dB)
{
    currentGainDb_ = juce::jlimit(MIN_GAIN_DB, MAX_GAIN_DB, dB);
    gainSlider_->setValue(currentGainDb_, false);
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
    height += 40 + SPACING_LARGE;                        // Timebase OscilSlider (integrated label)
    height += 40;                                        // Gain OscilSlider (integrated label)
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
