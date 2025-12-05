/*
    Oscil - Oscillator List Item
    Compact list item that expands when selected to show controls
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "ui/components/TestId.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilToggle.h"
#include "ui/components/SegmentedButtonBar.h"
#include "ui/components/InlineEditLabel.h"
#include "core/Oscillator.h"
#include <functional>

namespace oscil
{

class IInstanceRegistry;

/**
 * Oscillator list item component
 * Shows compact view for non-selected items, expands when selected to show mode/visibility controls
 */
class OscillatorListItemComponent : public juce::Component,
                                     public ThemeManagerListener,
                                     public TestIdSupport
{
public:
    /**
     * Listener interface for item actions
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscillatorSelected(const OscillatorId& /*id*/) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*id*/, bool /*visible*/) {}
        virtual void oscillatorModeChanged(const OscillatorId& /*id*/, ProcessingMode /*mode*/) {}
        virtual void oscillatorConfigRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorColorConfigRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorDragStarted(const OscillatorId& /*id*/) {}
        virtual void oscillatorMoveRequested(const OscillatorId& /*id*/, int /*direction*/) {}
        /**
         * Called when user tries to set an oscillator visible but it has no valid pane assignment.
         * Parent should show a pane selection dialog.
         */
        virtual void oscillatorPaneSelectionRequested(const OscillatorId& /*id*/) {}
        /**
         * Called when user edits the oscillator name inline.
         */
        virtual void oscillatorNameChanged(const OscillatorId& /*id*/, const juce::String& /*newName*/) {}
    };

    OscillatorListItemComponent(const Oscillator& oscillator, IInstanceRegistry& instanceRegistry, IThemeService& themeService);
    ~OscillatorListItemComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State management
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }

    void updateFromOscillator(const Oscillator& oscillator);

    OscillatorId getOscillatorId() const { return oscillatorId_; }
    bool isOscillatorVisible() const { return isVisible_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height depends on selected state
    int getPreferredHeight() const { return selected_ ? EXPANDED_HEIGHT : COMPACT_HEIGHT; }

    static constexpr int COMPACT_HEIGHT = 56;
    static constexpr int EXPANDED_HEIGHT = 100;
    static constexpr int PREFERRED_HEIGHT = COMPACT_HEIGHT;  // Default for layout calculations

private:
    void setupComponents(int orderIndex);
    void updateVisibility();
    
    bool isInDragZone(const juce::Point<int>& pos) const;

    // Data
    OscillatorId oscillatorId_;
    IInstanceRegistry& instanceRegistry_;
    IThemeService& themeService_;
    juce::String displayName_;
    juce::String trackName_;
    juce::Colour colour_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    bool isVisible_ = true;
    PaneId paneId_;  // For checking if oscillator has valid pane assignment

    // UI state
    bool selected_ = false;
    bool isHovered_ = false;
    bool isDragging_ = false;
    bool hasFocus_ = false;
    bool dragHandleHovered_ = false;
    juce::Point<int> dragStartPos_;

    // Child components
    std::unique_ptr<InlineEditLabel> nameLabel_;
    std::unique_ptr<juce::Label> trackLabel_;
    std::unique_ptr<OscilButton> deleteButton_;
    std::unique_ptr<OscilButton> settingsButton_;
    std::unique_ptr<OscilButton> visibilityButton_; // Used for icon-only visibility toggle
    std::unique_ptr<SegmentedButtonBar> modeButtons_;

    juce::ListenerList<Listener> listeners_;

    // Layout constants
    static constexpr int DRAG_HANDLE_WIDTH = 24;
    static constexpr int COLOR_INDICATOR_SIZE = 14;
    static constexpr int ICON_BUTTON_SIZE = 24;
    static constexpr int ROW_PADDING = 8;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorListItemComponent)
};

} // namespace oscil
