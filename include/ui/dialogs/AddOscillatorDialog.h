/*
    Oscil - Add Oscillator Dialog
    Content component for adding a new oscillator
    Designed to be hosted in OscilModal
*/

#pragma once

#include "core/InstanceRegistry.h"
#include "core/Pane.h"
#include "core/WaveformColorPalette.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilColorSwatches.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/PaneSelectorComponent.h"
#include "ui/components/TestId.h"
#include "ui/theme/IThemeService.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace oscil
{

/**
 * Content component for the add oscillator dialog
 * Designed to be hosted inside an OscilModal
 */
class AddOscillatorDialog
    : public juce::Component
    , public ThemeManagerListener
    , public TestIdSupport
{
public:
    /**
     * Result structure containing all data needed to create an oscillator
     */
    struct Result
    {
        SourceId sourceId;
        PaneId paneId;              // Empty if createNewPane is true
        bool createNewPane = false; // True if "New pane" was selected
        juce::String name;
        juce::Colour color;
        juce::String visualPresetId; // Visual preset to use
    };

    /**
     * Callback type for dialog completion
     * Called with the result when OK is clicked with valid data
     */
    using Callback = std::function<void(const Result&)>;
    using CancelCallback = std::function<void()>;

    AddOscillatorDialog(IThemeService& themeService);
    ~AddOscillatorDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Set up the dialog with available sources and panes
     * @param sources List of available audio sources
     * @param panes List of available panes
     */
    void setData(const std::vector<SourceInfo>& sources, const std::vector<Pane>& panes);

    /**
     * Set the callback for when OK is clicked with valid data
     */
    void setOnComplete(Callback callback) { callback_ = std::move(callback); }

    /**
     * Set the callback for when Cancel is clicked
     */
    void setOnCancel(CancelCallback callback) { cancelCallback_ = std::move(callback); }

    /**
     * Reset the dialog state for reuse
     */
    void reset();

    // Preferred dimensions for OscilModal
    int getPreferredWidth() const { return CONTENT_WIDTH; }
    int getPreferredHeight() const { return CONTENT_HEIGHT; }

    // Number of color swatches (from centralized palette)
    static constexpr size_t NUM_COLOR_SWATCHES = WaveformColorPalette::NUM_COLORS;

private:
    void setupComponents();
    void createFormFields();
    void populateSourceDropdown();
    void populateVisualPresetDropdown();
    void selectRandomColor();
    void updateNameFromSource();
    void handleSourceChange();
    void handleOkClick();
    void handleCancelClick();
    Result buildResult() const;
    void showError(const juce::String& message);
    void clearError();
    bool validateInput();

    // Cached data
    std::vector<SourceInfo> sources_;
    std::vector<Pane> panes_;
    Callback callback_;
    CancelCallback cancelCallback_;
    bool submitted_ = false;

    // Source section
    std::unique_ptr<juce::Label> sourceLabel_;
    std::unique_ptr<OscilDropdown> sourceDropdown_;

    // Pane section
    std::unique_ptr<juce::Label> paneLabel_;
    std::unique_ptr<PaneSelectorComponent> paneSelector_;

    // Name section
    std::unique_ptr<juce::Label> nameLabel_;
    std::unique_ptr<OscilTextField> nameField_;

    // Color section
    std::unique_ptr<juce::Label> colorLabel_;
    std::unique_ptr<OscilColorSwatches> colorSwatches_;

    // Visual Preset section
    std::unique_ptr<juce::Label> visualPresetLabel_;
    std::unique_ptr<OscilDropdown> visualPresetDropdown_;

    // Error message
    std::unique_ptr<juce::Label> errorLabel_;

    // Footer buttons
    std::unique_ptr<OscilButton> okButton_;
    std::unique_ptr<OscilButton> cancelButton_;

    // Theme cache
    ColorTheme theme_;
    IThemeService& themeService_;

    // Get default colors from centralized palette
    std::vector<juce::Colour> getDefaultColors() const { return WaveformColorPalette::getAllColors(); }

    // Layout constants - content only (no header, OscilModal provides title bar)
    static constexpr int CONTENT_WIDTH = 320;
    static constexpr int CONTENT_HEIGHT = 420;
    static constexpr int SECTION_SPACING = 12;
    static constexpr int LABEL_HEIGHT = 18;
    static constexpr int CONTROL_HEIGHT = 32;
    static constexpr int BUTTON_HEIGHT = 36;
    static constexpr int COLOR_PICKER_HEIGHT = 32;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AddOscillatorDialog)
};

} // namespace oscil
