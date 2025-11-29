/*
    Oscil - Timing Sidebar Section
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/sections/SectionConstants.h"
#include "ui/sections/DynamicHeightContent.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/TestId.h"
#include "dsp/TimingConfig.h"
#include <functional>

namespace oscil
{

/**
 * Waveform trigger/restart mode
 */
enum class WaveformMode
{
    FreeRunning,    // Waveforms keep running continuously
    RestartOnPlay,  // Waveforms reset when playback starts
    RestartOnNote   // Waveforms reset on MIDI note
};

inline juce::String waveformModeToString(WaveformMode mode)
{
    switch (mode)
    {
        case WaveformMode::FreeRunning:    return "Free Running";
        case WaveformMode::RestartOnPlay:  return "Restart on Play";
        case WaveformMode::RestartOnNote:  return "Restart on Note";
    }
    return "Free Running";
}

inline WaveformMode stringToWaveformMode(const juce::String& str)
{
    if (str == "Restart on Play")  return WaveformMode::RestartOnPlay;
    if (str == "Restart on Note")  return WaveformMode::RestartOnNote;
    return WaveformMode::FreeRunning;
}

/**
 * Timing section component for the sidebar
 * Provides TIME/MELODIC mode toggle, interval controls, and host sync options
 */
class TimingSidebarSection : public juce::Component,
                              public ThemeManagerListener,
                              public DynamicHeightContent
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
        virtual void waveformModeChanged(WaveformMode /*mode*/) {}
        virtual void bpmChanged(float /*bpm*/) {}
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
    void setWaveformMode(WaveformMode mode);
    void setHostBPM(float bpm);
    void setInternalBPM(float bpm);
    void setSyncStatus(bool synced);

    // State getters
    TimingMode getTimingMode() const { return currentMode_; }
    int getTimeIntervalMs() const { return currentTimeIntervalMs_; }
    NoteInterval getNoteInterval() const { return currentNoteInterval_; }
    bool isHostSyncEnabled() const { return hostSyncEnabled_; }
    WaveformMode getWaveformMode() const { return waveformMode_; }
    float getInternalBPM() const { return internalBPM_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout (DynamicHeightContent)
    int getPreferredHeight() const override;

private:
    void setupComponents();
    void updateModeVisibility();
    void populateNoteIntervalSelector();
    void populateWaveformModeSelector();

    void notifyTimingModeChanged();
    void notifyNoteIntervalChanged();
    void notifyTimeIntervalChanged();
    void notifyHostSyncChanged();
    void notifyWaveformModeChanged();
    void notifyBpmChanged();
    void notifyHeightChanged();

    // TIME/MELODIC toggle
    std::unique_ptr<SegmentedButtonBar> modeToggle_;

    // Waveform mode dropdown (both modes)
    std::unique_ptr<juce::Label> waveformModeLabel_;
    std::unique_ptr<OscilDropdown> waveformModeSelector_;

    // TIME mode controls
    std::unique_ptr<OscilTextField> timeIntervalField_;

    // MELODIC mode controls
    std::unique_ptr<OscilDropdown> noteIntervalSelector_;

    // Sync toggle (MELODIC mode only)
    std::unique_ptr<OscilToggle> syncToggle_;

    // BPM controls (MELODIC mode only)
    std::unique_ptr<juce::Label> bpmLabel_;
    std::unique_ptr<OscilTextField> bpmField_;       // Free Running mode
    std::unique_ptr<juce::Label> bpmValueLabel_;     // Host Sync mode (read-only)

    // Sync status indicator
    std::unique_ptr<juce::Label> syncStatusLabel_;

    // State
    TimingMode currentMode_ = TimingMode::TIME;
    int currentTimeIntervalMs_ = 50;
    NoteInterval currentNoteInterval_ = NoteInterval::QUARTER;
    WaveformMode waveformMode_ = WaveformMode::FreeRunning;
    float hostBPM_ = 120.0f;
    float internalBPM_ = 120.0f;
    bool hostSyncEnabled_ = false;
    bool isSynced_ = false;

    juce::ListenerList<Listener> listeners_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimingSidebarSection)
};

} // namespace oscil
