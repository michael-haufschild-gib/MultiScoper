/*
    Oscil - Options Sidebar Section
    Combined section with gain control and display options
    Merges Master Controls and Display Options sections
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "core/ServiceContext.h"
#include "ui/layout/sections/SectionConstants.h"
#include "ui/layout/sections/DynamicHeightContent.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/TestId.h"
#include "core/dsp/CaptureQualityConfig.h"
#include <functional>

namespace oscil
{

/**
 * Options section component for the sidebar
 * Combines:
 * - Gain slider (from Master Controls)
 * - Show Grid, Auto-Scale, Hold Display toggles (from Display Options)
 *
 * This section is designed to be wrapped in a CollapsibleSection
 */
class OptionsSection : public juce::Component,
                       public ThemeManagerListener,
                       public TestIdSupport,
                       public DynamicHeightContent
{
public:
    /**
     * Listener interface for options changes
     * Combines events from both Master Controls and Display Options
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        // Master controls events
        virtual void gainChanged(float /*dB*/) {}

        // Display options events
        virtual void showGridChanged(bool /*enabled*/) {}
        virtual void autoScaleChanged(bool /*enabled*/) {}
        virtual void holdDisplayChanged(bool /*enabled*/) {}

        // Layout and theme events (moved from toolbar)
        virtual void layoutChanged(int /*columnCount*/) {}
        virtual void themeChanged(const juce::String& /*themeName*/) {}

        // Rendering mode events
        virtual void gpuRenderingChanged(bool /*enabled*/) {}

        // Capture quality events
        virtual void qualityPresetChanged(QualityPreset /*preset*/) {}
        virtual void bufferDurationChanged(BufferDuration /*duration*/) {}
        virtual void autoAdjustQualityChanged(bool /*enabled*/) {}
    };

    explicit OptionsSection(ServiceContext& context);
    explicit OptionsSection(IThemeService& themeService);
    ~OptionsSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // Gain state
    void setGainDb(float dB);
    float getGainDb() const { return currentGainDb_; }

    // Display options state
    void setShowGrid(bool enabled);
    void setAutoScale(bool enabled);
    void setHoldDisplay(bool enabled);

    bool isShowGridEnabled() const { return showGridEnabled_; }
    bool isAutoScaleEnabled() const { return autoScaleEnabled_; }
    bool isHoldDisplayEnabled() const { return holdDisplayEnabled_; }

    // Layout state (moved from toolbar)
    void setColumnCount(int columns);
    int getColumnCount() const { return currentColumnCount_; }

    // Theme state (moved from toolbar)
    void setAvailableThemes(const std::vector<juce::String>& themeNames);
    void setCurrentTheme(const juce::String& themeName);
    juce::String getCurrentThemeName() const { return currentThemeName_; }

    // GPU rendering state
    void setGpuRenderingEnabled(bool enabled);
    bool isGpuRenderingEnabled() const { return gpuRenderingEnabled_; }

    // Capture quality state
    void setQualityPreset(QualityPreset preset);
    QualityPreset getQualityPreset() const { return currentQualityPreset_; }

    void setBufferDuration(BufferDuration duration);
    BufferDuration getBufferDuration() const { return currentBufferDuration_; }

    void setAutoAdjustQuality(bool enabled);
    bool isAutoAdjustQualityEnabled() const { return autoAdjustQualityEnabled_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    /**
     * Calculate the preferred height based on content
     * Used by CollapsibleSection for dynamic sizing (DynamicHeightContent)
     */
    int getPreferredHeight() const override;

    // For testing access
    OscilDropdown* getThemeDropdown() const { return themeDropdown_.get(); }

    // Gain range constants (from PRD)
    static constexpr float MIN_GAIN_DB = -60.0f;
    static constexpr float MAX_GAIN_DB = 12.0f;

private:
    void setupComponents();

    // Notification helpers
    void notifyGainChanged();
    void notifyShowGridChanged();
    void notifyAutoScaleChanged();
    void notifyHoldDisplayChanged();
    void notifyLayoutChanged();
    void notifyThemeChanged();
    void notifyGpuRenderingChanged();
    void notifyQualityPresetChanged();
    void notifyBufferDurationChanged();
    void notifyAutoAdjustQualityChanged();

    // Gain controls (from Master Controls)
    std::unique_ptr<juce::Label> gainLabel_;
    std::unique_ptr<OscilSlider> gainSlider_;

    // Display toggles (from Display Options)
    std::unique_ptr<juce::Label> displayLabel_;
    std::unique_ptr<OscilToggle> showGridToggle_;
    std::unique_ptr<OscilToggle> autoScaleToggle_;
    std::unique_ptr<OscilToggle> holdDisplayToggle_;

    // Layout and theme dropdowns (moved from toolbar)
    std::unique_ptr<juce::Label> layoutLabel_;
    std::unique_ptr<OscilDropdown> layoutDropdown_;
    std::unique_ptr<juce::Label> themeLabel_;
    std::unique_ptr<OscilDropdown> themeDropdown_;

    // GPU rendering toggle
    std::unique_ptr<juce::Label> renderingLabel_;
    std::unique_ptr<OscilToggle> gpuRenderingToggle_;

    // Capture quality controls
    std::unique_ptr<juce::Label> qualityLabel_;
    std::unique_ptr<OscilDropdown> qualityPresetDropdown_;
    std::unique_ptr<OscilDropdown> bufferDurationDropdown_;
    std::unique_ptr<OscilToggle> autoAdjustQualityToggle_;

    // State
    float currentGainDb_ = 0.0f;
    bool showGridEnabled_ = true;
    bool autoScaleEnabled_ = true;
    bool holdDisplayEnabled_ = false;
    int currentColumnCount_ = 1;
    juce::String currentThemeName_ = "Dark";
    bool gpuRenderingEnabled_ = true;  // Default to GPU mode when available
    QualityPreset currentQualityPreset_ = QualityPreset::Standard;
    BufferDuration currentBufferDuration_ = BufferDuration::Medium;
    bool autoAdjustQualityEnabled_ = true;

    juce::ListenerList<Listener> listeners_;

    IThemeService& themeService_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OptionsSection)
};

} // namespace oscil
