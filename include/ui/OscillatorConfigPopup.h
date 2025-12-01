/*
    Oscil - Oscillator Configuration Popup
    Modal popup for comprehensive oscillator configuration
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "ui/ThemeManager.h"
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

    OscillatorConfigPopup();
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

    // Number of default color swatches
    static constexpr int NUM_COLOR_SWATCHES = 12;

private:
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
    juce::Colour colour_{ 0xFF00FF00 };
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

    // Default color swatches
    std::array<juce::Colour, NUM_COLOR_SWATCHES> defaultColors_ = {{
        juce::Colour(0xFFFF355E),  // Radical Red
        juce::Colour(0xFFFF6037),  // Outrun Orange
        juce::Colour(0xFFFFCC00),  // VHS Gold
        juce::Colour(0xFFCCFF00),  // Mutant Green
        juce::Colour(0xFF39FF14),  // Arcade Green
        juce::Colour(0xFF00FF99),  // Turbo Teal
        juce::Colour(0xFF40E0D0),  // Poolside Aqua
        juce::Colour(0xFF00FFFF),  // Miami Cyan
        juce::Colour(0xFF1E90FF),  // Dodger Blue
        juce::Colour(0xFF3333FF),  // Video Blue
        juce::Colour(0xFF9933FF),  // Electric Violet
        juce::Colour(0xFFFF00CC)   // Hot Magenta
    }};

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorConfigPopup)
};

} // namespace oscil
