/*
    Oscil - Segmented Button Bar Component
    Reusable segmented control for exclusive selection (radio-button style)
    Now uses OscilButton internally for consistent styling and animations
*/

#pragma once

#include "ui/components/OscilButton.h"
#include "ui/components/ThemedComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace oscil
{

/**
 * Segmented button bar for exclusive selection
 * Used for: TIME/MELODIC toggle, Processing Mode selection, Filter tabs
 *
 * Now uses OscilButton internally with toggleable and segment position support
 * for consistent styling, animations, and accessibility across the UI.
 */
class SegmentedButtonBar : public ThemedComponent
{
public:
    explicit SegmentedButtonBar(IThemeService& themeService);
    ~SegmentedButtonBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Keyboard navigation
    bool keyPressed(const juce::KeyPress& key) override;

    /**
     * Add a button to the bar with text label
     * @param label Display text for the button
     * @param id Unique identifier for the button
     * @param testId Optional test ID for the button
     * @param tooltip Optional tooltip text for the button
     */
    void addButton(const juce::String& label, int id, const juce::String& testId = {},
                   const juce::String& tooltip = {});

    /**
     * Add a button to the bar with a path-based icon
     * @param iconPath Path to render as the button icon (uses theme colors)
     * @param id Unique identifier for the button
     * @param testId Optional test ID for the button
     * @param tooltip Optional tooltip text for the button
     */
    void addButtonWithPath(const juce::Path& iconPath, int id, const juce::String& testId = {},
                           const juce::String& tooltip = {});

    /**
     * Set the tooltip for a specific button by ID
     * @param id The button ID
     * @param tooltip The tooltip text
     */
    void setButtonTooltip(int id, const juce::String& tooltip);

    /**
     * Remove all buttons
     */
    void clearButtons();

    /**
     * Set the selected button by ID
     */
    void setSelectedId(int id);

    /**
     * Get the currently selected button ID
     * Returns -1 if no button is selected
     */
    int getSelectedId() const { return selectedId_; }

    /**
     * Enable or disable the entire bar
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    /**
     * Set minimum button width (default 60px)
     */
    void setMinButtonWidth(int width) { minButtonWidth_ = width; }

    /**
     * Callback when selection changes
     */
    std::function<void(int)> onSelectionChanged;

    // Preferred height for layout
    static constexpr int PREFERRED_HEIGHT = 28;

private:
    void handleButtonClick(int id);
    void updateButtonStates();
    int getSelectedIndex() const;

    std::vector<std::unique_ptr<OscilButton>> buttons_;
    int selectedId_ = -1;
    bool enabled_ = true;
    int minButtonWidth_ = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedButtonBar)
};

} // namespace oscil
