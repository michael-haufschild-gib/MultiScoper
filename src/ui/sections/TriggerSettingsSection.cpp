/*
    Oscil - Trigger Settings Sidebar Section Implementation
    Trigger source and mode controls for the sidebar
*/

#include "ui/sections/TriggerSettingsSection.h"

namespace oscil
{

TriggerSettingsSection::TriggerSettingsSection()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
    updateModeVisibility();
}

TriggerSettingsSection::~TriggerSettingsSection()
{
    ThemeManager::getInstance().removeListener(this);
}

void TriggerSettingsSection::setupComponents()
{
    // Section header
    sectionLabel_ = std::make_unique<juce::Label>();
    sectionLabel_->setText("TRIGGER SETTINGS", juce::dontSendNotification);
    sectionLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*sectionLabel_);

    // Trigger Source dropdown
    sourceLabel_ = std::make_unique<juce::Label>();
    sourceLabel_->setText("Source", juce::dontSendNotification);
    sourceLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*sourceLabel_);

    sourceSelector_ = std::make_unique<juce::ComboBox>();
    sourceSelector_->addItem("Self", 1);
    sourceSelector_->setSelectedId(1);
    sourceSelector_->onChange = [this]()
    {
        currentSource_ = sourceSelector_->getText();
        notifyTriggerSourceChanged();
    };
    addAndMakeVisible(*sourceSelector_);

    // Mode selector
    modeLabel_ = std::make_unique<juce::Label>();
    modeLabel_->setText("Mode", juce::dontSendNotification);
    modeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*modeLabel_);

    modeSelector_ = std::make_unique<juce::ComboBox>();
    modeSelector_->addItem("Free Running", static_cast<int>(TriggerMode::FREE_RUNNING) + 1);
    modeSelector_->addItem("Host Sync", static_cast<int>(TriggerMode::HOST_SYNC) + 1);
    modeSelector_->addItem("Triggered", static_cast<int>(TriggerMode::TRIGGERED) + 1);
    modeSelector_->setSelectedId(static_cast<int>(currentMode_) + 1);
    modeSelector_->onChange = [this]()
    {
        int selectedId = modeSelector_->getSelectedId();
        if (selectedId > 0)
        {
            currentMode_ = static_cast<TriggerMode>(selectedId - 1);
            updateModeVisibility();
            notifyTriggerModeChanged();
        }
    };
    addAndMakeVisible(*modeSelector_);

    // Threshold controls
    thresholdLabel_ = std::make_unique<juce::Label>();
    thresholdLabel_->setText("Threshold", juce::dontSendNotification);
    thresholdLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*thresholdLabel_);

    thresholdSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    thresholdSlider_->setRange(MIN_THRESHOLD_DBFS, MAX_THRESHOLD_DBFS, 0.5);
    thresholdSlider_->setValue(currentThreshold_);
    thresholdSlider_->onValueChange = [this]()
    {
        currentThreshold_ = static_cast<float>(thresholdSlider_->getValue());
        thresholdValueLabel_->setText(juce::String(currentThreshold_, 1) + " dB",
                                       juce::dontSendNotification);
        notifyTriggerThresholdChanged();
    };
    addAndMakeVisible(*thresholdSlider_);

    thresholdValueLabel_ = std::make_unique<juce::Label>();
    thresholdValueLabel_->setText(juce::String(currentThreshold_, 1) + " dB",
                                   juce::dontSendNotification);
    thresholdValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*thresholdValueLabel_);

    // Edge selector
    edgeLabel_ = std::make_unique<juce::Label>();
    edgeLabel_->setText("Edge", juce::dontSendNotification);
    edgeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*edgeLabel_);

    edgeToggle_ = std::make_unique<SegmentedButtonBar>();
    edgeToggle_->addButton("Rising", static_cast<int>(TriggerEdge::Rising));
    edgeToggle_->addButton("Falling", static_cast<int>(TriggerEdge::Falling));
    edgeToggle_->addButton("Both", static_cast<int>(TriggerEdge::Both));
    edgeToggle_->setSelectedId(static_cast<int>(currentEdge_));
    edgeToggle_->onSelectionChanged = [this](int id)
    {
        currentEdge_ = static_cast<TriggerEdge>(id);
        notifyTriggerEdgeChanged();
    };
    addAndMakeVisible(*edgeToggle_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void TriggerSettingsSection::paint(juce::Graphics& g)
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

void TriggerSettingsSection::resized()
{
    using namespace SectionLayout;

    auto bounds = getLocalBounds().reduced(SECTION_PADDING);
    int y = 0;

    int labelWidth = 60;

    // Section header
    sectionLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_LARGE;

    // Trigger Source row
    sourceLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
    sourceSelector_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // Mode row
    modeLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
    modeSelector_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                              bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // Triggered-mode-only controls
    if (currentMode_ == TriggerMode::TRIGGERED)
    {
        // Threshold controls
        int valueWidth = 60;
        thresholdLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
        thresholdValueLabel_->setBounds(bounds.getRight() - valueWidth, y, valueWidth, ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_SMALL;

        thresholdSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;

        // Edge selector
        edgeLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
        edgeToggle_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
    }
}

void TriggerSettingsSection::themeChanged(const ColorTheme& newTheme)
{
    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Labels
    sourceLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    modeLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    thresholdLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    edgeLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    thresholdValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);

    // ComboBox styling
    auto styleComboBox = [&newTheme](juce::ComboBox* box)
    {
        box->setColour(juce::ComboBox::backgroundColourId, newTheme.controlBackground);
        box->setColour(juce::ComboBox::textColourId, newTheme.textPrimary);
        box->setColour(juce::ComboBox::outlineColourId, newTheme.controlBorder);
        box->setColour(juce::ComboBox::arrowColourId, newTheme.textSecondary);
    };

    styleComboBox(sourceSelector_.get());
    styleComboBox(modeSelector_.get());

    // Slider styling
    thresholdSlider_->setColour(juce::Slider::thumbColourId, newTheme.controlActive);
    thresholdSlider_->setColour(juce::Slider::trackColourId, newTheme.controlBackground);
    thresholdSlider_->setColour(juce::Slider::backgroundColourId, newTheme.controlBackground);

    repaint();
}

void TriggerSettingsSection::updateModeVisibility()
{
    bool isTriggeredMode = (currentMode_ == TriggerMode::TRIGGERED);

    thresholdLabel_->setVisible(isTriggeredMode);
    thresholdSlider_->setVisible(isTriggeredMode);
    thresholdValueLabel_->setVisible(isTriggeredMode);
    edgeLabel_->setVisible(isTriggeredMode);
    edgeToggle_->setVisible(isTriggeredMode);

    resized();
    repaint();
}

void TriggerSettingsSection::setAvailableSources(const juce::StringArray& sources)
{
    sourceSelector_->clear();

    // Always add "Self" as first option
    sourceSelector_->addItem("Self", 1);

    // Add other sources
    for (int i = 0; i < sources.size(); ++i)
    {
        sourceSelector_->addItem(sources[i], i + 2);
    }

    // Restore current selection if still valid
    bool found = false;
    for (int i = 0; i < sourceSelector_->getNumItems(); ++i)
    {
        if (sourceSelector_->getItemText(i) == currentSource_)
        {
            sourceSelector_->setSelectedItemIndex(i, juce::dontSendNotification);
            found = true;
            break;
        }
    }

    if (!found)
    {
        sourceSelector_->setSelectedId(1, juce::dontSendNotification);
        currentSource_ = "Self";
    }
}

void TriggerSettingsSection::setTriggerSource(const juce::String& sourceName)
{
    currentSource_ = sourceName;
    for (int i = 0; i < sourceSelector_->getNumItems(); ++i)
    {
        if (sourceSelector_->getItemText(i) == sourceName)
        {
            sourceSelector_->setSelectedItemIndex(i, juce::dontSendNotification);
            break;
        }
    }
}

void TriggerSettingsSection::setTriggerMode(TriggerMode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        modeSelector_->setSelectedId(static_cast<int>(mode) + 1, juce::dontSendNotification);
        updateModeVisibility();
    }
}

void TriggerSettingsSection::setTriggerThreshold(float dBFS)
{
    currentThreshold_ = juce::jlimit(MIN_THRESHOLD_DBFS, MAX_THRESHOLD_DBFS, dBFS);
    thresholdSlider_->setValue(currentThreshold_, juce::dontSendNotification);
    thresholdValueLabel_->setText(juce::String(currentThreshold_, 1) + " dB",
                                   juce::dontSendNotification);
}

void TriggerSettingsSection::setTriggerEdge(TriggerEdge edge)
{
    currentEdge_ = edge;
    edgeToggle_->setSelectedId(static_cast<int>(edge));
}

void TriggerSettingsSection::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void TriggerSettingsSection::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

int TriggerSettingsSection::getPreferredHeight() const
{
    using namespace SectionLayout;

    int height = SECTION_PADDING * 2;              // Top and bottom padding
    height += LABEL_HEIGHT + SPACING_LARGE;        // Section header
    height += ROW_HEIGHT + SPACING_LARGE;          // Source row
    height += ROW_HEIGHT + SPACING_LARGE;          // Mode row

    if (currentMode_ == TriggerMode::TRIGGERED)
    {
        height += ROW_HEIGHT + SPACING_SMALL;      // Threshold label row
        height += ROW_HEIGHT + SPACING_LARGE;      // Threshold slider
        height += ROW_HEIGHT;                      // Edge selector
    }

    return height;
}

void TriggerSettingsSection::notifyTriggerSourceChanged()
{
    listeners_.call([this](Listener& l) { l.triggerSourceChanged(currentSource_); });
}

void TriggerSettingsSection::notifyTriggerModeChanged()
{
    listeners_.call([this](Listener& l) { l.triggerModeChanged(currentMode_); });
}

void TriggerSettingsSection::notifyTriggerThresholdChanged()
{
    listeners_.call([this](Listener& l) { l.triggerThresholdChanged(currentThreshold_); });
}

void TriggerSettingsSection::notifyTriggerEdgeChanged()
{
    listeners_.call([this](Listener& l) { l.triggerEdgeChanged(currentEdge_); });
}

} // namespace oscil
