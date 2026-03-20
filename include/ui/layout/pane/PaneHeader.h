/*
    Oscil - Pane Header
    Header component for pane containing title, badges, actions, and close button
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Oscillator.h"
#include "ui/theme/IThemeService.h"
#include "ui/components/OscilButton.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/layout/pane/PaneActionBar.h"
#include "ui/components/TestId.h"
#include <functional>
#include <memory>
#include <optional>

namespace oscil
{

/**
 * Header component for a pane.
 *
 * Contains:
 * - Drag handle (painted, for initiating pane drag)
 * - Title (InlineEditLabel for editable name)
 * - Processing badge (shows mode of primary oscillator)
 * - Action bar (stats toggle, etc.)
 * - Close button
 */
class PaneHeader : public juce::Component,
                    public TestIdSupport
{
public:
    explicit PaneHeader(IThemeService& themeService);
    ~PaneHeader() override = default;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // Name
    void setPaneName(const juce::String& name);
    juce::String getPaneName() const;

    // Badge info
    void setPrimaryOscillator(const Oscillator* oscillator);
    void setOscillatorCount(int count);

    // Callbacks
    std::function<void(const juce::String& newName)> onNameChanged;
    std::function<void()> onCloseRequested;
    std::function<void(PaneAction action, bool state)> onActionTriggered;
    std::function<void(const juce::MouseEvent& event)> onDragStarted;

    // Action bar access
    PaneActionBar* getActionBar() { return actionBar_.get(); }

    // Check if point is in drag zone
    bool isInDragZone(juce::Point<int> pos) const;

    // Height
    static constexpr int HEIGHT = 24;

private:
    void setupComponents();

    IThemeService& themeService_;

    // Components
    std::unique_ptr<InlineEditLabel> nameLabel_;
    std::unique_ptr<PaneActionBar> actionBar_;
    std::unique_ptr<OscilButton> closeButton_;

    // Badge data - stored by value to avoid dangling pointer when vector reallocates
    std::optional<Oscillator> primaryOscillator_;
    int oscillatorCount_ = 0;

    // Drag state
    juce::Point<int> dragStartPos_;
    bool dragStartedForGesture_ = false;

    // Layout constants
    static constexpr int DRAG_HANDLE_WIDTH = 20;
    static constexpr int DRAG_HANDLE_LINE_WIDTH = 10;
    static constexpr int DRAG_HANDLE_LINE_HEIGHT = 2;
    static constexpr int DRAG_HANDLE_LINE_SPACING = 4;
    static constexpr int DRAG_HANDLE_LEFT_MARGIN = 4;
    static constexpr int NAME_LABEL_WIDTH = 120;
    static constexpr int CLOSE_BUTTON_SIZE = 20;
    static constexpr int PADDING = 4;
    static constexpr int BADGE_WIDTH = 100;
    static constexpr int DRAG_START_THRESHOLD_PX = 5;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneHeader)
};

} // namespace oscil
