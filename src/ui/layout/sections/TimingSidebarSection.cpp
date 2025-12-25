/*
    Oscil - Timing Sidebar Section Implementation
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#include "ui/layout/sections/TimingSidebarSection.h"

namespace oscil
{

TimingSidebarSection::TimingSidebarSection(ServiceContext& context)
    : presenter_(std::make_unique<TimingPresenter>())
    , themeService_(context.themeService)
{
    OSCIL_REGISTER_TEST_ID("sidebar_timing");
    setupComponents();
    setupPresenterCallbacks();
    themeService_.addListener(this);
    updateModeVisibility();
}

TimingSidebarSection::TimingSidebarSection(IThemeService& themeService)
    : presenter_(std::make_unique<TimingPresenter>())
    , themeService_(themeService)
{
    OSCIL_REGISTER_TEST_ID("sidebar_timing");
    setupComponents();
    setupPresenterCallbacks();
    themeService_.addListener(this);
    updateModeVisibility();
}

TimingSidebarSection::~TimingSidebarSection()
{
    themeService_.removeListener(this);
}

void TimingSidebarSection::setupPresenterCallbacks()
{
    presenter_->setOnTimingModeChanged([this](TimingMode mode) {
        listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
    });

    presenter_->setOnNoteIntervalChanged([this](NoteInterval interval) {
        listeners_.call([interval](Listener& l) { l.noteIntervalChanged(interval); });
    });

    presenter_->setOnTimeIntervalChanged([this](float ms) {
        listeners_.call([ms](Listener& l) { l.timeIntervalChanged(ms); });
    });

    presenter_->setOnHostSyncChanged([this](bool enabled) {
        listeners_.call([enabled](Listener& l) { l.hostSyncChanged(enabled); });
    });

    presenter_->setOnWaveformModeChanged([this](WaveformMode mode) {
        listeners_.call([mode](Listener& l) { l.waveformModeChanged(mode); });
    });

    presenter_->setOnBPMChanged([this](float bpm) {
        listeners_.call([bpm](Listener& l) { l.bpmChanged(bpm); });
    });

    presenter_->setOnStateChanged([this]() {
        updateUi();
        updateModeVisibility();
    });
}

void TimingSidebarSection::setupComponents()
{
    // TIME/MELODIC toggle with helpful tooltips
    modeToggle_ = std::make_unique<SegmentedButtonBar>(themeService_);
    modeToggle_->addButton("TIME", static_cast<int>(TimingMode::TIME), "sidebar_timing_modeToggle_time",
                           "Time Mode: Display fixed time intervals (milliseconds)");
    modeToggle_->addButton("MELODIC", static_cast<int>(TimingMode::MELODIC), "sidebar_timing_modeToggle_melodic",
                           "Melodic Mode: Display musical note intervals (syncs with BPM)");
    modeToggle_->setSelectedId(static_cast<int>(presenter_->getTimingMode()));
    modeToggle_->onSelectionChanged = [this](int id)
    {
        presenter_->setTimingMode(static_cast<TimingMode>(id));
    };
    addAndMakeVisible(*modeToggle_);
    OSCIL_REGISTER_CHILD_TEST_ID(*modeToggle_, "sidebar_timing_modeToggle");

    // Waveform mode dropdown (both modes)
    waveformModeLabel_ = std::make_unique<juce::Label>();
    waveformModeLabel_->setText("Mode", juce::dontSendNotification);
    waveformModeLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*waveformModeLabel_);

    waveformModeSelector_ = std::make_unique<OscilDropdown>(themeService_, "Select mode...", "sidebar_timing_waveformModeDropdown");
    populateWaveformModeSelector();
    waveformModeSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index <= 2)
        {
            presenter_->setWaveformMode(static_cast<WaveformMode>(index));
        }
    };
    addAndMakeVisible(*waveformModeSelector_);

    // TIME mode controls - input field only (full width)
    timeIntervalField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Number, "sidebar_timing_intervalField");
    timeIntervalField_->setRange(0.1, 4000.0);
    timeIntervalField_->setStep(0.1);
    timeIntervalField_->setDecimalPlaces(1);
    timeIntervalField_->setSuffix("ms");
    timeIntervalField_->setNumericValue(presenter_->getTimeIntervalMs(), false);
    timeIntervalField_->onValueChanged = [this](double value)
    {
        presenter_->setTimeIntervalMs(static_cast<float>(value));
    };
    addAndMakeVisible(*timeIntervalField_);

    // MELODIC mode controls - dropdown only (full width)
    noteIntervalSelector_ = std::make_unique<OscilDropdown>(themeService_, "Select note...", "sidebar_timing_noteDropdown");
    populateNoteIntervalSelector();
    noteIntervalSelector_->onSelectionChanged = [this](int index)
    {
        if (index >= 0 && index <= 16)
        {
            presenter_->setNoteInterval(static_cast<NoteInterval>(index));
        }
    };
    addAndMakeVisible(*noteIntervalSelector_);

    // Sync toggle (MELODIC mode only)
    syncToggle_ = std::make_unique<OscilToggle>(themeService_, "Sync", "sidebar_timing_syncToggle");
    syncToggle_->setValue(presenter_->isHostSyncEnabled(), false);
    syncToggle_->onValueChanged = [this](bool value)
    {
        presenter_->setHostSyncEnabled(value);
    };
    addAndMakeVisible(*syncToggle_);

    // BPM controls (MELODIC mode only)
    bpmLabel_ = std::make_unique<juce::Label>();
    bpmLabel_->setText("BPM", juce::dontSendNotification);
    bpmLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*bpmLabel_);

    // BPM field (Free Running mode)
    bpmField_ = std::make_unique<OscilTextField>(themeService_, TextFieldVariant::Number, "sidebar_timing_bpmField");
    bpmField_->setRange(20.0, 300.0);
    bpmField_->setStep(0.1);
    bpmField_->setDecimalPlaces(1);
    bpmField_->setNumericValue(presenter_->getInternalBPM(), false);
    bpmField_->onValueChanged = [this](double value)
    {
        presenter_->setInternalBPM(static_cast<float>(value));
    };
    addAndMakeVisible(*bpmField_);

    // BPM value label (Host Sync mode - read-only)
    bpmValueLabel_ = std::make_unique<juce::Label>();
    bpmValueLabel_->setText(juce::String(presenter_->getHostBPM(), 1), juce::dontSendNotification);
    bpmValueLabel_->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*bpmValueLabel_);
    OSCIL_REGISTER_CHILD_TEST_ID(*bpmValueLabel_, "sidebar_timing_bpmDisplay");

    // Apply initial theme
    themeChanged(themeService_.getCurrentTheme());
}

void TimingSidebarSection::populateWaveformModeSelector()
{
    waveformModeSelector_->clearItems();
    waveformModeSelector_->addItem("Free Running");
    waveformModeSelector_->addItem("Restart on Play");
    waveformModeSelector_->addItem("Restart on Note");
    waveformModeSelector_->setSelectedIndex(static_cast<int>(presenter_->getWaveformMode()), false);
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
    noteIntervalSelector_->setSelectedIndex(static_cast<int>(presenter_->getNoteInterval()), false);
}

void TimingSidebarSection::paint(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds();

    // Background
    g.setColour(theme.backgroundPane);
    g.fillRect(bounds);

    // Top separator line
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    // Draw styled SYNCED pill badge if synced (MELODIC mode + Host Sync)
    if (presenter_->shouldShowSyncedBadge())
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
    int selectorWidth = std::max(0, bounds.getWidth() - labelWidth - SPACING_MEDIUM);
    waveformModeSelector_->setBounds(bounds.getX() + labelWidth + SPACING_MEDIUM, y,
                                      selectorWidth, ROW_HEIGHT);
    y += ROW_HEIGHT + SPACING_LARGE;

    // TIME mode controls
    if (presenter_->isTimeMode())
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
        int bpmFieldWidth = std::max(0, bounds.getWidth() - bpmLabelWidth - syncToggleWidth - SPACING_MEDIUM * 2);

        bpmLabel_->setBounds(bounds.getX(), y, bpmLabelWidth, ROW_HEIGHT);

        if (presenter_->shouldShowBPMValue())
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

    repaint();
}

void TimingSidebarSection::updateModeVisibility()
{
    // TIME mode controls
    timeIntervalField_->setVisible(presenter_->isTimeMode());

    // MELODIC mode controls
    noteIntervalSelector_->setVisible(presenter_->isMelodicMode());
    syncToggle_->setVisible(presenter_->shouldShowSyncToggle());

    bpmLabel_->setVisible(presenter_->isMelodicMode());
    bpmField_->setVisible(presenter_->shouldShowBPMField());
    bpmValueLabel_->setVisible(presenter_->shouldShowBPMValue());

    resized();
    repaint();

    // Notify parent (CollapsibleSection) that our preferred height may have changed
    notifyHeightChanged();
}

void TimingSidebarSection::updateUi()
{
    modeToggle_->setSelectedId(static_cast<int>(presenter_->getTimingMode()));
    waveformModeSelector_->setSelectedIndex(static_cast<int>(presenter_->getWaveformMode()), false);
    
    timeIntervalField_->setNumericValue(presenter_->getTimeIntervalMs(), false);
    
    noteIntervalSelector_->setSelectedIndex(static_cast<int>(presenter_->getNoteInterval()), false);
    
    syncToggle_->setValue(presenter_->isHostSyncEnabled(), false);
    
    bpmField_->setNumericValue(presenter_->getInternalBPM(), false);
    
    bpmValueLabel_->setText(juce::String(presenter_->getHostBPM(), 1), juce::dontSendNotification);
    
    // Repaint to show/hide SYNCED pill badge which relies on paint() checking isSynced()
    repaint();
}

void TimingSidebarSection::setTimingMode(TimingMode mode)
{
    presenter_->setTimingMode(mode);
}

void TimingSidebarSection::setTimeIntervalMs(float ms)
{
    presenter_->setTimeIntervalMs(ms);
}

void TimingSidebarSection::setNoteInterval(NoteInterval interval)
{
    presenter_->setNoteInterval(interval);
}

void TimingSidebarSection::setHostSyncEnabled(bool enabled)
{
    presenter_->setHostSyncEnabled(enabled);
}

void TimingSidebarSection::setWaveformMode(WaveformMode mode)
{
    presenter_->setWaveformMode(mode);
}

void TimingSidebarSection::setHostBPM(float bpm)
{
    presenter_->setHostBPM(bpm);
}

void TimingSidebarSection::setInternalBPM(float bpm)
{
    presenter_->setInternalBPM(bpm);
}

void TimingSidebarSection::setSyncStatus(bool synced)
{
    presenter_->setSyncStatus(synced);
}

TimingMode TimingSidebarSection::getTimingMode() const
{
    return presenter_->getTimingMode();
}

float TimingSidebarSection::getTimeIntervalMs() const
{
    return presenter_->getTimeIntervalMs();
}

NoteInterval TimingSidebarSection::getNoteInterval() const
{
    return presenter_->getNoteInterval();
}

bool TimingSidebarSection::isHostSyncEnabled() const
{
    return presenter_->isHostSyncEnabled();
}

WaveformMode TimingSidebarSection::getWaveformMode() const
{
    return presenter_->getWaveformMode();
}

float TimingSidebarSection::getInternalBPM() const
{
    return presenter_->getInternalBPM();
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

    if (presenter_->isTimeMode())
    {
        height += 32 + SPACING_LARGE;                    // Time interval input field (full width)
    }
    else
    {
        height += ROW_HEIGHT + SPACING_LARGE;            // Note interval dropdown (full width)
        height += ROW_HEIGHT + SPACING_LARGE;            // BPM label + field + sync toggle row

        if (presenter_->shouldShowSyncedBadge())
        {
            height += LABEL_HEIGHT;                      // Sync status badge
        }
    }

    return height;
}

void TimingSidebarSection::notifyHeightChanged()
{
    if (onPreferredHeightChanged)
        onPreferredHeightChanged();
}

} // namespace oscil
