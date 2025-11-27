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

    sourceSelector_ = std::make_unique<OscilDropdown>();
    sourceSelector_->addItem("Self");
    sourceSelector_->setSelectedIndex(0, false);
    sourceSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index < sourceSelector_->getNumItems())
        {
            // Get the text from the selected index
            // We'll need to track this differently since OscilDropdown doesn't have getText()
            // For now, we'll use a simple approach
            if (index == 0)
                currentSource_ = "Self";
            else
                currentSource_ = "Source " + juce::String(index);
            notifyTriggerSourceChanged();
        }
    };
    addAndMakeVisible(*sourceSelector_);

    // Mode selector
    modeLabel_ = std::make_unique<juce::Label>();
    modeLabel_->setText("Mode", juce::dontSendNotification);
    modeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*modeLabel_);

    modeSelector_ = std::make_unique<OscilDropdown>();
    modeSelector_->addItem("Free Running");  // index 0 = TriggerMode::FREE_RUNNING
    modeSelector_->addItem("Host Sync");     // index 1 = TriggerMode::HOST_SYNC
    modeSelector_->addItem("Triggered");     // index 2 = TriggerMode::TRIGGERED
    modeSelector_->setSelectedIndex(static_cast<int>(currentMode_), false);
    modeSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0)
        {
            currentMode_ = static_cast<TriggerMode>(index);
            updateModeVisibility();
            notifyTriggerModeChanged();
        }
    };
    addAndMakeVisible(*modeSelector_);

    // Threshold controls
    thresholdSlider_ = std::make_unique<OscilSlider>();
    thresholdSlider_->setLabel("Threshold");
    thresholdSlider_->setRange(MIN_THRESHOLD_DBFS, MAX_THRESHOLD_DBFS);
    thresholdSlider_->setStep(0.5);
    thresholdSlider_->setValue(currentThreshold_, false);
    thresholdSlider_->setSuffix(" dB");
    thresholdSlider_->onValueChanged = [this](double value)
    {
        currentThreshold_ = static_cast<float>(value);
        notifyTriggerThresholdChanged();
    };
    addAndMakeVisible(*thresholdSlider_);

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
        // Threshold OscilSlider (integrated label)
        thresholdSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), 40);
        y += 40 + SPACING_LARGE;

        // Edge selector
        edgeLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
        edgeToggle_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
    }
}

void TriggerSettingsSection::themeChanged(const ColorTheme& newTheme)
{
    // Oscil components (OscilSlider, OscilDropdown) handle their own theming automatically
    // via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Row labels
    sourceLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    modeLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    edgeLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);

    repaint();
}

void TriggerSettingsSection::updateModeVisibility()
{
    bool isTriggeredMode = (currentMode_ == TriggerMode::TRIGGERED);

    thresholdSlider_->setVisible(isTriggeredMode);
    edgeLabel_->setVisible(isTriggeredMode);
    edgeToggle_->setVisible(isTriggeredMode);

    resized();
    repaint();
}

void TriggerSettingsSection::setAvailableSources(const juce::StringArray& sources)
{
    sourceSelector_->clearItems();

    // Always add "Self" as first option (index 0)
    sourceSelector_->addItem("Self");

    // Add other sources
    for (int i = 0; i < sources.size(); ++i)
    {
        sourceSelector_->addItem(sources[i]);
    }

    // Restore current selection if still valid
    bool found = false;

    // Check if "Self" is selected
    if (currentSource_ == "Self")
    {
        sourceSelector_->setSelectedIndex(0, false);
        found = true;
    }
    else
    {
        // Search through added sources
        for (int i = 0; i < sources.size(); ++i)
        {
            if (sources[i] == currentSource_)
            {
                sourceSelector_->setSelectedIndex(i + 1, false);  // +1 because "Self" is at index 0
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        sourceSelector_->setSelectedIndex(0, false);
        currentSource_ = "Self";
    }
}

void TriggerSettingsSection::setTriggerSource(const juce::String& sourceName)
{
    currentSource_ = sourceName;

    // Try to find and select the source
    // Note: We need to track sources added via setAvailableSources
    // For now, just try to match "Self" or assume it's in the list
    if (sourceName == "Self")
    {
        sourceSelector_->setSelectedIndex(0, false);
    }
    // If we had a way to get item text from OscilDropdown, we'd search here
    // For now, this is a limitation we'll need to address in the component
}

void TriggerSettingsSection::setTriggerMode(TriggerMode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        modeSelector_->setSelectedIndex(static_cast<int>(mode), false);
        updateModeVisibility();
    }
}

void TriggerSettingsSection::setTriggerThreshold(float dBFS)
{
    currentThreshold_ = juce::jlimit(MIN_THRESHOLD_DBFS, MAX_THRESHOLD_DBFS, dBFS);
    thresholdSlider_->setValue(currentThreshold_, false);
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
        height += 40 + SPACING_LARGE;              // Threshold OscilSlider (integrated label)
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
