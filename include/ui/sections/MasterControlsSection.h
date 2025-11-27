/*
    Oscil - Master Controls Sidebar Section
    Timebase and Gain controls for the sidebar
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/sections/SectionConstants.h"
#include "ui/components/OscilSlider.h"
#include <functional>

namespace oscil
{

/**
 * Master controls section component for the sidebar
 * Provides Timebase (ms) and Gain (dB) sliders
 */
class MasterControlsSection : public juce::Component,
                               public ThemeManagerListener
{
public:
    /**
     * Listener interface for master control changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void timebaseChanged(float /*ms*/) {}
        virtual void gainChanged(float /*dB*/) {}
    };

    MasterControlsSection();
    ~MasterControlsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters
    void setTimebaseMs(float ms);
    void setGainDb(float dB);

    // State getters
    float getTimebaseMs() const { return currentTimebaseMs_; }
    float getGainDb() const { return currentGainDb_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

    // PRD ranges
    static constexpr float MIN_TIMEBASE_MS = 1.0f;
    static constexpr float MAX_TIMEBASE_MS = 60000.0f;
    static constexpr float MIN_GAIN_DB = -60.0f;
    static constexpr float MAX_GAIN_DB = 12.0f;

private:
    void setupComponents();
    void notifyTimebaseChanged();
    void notifyGainChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // Timebase controls
    std::unique_ptr<OscilSlider> timebaseSlider_;

    // Gain controls
    std::unique_ptr<OscilSlider> gainSlider_;

    // State
    float currentTimebaseMs_ = 50.0f;
    float currentGainDb_ = 0.0f;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterControlsSection)
};

} // namespace oscil
