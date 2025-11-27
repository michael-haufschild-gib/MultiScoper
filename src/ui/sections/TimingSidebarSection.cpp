/*
    Oscil - Timing Sidebar Section Implementation
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#include "ui/sections/TimingSidebarSection.h"

namespace oscil
{

TimingSidebarSection::TimingSidebarSection()
{
    setupComponents();
    ThemeManager::getInstance().addListener(this);
    updateModeVisibility();
}

TimingSidebarSection::~TimingSidebarSection()
{
    ThemeManager::getInstance().removeListener(this);
}

void TimingSidebarSection::setupComponents()
{
    // Section header
    sectionLabel_ = std::make_unique<juce::Label>();
    sectionLabel_->setText("TIMING", juce::dontSendNotification);
    sectionLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*sectionLabel_);

    // TIME/MELODIC toggle
    modeToggle_ = std::make_unique<SegmentedButtonBar>();
    modeToggle_->addButton("TIME", static_cast<int>(TimingMode::TIME));
    modeToggle_->addButton("MELODIC", static_cast<int>(TimingMode::MELODIC));
    modeToggle_->setSelectedId(static_cast<int>(currentMode_));
    modeToggle_->onSelectionChanged = [this](int id)
    {
        currentMode_ = static_cast<TimingMode>(id);
        updateModeVisibility();
        notifyTimingModeChanged();
    };
    addAndMakeVisible(*modeToggle_);

    // TIME mode controls
    timeIntervalLabel_ = std::make_unique<juce::Label>();
    timeIntervalLabel_->setText("Interval", juce::dontSendNotification);
    timeIntervalLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*timeIntervalLabel_);

    timeIntervalSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    timeIntervalSlider_->setRange(1.0, 1000.0, 1.0);
    timeIntervalSlider_->setValue(currentTimeIntervalMs_);
    timeIntervalSlider_->setSkewFactorFromMidPoint(100.0);
    timeIntervalSlider_->onValueChange = [this]()
    {
        currentTimeIntervalMs_ = static_cast<int>(timeIntervalSlider_->getValue());
        updateCalculatedInterval();
        notifyTimeIntervalChanged();
    };
    addAndMakeVisible(*timeIntervalSlider_);

    timeIntervalValueLabel_ = std::make_unique<juce::Label>();
    timeIntervalValueLabel_->setText(juce::String(currentTimeIntervalMs_) + " ms", juce::dontSendNotification);
    timeIntervalValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*timeIntervalValueLabel_);

    // MELODIC mode controls (per design: "Note Interval")
    noteIntervalLabel_ = std::make_unique<juce::Label>();
    noteIntervalLabel_->setText("Note Interval", juce::dontSendNotification);
    noteIntervalLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*noteIntervalLabel_);

    noteIntervalSelector_ = std::make_unique<juce::ComboBox>();
    populateNoteIntervalSelector();
    noteIntervalSelector_->onChange = [this]()
    {
        int selectedId = noteIntervalSelector_->getSelectedId();
        if (selectedId > 0)
        {
            currentNoteInterval_ = static_cast<NoteInterval>(selectedId - 1);
            updateCalculatedInterval();
            notifyNoteIntervalChanged();
        }
    };
    addAndMakeVisible(*noteIntervalSelector_);

    // BPM display (per design: "Host BPM")
    bpmLabel_ = std::make_unique<juce::Label>();
    bpmLabel_->setText("Host BPM", juce::dontSendNotification);
    bpmLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*bpmLabel_);

    bpmValueLabel_ = std::make_unique<juce::Label>();
    bpmValueLabel_->setText(juce::String(hostBPM_, 1), juce::dontSendNotification);
    bpmValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*bpmValueLabel_);

    // Calculated interval display
    calculatedIntervalLabel_ = std::make_unique<juce::Label>();
    calculatedIntervalLabel_->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*calculatedIntervalLabel_);
    updateCalculatedInterval();

    // Host sync toggle
    hostSyncToggle_ = std::make_unique<juce::ToggleButton>("Sync to Host");
    hostSyncToggle_->setToggleState(hostSyncEnabled_, juce::dontSendNotification);
    hostSyncToggle_->onClick = [this]()
    {
        hostSyncEnabled_ = hostSyncToggle_->getToggleState();
        notifyHostSyncChanged();
    };
    addAndMakeVisible(*hostSyncToggle_);

    // Reset on play toggle
    resetOnPlayToggle_ = std::make_unique<juce::ToggleButton>("Reset on Play");
    resetOnPlayToggle_->setToggleState(resetOnPlayEnabled_, juce::dontSendNotification);
    resetOnPlayToggle_->onClick = [this]()
    {
        resetOnPlayEnabled_ = resetOnPlayToggle_->getToggleState();
        notifyResetOnPlayChanged();
    };
    addAndMakeVisible(*resetOnPlayToggle_);

    // Sync status
    syncStatusLabel_ = std::make_unique<juce::Label>();
    syncStatusLabel_->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*syncStatusLabel_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void TimingSidebarSection::populateNoteIntervalSelector()
{
    noteIntervalSelector_->clear();

    // Standard notes (per design: "1/16th", "1/8th", "1/4", "1/2", "1 Bar")
    noteIntervalSelector_->addItem("1/32nd", static_cast<int>(NoteInterval::THIRTY_SECOND) + 1);
    noteIntervalSelector_->addItem("1/16th", static_cast<int>(NoteInterval::SIXTEENTH) + 1);
    noteIntervalSelector_->addItem("1/12th", static_cast<int>(NoteInterval::TWELFTH) + 1);
    noteIntervalSelector_->addItem("1/8th", static_cast<int>(NoteInterval::EIGHTH) + 1);
    noteIntervalSelector_->addItem("1/4", static_cast<int>(NoteInterval::QUARTER) + 1);
    noteIntervalSelector_->addItem("1/2", static_cast<int>(NoteInterval::HALF) + 1);
    noteIntervalSelector_->addItem("1 Bar", static_cast<int>(NoteInterval::WHOLE) + 1);

    noteIntervalSelector_->addSeparator();

    // Multi-bar
    noteIntervalSelector_->addItem("2 Bars", static_cast<int>(NoteInterval::TWO_BARS) + 1);
    noteIntervalSelector_->addItem("3 Bars", static_cast<int>(NoteInterval::THREE_BARS) + 1);
    noteIntervalSelector_->addItem("4 Bars", static_cast<int>(NoteInterval::FOUR_BARS) + 1);
    noteIntervalSelector_->addItem("8 Bars", static_cast<int>(NoteInterval::EIGHT_BARS) + 1);

    noteIntervalSelector_->addSeparator();

    // Dotted
    noteIntervalSelector_->addItem("1/8th.", static_cast<int>(NoteInterval::DOTTED_EIGHTH) + 1);
    noteIntervalSelector_->addItem("1/4.", static_cast<int>(NoteInterval::DOTTED_QUARTER) + 1);
    noteIntervalSelector_->addItem("1/2.", static_cast<int>(NoteInterval::DOTTED_HALF) + 1);

    noteIntervalSelector_->addSeparator();

    // Triplets
    noteIntervalSelector_->addItem("1/8th T", static_cast<int>(NoteInterval::TRIPLET_EIGHTH) + 1);
    noteIntervalSelector_->addItem("1/4 T", static_cast<int>(NoteInterval::TRIPLET_QUARTER) + 1);
    noteIntervalSelector_->addItem("1/2 T", static_cast<int>(NoteInterval::TRIPLET_HALF) + 1);

    // Set current selection
    noteIntervalSelector_->setSelectedId(static_cast<int>(currentNoteInterval_) + 1);
}

void TimingSidebarSection::paint(juce::Graphics& g)
{
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds();

    // Background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);

    // Top separator line
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    auto headerBounds = bounds.reduced(SectionLayout::SECTION_PADDING);

    // Draw lock icon next to BPM value in MELODIC mode
    if (currentMode_ == TimingMode::MELODIC && hostSyncEnabled_)
    {
        int y = SectionLayout::LABEL_HEIGHT + SectionLayout::SPACING_MEDIUM +
                SectionLayout::ROW_HEIGHT + SectionLayout::SPACING_LARGE +
                SectionLayout::ROW_HEIGHT + SectionLayout::SPACING_LARGE;

        g.setColour(theme.textSecondary);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(juce::String::charToString(0x1F512),  // Lock emoji
                   headerBounds.getRight() - 75, y,
                   16, SectionLayout::ROW_HEIGHT,
                   juce::Justification::centred);
    }

    // Draw styled SYNCED pill badge if synced
    if (hostSyncEnabled_ && isSynced_)
    {
        int y = getPreferredHeight() - SectionLayout::SECTION_PADDING - SectionLayout::LABEL_HEIGHT;
        auto badgeBounds = juce::Rectangle<float>(
            static_cast<float>(headerBounds.getX()),
            static_cast<float>(y),
            static_cast<float>(headerBounds.getWidth()),
            static_cast<float>(SectionLayout::LABEL_HEIGHT)
        );

        // Center the pill badge
        float pillWidth = 70.0f;
        float pillX = badgeBounds.getCentreX() - pillWidth / 2;
        juce::Rectangle<float> pillRect(pillX, badgeBounds.getY() + 2, pillWidth, badgeBounds.getHeight() - 4);

        // Green pill background
        g.setColour(theme.statusActive.withAlpha(0.2f));
        g.fillRoundedRectangle(pillRect, 10.0f);

        // Green text
        g.setColour(theme.statusActive);
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("SYNCED", pillRect.toNearestInt(), juce::Justification::centred);
    }
}

void TimingSidebarSection::resized()
{
    using namespace SectionLayout;

    auto bounds = getLocalBounds().reduced(SECTION_PADDING);
    int y = 0;

    // Section header
    sectionLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
    y += LABEL_HEIGHT + SPACING_MEDIUM;

    // Mode toggle
    modeToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // TIME mode controls
    if (currentMode_ == TimingMode::TIME)
    {
        // Interval label and value on same row
        int labelWidth = 50;
        int valueWidth = 60;
        timeIntervalLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
        timeIntervalValueLabel_->setBounds(bounds.getRight() - valueWidth, y, valueWidth, ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_SMALL;

        // Slider below
        timeIntervalSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;
    }
    else // MELODIC mode
    {
        // Note interval selector
        int labelWidth = 80;  // Larger to fit "Note Interval"
        noteIntervalLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
        noteIntervalSelector_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                          bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;

        // BPM display (per design: "Host BPM" label)
        int bpmLabelWidth = 70;  // Ensure "Host BPM" fits without truncation
        int bpmValueWidth = 60;
        bpmLabel_->setBounds(bounds.getX(), y, bpmLabelWidth, ROW_HEIGHT);
        bpmValueLabel_->setBounds(bounds.getRight() - bpmValueWidth, y, bpmValueWidth, ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_MEDIUM;

        // Calculated interval
        calculatedIntervalLabel_->setBounds(bounds.getX(), y, bounds.getWidth(), LABEL_HEIGHT);
        y += LABEL_HEIGHT + SPACING_LARGE;
    }

    // Host sync toggle
    hostSyncToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    // Reset on play toggle
    resetOnPlayToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_MEDIUM;

    // Sync status - hide the label since we draw a styled pill badge in paint()
    syncStatusLabel_->setVisible(false);
}

void TimingSidebarSection::themeChanged(const ColorTheme& newTheme)
{
    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // TIME mode
    timeIntervalLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    timeIntervalValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);
    timeIntervalSlider_->setColour(juce::Slider::thumbColourId, newTheme.controlActive);
    timeIntervalSlider_->setColour(juce::Slider::trackColourId, newTheme.controlBackground);

    // MELODIC mode
    noteIntervalLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    bpmLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    bpmValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);
    calculatedIntervalLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);

    // ComboBox styling
    noteIntervalSelector_->setColour(juce::ComboBox::backgroundColourId, newTheme.controlBackground);
    noteIntervalSelector_->setColour(juce::ComboBox::textColourId, newTheme.textPrimary);
    noteIntervalSelector_->setColour(juce::ComboBox::outlineColourId, newTheme.controlBorder);
    noteIntervalSelector_->setColour(juce::ComboBox::arrowColourId, newTheme.textSecondary);

    // Toggles
    hostSyncToggle_->setColour(juce::ToggleButton::textColourId, newTheme.textPrimary);
    hostSyncToggle_->setColour(juce::ToggleButton::tickColourId, newTheme.controlActive);
    hostSyncToggle_->setColour(juce::ToggleButton::tickDisabledColourId, newTheme.textSecondary.withAlpha(0.5f));

    resetOnPlayToggle_->setColour(juce::ToggleButton::textColourId, newTheme.textPrimary);
    resetOnPlayToggle_->setColour(juce::ToggleButton::tickColourId, newTheme.controlActive);
    resetOnPlayToggle_->setColour(juce::ToggleButton::tickDisabledColourId, newTheme.textSecondary.withAlpha(0.5f));

    // Sync status
    syncStatusLabel_->setColour(juce::Label::textColourId,
                                 isSynced_ ? newTheme.statusActive : newTheme.textSecondary);

    repaint();
}

void TimingSidebarSection::updateModeVisibility()
{
    bool isTimeMode = (currentMode_ == TimingMode::TIME);

    // TIME mode controls
    timeIntervalLabel_->setVisible(isTimeMode);
    timeIntervalSlider_->setVisible(isTimeMode);
    timeIntervalValueLabel_->setVisible(isTimeMode);

    // MELODIC mode controls
    noteIntervalLabel_->setVisible(!isTimeMode);
    noteIntervalSelector_->setVisible(!isTimeMode);
    bpmLabel_->setVisible(!isTimeMode);
    bpmValueLabel_->setVisible(!isTimeMode);
    calculatedIntervalLabel_->setVisible(!isTimeMode);

    resized();
    repaint();
}

void TimingSidebarSection::updateCalculatedInterval()
{
    if (currentMode_ == TimingMode::TIME)
    {
        timeIntervalValueLabel_->setText(juce::String(currentTimeIntervalMs_) + " ms",
                                          juce::dontSendNotification);
    }
    else
    {
        // Calculate interval from BPM and note
        float msPerBeat = 60000.0f / hostBPM_;
        float multiplier = TimingConfig::getNoteIntervalMultiplier(currentNoteInterval_);
        float calculatedMs = msPerBeat * multiplier;

        juce::String text;
        if (calculatedMs >= 1000.0f)
        {
            text = juce::String(calculatedMs / 1000.0f, 2) + " s";
        }
        else
        {
            text = juce::String(static_cast<int>(calculatedMs)) + " ms";
        }
        calculatedIntervalLabel_->setText("= " + text, juce::dontSendNotification);
    }
}

void TimingSidebarSection::setTimingMode(TimingMode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        modeToggle_->setSelectedId(static_cast<int>(mode));
        updateModeVisibility();
    }
}

void TimingSidebarSection::setTimeIntervalMs(int ms)
{
    currentTimeIntervalMs_ = ms;
    timeIntervalSlider_->setValue(ms, juce::dontSendNotification);
    updateCalculatedInterval();
}

void TimingSidebarSection::setNoteInterval(NoteInterval interval)
{
    currentNoteInterval_ = interval;
    noteIntervalSelector_->setSelectedId(static_cast<int>(interval) + 1, juce::dontSendNotification);
    updateCalculatedInterval();
}

void TimingSidebarSection::setHostSyncEnabled(bool enabled)
{
    hostSyncEnabled_ = enabled;
    hostSyncToggle_->setToggleState(enabled, juce::dontSendNotification);
    resized();
}

void TimingSidebarSection::setResetOnPlayEnabled(bool enabled)
{
    resetOnPlayEnabled_ = enabled;
    resetOnPlayToggle_->setToggleState(enabled, juce::dontSendNotification);
}

void TimingSidebarSection::setHostBPM(float bpm)
{
    hostBPM_ = bpm;
    bpmValueLabel_->setText(juce::String(bpm, 1), juce::dontSendNotification);
    updateCalculatedInterval();
}

void TimingSidebarSection::setSyncStatus(bool synced)
{
    if (isSynced_ != synced)
    {
        isSynced_ = synced;
        const auto& theme = ThemeManager::getInstance().getCurrentTheme();
        syncStatusLabel_->setColour(juce::Label::textColourId,
                                     synced ? theme.statusActive : theme.textSecondary);
        syncStatusLabel_->setText(synced ? "Synced" : "Not synced", juce::dontSendNotification);
    }
}

void TimingSidebarSection::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void TimingSidebarSection::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

int TimingSidebarSection::getPreferredHeight() const
{
    using namespace SectionLayout;

    int height = SECTION_PADDING * 2;                    // Top and bottom padding
    height += LABEL_HEIGHT + SPACING_MEDIUM;             // Section header
    height += ROW_HEIGHT + SPACING_LARGE;                // Mode toggle

    if (currentMode_ == TimingMode::TIME)
    {
        height += ROW_HEIGHT + SPACING_SMALL;            // Interval label/value row
        height += ROW_HEIGHT + SPACING_LARGE;            // Slider
    }
    else
    {
        height += ROW_HEIGHT + SPACING_LARGE;            // Note interval row
        height += ROW_HEIGHT + SPACING_MEDIUM;           // BPM row
        height += LABEL_HEIGHT + SPACING_LARGE;          // Calculated interval
    }

    height += ROW_HEIGHT + SPACING_MEDIUM;               // Host sync toggle
    height += ROW_HEIGHT + SPACING_MEDIUM;               // Reset on play toggle

    if (hostSyncEnabled_)
    {
        height += LABEL_HEIGHT;                          // Sync status
    }

    return height;
}

void TimingSidebarSection::notifyTimingModeChanged()
{
    listeners_.call([this](Listener& l) { l.timingModeChanged(currentMode_); });
}

void TimingSidebarSection::notifyNoteIntervalChanged()
{
    listeners_.call([this](Listener& l) { l.noteIntervalChanged(currentNoteInterval_); });
}

void TimingSidebarSection::notifyTimeIntervalChanged()
{
    listeners_.call([this](Listener& l) { l.timeIntervalChanged(currentTimeIntervalMs_); });
}

void TimingSidebarSection::notifyHostSyncChanged()
{
    listeners_.call([this](Listener& l) { l.hostSyncChanged(hostSyncEnabled_); });
}

void TimingSidebarSection::notifyResetOnPlayChanged()
{
    listeners_.call([this](Listener& l) { l.resetOnPlayChanged(resetOnPlayEnabled_); });
}

} // namespace oscil
