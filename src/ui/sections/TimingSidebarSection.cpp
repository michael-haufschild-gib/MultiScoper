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
    // TIME/MELODIC toggle
    modeToggle_ = std::make_unique<SegmentedButtonBar>();
    modeToggle_->addButton("TIME", static_cast<int>(TimingMode::TIME), "sidebar_timing_modeToggle_time");
    modeToggle_->addButton("MELODIC", static_cast<int>(TimingMode::MELODIC), "sidebar_timing_modeToggle_melodic");
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

    // TIME mode controls - input field only (full width)
    timeIntervalField_ = std::make_unique<OscilTextField>(TextFieldVariant::Number, "sidebar_timing_intervalField");
    timeIntervalField_->setRange(0.1, 4000.0);
    timeIntervalField_->setStep(0.1);
    timeIntervalField_->setDecimalPlaces(1);
    timeIntervalField_->setSuffix("ms");
    timeIntervalField_->setNumericValue(currentTimeIntervalMs_, false);
    timeIntervalField_->onValueChanged = [this](double value)
    {
        currentTimeIntervalMs_ = static_cast<float>(value);
        notifyTimeIntervalChanged();
    };
    addAndMakeVisible(*timeIntervalField_);

    // MELODIC mode controls - dropdown only (full width)
    noteIntervalSelector_ = std::make_unique<OscilDropdown>("Select note...", "sidebar_timing_noteDropdown");
    populateNoteIntervalSelector();
    noteIntervalSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index <= 16)
        {
            currentNoteInterval_ = static_cast<NoteInterval>(index);
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

    // BPM field (Free Running mode)
    bpmField_ = std::make_unique<OscilTextField>(TextFieldVariant::Number, "sidebar_timing_bpmField");
    bpmField_->setRange(20.0, 300.0);
    bpmField_->setStep(0.1);
    bpmField_->setDecimalPlaces(1);
    bpmField_->setNumericValue(internalBPM_, false);
    bpmField_->onValueChanged = [this](double value)
    {
        internalBPM_ = static_cast<float>(value);
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
    int y = bounds.getY();

    // MODE toggle (TIME/MELODIC) - First element now
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
        // Input field spans full width
        int fieldHeight = 32;
        timeIntervalField_->setBounds(bounds.getX(), y, bounds.getWidth(), fieldHeight);
        y += fieldHeight + SPACING_LARGE;
    }
    else // MELODIC mode
    {
        // Note interval dropdown spans full width
        noteIntervalSelector_->setBounds(bounds.getX(), y, bounds.getWidth(), ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;

        // BPM row: BPM label + field/value + sync toggle
        int bpmLabelWidth = 30;
        int syncToggleWidth = 70;
        int bpmFieldWidth = bounds.getWidth() - bpmLabelWidth - syncToggleWidth - SPACING_MEDIUM * 2;

        bpmLabel_->setBounds(bounds.getX(), y, bpmLabelWidth, ROW_HEIGHT);

        if (hostSyncEnabled_)
        {
            // Host Sync mode: Show read-only BPM value
            bpmValueLabel_->setBounds(bounds.getX() + bpmLabelWidth + SPACING_MEDIUM, y, bpmFieldWidth, ROW_HEIGHT);
        }
        else
        {
            // Free Running mode: Show editable BPM field
            bpmField_->setBounds(bounds.getX() + bpmLabelWidth + SPACING_MEDIUM, y, bpmFieldWidth, ROW_HEIGHT);
        }

        syncToggle_->setBounds(bounds.getX() + bpmLabelWidth + SPACING_MEDIUM + bpmFieldWidth + SPACING_MEDIUM, y, syncToggleWidth, ROW_HEIGHT);
        y += ROW_HEIGHT + SPACING_LARGE;
    }

    // Sync status - hide the label since we draw a styled pill badge in paint()
    syncStatusLabel_->setVisible(false);
}

void TimingSidebarSection::themeChanged(const ColorTheme& newTheme)
{
    // Oscil components handle their own theming
    // Style the remaining JUCE Labels

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
    timeIntervalField_->setVisible(isTimeMode);

    // MELODIC mode controls
    noteIntervalSelector_->setVisible(!isTimeMode);
    syncToggle_->setVisible(!isTimeMode);

    // BPM controls visibility depends on sync mode
    bool showFreeRunningBPM = !isTimeMode && !hostSyncEnabled_;
    bool showHostSyncBPM = !isTimeMode && hostSyncEnabled_;

    bpmLabel_->setVisible(!isTimeMode);
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

void TimingSidebarSection::setTimeIntervalMs(float ms)
{
    currentTimeIntervalMs_ = ms;
    timeIntervalField_->setNumericValue(static_cast<double>(ms), false);
}

void TimingSidebarSection::setNoteInterval(NoteInterval interval)
{
    currentNoteInterval_ = interval;
    int index = static_cast<int>(interval);
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
    height += ROW_HEIGHT + SPACING_LARGE;                // Mode toggle (TIME/MELODIC)
    height += ROW_HEIGHT + SPACING_LARGE;                // Waveform mode dropdown

    if (currentMode_ == TimingMode::TIME)
    {
        height += 32 + SPACING_LARGE;                    // Time interval input field (full width)
    }
    else
    {
        height += ROW_HEIGHT + SPACING_LARGE;            // Note interval dropdown (full width)
        height += ROW_HEIGHT + SPACING_LARGE;            // BPM label + field + sync toggle row

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

void TimingSidebarSection::notifyHeightChanged()
{
    if (onPreferredHeightChanged)
        onPreferredHeightChanged();
}

} // namespace oscil
