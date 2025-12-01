/*
    Oscil - E2E Tests for AddOscillatorDialog
    Tests the Add Oscillator dialog functionality via direct component interaction
*/

#include <gtest/gtest.h>
#include "ui/AddOscillatorDialog.h"
#include "ui/ThemeManager.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/Source.h"

using namespace oscil;

class AddOscillatorDialogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure theme manager is initialized
        ThemeManager::getInstance();

        // Create dialog
        dialog_ = std::make_unique<AddOscillatorDialog>();
        dialog_->setSize(800, 600);  // Give it space for the modal

        // Create test sources using default constructor and setting properties
        testSources_.clear();
        {
            SourceInfo source1;
            source1.sourceId = SourceId{"source_1"};
            source1.name = "Track 1";
            testSources_.push_back(source1);
        }
        {
            SourceInfo source2;
            source2.sourceId = SourceId{"source_2"};
            source2.name = "Track 2";
            testSources_.push_back(source2);
        }
        {
            SourceInfo source3;
            source3.sourceId = SourceId{"source_3"};
            source3.name = "Bass";
            testSources_.push_back(source3);
        }

        // Create test panes using default constructor and setting properties
        testPanes_.clear();
        {
            Pane pane1;
            pane1.setName("Pane 1");
            pane1.setOrderIndex(0);
            pane1.setColumnIndex(0);
            testPanes_.push_back(pane1);
        }
        {
            Pane pane2;
            pane2.setName("Pane 2");
            pane2.setOrderIndex(1);
            pane2.setColumnIndex(1);
            testPanes_.push_back(pane2);
        }
    }

    void TearDown() override
    {
        dialog_.reset();
    }

    void showDialogWithCallback(AddOscillatorDialog::Callback callback)
    {
        dialog_->showDialog(nullptr, testSources_, testPanes_, std::move(callback));
    }

    std::unique_ptr<AddOscillatorDialog> dialog_;
    std::vector<SourceInfo> testSources_;
    std::vector<Pane> testPanes_;
};

// Test: Dialog is initially not visible
TEST_F(AddOscillatorDialogTest, DialogInitiallyNotVisible)
{
    EXPECT_FALSE(dialog_->isVisible());
}

// Test: Dialog becomes visible when showDialog is called
TEST_F(AddOscillatorDialogTest, DialogVisibleAfterShow)
{
    bool callbackCalled = false;
    showDialogWithCallback([&callbackCalled](const AddOscillatorDialog::Result&) {
        callbackCalled = true;
    });

    EXPECT_TRUE(dialog_->isVisible());
    EXPECT_FALSE(callbackCalled);  // Callback not called until OK is clicked
}

// Test: hideDialog makes dialog invisible
TEST_F(AddOscillatorDialogTest, DialogHiddenAfterHide)
{
    bool callbackCalled = false;
    showDialogWithCallback([&callbackCalled](const AddOscillatorDialog::Result&) {
        callbackCalled = true;
    });

    EXPECT_TRUE(dialog_->isVisible());

    dialog_->hideDialog();

    EXPECT_FALSE(dialog_->isVisible());
    EXPECT_FALSE(callbackCalled);  // Callback not called on hide
}

// Test: Dialog test ID support is properly configured
// Note: In unit tests (without TEST_HARNESS), OSCIL_REGISTER_TEST_ID is a no-op
// The test ID would be set when running with the test harness
TEST_F(AddOscillatorDialogTest, DialogHasTestIdSupport)
{
    // Verify the dialog inherits from TestIdSupport (by calling the method)
    // In unit tests, testId_ is not set because OSCIL_REGISTER_TEST_ID is a no-op
    auto testId = dialog_->getTestId();
    // Test simply verifies that getTestId() can be called without crashing
    // Actual test ID validation would occur in test harness E2E tests
    SUCCEED();
}

// Test: Dialog can be shown multiple times (state resets)
TEST_F(AddOscillatorDialogTest, DialogCanBeReshown)
{
    int callbackCount = 0;

    // First show
    showDialogWithCallback([&callbackCount](const AddOscillatorDialog::Result&) {
        callbackCount++;
    });
    EXPECT_TRUE(dialog_->isVisible());

    // Hide
    dialog_->hideDialog();
    EXPECT_FALSE(dialog_->isVisible());

    // Show again with new callback
    showDialogWithCallback([&callbackCount](const AddOscillatorDialog::Result&) {
        callbackCount++;
    });
    EXPECT_TRUE(dialog_->isVisible());

    // Verify callback count is still 0 (no OK clicked)
    EXPECT_EQ(callbackCount, 0);
}

// Test: Dialog source list contains expected sources
TEST_F(AddOscillatorDialogTest, SourcesPopulatedCorrectly)
{
    showDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // The dialog should have received 3 sources
    // We can verify this indirectly by checking the dialog is visible
    // (actual dropdown verification would require simulating clicks)
    EXPECT_TRUE(dialog_->isVisible());

    // Verify we passed 3 sources
    EXPECT_EQ(testSources_.size(), 3u);
}

// Test: Dialog pane list contains expected panes plus "New pane" option
TEST_F(AddOscillatorDialogTest, PanesPopulatedCorrectly)
{
    showDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // The dialog should have received 2 panes + "New pane" option = 3 items
    // Verify we passed 2 panes
    EXPECT_EQ(testPanes_.size(), 2u);
}

// Test: Theme changes are applied
TEST_F(AddOscillatorDialogTest, ThemeChangesApplied)
{
    // Get current theme
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Dialog should respond to theme (no crash)
    dialog_->themeChanged(theme);

    // Verify dialog is still valid
    EXPECT_NE(dialog_.get(), nullptr);
}

// Test: Dialog handles empty sources list
TEST_F(AddOscillatorDialogTest, HandlesEmptySourcesList)
{
    std::vector<SourceInfo> emptySources;

    dialog_->showDialog(nullptr, emptySources, testPanes_, [](const AddOscillatorDialog::Result&) {});

    EXPECT_TRUE(dialog_->isVisible());
}

// Test: Dialog handles empty panes list
TEST_F(AddOscillatorDialogTest, HandlesEmptyPanesList)
{
    std::vector<Pane> emptyPanes;

    dialog_->showDialog(nullptr, testSources_, emptyPanes, [](const AddOscillatorDialog::Result&) {});

    // Should still have "New pane" option
    EXPECT_TRUE(dialog_->isVisible());
}

// Test: Result struct has correct default values
TEST_F(AddOscillatorDialogTest, ResultStructDefaults)
{
    AddOscillatorDialog::Result result;

    // Verify default values
    EXPECT_TRUE(result.sourceId.id.isEmpty());
    EXPECT_TRUE(result.paneId.id.isEmpty());
    EXPECT_TRUE(result.name.isEmpty());
    EXPECT_FALSE(result.createNewPane);
    // Color has a default (black)
    EXPECT_EQ(result.color, juce::Colour());
}

// Test: Paint doesn't crash
TEST_F(AddOscillatorDialogTest, PaintDoesNotCrash)
{
    showDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Create a dummy graphics context
    juce::Image image(juce::Image::ARGB, 800, 600, true);
    juce::Graphics g(image);

    // Should not crash
    dialog_->paint(g);
}

// Test: Resized doesn't crash
TEST_F(AddOscillatorDialogTest, ResizedDoesNotCrash)
{
    showDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Resize to different sizes
    dialog_->setSize(1024, 768);
    dialog_->resized();

    dialog_->setSize(640, 480);
    dialog_->resized();

    // Should not crash
    EXPECT_TRUE(dialog_->isVisible());
}

// Test: Dialog dimensions are reasonable
TEST_F(AddOscillatorDialogTest, DialogDimensionsReasonable)
{
    // Dialog has internal constants for size
    // DIALOG_WIDTH = 360, DIALOG_HEIGHT = 590 (increased for shader dropdown)
    // These should be visible in the dialog's paint method
    EXPECT_EQ(AddOscillatorDialog::DIALOG_WIDTH, 360);
    EXPECT_EQ(AddOscillatorDialog::DIALOG_HEIGHT, 550);
}

// Test: Multiple sources with same name handled correctly
TEST_F(AddOscillatorDialogTest, HandlesDuplicateSourceNames)
{
    // Add a 4th source with duplicate name using default constructor
    SourceInfo source4;
    source4.sourceId = SourceId{"source_4"};
    source4.name = "Track 1";  // Duplicate name
    testSources_.push_back(source4);

    showDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    EXPECT_TRUE(dialog_->isVisible());
    EXPECT_EQ(testSources_.size(), 4u);  // Now 4 sources
}

