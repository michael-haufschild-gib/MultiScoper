/*
    Oscil - Source Item Component
    Displays a single audio source in the sidebar with add-to-pane functionality
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/InstanceRegistry.h"
#include "ui/layout/Pane.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/TestId.h"
#include <functional>
#include <memory>

namespace oscil
{

/**
 * Component that displays a single audio source in the sidebar.
 * Shows source name, track info, activity indicator, and add-to-pane dropdown.
 */
class SourceItemComponent : public juce::Component
{
public:
    explicit SourceItemComponent(const SourceInfo& sourceInfo);
    ~SourceItemComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    /**
     * Update the source information displayed
     */
    void updateSourceInfo(const SourceInfo& info);

    /**
     * Update the list of available panes in the dropdown
     */
    void updateAvailablePanes(const std::vector<Pane>& panes);

    /**
     * Set whether this source is currently active (receiving audio)
     */
    void setActive(bool isActive);

    /**
     * Get the source ID
     */
    const SourceId& getSourceId() const { return sourceId_; }

    /**
     * Callback when source is selected (clicked)
     */
    std::function<void(const SourceId&)> onSelected;

    /**
     * Callback when user wants to add an oscillator from this source to a pane
     * Parameters: sourceId, targetPaneId
     */
    std::function<void(const SourceId&, const PaneId&)> onAddToPane;

private:
    void handleAddToPaneSelection();

    SourceId sourceId_;
    juce::String displayName_;
    juce::String trackName_;
    bool isActive_ = false;
    bool isHovered_ = false;
    bool isSelected_ = false;

    std::unique_ptr<OscilDropdown> addToPaneDropdown_;
    std::vector<PaneId> paneIds_;  // Maps dropdown index to PaneId

    static constexpr int ITEM_HEIGHT = 36;
    static constexpr int ACTIVITY_DOT_SIZE = 8;
    static constexpr int DROPDOWN_WIDTH = 100;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceItemComponent)
};

} // namespace oscil
