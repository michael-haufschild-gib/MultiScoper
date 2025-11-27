/*
    Oscil - Display Options Sidebar Section
    Display toggles for grid, auto-scale, and hold
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/sections/SectionConstants.h"
#include "ui/components/OscilToggle.h"
#include <functional>

namespace oscil
{

/**
 * Display options section component for the sidebar
 * Provides Show Grid, Auto-Scale, and Hold toggles
 */
class DisplayOptionsSection : public juce::Component,
                               public ThemeManagerListener
{
public:
    /**
     * Listener interface for display option changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void showGridChanged(bool /*enabled*/) {}
        virtual void autoScaleChanged(bool /*enabled*/) {}
        virtual void holdDisplayChanged(bool /*enabled*/) {}
    };

    DisplayOptionsSection();
    ~DisplayOptionsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters
    void setShowGrid(bool enabled);
    void setAutoScale(bool enabled);
    void setHoldDisplay(bool enabled);

    // State getters
    bool isShowGridEnabled() const { return showGridEnabled_; }
    bool isAutoScaleEnabled() const { return autoScaleEnabled_; }
    bool isHoldDisplayEnabled() const { return holdDisplayEnabled_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

private:
    void setupComponents();
    void notifyShowGridChanged();
    void notifyAutoScaleChanged();
    void notifyHoldDisplayChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // Toggles
    std::unique_ptr<OscilToggle> showGridToggle_;
    std::unique_ptr<OscilToggle> autoScaleToggle_;
    std::unique_ptr<OscilToggle> holdDisplayToggle_;

    // State
    bool showGridEnabled_ = true;
    bool autoScaleEnabled_ = true;
    bool holdDisplayEnabled_ = false;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplayOptionsSection)
};

} // namespace oscil
