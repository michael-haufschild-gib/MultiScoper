/*
    Oscil - Timing Sidebar Section
    Timing controls section for the sidebar (TIME/MELODIC modes)
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "core/ServiceContext.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/layout/sections/SectionConstants.h"
#include "ui/layout/sections/DynamicHeightContent.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/TestId.h"
#include "core/dsp/TimingConfig.h"
#include "ui/panels/TimingPresenter.h"
#include <functional>

namespace oscil
{

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
        virtual void timeIntervalChanged(float /*ms*/) {}
        virtual void hostSyncChanged(bool /*enabled*/) {}
        virtual void waveformModeChanged(WaveformMode /*mode*/) {}
        virtual void bpmChanged(float /*bpm*/) {}
    };

    /// Construct the timing controls section with the given service context.
    explicit TimingSidebarSection(ServiceContext& context);
    explicit TimingSidebarSection(IThemeService& themeService);
    ~TimingSidebarSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters (for external updates)
    void setTimingMode(TimingMode mode);
    void setTimeIntervalMs(float ms);
    void setNoteInterval(NoteInterval interval);
    void setHostSyncEnabled(bool enabled);
    void setWaveformMode(WaveformMode mode);
    void setHostBPM(float bpm);
    void setInternalBPM(float bpm);
    void setSyncStatus(bool synced);

    // State getters
    TimingMode getTimingMode() const;
    float getTimeIntervalMs() const;
    NoteInterval getNoteInterval() const;
    bool isHostSyncEnabled() const;
    WaveformMode getWaveformMode() const;
    float getInternalBPM() const;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout (DynamicHeightContent)
    int getPreferredHeight() const override;

private:
    void setupComponents();
    void setupModeToggle();
    void setupTimeControls();
    void setupMelodicControls();
    void setupPresenterCallbacks();
    void updateModeVisibility();
    void updateUi(); // Sync UI from presenter
    void populateNoteIntervalSelector();
    void populateWaveformModeSelector();

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

    std::unique_ptr<TimingPresenter> presenter_;

    juce::ListenerList<Listener> listeners_;

    IThemeService& themeService_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimingSidebarSection)
};

} // namespace oscil
