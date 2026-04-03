/*
    Oscil - Select Pane Dialog
    Modal dialog for selecting a pane to assign to an oscillator
    Used when trying to make an unassigned oscillator visible
*/

#pragma once

#include "core/Pane.h"
#include "ui/components/OscilButton.h"
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
 * Content component for the "Select Pane" dialog
 * Designed to be hosted inside an OscilModal with title "Select Pane"
 *
 * Shows a dropdown to select an existing pane or create a new one,
 * with OK/Cancel buttons and error display.
 */
class SelectPaneDialog
    : public juce::Component
    , public ThemeManagerListener
    , public TestIdSupport
{
public:
    /**
     * Result structure containing the selection
     */
    struct Result
    {
        PaneId paneId;        // Valid if createNewPane is false
        bool createNewPane{}; // True if "New pane" was selected
    };

    /**
     * Callback type for dialog completion
     */
    using Callback = std::function<void(const Result&)>;
    using CancelCallback = std::function<void()>;

    explicit SelectPaneDialog(IThemeService& themeService);
    ~SelectPaneDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Set up the dialog with available panes
     * @param panes List of available panes
     */
    void setAvailablePanes(const std::vector<Pane>& panes);

    /**
     * Set up the dialog with available panes (pair format)
     * @param panes List of (PaneId, name) pairs
     */
    void setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes);

    /**
     * Set the callback for when OK is clicked with valid selection
     */
    void setOnComplete(Callback callback) { callback_ = std::move(callback); }

    /**
     * Set the callback for when Cancel is clicked or dialog is closed
     */
    void setOnCancel(CancelCallback callback) { cancelCallback_ = std::move(callback); }

    /**
     * Reset the dialog state for reuse
     */
    void reset();

    // Preferred dimensions for OscilModal
    int getPreferredWidth() const { return CONTENT_WIDTH; }
    int getPreferredHeight() const { return CONTENT_HEIGHT; }

private:
    void setupComponents();
    void handleOkClick();
    void handleCancelClick();
    void handlePaneSelectionChange(const PaneId& paneId, bool isNewPane);
    void showError(const juce::String& message);
    void clearError();
    bool validateSelection();

    IThemeService& themeService_;
    Callback callback_;
    CancelCallback cancelCallback_;

    // Pane selector
    std::unique_ptr<juce::Label> instructionLabel_;
    std::unique_ptr<PaneSelectorComponent> paneSelector_;

    // Error message
    std::unique_ptr<juce::Label> errorLabel_;

    // Footer buttons
    std::unique_ptr<OscilButton> okButton_;
    std::unique_ptr<OscilButton> cancelButton_;

    // Theme cache
    ColorTheme theme_;

    // Layout constants
    static constexpr int CONTENT_WIDTH = 300;
    static constexpr int CONTENT_HEIGHT = 180;
    static constexpr int SECTION_SPACING = 12;
    static constexpr int LABEL_HEIGHT = 18;
    static constexpr int CONTROL_HEIGHT = 32;
    static constexpr int BUTTON_HEIGHT = 36;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectPaneDialog)
};

} // namespace oscil
