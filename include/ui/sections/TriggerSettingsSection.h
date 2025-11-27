/*
    Oscil - Trigger Settings Sidebar Section
    Trigger source and mode controls for the sidebar
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/SegmentedButtonBar.h"
#include "ui/sections/SectionConstants.h"
#include "dsp/TimingConfig.h"
#include <functional>

namespace oscil
{

// TriggerEdge enum is defined in dsp/TimingConfig.h alongside TriggerMode

/**
 * Trigger settings section component for the sidebar
 * Provides trigger source, mode selection, threshold, and edge options
 */
class TriggerSettingsSection : public juce::Component,
                                public ThemeManagerListener
{
public:
    /**
     * Listener interface for trigger setting changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void triggerSourceChanged(const juce::String& /*sourceName*/) {}
        virtual void triggerModeChanged(TriggerMode /*mode*/) {}
        virtual void triggerThresholdChanged(float /*dBFS*/) {}
        virtual void triggerEdgeChanged(TriggerEdge /*edge*/) {}
    };

    TriggerSettingsSection();
    ~TriggerSettingsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters
    void setTriggerSource(const juce::String& sourceName);
    void setTriggerMode(TriggerMode mode);
    void setTriggerThreshold(float dBFS);
    void setTriggerEdge(TriggerEdge edge);

    // Update available sources (for dropdown population)
    void setAvailableSources(const juce::StringArray& sources);

    // State getters
    juce::String getTriggerSource() const { return currentSource_; }
    TriggerMode getTriggerMode() const { return currentMode_; }
    float getTriggerThreshold() const { return currentThreshold_; }
    TriggerEdge getTriggerEdge() const { return currentEdge_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

    // Threshold range
    static constexpr float MIN_THRESHOLD_DBFS = -60.0f;
    static constexpr float MAX_THRESHOLD_DBFS = 0.0f;

private:
    void setupComponents();
    void updateModeVisibility();
    void notifyTriggerSourceChanged();
    void notifyTriggerModeChanged();
    void notifyTriggerThresholdChanged();
    void notifyTriggerEdgeChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // Trigger Source dropdown
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<juce::ComboBox> sourceSelector_;

    // Mode selector
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<juce::ComboBox> modeSelector_;

    // Threshold controls (visible only in Triggered mode)
    std::unique_ptr<juce::Label> thresholdLabel_;
    std::unique_ptr<juce::Slider> thresholdSlider_;
    std::unique_ptr<juce::Label> thresholdValueLabel_;

    // Edge selector (visible only in Triggered mode)
    std::unique_ptr<juce::Label> edgeLabel_;
    std::unique_ptr<SegmentedButtonBar> edgeToggle_;

    // State
    juce::String currentSource_ = "Self";
    TriggerMode currentMode_ = TriggerMode::FREE_RUNNING;
    float currentThreshold_ = -20.0f;
    TriggerEdge currentEdge_ = TriggerEdge::Rising;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerSettingsSection)
};

} // namespace oscil
