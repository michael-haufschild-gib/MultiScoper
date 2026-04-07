/*
    Oscil - SelectPaneDialog Tests
    Tests for the select pane dialog component
*/

#include "ui/dialogs/SelectPaneDialog.h"
#include "ui/theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <gtest/gtest.h>

namespace oscil::test
{

class SelectPaneDialogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        juce::MessageManager::getInstance();
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

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

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(SelectPaneDialogTest, Construction)
{
    SelectPaneDialog dialog(getThemeManager());
    EXPECT_GT(dialog.getPreferredWidth(), 0);
    EXPECT_GT(dialog.getPreferredHeight(), 0);
}

TEST_F(SelectPaneDialogTest, SetAvailablePanesFromPaneVector)
{
    SelectPaneDialog dialog(getThemeManager());
    auto panes = createTestPanes(3);
    dialog.setAvailablePanes(panes);

    // Verify dialog is in a valid state after setting panes
    EXPECT_GT(dialog.getPreferredWidth(), 0);
}

TEST_F(SelectPaneDialogTest, SetAvailablePanesFromPairVector)
{
    SelectPaneDialog dialog(getThemeManager());
    auto panes = createTestPanePairs(3);
    dialog.setAvailablePanes(panes);

    EXPECT_GT(dialog.getPreferredWidth(), 0);
}

TEST_F(SelectPaneDialogTest, OkClickWithNewPaneFiresCallbackWithCreateNewPaneTrue)
{
    SelectPaneDialog dialog(getThemeManager());
    auto panes = createTestPanes(2);
    dialog.setAvailablePanes(panes);

    // Default selection is "New pane" after setAvailablePanes calls reset()
    bool callbackFired = false;
    bool createNewPane = false;

    dialog.setOnComplete([&](const SelectPaneDialog::Result& result) {
        callbackFired = true;
        createNewPane = result.createNewPane;
    });

    // Simulate OK click via the public onClick callback
    // The OK button's onClick is wired to handleOkClick in setupComponents
    // We need to trigger it — use the child component directly
    for (int i = 0; i < dialog.getNumChildComponents(); ++i)
    {
        auto* child = dialog.getChildComponent(i);
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            // Find the OK button (Primary variant)
            if (btn->getVariant() == ButtonVariant::Primary && btn->onClick)
            {
                btn->onClick();
                break;
            }
        }
    }

    EXPECT_TRUE(callbackFired);
    EXPECT_TRUE(createNewPane);
}

TEST_F(SelectPaneDialogTest, CancelClickFiresCancelCallback)
{
    SelectPaneDialog dialog(getThemeManager());
    auto panes = createTestPanes(2);
    dialog.setAvailablePanes(panes);

    bool cancelFired = false;
    dialog.setOnCancel([&]() { cancelFired = true; });

    // Find and click the Cancel button (Secondary variant)
    for (int i = 0; i < dialog.getNumChildComponents(); ++i)
    {
        auto* child = dialog.getChildComponent(i);
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            if (btn->getVariant() == ButtonVariant::Secondary && btn->onClick)
            {
                btn->onClick();
                break;
            }
        }
    }

    EXPECT_TRUE(cancelFired);
}

TEST_F(SelectPaneDialogTest, ResetClearsState)
{
    SelectPaneDialog dialog(getThemeManager());
    auto panes = createTestPanes(2);
    dialog.setAvailablePanes(panes);

    dialog.reset();

    // After reset, default is "New pane" selected — OK should fire with createNewPane=true
    bool createNewPane = false;
    dialog.setOnComplete([&](const SelectPaneDialog::Result& result) { createNewPane = result.createNewPane; });

    for (int i = 0; i < dialog.getNumChildComponents(); ++i)
    {
        auto* child = dialog.getChildComponent(i);
        if (auto* btn = dynamic_cast<OscilButton*>(child))
        {
            if (btn->getVariant() == ButtonVariant::Primary && btn->onClick)
            {
                btn->onClick();
                break;
            }
        }
    }

    EXPECT_TRUE(createNewPane);
}

TEST_F(SelectPaneDialogTest, PreferredDimensionsPositive)
{
    SelectPaneDialog dialog(getThemeManager());
    EXPECT_GT(dialog.getPreferredWidth(), 0);
    EXPECT_GT(dialog.getPreferredHeight(), 0);
}

TEST_F(SelectPaneDialogTest, ResizeDoesNotThrow)
{
    SelectPaneDialog dialog(getThemeManager());
    dialog.setSize(dialog.getPreferredWidth(), dialog.getPreferredHeight());
    EXPECT_NO_THROW({ dialog.resized(); });
}

} // namespace oscil::test
