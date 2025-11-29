/*
    Oscil - Options Sidebar Section Implementation
    Combined section with gain control and display options
*/

#include "ui/sections/OptionsSection.h"

namespace oscil
{

OptionsSection::OptionsSection()
{
    OSCIL_REGISTER_TEST_ID("sidebar_options");
    setupComponents();
    ThemeManager::getInstance().addListener(this);
}

OptionsSection::~OptionsSection()
{
    ThemeManager::getInstance().removeListener(this);
}

void OptionsSection::setupComponents()
{
    // Gain controls (from Master Controls)
    gainSlider_ = std::make_unique<OscilSlider>("sidebar_options_gainSlider");
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

    // Display section label
    displayLabel_ = std::make_unique<juce::Label>();
    displayLabel_->setText("DISPLAY", juce::dontSendNotification);
    displayLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*displayLabel_);

    // Show Grid toggle
    showGridToggle_ = std::make_unique<OscilToggle>("Show Grid", "sidebar_options_gridToggle");
    showGridToggle_->setValue(showGridEnabled_, false);
    showGridToggle_->onValueChanged = [this](bool value)
    {
        showGridEnabled_ = value;
        notifyShowGridChanged();
    };
    addAndMakeVisible(*showGridToggle_);

    // Auto-Scale toggle
    autoScaleToggle_ = std::make_unique<OscilToggle>("Auto-Scale", "sidebar_options_autoScaleToggle");
    autoScaleToggle_->setValue(autoScaleEnabled_, false);
    autoScaleToggle_->onValueChanged = [this](bool value)
    {
        autoScaleEnabled_ = value;
        notifyAutoScaleChanged();
    };
    addAndMakeVisible(*autoScaleToggle_);

    // Hold Display toggle
    holdDisplayToggle_ = std::make_unique<OscilToggle>("Hold", "sidebar_options_holdToggle");
    holdDisplayToggle_->setValue(holdDisplayEnabled_, false);
    holdDisplayToggle_->onValueChanged = [this](bool value)
    {
        holdDisplayEnabled_ = value;
        notifyHoldDisplayChanged();
    };
    addAndMakeVisible(*holdDisplayToggle_);

    // Layout section label
    layoutLabel_ = std::make_unique<juce::Label>();
    layoutLabel_->setText("LAYOUT", juce::dontSendNotification);
    layoutLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*layoutLabel_);

    // Layout dropdown
    layoutDropdown_ = std::make_unique<OscilDropdown>("Select layout", "sidebar_options_layoutDropdown");
    layoutDropdown_->addItem("1 Column", "1");
    layoutDropdown_->addItem("2 Columns", "2");
    layoutDropdown_->addItem("3 Columns", "3");
    layoutDropdown_->setSelectedIndex(currentColumnCount_ - 1, false);
    layoutDropdown_->onSelectionChanged = [this](int index)
    {
        currentColumnCount_ = index + 1;
        notifyLayoutChanged();
    };
    addAndMakeVisible(*layoutDropdown_);

    // Theme section label
    themeLabel_ = std::make_unique<juce::Label>();
    themeLabel_->setText("THEME", juce::dontSendNotification);
    themeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*themeLabel_);

    // Theme dropdown (items populated via setAvailableThemes)
    themeDropdown_ = std::make_unique<OscilDropdown>("Select theme", "sidebar_options_themeDropdown");
    themeDropdown_->onSelectionChangedId = [this](const juce::String& themeId)
    {
        currentThemeName_ = themeId;
        notifyThemeChanged();
    };
    addAndMakeVisible(*themeDropdown_);

    // Rendering section label
    renderingLabel_ = std::make_unique<juce::Label>();
    renderingLabel_->setText("RENDERING", juce::dontSendNotification);
    renderingLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*renderingLabel_);

    // GPU rendering toggle
    gpuRenderingToggle_ = std::make_unique<OscilToggle>("GPU Acceleration", "sidebar_options_gpuRenderingToggle");
    gpuRenderingToggle_->setValue(gpuRenderingEnabled_, false);
    gpuRenderingToggle_->onValueChanged = [this](bool value)
    {
        gpuRenderingEnabled_ = value;
        notifyGpuRenderingChanged();
    };
    addAndMakeVisible(*gpuRenderingToggle_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void OptionsSection::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds();

    // Background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);
}

void OptionsSection::resized()
{
    using namespace SectionLayout;

    auto bounds = getLocalBounds().reduced(SECTION_PADDING);
    int y = bounds.getY();

    // Gain slider (OscilSlider has integrated label)
    gainSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), 40);
    y += 40 + SPACING_LARGE;

    // Display section label
    displayLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_MEDIUM;

    // Display toggles
    showGridToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    autoScaleToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    holdDisplayToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // Layout section label and dropdown
    layoutLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_MEDIUM;

    layoutDropdown_->setBounds(bounds.getX(), y, bounds.getWidth(), DROPDOWN_HEIGHT);
    y += DROPDOWN_HEIGHT + SPACING_LARGE;

    // Theme section label and dropdown
    themeLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_MEDIUM;

    themeDropdown_->setBounds(bounds.getX(), y, bounds.getWidth(), DROPDOWN_HEIGHT);
    y += DROPDOWN_HEIGHT + SPACING_LARGE;

    // Rendering section label and toggle
    renderingLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_MEDIUM;

    gpuRenderingToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
}

void OptionsSection::themeChanged(const ColorTheme& newTheme)
{
    // OscilSlider and OscilToggle components handle their own theming automatically
    // via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    // Display section label
    displayLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    displayLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Layout section label
    layoutLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    layoutLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Theme section label
    themeLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    themeLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Rendering section label
    renderingLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    renderingLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    repaint();
}

void OptionsSection::setGainDb(float dB)
{
    currentGainDb_ = juce::jlimit(MIN_GAIN_DB, MAX_GAIN_DB, dB);
    gainSlider_->setValue(currentGainDb_, false);
}

void OptionsSection::setShowGrid(bool enabled)
{
    showGridEnabled_ = enabled;
    showGridToggle_->setValue(enabled, false);
}

void OptionsSection::setAutoScale(bool enabled)
{
    autoScaleEnabled_ = enabled;
    autoScaleToggle_->setValue(enabled, false);
}

void OptionsSection::setHoldDisplay(bool enabled)
{
    holdDisplayEnabled_ = enabled;
    holdDisplayToggle_->setValue(enabled, false);
}

void OptionsSection::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OptionsSection::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

int OptionsSection::getPreferredHeight() const
{
    using namespace SectionLayout;

    int height = SECTION_PADDING * 2;                // Top and bottom padding
    height += 40 + SPACING_LARGE;                    // Gain slider
    height += LABEL_HEIGHT + SPACING_MEDIUM;         // Display label
    height += ROW_HEIGHT + SPACING_MEDIUM;           // Show Grid
    height += ROW_HEIGHT + SPACING_MEDIUM;           // Auto-Scale
    height += ROW_HEIGHT + SPACING_LARGE;            // Hold
    height += LABEL_HEIGHT + SPACING_MEDIUM;         // Layout label
    height += DROPDOWN_HEIGHT + SPACING_LARGE;       // Layout dropdown
    height += LABEL_HEIGHT + SPACING_MEDIUM;         // Theme label
    height += DROPDOWN_HEIGHT + SPACING_LARGE;       // Theme dropdown
    height += LABEL_HEIGHT + SPACING_MEDIUM;         // Rendering label
    height += ROW_HEIGHT;                            // GPU toggle
    return height;
}

void OptionsSection::setColumnCount(int columns)
{
    currentColumnCount_ = juce::jlimit(1, 3, columns);
    if (layoutDropdown_ != nullptr)
        layoutDropdown_->setSelectedIndex(currentColumnCount_ - 1, false);
}

void OptionsSection::setAvailableThemes(const std::vector<juce::String>& themeNames)
{
    if (themeDropdown_ == nullptr)
        return;

    themeDropdown_->clearItems();
    for (const auto& name : themeNames)
        themeDropdown_->addItem(name, name);

    // Re-select current theme if it exists
    for (int i = 0; i < themeDropdown_->getNumItems(); ++i)
    {
        if (themeDropdown_->getItem(i).id == currentThemeName_)
        {
            themeDropdown_->setSelectedIndex(i, false);
            break;
        }
    }
}

void OptionsSection::setCurrentTheme(const juce::String& themeName)
{
    currentThemeName_ = themeName;
    if (themeDropdown_ == nullptr)
        return;

    for (int i = 0; i < themeDropdown_->getNumItems(); ++i)
    {
        if (themeDropdown_->getItem(i).id == themeName)
        {
            themeDropdown_->setSelectedIndex(i, false);
            break;
        }
    }
}

void OptionsSection::notifyLayoutChanged()
{
    listeners_.call([this](Listener& l) { l.layoutChanged(currentColumnCount_); });
}

void OptionsSection::notifyThemeChanged()
{
    listeners_.call([this](Listener& l) { l.themeChanged(currentThemeName_); });
}

void OptionsSection::notifyGainChanged()
{
    listeners_.call([this](Listener& l) { l.gainChanged(currentGainDb_); });
}

void OptionsSection::notifyShowGridChanged()
{
    listeners_.call([this](Listener& l) { l.showGridChanged(showGridEnabled_); });
}

void OptionsSection::notifyAutoScaleChanged()
{
    listeners_.call([this](Listener& l) { l.autoScaleChanged(autoScaleEnabled_); });
}

void OptionsSection::notifyHoldDisplayChanged()
{
    listeners_.call([this](Listener& l) { l.holdDisplayChanged(holdDisplayEnabled_); });
}

void OptionsSection::setGpuRenderingEnabled(bool enabled)
{
    gpuRenderingEnabled_ = enabled;
    if (gpuRenderingToggle_ != nullptr)
        gpuRenderingToggle_->setValue(enabled, false);
}

void OptionsSection::notifyGpuRenderingChanged()
{
    listeners_.call([this](Listener& l) { l.gpuRenderingChanged(gpuRenderingEnabled_); });
}

} // namespace oscil
