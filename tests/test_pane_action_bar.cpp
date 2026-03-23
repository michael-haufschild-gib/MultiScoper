/*
    Oscil - Pane Action Bar Tests
*/

#include "ui/layout/pane/PaneActionBar.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

namespace
{
OscilButton* findHoldButton(PaneActionBar& actionBar)
{
    for (auto* child : actionBar.getChildren())
    {
        if (auto* button = dynamic_cast<OscilButton*>(child))
        {
            if (button->getTooltip().contains("Hold/Pause"))
                return button;
        }
    }
    return nullptr;
}

bool imagesEqual(const juce::Image& a, const juce::Image& b)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return false;

    for (int y = 0; y < a.getHeight(); ++y)
    {
        for (int x = 0; x < a.getWidth(); ++x)
        {
            if (a.getPixelAt(x, y) != b.getPixelAt(x, y))
                return false;
        }
    }
    return true;
}
} // namespace

class PaneActionBarTest : public ::testing::Test
{
protected:
    ThemeManager themeManager;
    PaneActionBar actionBar{themeManager};
};

TEST_F(PaneActionBarTest, ProgrammaticHoldToggleUpdatesToggleState)
{
    auto* holdButton = findHoldButton(actionBar);
    ASSERT_NE(holdButton, nullptr);

    actionBar.setActionToggled(PaneAction::ToggleHold, true);
    EXPECT_TRUE(actionBar.isActionToggled(PaneAction::ToggleHold));

    actionBar.setActionToggled(PaneAction::ToggleHold, false);
    EXPECT_FALSE(actionBar.isActionToggled(PaneAction::ToggleHold));
}

TEST_F(PaneActionBarTest, UserAndProgrammaticHoldToggleHaveConsistentVisualState)
{
    PaneActionBar userPathBar{themeManager};
    PaneActionBar programmaticPathBar{themeManager};
    userPathBar.setBounds(0, 0, 80, 24);
    programmaticPathBar.setBounds(0, 0, 80, 24);

    auto* userButton = findHoldButton(userPathBar);
    auto* programmaticButton = findHoldButton(programmaticPathBar);
    ASSERT_NE(userButton, nullptr);
    ASSERT_NE(programmaticButton, nullptr);

    userButton->triggerClick();
    programmaticPathBar.setActionToggled(PaneAction::ToggleHold, true);

    auto userSnapshot = userButton->createComponentSnapshot(userButton->getLocalBounds(), true, 1.0f);
    auto programmaticSnapshot =
        programmaticButton->createComponentSnapshot(programmaticButton->getLocalBounds(), true, 1.0f);

    EXPECT_TRUE(imagesEqual(userSnapshot, programmaticSnapshot));
}
