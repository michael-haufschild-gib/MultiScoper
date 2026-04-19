/*
    MultiScoper - Oscillator Configuration Dialog
    Modal dialog for comprehensive oscillator configuration
*/

#pragma once

#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/WaveformColorPalette.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/MultiScoperButton.h"
#include "ui/components/MultiScoperColorSwatches.h"
#include "ui/components/MultiScoperDropdown.h"
#include "ui/components/MultiScoperSlider.h"
#include "ui/components/MultiScoperTextField.h"
#include "ui/components/PaneSelectorComponent.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/components/TestId.h"
#include "ui/components/ThemedComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace multiscoper
{

// Forward declarations
class SourceSelectorComponent;
class IThemeService;

/**
 * Modal dialog for configuring an oscillator's settings
 * Provides comprehensive controls as per PRD requirements
 */
class OscillatorConfigDialog : public ThemedComponent
{
public:
    /**
     * Listener interface for dialog actions
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscillatorConfigChanged(const OscillatorId& /*id*/, const Oscillator& /*oscillator*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        virtual void configDialogClosed() {}
    };

    explicit OscillatorConfigDialog(IThemeService& themeService, IInstanceRegistry& instanceRegistry);
    ~OscillatorConfigDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Apply updated theme colors to labels and child components.
    void onThemeChanged(const ColorTheme& newTheme) override;

    /**
     * Show the dialog for an oscillator
     */
    void showForOscillator(const Oscillator& oscillator);

    /**
     * Update available panes for the pane selector
     */
    void setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes);

    /**
     * Invoked by the hosting modal when it has finished closing (via the X button,
     * Escape, backdrop click, or the content's own Close button path). Flushes any
     * pending debounced edits and fires configDialogClosed on all listeners, so
     * every close path goes through exactly one notification point.
     */
    void onExternalClose();

    /**
     * Get the oscillator ID being edited
     */
    OscillatorId getOscillatorId() const { return oscillatorId_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    std::function<void()> onClose;

    // Preferred dimensions
    static constexpr int DIALOG_WIDTH = 360;
    static constexpr int DIALOG_HEIGHT = 550;

    int getPreferredWidth() const { return DIALOG_WIDTH; }
    int getPreferredHeight() const { return DIALOG_HEIGHT; }

    // Layout constants (spacing uses shared ComponentLayout values)
    static constexpr int PADDING = ComponentLayout::SPACING_LG;
    static constexpr int HEADER_HEIGHT = 0; // Removed custom header
    static constexpr int LABEL_HEIGHT = 20;
    static constexpr int CONTROL_HEIGHT = 28;
    static constexpr int COLOR_PICKER_HEIGHT = 32;
    static constexpr int SLIDER_ROW_HEIGHT = 40;
    static constexpr int FOOTER_HEIGHT = 36;

    // Number of default color swatches (from centralized palette)
    static constexpr size_t NUM_COLOR_SWATCHES = WaveformColorPalette::NUM_COLORS;

private:
    /// Generic single-shot debounce timer used to coalesce high-frequency events
    /// (e.g. keystrokes in the name editor) into a single listener notification.
    /// Declared here so it can live as a member of OscillatorConfigDialog.
    class DebounceTimer : public juce::Timer
    {
    public:
        std::function<void()> onFire;
        void timerCallback() override;
    };

    IInstanceRegistry& instanceRegistry_;

    void setupComponents();
    void setupSourceAndMode();
    void setupAppearanceControls();
    void setupPaneAndFooter();
    void updateFromOscillator(const Oscillator& oscillator);
    void notifyConfigChanged();
    void handleClose() const;
    void handleNameEdit();
    /// Flush any pending debounced name edit immediately.
    void flushPendingNameEdit();
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
    juce::Colour colour_{WaveformColorPalette::getColor(0)};
    float opacity_ = 1.0f;
    bool visible_ = true;
    juce::String name_;
    float lineWidth_ = Oscillator::DEFAULT_LINE_WIDTH;
    PaneId paneId_;
    int orderIndex_ = 0; // Track order to preserve during config changes
    juce::ValueTree visualOverrides_;

    // Header section
    std::unique_ptr<MultiScoperTextField> nameEditor_;

    // Source section
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<SourceSelectorComponent> sourceSelector_;

    // Processing mode section
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<SegmentedButtonBar> modeButtons_;

    // Color section
    std::unique_ptr<juce::Label> colorLabel_;
    std::unique_ptr<MultiScoperColorSwatches> colorSwatches_;

    // Visual preset section
    std::unique_ptr<juce::Label> visualPresetLabel_;
    std::unique_ptr<MultiScoperDropdown> visualPresetDropdown_;
    juce::String visualPresetId_ = "default";

    // Line Width slider
    std::unique_ptr<MultiScoperSlider> lineWidthSlider_;

    // Opacity slider
    std::unique_ptr<MultiScoperSlider> opacitySlider_;

    // Pane selector
    std::unique_ptr<juce::Label> paneLabel_;
    std::unique_ptr<PaneSelectorComponent> paneSelectorComponent_;

    // Footer buttons
    std::unique_ptr<MultiScoperButton> footerCloseButton_;

    // Debounces keystroke-driven name edits so we do not churn the entire
    // oscillator ValueTree on every character. Flushed on Close / Enter.
    DebounceTimer nameDebounce_;
    static constexpr int NAME_DEBOUNCE_MS = 250;

    juce::ListenerList<Listener> listeners_;

    // Get default colors from centralized palette
    std::vector<juce::Colour> getDefaultColors() const { return WaveformColorPalette::getAllColors(); }

    MULTISCOPER_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorConfigDialog)
};

} // namespace multiscoper
