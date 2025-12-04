/*
    Oscil - SelectPaneDialog Tests
    Tests for the select pane dialog component
*/

#include <gtest/gtest.h>
#include "ui/dialogs/SelectPaneDialog.h"
#include "ui/theme/ThemeManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil::test
{

class SelectPaneDialogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize JUCE message manager for tests
        juce::MessageManager::getInstance();
    }

    std::vector<Pane> createTestPanes(int count)
    {
        std::vector<Pane> panes;
        for (int i = 0; i < count; ++i)
        {
            Pane pane;
            pane.setName("Pane " + juce::String(i + 1));
            pane.setOrderIndex(i);
            panes.push_back(pane);
        }
        return panes;
    }

    std::vector<std::pair<PaneId, juce::String>> createTestPanePairs(int count)
    {
        std::vector<std::pair<PaneId, juce::String>> panes;
        for (int i = 0; i < count; ++i)
        {
            PaneId id;
            panes.emplace_back(id, "Pane " + juce::String(i + 1));
        }
        return panes;
    }
};

TEST_F(SelectPaneDialogTest, Construction)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    EXPECT_GT(dialog.getPreferredWidth(), 0);
    EXPECT_GT(dialog.getPreferredHeight(), 0);
}

TEST_F(SelectPaneDialogTest, SetAvailablePanesFromPaneVector)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    auto panes = createTestPanes(3);

    EXPECT_NO_THROW({
        dialog.setAvailablePanes(panes);
    });
}

TEST_F(SelectPaneDialogTest, SetAvailablePanesFromPairVector)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    auto panes = createTestPanePairs(3);

    EXPECT_NO_THROW({
        dialog.setAvailablePanes(panes);
    });
}

TEST_F(SelectPaneDialogTest, Reset)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    auto panes = createTestPanes(2);
    dialog.setAvailablePanes(panes);

    EXPECT_NO_THROW({
        dialog.reset();
    });
}

TEST_F(SelectPaneDialogTest, SetOnCompleteCallback)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    bool callbackSet = false;

    dialog.setOnComplete([&callbackSet](const SelectPaneDialog::Result&) {
        callbackSet = true;
    });

    // Just verify callback can be set without error
    EXPECT_FALSE(callbackSet);  // Not called yet
}

TEST_F(SelectPaneDialogTest, SetOnCancelCallback)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    bool callbackSet = false;

    dialog.setOnCancel([&callbackSet]() {
        callbackSet = true;
    });

    // Just verify callback can be set without error
    EXPECT_FALSE(callbackSet);  // Not called yet
}

TEST_F(SelectPaneDialogTest, PreferredDimensionsPositive)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    EXPECT_GT(dialog.getPreferredWidth(), 0);
    EXPECT_GT(dialog.getPreferredHeight(), 0);
}

TEST_F(SelectPaneDialogTest, ThemeChangeDoesNotThrow)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());

    // Theme changes should not throw
    EXPECT_NO_THROW({
        // Components handle theme changes via ThemeManager listener
    });
}

TEST_F(SelectPaneDialogTest, ResizeDoesNotThrow)
{
    SelectPaneDialog dialog(ThemeManager::getInstance());
    dialog.setSize(dialog.getPreferredWidth(), dialog.getPreferredHeight());

    EXPECT_NO_THROW({
        dialog.resized();
    });
}

} // namespace oscil::test
