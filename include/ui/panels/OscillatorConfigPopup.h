/*
    Oscil - Oscillator Configuration Popup
    Modal popup for comprehensive oscillator configuration
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "ui/layout/Pane.h"
#include "ui/theme/WaveformColorPalette.h"
#include "ui/theme/ThemeManager.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilColorSwatches.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilSlider.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/components/TestId.h"
#include <functional>
#include <vector>

namespace oscil
{

// Forward declarations
class SourceSelectorComponent;
class IThemeService;

/**
 * Modal popup for configuring an oscillator's settings
 * Provides comprehensive controls as per PRD requirements
 */
class OscillatorConfigPopup : public juce::Component,
                               public ThemeManagerListener
{
public:
    /**
     * Listener interface for popup actions
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscillatorConfigChanged(const OscillatorId& /*id*/, const Oscillator& /*oscillator*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        virtual void configPopupClosed() {}
    };

    explicit OscillatorConfigPopup(IThemeService& themeService);
    ~OscillatorConfigPopup() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Show the popup for an oscillator
     */
    void showForOscillator(const Oscillator& oscillator);

    /**
     * Update available panes for the pane selector
     */
    void setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes);

    /**
     * Get the oscillator ID being edited
     */
    OscillatorId getOscillatorId() const { return oscillatorId_; }

    /**
     * Check if popup is visible
     */
    bool isPopupVisible() const { return Component::isVisible(); }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Preferred dimensions
    static constexpr int POPUP_WIDTH = 360;
    static constexpr int POPUP_HEIGHT = 550;

    // Layout constants
    static constexpr int PADDING = 16;
    static constexpr int HEADER_HEIGHT = 32;
    static constexpr int LABEL_HEIGHT = 20;
    static constexpr int CONTROL_HEIGHT = 28;
    static constexpr int COLOR_PICKER_HEIGHT = 32;
    static constexpr int SLIDER_ROW_HEIGHT = 40;
    static constexpr int FOOTER_HEIGHT = 36;
    static constexpr int SPACING_SMALL = 4;
    static constexpr int SPACING_MEDIUM = 8;
    static constexpr int SPACING_LARGE = 12;
    static constexpr int SPACING_SECTION = 16;

    // Number of default color swatches (from centralized palette)
    static constexpr size_t NUM_COLOR_SWATCHES = WaveformColorPalette::NUM_COLORS;

private:
    IThemeService& themeService_;

    void setupComponents();
    void updateFromOscillator(const Oscillator& oscillator);
    void notifyConfigChanged();
    void handleClose();
    void handleDelete();
    void handleNameEdit();
    void handleVisibilityToggle();
    void handleSourceChange(const SourceId& sourceId);
    void handleProcessingModeChange(int modeId);
    void handleColorSelect(juce::Colour colour);
    void handleLineWidthChange();
    void handleOpacityChange();
    void handlePaneChange();
    void handleVisualPresetChange();

    // Current oscillator data
    OscillatorId oscillatorId_;
    SourceId sourceId_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    juce::Colour colour_{ WaveformColorPalette::getColor(0) };
    float opacity_ = 1.0f;
    bool visible_ = true;
    juce::String name_;
    float lineWidth_ = Oscillator::DEFAULT_LINE_WIDTH;
    PaneId paneId_;
    int orderIndex_ = 0;  // Track order to preserve during config changes
    juce::ValueTree visualOverrides_;

    // Header section
    std::unique_ptr<OscilTextField> nameEditor_;
    std::unique_ptr<OscilToggle> visibilityToggle_;
    std::unique_ptr<OscilButton> closeButton_;

    // Source section
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<SourceSelectorComponent> sourceSelector_;

    // Processing mode section
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<SegmentedButtonBar> modeButtons_;

    // Color section
    std::unique_ptr<juce::Label> colorLabel_;
    std::unique_ptr<OscilColorSwatches> colorSwatches_;

    // Visual preset section
    std::unique_ptr<juce::Label> visualPresetLabel_;
    std::unique_ptr<OscilDropdown> visualPresetDropdown_;
    juce::String visualPresetId_ = "default";

    // Line Width slider
    std::unique_ptr<OscilSlider> lineWidthSlider_;

    // Opacity slider
    std::unique_ptr<OscilSlider> opacitySlider_;

    // Pane selector
    std::unique_ptr<juce::Label> paneLabel_;
    std::unique_ptr<OscilDropdown> paneSelector_;
    std::vector<std::pair<PaneId, juce::String>> availablePanes_;

    // Footer buttons
    std::unique_ptr<OscilButton> deleteButton_;
    std::unique_ptr<OscilButton> footerCloseButton_;

    juce::ListenerList<Listener> listeners_;

    // Get default colors from centralized palette
    std::vector<juce::Colour> getDefaultColors() const { return WaveformColorPalette::getAllColors(); }

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorConfigPopup)
};

} // namespace oscil
