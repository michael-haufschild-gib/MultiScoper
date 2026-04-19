/*
    MultiScoper - Keyboard pass-through regression tests

    Plugin hosts (Ableton, Bitwig, Logic, Reaper, FL, etc.) use spacebar and
    return for transport start/stop and other global shortcuts. When a JUCE
    component's keyPressed() returns true for these keys, JUCE stops the event
    from bubbling and the host never sees it — transport shortcuts die.

    These tests lock in the contract: our focusable widgets MUST NOT consume
    spaceKey or returnKey. Text input widgets (juce::TextEditor and anything
    built on it) keep their own keystrokes while they hold focus — that path
    is not exercised here because it is framework behavior, not ours to own.
*/

#include "ui/components/MultiScoperAccordion.h"
#include "ui/components/MultiScoperButton.h"
#include "ui/components/MultiScoperCheckbox.h"
#include "ui/components/MultiScoperColorSwatches.h"
#include "ui/components/MultiScoperDropdown.h"
#include "ui/components/MultiScoperRadioButton.h"
#include "ui/components/MultiScoperToggle.h"
#include "ui/theme/ThemeManager.h"

#include "MultiScoperTestFixtures.h"

#include <gtest/gtest.h>

using namespace multiscoper;
using namespace multiscoper::test;

namespace
{

const juce::KeyPress kSpace{juce::KeyPress::spaceKey};
const juce::KeyPress kReturn{juce::KeyPress::returnKey};

} // namespace

class KeyPassthroughTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }
    void TearDown() override { themeManager_.reset(); }

    ThemeManager& theme() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// -----------------------------------------------------------------------------
// Button
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, ButtonDoesNotConsumeSpaceOrReturn)
{
    MultiScoperButton button(theme());
    int clicks = 0;
    button.onClick = [&clicks] { ++clicks; };

    EXPECT_FALSE(button.keyPressed(kSpace));
    EXPECT_FALSE(button.keyPressed(kReturn));
    EXPECT_EQ(clicks, 0) << "Space/return must not trigger button click in a plugin UI";
}

TEST_F(KeyPassthroughTest, ButtonStillHonoursExplicitShortcut)
{
    MultiScoperButton button(theme());
    int clicks = 0;
    button.onClick = [&clicks] { ++clicks; };
    button.setShortcut(juce::KeyPress('k'));

    EXPECT_TRUE(button.keyPressed(juce::KeyPress('k')));
    EXPECT_EQ(clicks, 1) << "setShortcut() must remain functional for opt-in bindings";
}

// -----------------------------------------------------------------------------
// Toggle / Checkbox
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, ToggleDoesNotConsumeSpaceOrReturn)
{
    MultiScoperToggle toggle(theme());
    bool const initial = toggle.getValue();

    EXPECT_FALSE(toggle.keyPressed(kSpace));
    EXPECT_FALSE(toggle.keyPressed(kReturn));
    EXPECT_EQ(toggle.getValue(), initial) << "Toggle state must not change from space/return";
}

TEST_F(KeyPassthroughTest, CheckboxDoesNotConsumeSpaceOrReturn)
{
    MultiScoperCheckbox checkbox(theme());
    bool const initial = checkbox.isChecked();

    EXPECT_FALSE(checkbox.keyPressed(kSpace));
    EXPECT_FALSE(checkbox.keyPressed(kReturn));
    EXPECT_EQ(checkbox.isChecked(), initial) << "Checkbox state must not change from space/return";
}

// -----------------------------------------------------------------------------
// Radio button (individual — group still handles arrows, tested elsewhere)
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, RadioButtonDoesNotConsumeSpaceOrReturn)
{
    MultiScoperRadioButton radio(theme());

    EXPECT_FALSE(radio.keyPressed(kSpace));
    EXPECT_FALSE(radio.keyPressed(kReturn));
}

// -----------------------------------------------------------------------------
// Accordion section
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, AccordionSectionDoesNotConsumeSpaceOrReturn)
{
    MultiScoperAccordionSection section(theme(), "Section");
    bool const initial = section.isExpanded();

    EXPECT_FALSE(section.keyPressed(kSpace));
    EXPECT_FALSE(section.keyPressed(kReturn));
    EXPECT_EQ(section.isExpanded(), initial) << "Accordion must not expand/collapse from space/return";
}

// -----------------------------------------------------------------------------
// Dropdown trigger (popup internals are a separate modal-like context)
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, DropdownDoesNotConsumeSpaceOrReturn)
{
    MultiScoperDropdown dropdown(theme());
    dropdown.addItem("One");
    dropdown.addItem("Two");

    EXPECT_FALSE(dropdown.keyPressed(kSpace));
    EXPECT_FALSE(dropdown.keyPressed(kReturn));
}

// -----------------------------------------------------------------------------
// Color swatches
// -----------------------------------------------------------------------------

TEST_F(KeyPassthroughTest, ColorSwatchesDoNotConsumeSpaceOrReturn)
{
    MultiScoperColorSwatches swatches(theme());
    swatches.setColors({juce::Colours::red, juce::Colours::green, juce::Colours::blue});
    int selectionChanges = 0;
    swatches.onColorSelected = [&selectionChanges](int, juce::Colour) { ++selectionChanges; };

    EXPECT_FALSE(swatches.keyPressed(kSpace));
    EXPECT_FALSE(swatches.keyPressed(kReturn));
    EXPECT_EQ(selectionChanges, 0) << "Swatch selection must not fire from space/return";
}
