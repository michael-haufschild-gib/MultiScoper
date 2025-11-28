/*
    Oscil - Add Oscillator Dialog
    Modal popup for creating a new oscillator with source and pane selection
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/InstanceRegistry.h"
#include "core/Pane.h"
#include "ui/ThemeManager.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/OscilColorSwatches.h"
#include "ui/components/TestId.h"
#include <functional>
#include <vector>

namespace oscil
{

/**
 * Modal dialog for adding a new oscillator
 * Allows selection of source, pane, name, and color
 */
class AddOscillatorDialog : public juce::Component,
                             public ThemeManagerListener,
                             public juce::ChangeListener,
                             public TestIdSupport
{
public:
    /**
     * Result structure containing all data needed to create an oscillator
     */
    struct Result
    {
        SourceId sourceId;
        PaneId paneId;          // Empty if createNewPane is true
        bool createNewPane;     // True if "New pane" was selected
        juce::String name;
        juce::Colour color;
    };

    /**
     * Callback type for dialog completion
     * Called with the result when OK is clicked with valid data
     */
    using Callback = std::function<void(const Result&)>;

    AddOscillatorDialog();
    ~AddOscillatorDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // ChangeListener (for ColourSelector)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /**
     * Show the dialog with available sources and panes
     * @param parent Component to show dialog over (for modal positioning)
     * @param sources List of available audio sources
     * @param panes List of available panes
     * @param onComplete Callback when oscillator creation is confirmed
     */
    void showDialog(juce::Component* parent,
                    const std::vector<SourceInfo>& sources,
                    const std::vector<Pane>& panes,
                    Callback onComplete);

    /**
     * Hide the dialog without completing
     */
    void hideDialog();

    /**
     * Check if the dialog is currently visible
     */
    bool isDialogVisible() const { return isVisible(); }

    // Preferred dimensions
    static constexpr int DIALOG_WIDTH = 360;
    static constexpr int DIALOG_HEIGHT = 530;

    // Number of color swatches
    static constexpr int NUM_COLOR_SWATCHES = 10;

private:
    void setupComponents();
    void populateSourceDropdown();
    void populatePaneDropdown();
    void selectRandomColor();
    void updateNameFromSource();
    void handleSourceChange();
    void handlePaneChange();
    void handleOkClick();
    void handleCancelClick();
    void handleCloseClick();
    void showError(const juce::String& message);
    void clearError();
    bool validateInput();

    // Cached data
    std::vector<SourceInfo> sources_;
    std::vector<Pane> panes_;
    Callback callback_;

    // Header
    std::unique_ptr<juce::Label> titleLabel_;
    std::unique_ptr<OscilButton> closeButton_;

    // Source section
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<OscilDropdown> sourceDropdown_;

    // Pane section
    std::unique_ptr<juce::Label> paneLabel_;
    std::unique_ptr<OscilDropdown> paneDropdown_;

    // Name section
    std::unique_ptr<juce::Label> nameLabel_;
    std::unique_ptr<OscilTextField> nameField_;

    // Color section
    std::unique_ptr<juce::Label> colorLabel_;
    std::unique_ptr<OscilColorSwatches> colorSwatches_;
    std::unique_ptr<OscilButton> customColorButton_;
    juce::Colour customColor_;  // Stores custom color if selected
    juce::ColourSelector* colourSelector_ = nullptr;  // Weak reference, CallOutBox owns it

    // Error message
    std::unique_ptr<juce::Label> errorLabel_;

    // Footer buttons
    std::unique_ptr<OscilButton> okButton_;
    std::unique_ptr<OscilButton> cancelButton_;

    // Theme cache
    ColorTheme theme_;

    // Default color swatches (same as OscillatorConfigPopup)
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

    // Layout constants
    static constexpr int HEADER_HEIGHT = 40;
    static constexpr int SECTION_SPACING = 16;
    static constexpr int LABEL_HEIGHT = 20;
    static constexpr int CONTROL_HEIGHT = 32;
    static constexpr int BUTTON_HEIGHT = 36;
    static constexpr int PADDING = 20;
    static constexpr int COLOR_SECTION_HEIGHT = 80;

    // Special index for "New pane" option
    static constexpr int NEW_PANE_OPTION_INDEX = 0;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AddOscillatorDialog)
};

} // namespace oscil
