/*
    Oscil - Master Controls Sidebar Section
    Gain control for the sidebar
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/sections/SectionConstants.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/TestId.h"
#include <functional>

namespace oscil
{

/**
 * Master controls section component for the sidebar
 * Provides Gain (dB) slider only
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
        virtual void gainChanged(float /*dB*/) {}
    };

    MasterControlsSection();
    ~MasterControlsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters
    void setGainDb(float dB);

    // State getters
    float getGainDb() const { return currentGainDb_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

    // PRD ranges
    static constexpr float MIN_GAIN_DB = -60.0f;
    static constexpr float MAX_GAIN_DB = 12.0f;

private:
    void setupComponents();
    void notifyGainChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // Gain controls
    std::unique_ptr<OscilSlider> gainSlider_;

    // State
    float currentGainDb_ = 0.0f;

    juce::ListenerList<Listener> listeners_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterControlsSection)
};

} // namespace oscil
