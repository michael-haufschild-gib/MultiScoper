/*
    Oscil - Pane Selector Component
    Reusable dropdown for selecting an existing pane or creating a new one
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/OscilDropdown.h"
#include "core/Pane.h"
#include "ui/components/TestId.h"
#include "ui/theme/IThemeService.h"
#include <functional>
#include <vector>

namespace oscil
{

/**
 * Reusable component for pane selection with optional "New pane" option.
 * Consolidates pane dropdown logic from AddOscillatorDialog and OscillatorConfigPopup.
 */
class PaneSelectorComponent : public juce::Component,
                               public TestIdSupport
{
public:
    /**
     * Create a pane selector
     * @param themeService The theme service to use
     * @param allowNewPane If true, adds "New pane" as the first option
     * @param testId Optional test ID for the component
     */
    PaneSelectorComponent(IThemeService& themeService, bool allowNewPane = true, const juce::String& testId = "");
    ~PaneSelectorComponent() override;

    void resized() override;

    /**
     * Set the available panes for selection
     * @param panes Vector of (PaneId, name) pairs
     */
    void setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes);

    /**
     * Set the available panes from Pane objects
     * @param panes Vector of Pane objects
     */
    void setAvailablePanes(const std::vector<Pane>& panes);

    /**
     * Get the selected pane ID
     * Returns invalid PaneId if "New pane" is selected or nothing selected
     */
    PaneId getSelectedPaneId() const;

    /**
     * Check if "New pane" option is selected
     */
    bool isNewPaneSelected() const;

    /**
     * Check if a valid existing pane is selected
     */
    bool hasValidSelection() const;

    /**
     * Set the selected pane by ID
     * If paneId is not found, selects "New pane" if available, otherwise nothing
     */
    void setSelectedPaneId(const PaneId& paneId, bool notify = true);

    /**
     * Select the "New pane" option (if available)
     */
    void selectNewPane(bool notify = true);

    /**
     * Get the dropdown index for "New pane" option
     * Returns -1 if allowNewPane is false
     */
    int getNewPaneOptionIndex() const { return allowNewPane_ ? NEW_PANE_OPTION_INDEX : -1; }

    /**
     * Callback fired when selection changes
     * Parameters: (selectedPaneId, isNewPaneSelected)
     * selectedPaneId is invalid if isNewPaneSelected is true
     */
    std::function<void(const PaneId&, bool)> onSelectionChanged;

private:
    void setupDropdown();
    void populateDropdown();
    void handleSelectionChange(int index);

    IThemeService& themeService_;
    bool allowNewPane_;
    std::unique_ptr<OscilDropdown> dropdown_;
    std::vector<std::pair<PaneId, juce::String>> panes_;

    // Special index for "New pane" option (always at index 0 when enabled)
    static constexpr int NEW_PANE_OPTION_INDEX = 0;
    static constexpr char NEW_PANE_VALUE[] = "new_pane";

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneSelectorComponent)
};

} // namespace oscil
