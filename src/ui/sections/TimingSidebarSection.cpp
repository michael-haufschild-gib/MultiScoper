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

    // Waveform mode dropdown (both modes)
    waveformModeLabel_ = std::make_unique<juce::Label>();
    waveformModeLabel_->setText("Mode", juce::dontSendNotification);
    waveformModeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*waveformModeLabel_);

    waveformModeSelector_ = std::make_unique<OscilDropdown>("Select mode...", "sidebar_timing_waveformModeDropdown");
    populateWaveformModeSelector();
    waveformModeSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index <= 2)
        {
            waveformMode_ = static_cast<WaveformMode>(index);
            notifyWaveformModeChanged();
        }
    };
    addAndMakeVisible(*waveformModeSelector_);

    // TIME mode controls
    timeIntervalSlider_ = std::make_unique<OscilSlider>("sidebar_timing_intervalSlider");
    timeIntervalSlider_->setLabel("Interval");
    timeIntervalSlider_->setRange(1.0, 1000.0);
    timeIntervalSlider_->setStep(1.0);
    timeIntervalSlider_->setValue(currentTimeIntervalMs_, false);
    timeIntervalSlider_->setSuffix(" ms");
    timeIntervalSlider_->setSkewFactor(0.3);  // Log-like behavior
    timeIntervalSlider_->onValueChanged = [this](double value)
    {
        currentTimeIntervalMs_ = static_cast<int>(value);
        timeIntervalField_->setNumericValue(value, false);
        notifyTimeIntervalChanged();
    };
    addAndMakeVisible(*timeIntervalSlider_);

    timeIntervalField_ = std::make_unique<OscilTextField>(TextFieldVariant::Number, "sidebar_timing_intervalField");
    timeIntervalField_->setRange(1.0, 1000.0);
    timeIntervalField_->setStep(1.0);
    timeIntervalField_->setDecimalPlaces(0);
    timeIntervalField_->setSuffix(" ms");
    timeIntervalField_->setNumericValue(currentTimeIntervalMs_, false);
    timeIntervalField_->onValueChanged = [this](double value)
    {
        currentTimeIntervalMs_ = static_cast<int>(value);
        timeIntervalSlider_->setValue(value, false);
        notifyTimeIntervalChanged();
    };
    addAndMakeVisible(*timeIntervalField_);

    // MELODIC mode controls - Note interval slider
    noteIntervalSlider_ = std::make_unique<OscilSlider>("sidebar_timing_noteSlider");
    noteIntervalSlider_->setLabel("Note Interval");
    noteIntervalSlider_->setRange(0.0, 16.0);  // 17 note intervals (0-16)
    noteIntervalSlider_->setStep(1.0);
    noteIntervalSlider_->setValue(static_cast<double>(currentNoteInterval_), false);
    noteIntervalSlider_->onValueChanged = [this](double value)
    {
        int index = static_cast<int>(value);
        if (index >= 0 && index <= 16)
        {
            currentNoteInterval_ = static_cast<NoteInterval>(index);
            noteIntervalSelector_->setSelectedIndex(index, false);
            notifyNoteIntervalChanged();
        }
    };
    // Custom formatter to show note names
    noteIntervalSlider_->setValueFormatter([](double value) -> juce::String
    {
        int index = static_cast<int>(value);
        if (index >= 0 && index <= 16)
        {
            return noteIntervalToString(static_cast<NoteInterval>(index));
        }
        return "1/4";
    });
    addAndMakeVisible(*noteIntervalSlider_);

    // Note interval dropdown
    noteIntervalSelector_ = std::make_unique<OscilDropdown>("Select note...", "sidebar_timing_noteDropdown");
    populateNoteIntervalSelector();
    noteIntervalSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index <= 16)
        {
            currentNoteInterval_ = static_cast<NoteInterval>(index);
            noteIntervalSlider_->setValue(static_cast<double>(index), false);
            notifyNoteIntervalChanged();
        }
    };
    addAndMakeVisible(*noteIntervalSelector_);

    // Sync toggle (MELODIC mode only)
    syncToggle_ = std::make_unique<OscilToggle>("Sync", "sidebar_timing_syncToggle");
    syncToggle_->setValue(hostSyncEnabled_, false);
    syncToggle_->onValueChanged = [this](bool value)
    {
        hostSyncEnabled_ = value;
        updateModeVisibility();  // Update visibility of BPM controls
        notifyHostSyncChanged();
    };
    addAndMakeVisible(*syncToggle_);

    // BPM controls (MELODIC mode only)
    bpmLabel_ = std::make_unique<juce::Label>();
    bpmLabel_->setText("BPM", juce::dontSendNotification);
    bpmLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*bpmLabel_);

    // BPM slider (Free Running mode)
    bpmSlider_ = std::make_unique<OscilSlider>("sidebar_timing_bpmSlider");
    bpmSlider_->setLabel("BPM");
    bpmSlider_->setRange(20.0, 300.0);
    bpmSlider_->setStep(0.1);
    bpmSlider_->setValue(internalBPM_, false);
    bpmSlider_->setDecimalPlaces(1);
    bpmSlider_->onValueChanged = [this](double value)
    {
        internalBPM_ = static_cast<float>(value);
        bpmField_->setNumericValue(value, false);
        notifyBpmChanged();
    };
    addAndMakeVisible(*bpmSlider_);

    // BPM field (Free Running mode)
    bpmField_ = std::make_unique<OscilTextField>(TextFieldVariant::Number, "sidebar_timing_bpmField");
    bpmField_->setRange(20.0, 300.0);
    bpmField_->setStep(0.1);
    bpmField_->setDecimalPlaces(1);
    bpmField_->setNumericValue(internalBPM_, false);
    bpmField_->onValueChanged = [this](double value)
    {
        internalBPM_ = static_cast<float>(value);
        bpmSlider_->setValue(value, false);
        notifyBpmChanged();
    };
    addAndMakeVisible(*bpmField_);

    // BPM value label (Host Sync mode - read-only)
    bpmValueLabel_ = std::make_unique<juce::Label>();
    bpmValueLabel_->setText(juce::String(hostBPM_, 1), juce::dontSendNotification);
    bpmValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*bpmValueLabel_);
    OSCIL_REGISTER_CHILD_TEST_ID(*bpmValueLabel_, "sidebar_timing_bpmDisplay");

    // Sync status
    syncStatusLabel_ = std::make_unique<juce::Label>();
    syncStatusLabel_->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*syncStatusLabel_);

    // Apply initial theme
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

void TimingSidebarSection::populateWaveformModeSelector()
{
    waveformModeSelector_->clearItems();
    waveformModeSelector_->addItem("Free Running");
    waveformModeSelector_->addItem("Restart on Play");
    waveformModeSelector_->addItem("Restart on Note");
    waveformModeSelector_->setSelectedIndex(static_cast<int>(waveformMode_), false);
}

void TimingSidebarSection::populateNoteIntervalSelector()
{
    noteIntervalSelector_->clearItems();

    // All 17 note intervals in order
    noteIntervalSelector_->addItem("1/32nd");     // 0 = THIRTY_SECOND
    noteIntervalSelector_->addItem("1/16th");     // 1 = SIXTEENTH
    noteIntervalSelector_->addItem("1/12th");     // 2 = TWELFTH
    noteIntervalSelector_->addItem("1/8th");      // 3 = EIGHTH
    noteIntervalSelector_->addItem("1/4");        // 4 = QUARTER
    noteIntervalSelector_->addItem("1/2");        // 5 = HALF
    noteIntervalSelector_->addItem("1 Bar");      // 6 = WHOLE
    noteIntervalSelector_->addItem("2 Bars");     // 7 = TWO_BARS
    noteIntervalSelector_->addItem("3 Bars");     // 8 = THREE_BARS
    noteIntervalSelector_->addItem("4 Bars");     // 9 = FOUR_BARS
    noteIntervalSelector_->addItem("8 Bars");     // 10 = EIGHT_BARS
    noteIntervalSelector_->addItem("1/8th.");     // 11 = DOTTED_EIGHTH
    noteIntervalSelector_->addItem("1/4.");       // 12 = DOTTED_QUARTER
    noteIntervalSelector_->addItem("1/2.");       // 13 = DOTTED_HALF
    noteIntervalSelector_->addItem("1/8th T");    // 14 = TRIPLET_EIGHTH
    noteIntervalSelector_->addItem("1/4 T");      // 15 = TRIPLET_QUARTER
    noteIntervalSelector_->addItem("1/2 T");      // 16 = TRIPLET_HALF

    // Set current selection
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

    // Draw styled SYNCED pill badge if synced (MELODIC mode + Host Sync)
    if (currentMode_ == TimingMode::MELODIC && hostSyncEnabled_ && isSynced_)
    {
        int y = getPreferredHeight() - SectionLayout::SECTION_PADDING - SectionLayout::LABEL_HEIGHT;
        auto badgeBounds = juce::Rectangle<float>(
            static_cast<float>(SectionLayout::SECTION_PADDING),
            static_cast<float>(y),
            static_cast<float>(getWidth() - SectionLayout::SECTION_PADDING * 2),
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

    // MODE toggle (TIME/MELODIC)
    modeToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // Waveform mode dropdown (both modes)
    int labelWidth = 40;
    waveformModeLabel_->setBounds(bounds.getX(), y, labelWidth, ROW_HEIGHT);
    waveformModeSelector_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                      bounds.getWidth() - labelWidth - SPACING_MEDIUM, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // TIME mode controls
    if (currentMode_ == TimingMode::TIME)
    {
        // Interval slider
        int sliderWidth = bounds.getWidth() - 70;  // Leave space for text field
        timeIntervalSlider_->setBounds(bounds.getX(), y, sliderWidth, 40);

        // Text field next to slider
        timeIntervalField_->setBounds(bounds.getX() + sliderWidth + SPACING_MEDIUM, y, 60, ROW_HEIGHT);
        y += 40 + SPACING_LARGE;
    }
    else // MELODIC mode
    {
        // Note interval slider
        int sliderWidth = bounds.getWidth() - 70;  // Leave space for dropdown
        noteIntervalSlider_->setBounds(bounds.getX(), y, sliderWidth, 40);

        // Note interval dropdown next to slider
        noteIntervalSelector_->setBounds(bounds.getX() + sliderWidth + SPACING_MEDIUM, y, 60, ROW_HEIGHT);
        y += 40 + SPACING_LARGE;

        // Sync toggle
        syncToggle_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;

        // BPM controls
        if (hostSyncEnabled_)
        {
            // Host Sync mode: Show BPM label and read-only value
            int bpmLabelWidth = 40;
            int bpmValueWidth = bounds.getWidth() - bpmLabelWidth - SPACING_MEDIUM;
            bpmLabel_->setBounds(bounds.getX(), y, bpmLabelWidth, ROW_HEIGHT);
            bpmValueLabel_->setBounds(bounds.getX() + bpmLabelWidth + SPACING_MEDIUM, y, bpmValueWidth, ROW_HEIGHT);
            y += ROW_HEIGHT + SPACING_MEDIUM;
        }
        else
        {
            // Free Running mode: Show BPM slider and field
            int sliderWidth = bounds.getWidth() - 70;
            bpmSlider_->setBounds(bounds.getX(), y, sliderWidth, 40);
            bpmField_->setBounds(bounds.getX() + sliderWidth + SPACING_MEDIUM, y, 60, ROW_HEIGHT);
            y += 40 + SPACING_LARGE;
        }
    }

    // Sync status - hide the label since we draw a styled pill badge in paint()
    syncStatusLabel_->setVisible(false);
}

void TimingSidebarSection::themeChanged(const ColorTheme& newTheme)
{
    // Oscil components handle their own theming
    // Style the remaining JUCE Labels

    // Section label
    sectionLabel_->setColour(juce::Label::textColourId, newTheme.textSecondary);
    sectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));

    // Waveform mode label
    waveformModeLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);

    // MELODIC mode labels
    bpmLabel_->setColour(juce::Label::textColourId, newTheme.textPrimary);
    bpmValueLabel_->setColour(juce::Label::textColourId, newTheme.textHighlight);

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
    timeIntervalField_->setVisible(isTimeMode);

    // MELODIC mode controls
    noteIntervalSlider_->setVisible(!isTimeMode);
    noteIntervalSelector_->setVisible(!isTimeMode);
    syncToggle_->setVisible(!isTimeMode);

    // BPM controls visibility depends on sync mode
    bool showFreeRunningBPM = !isTimeMode && !hostSyncEnabled_;
    bool showHostSyncBPM = !isTimeMode && hostSyncEnabled_;

    bpmLabel_->setVisible(!isTimeMode);
    bpmSlider_->setVisible(showFreeRunningBPM);
    bpmField_->setVisible(showFreeRunningBPM);
    bpmValueLabel_->setVisible(showHostSyncBPM);

    resized();
    repaint();

    // Notify parent (CollapsibleSection) that our preferred height may have changed
    notifyHeightChanged();
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
    timeIntervalField_->setNumericValue(ms, false);
}

void TimingSidebarSection::setNoteInterval(NoteInterval interval)
{
    currentNoteInterval_ = interval;
    int index = static_cast<int>(interval);
    noteIntervalSlider_->setValue(index, false);
    noteIntervalSelector_->setSelectedIndex(index, false);
}

void TimingSidebarSection::setHostSyncEnabled(bool enabled)
{
    hostSyncEnabled_ = enabled;
    syncToggle_->setValue(enabled, false);
    updateModeVisibility();
}

void TimingSidebarSection::setWaveformMode(WaveformMode mode)
{
    waveformMode_ = mode;
    waveformModeSelector_->setSelectedIndex(static_cast<int>(mode), false);
}

void TimingSidebarSection::setHostBPM(float bpm)
{
    hostBPM_ = bpm;
    bpmValueLabel_->setText(juce::String(bpm, 1), juce::dontSendNotification);
}

void TimingSidebarSection::setInternalBPM(float bpm)
{
    internalBPM_ = bpm;
    bpmSlider_->setValue(bpm, false);
    bpmField_->setNumericValue(bpm, false);
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
        repaint();  // Repaint to show/hide SYNCED pill badge
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
    height += ROW_HEIGHT + SPACING_LARGE;                // Mode toggle (TIME/MELODIC)
    height += ROW_HEIGHT + SPACING_LARGE;                // Waveform mode dropdown

    if (currentMode_ == TimingMode::TIME)
    {
        height += 40 + SPACING_LARGE;                    // Interval slider + field
    }
    else
    {
        height += 40 + SPACING_LARGE;                    // Note interval slider + dropdown
        height += ROW_HEIGHT + SPACING_LARGE;            // Sync toggle

        if (hostSyncEnabled_)
        {
            height += ROW_HEIGHT + SPACING_MEDIUM;       // BPM label + value (Host Sync)
        }
        else
        {
            height += 40 + SPACING_LARGE;                // BPM slider + field (Free Running)
        }

        if (hostSyncEnabled_ && isSynced_)
        {
            height += LABEL_HEIGHT;                      // Sync status badge
        }
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

void TimingSidebarSection::notifyWaveformModeChanged()
{
    listeners_.call([this](Listener& l) { l.waveformModeChanged(waveformMode_); });
}

void TimingSidebarSection::notifyBpmChanged()
{
    listeners_.call([this](Listener& l) { l.bpmChanged(internalBPM_); });
}

} // namespace oscil
