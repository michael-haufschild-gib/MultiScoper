/*
    Oscil - Oscillator Configuration Popup
    Modal popup for comprehensive oscillator configuration
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "ui/ThemeManager.h"
#include "ui/SegmentedButtonBar.h"
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
    bool isPopupVisible() const { return isVisible(); }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Preferred dimensions
    static constexpr int POPUP_WIDTH = 360;
    static constexpr int POPUP_HEIGHT = 520;

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
    void handleVerticalScaleChange();
    void handleVerticalOffsetChange();
    void handlePaneChange();

    // Current oscillator data
    OscillatorId oscillatorId_;
    SourceId sourceId_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    juce::Colour colour_{ 0xFF00FF00 };
    float opacity_ = 1.0f;
    bool visible_ = true;
    juce::String name_;
    float lineWidth_ = Oscillator::DEFAULT_LINE_WIDTH;
    float verticalScale_ = Oscillator::DEFAULT_VERTICAL_SCALE;
    float verticalOffset_ = Oscillator::DEFAULT_VERTICAL_OFFSET;
    PaneId paneId_;
    int orderIndex_ = 0;  // Track order to preserve during config changes

    // Header section
    std::unique_ptr<juce::TextEditor> nameEditor_;
    std::unique_ptr<juce::TextButton> visibilityButton_;
    std::unique_ptr<juce::TextButton> closeButton_;

    // Source section
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<SourceSelectorComponent> sourceSelector_;

    // Processing mode section
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<SegmentedButtonBar> modeButtons_;

    // Color section
    std::unique_ptr<juce::Label> colorLabel_;
    std::vector<std::unique_ptr<juce::TextButton>> colorSwatches_;
    static constexpr int NUM_COLOR_SWATCHES = 10;

    // Line Width slider
    std::unique_ptr<juce::Label> lineWidthLabel_;
    std::unique_ptr<juce::Slider> lineWidthSlider_;

    // Opacity slider
    std::unique_ptr<juce::Label> opacityLabel_;
    std::unique_ptr<juce::Slider> opacitySlider_;

    // Vertical Scale slider
    std::unique_ptr<juce::Label> scaleLabel_;
    std::unique_ptr<juce::Slider> scaleSlider_;

    // Vertical Offset slider
    std::unique_ptr<juce::Label> offsetLabel_;
    std::unique_ptr<juce::Slider> offsetSlider_;

    // Pane selector
    std::unique_ptr<juce::Label> paneLabel_;
    std::unique_ptr<juce::ComboBox> paneSelector_;
    std::vector<std::pair<PaneId, juce::String>> availablePanes_;

    // Footer buttons
    std::unique_ptr<juce::TextButton> deleteButton_;
    std::unique_ptr<juce::TextButton> footerCloseButton_;

    juce::ListenerList<Listener> listeners_;

    // Default color swatches
    std::array<juce::Colour, NUM_COLOR_SWATCHES> defaultColors_ = {{
        juce::Colour(0xFF00FF00),  // Green
        juce::Colour(0xFF00BFFF),  // Deep Sky Blue
        juce::Colour(0xFFFF6B6B),  // Coral Red
        juce::Colour(0xFFFFD93D),  // Gold
        juce::Colour(0xFFFF00FF),  // Magenta
        juce::Colour(0xFF00FFFF),  // Cyan
        juce::Colour(0xFFFF8C00),  // Dark Orange
        juce::Colour(0xFF9370DB),  // Medium Purple
        juce::Colour(0xFF32CD32),  // Lime Green
        juce::Colour(0xFFFF1493)   // Deep Pink
    }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorConfigPopup)
};

} // namespace oscil
