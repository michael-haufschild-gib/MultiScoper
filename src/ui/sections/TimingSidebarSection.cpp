/*
    Oscil - Timing Sidebar Section Implementation
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#include "ui/sections/TimingSidebarSection.h"

namespace oscil
{

TimingSidebarSection::TimingSidebarSection()
{
    OSCIL_REGISTER_TEST_ID("sidebar_timing");
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
    OSCIL_REGISTER_CHILD_TEST_ID(*modeToggle_, "sidebar_timing_modeToggle");

    // TIME mode controls
    timeIntervalSlider_ = std::make_unique<OscilSlider>("sidebar_timing_intervalSlider");
    timeIntervalSlider_->setLabel("Interval");
    timeIntervalSlider_->setRange(1.0, 1000.0);
    timeIntervalSlider_->setStep(1.0);
    timeIntervalSlider_->setValue(currentTimeIntervalMs_, false);
    timeIntervalSlider_->setSuffix(" ms");
    timeIntervalSlider_->setSkewFactor(0.3);  // Log-like behavior (lower value = more log-like)
    timeIntervalSlider_->onValueChanged = [this](double value)
    {
        currentTimeIntervalMs_ = static_cast<int>(value);
        updateCalculatedInterval();
        notifyTimeIntervalChanged();
    };
    addAndMakeVisible(*timeIntervalSlider_);

    // MELODIC mode controls (per design: "Note Interval")
    noteIntervalLabel_ = std::make_unique<juce::Label>();
    noteIntervalLabel_->setText("Note Interval", juce::dontSendNotification);
    noteIntervalLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*noteIntervalLabel_);

    noteIntervalSelector_ = std::make_unique<OscilDropdown>("sidebar_timing_noteDropdown");
    populateNoteIntervalSelector();
    noteIntervalSelector_->onSelectionChanged = [this](int index)
    {
        // OscilDropdown uses 0-based indices
        if (index >= 0)
        {
            // Map index back to NoteInterval enum
            // This needs to account for separators which aren't in the enum
            // For simplicity, we'll track the mapping in populateNoteIntervalSelector
            currentNoteInterval_ = static_cast<NoteInterval>(index);
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
    OSCIL_REGISTER_CHILD_TEST_ID(*bpmValueLabel_, "sidebar_timing_bpmDisplay");

    // Calculated interval display
    calculatedIntervalLabel_ = std::make_unique<juce::Label>();
    calculatedIntervalLabel_->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*calculatedIntervalLabel_);
    OSCIL_REGISTER_CHILD_TEST_ID(*calculatedIntervalLabel_, "sidebar_timing_intervalDisplay");
    updateCalculatedInterval();

    // Host sync toggle
    hostSyncToggle_ = std::make_unique<OscilToggle>("Sync to Host", "sidebar_timing_hostSyncToggle");
    hostSyncToggle_->setValue(hostSyncEnabled_, false);
    hostSyncToggle_->onValueChanged = [this](bool value)
    {
        hostSyncEnabled_ = value;
        notifyHostSyncChanged();
    };
    addAndMakeVisible(*hostSyncToggle_);

    // Reset on play toggle
    resetOnPlayToggle_ = std::make_unique<OscilToggle>("Reset on Play", "sidebar_timing_resetOnPlayToggle");
    resetOnPlayToggle_->setValue(resetOnPlayEnabled_, false);
    resetOnPlayToggle_->onValueChanged = [this](bool value)
    {
        resetOnPlayEnabled_ = value;
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
    noteIntervalSelector_->clearItems();

    // OscilDropdown uses 0-based indices, not IDs
    // We need to map index to NoteInterval enum value
    // Store the mapping in order

    // Standard notes (per design: "1/16th", "1/8th", "1/4", "1/2", "1 Bar")
    noteIntervalSelector_->addItem("1/32nd");     // index 0 = NoteInterval::THIRTY_SECOND
    noteIntervalSelector_->addItem("1/16th");     // index 1 = NoteInterval::SIXTEENTH
    noteIntervalSelector_->addItem("1/12th");     // index 2 = NoteInterval::TWELFTH
    noteIntervalSelector_->addItem("1/8th");      // index 3 = NoteInterval::EIGHTH
    noteIntervalSelector_->addItem("1/4");        // index 4 = NoteInterval::QUARTER
    noteIntervalSelector_->addItem("1/2");        // index 5 = NoteInterval::HALF
    noteIntervalSelector_->addItem("1 Bar");      // index 6 = NoteInterval::WHOLE
    noteIntervalSelector_->addItem("2 Bars");     // index 7 = NoteInterval::TWO_BARS
    noteIntervalSelector_->addItem("3 Bars");     // index 8 = NoteInterval::THREE_BARS
    noteIntervalSelector_->addItem("4 Bars");     // index 9 = NoteInterval::FOUR_BARS
    noteIntervalSelector_->addItem("8 Bars");     // index 10 = NoteInterval::EIGHT_BARS
    noteIntervalSelector_->addItem("1/8th.");     // index 11 = NoteInterval::DOTTED_EIGHTH
    noteIntervalSelector_->addItem("1/4.");       // index 12 = NoteInterval::DOTTED_QUARTER
    noteIntervalSelector_->addItem("1/2.");       // index 13 = NoteInterval::DOTTED_HALF
    noteIntervalSelector_->addItem("1/8th T");    // index 14 = NoteInterval::TRIPLET_EIGHTH
    noteIntervalSelector_->addItem("1/4 T");      // index 15 = NoteInterval::TRIPLET_QUARTER
    noteIntervalSelector_->addItem("1/2 T");      // index 16 = NoteInterval::TRIPLET_HALF

    // Set current selection using index (NoteInterval enum values are sequential)
    noteIntervalSelector_->setSelectedIndex(static_cast<int>(currentNoteInterval_), false);
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
        // OscilSlider with integrated label
        timeIntervalSlider_->setBounds(bounds.getX(), y, bounds.getWidth(), 40);
        y += 40 + SPACING_LARGE;
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
    // Oscil components (OscilSlider, OscilDropdown, OscilToggle) handle their own theming
    // automatically via ThemeManagerListener. We only need to style the remaining JUCE Labels.

    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // MELODIC mode labels
    noteIntervalLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    bpmLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    bpmValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);
    calculatedIntervalLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);

    // Sync status
    syncStatusLabel_->setColour(juce::Label::textColourId,
                                 isSynced_ ? newTheme.statusActive : newTheme.textSecondary);

    repaint();
}

void TimingSidebarSection::updateModeVisibility()
{
    bool isTimeMode = (currentMode_ == TimingMode::TIME);

    // TIME mode controls
    timeIntervalSlider_->setVisible(isTimeMode);

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
        // OscilSlider handles its own display - nothing to update
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
    timeIntervalSlider_->setValue(ms, false);
    updateCalculatedInterval();
}

void TimingSidebarSection::setNoteInterval(NoteInterval interval)
{
    currentNoteInterval_ = interval;
    noteIntervalSelector_->setSelectedIndex(static_cast<int>(interval), false);
    updateCalculatedInterval();
}

void TimingSidebarSection::setHostSyncEnabled(bool enabled)
{
    hostSyncEnabled_ = enabled;
    hostSyncToggle_->setValue(enabled, false);
    resized();
}

void TimingSidebarSection::setResetOnPlayEnabled(bool enabled)
{
    resetOnPlayEnabled_ = enabled;
    resetOnPlayToggle_->setValue(enabled, false);
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
        height += 40 + SPACING_LARGE;                    // OscilSlider (integrated label)
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
