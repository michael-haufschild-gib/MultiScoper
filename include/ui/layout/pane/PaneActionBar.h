/*
    Oscil - Pane Action Bar
    Container for action buttons in the pane header
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/IThemeService.h"
#include "ui/components/OscilButton.h"
#include "ui/components/TestId.h"
#include <functional>
#include <memory>

namespace oscil
{

/**
 * Action identifiers for pane header actions
 */
enum class PaneAction
{
    ToggleStats,     // Toggle statistics overlay
    ToggleHold,      // Toggle hold/pause display
    Screenshot,      // Capture screenshot (future)
    ReorderWaveforms // Reorder waveforms (future)
};

/**
 * Container for action buttons in the pane header.
 *
 * Features:
 * - Horizontal layout of icon buttons
 * - Toggle state support for actions like stats
 * - Tooltips for each action
 * - Extensible - easy to add new actions
 */
class PaneActionBar : public juce::Component,
                       public TestIdSupport
{
public:
    explicit PaneActionBar(IThemeService& themeService);
    ~PaneActionBar() override = default;

    // Component overrides
    void resized() override;

    // Callback for action triggered
    // bool parameter is toggle state (true if toggled on)
    std::function<void(PaneAction action, bool toggleState)> onActionTriggered;

    // Get toggle state for an action
    bool isActionToggled(PaneAction action) const;

    // Set toggle state for an action
    void setActionToggled(PaneAction action, bool toggled);

    // Enable/disable an action
    void setActionEnabled(PaneAction action, bool enabled);

    // Get preferred width (sum of all button widths + spacing)
    int getPreferredWidth() const;

private:
    void setupButtons();

    IThemeService& themeService_;

    // Action buttons
    std::unique_ptr<OscilButton> statsButton_;
    std::unique_ptr<OscilButton> holdButton_;

    // Layout constants
    static constexpr int BUTTON_SIZE = 20;
    static constexpr int BUTTON_SPACING = 4;

    // TestIdSupport
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaneActionBar)
};

} // namespace oscil
