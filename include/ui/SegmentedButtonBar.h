/*
    Oscil - Segmented Button Bar Component
    Reusable segmented control for exclusive selection (radio-button style)
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include <vector>
#include <functional>

namespace oscil
{

/**
 * A single segment button within the SegmentedButtonBar
 */
class SegmentButton : public juce::Component
{
public:
    SegmentButton(const juce::String& label, int buttonId);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSelected(bool selected);
    bool isSelected() const { return selected_; }

    int getButtonId() const { return buttonId_; }
    const juce::String& getLabel() const { return label_; }

    void setFirst(bool first) { isFirst_ = first; }
    void setLast(bool last) { isLast_ = last; }

    std::function<void(int)> onClick;

private:
    juce::String label_;
    int buttonId_;
    bool selected_ = false;
    bool isHovered_ = false;
    bool isFirst_ = false;
    bool isLast_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentButton)
};

/**
 * Segmented button bar for exclusive selection
 * Used for: TIME/MELODIC toggle, Processing Mode selection, Filter tabs
 */
class SegmentedButtonBar : public juce::Component,
                            public ThemeManagerListener
{
public:
    SegmentedButtonBar();
    ~SegmentedButtonBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    /**
     * Add a button to the bar
     * @param label Display text for the button
     * @param id Unique identifier for the button
     */
    void addButton(const juce::String& label, int id);

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

    std::vector<std::unique_ptr<SegmentButton>> buttons_;
    int selectedId_ = -1;
    bool enabled_ = true;
    int minButtonWidth_ = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedButtonBar)
};

} // namespace oscil
