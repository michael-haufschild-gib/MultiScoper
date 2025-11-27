/*
    Oscil - Timing Sidebar Section
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/SegmentedButtonBar.h"
#include "ui/sections/SectionConstants.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilToggle.h"
#include "dsp/TimingConfig.h"
#include <functional>

namespace oscil
{

/**
 * Timing section component for the sidebar
 * Provides TIME/MELODIC mode toggle, interval controls, and host sync options
 */
class TimingSidebarSection : public juce::Component,
                              public ThemeManagerListener
{
public:
    /**
     * Listener interface for timing changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void timingModeChanged(TimingMode /*mode*/) {}
        virtual void noteIntervalChanged(NoteInterval /*interval*/) {}
        virtual void timeIntervalChanged(int /*ms*/) {}
        virtual void hostSyncChanged(bool /*enabled*/) {}
        virtual void resetOnPlayChanged(bool /*enabled*/) {}
    };

    TimingSidebarSection();
    ~TimingSidebarSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters (for external updates)
    void setTimingMode(TimingMode mode);
    void setTimeIntervalMs(int ms);
    void setNoteInterval(NoteInterval interval);
    void setHostSyncEnabled(bool enabled);
    void setResetOnPlayEnabled(bool enabled);
    void setHostBPM(float bpm);
    void setSyncStatus(bool synced);

    // State getters
    TimingMode getTimingMode() const { return currentMode_; }
    int getTimeIntervalMs() const { return currentTimeIntervalMs_; }
    NoteInterval getNoteInterval() const { return currentNoteInterval_; }
    bool isHostSyncEnabled() const { return hostSyncEnabled_; }
    bool isResetOnPlayEnabled() const { return resetOnPlayEnabled_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

private:
    void setupComponents();
    void updateModeVisibility();
    void updateCalculatedInterval();
    void populateNoteIntervalSelector();

    void notifyTimingModeChanged();
    void notifyNoteIntervalChanged();
    void notifyTimeIntervalChanged();
    void notifyHostSyncChanged();
    void notifyResetOnPlayChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // TIME/MELODIC toggle
    std::unique_ptr<SegmentedButtonBar> modeToggle_;

    // TIME mode controls
    std::unique_ptr<OscilSlider> timeIntervalSlider_;

    // MELODIC mode controls
    std::unique_ptr<juce::Label> noteIntervalLabel_;
    std::unique_ptr<OscilDropdown> noteIntervalSelector_;

    // BPM display
    std::unique_ptr<juce::Label> bpmLabel_;
    std::unique_ptr<juce::Label> bpmValueLabel_;

    // Calculated interval display
    std::unique_ptr<juce::Label> calculatedIntervalLabel_;

    // Host sync toggle
    std::unique_ptr<OscilToggle> hostSyncToggle_;

    // Reset on play toggle
    std::unique_ptr<OscilToggle> resetOnPlayToggle_;

    // Sync status indicator
    std::unique_ptr<juce::Label> syncStatusLabel_;

    // State
    TimingMode currentMode_ = TimingMode::TIME;
    int currentTimeIntervalMs_ = 50;
    NoteInterval currentNoteInterval_ = NoteInterval::QUARTER;
    float hostBPM_ = 120.0f;
    bool hostSyncEnabled_ = false;
    bool resetOnPlayEnabled_ = false;
    bool isSynced_ = false;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimingSidebarSection)
};

} // namespace oscil
