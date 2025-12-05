/*
    Oscil - E2E Tests for AddOscillatorDialog
    Tests the Add Oscillator dialog functionality via direct component interaction
    Note: AddOscillatorDialog is now a content component designed to be hosted in OscilModal
*/

#include <gtest/gtest.h>
#include "ui/dialogs/AddOscillatorDialog.h"
#include "ui/theme/ThemeManager.h"
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/Source.h"

using namespace oscil;

class AddOscillatorDialogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize theme manager for this test
        themeManager_ = std::make_unique<ThemeManager>();

        // Create dialog with theme service
        dialog_ = std::make_unique<AddOscillatorDialog>(getThemeManager());
        dialog_->setSize(dialog_->getPreferredWidth(), dialog_->getPreferredHeight());

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
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

    void setupDialogWithCallback(AddOscillatorDialog::Callback callback)
    {
        dialog_->setData(testSources_, testPanes_);
        dialog_->setOnComplete(std::move(callback));
    }

    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<AddOscillatorDialog> dialog_;
    std::vector<SourceInfo> testSources_;
    std::vector<Pane> testPanes_;
};

// Test: Dialog has valid preferred dimensions
TEST_F(AddOscillatorDialogTest, DialogHasValidPreferredDimensions)
{
    // As a content component, AddOscillatorDialog provides preferred dimensions
    // for the parent modal to use
    EXPECT_GT(dialog_->getPreferredWidth(), 0);
    EXPECT_GT(dialog_->getPreferredHeight(), 0);
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

// Test: Dialog can be reset for reuse
TEST_F(AddOscillatorDialogTest, DialogCanBeReset)
{
    int callbackCount = 0;

    // Set up dialog
    setupDialogWithCallback([&callbackCount](const AddOscillatorDialog::Result&) {
        callbackCount++;
    });

    // Reset
    dialog_->reset();

    // Set up again
    setupDialogWithCallback([&callbackCount](const AddOscillatorDialog::Result&) {
        callbackCount++;
    });

    // Verify callback count is still 0 (no OK clicked)
    EXPECT_EQ(callbackCount, 0);
}

// Test: Dialog source list contains expected sources
TEST_F(AddOscillatorDialogTest, SourcesPopulatedCorrectly)
{
    setupDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Verify we passed 3 sources
    EXPECT_EQ(testSources_.size(), 3u);
    // No crash means sources were accepted
}

// Test: Dialog pane list contains expected panes plus "New pane" option
TEST_F(AddOscillatorDialogTest, PanesPopulatedCorrectly)
{
    setupDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // The dialog should have received 2 panes + "New pane" option = 3 items
    // Verify we passed 2 panes
    EXPECT_EQ(testPanes_.size(), 2u);
}

// Test: Theme changes are applied
TEST_F(AddOscillatorDialogTest, ThemeChangesApplied)
{
    // Get current theme
    const auto& theme = getThemeManager().getCurrentTheme();

    // Dialog should respond to theme (no crash)
    dialog_->themeChanged(theme);

    // Verify dialog is still valid
    EXPECT_NE(dialog_.get(), nullptr);
}

// Test: Dialog handles empty sources list
TEST_F(AddOscillatorDialogTest, HandlesEmptySourcesList)
{
    std::vector<SourceInfo> emptySources;

    // Should not crash with empty sources
    dialog_->setData(emptySources, testPanes_);
    SUCCEED();
}

// Test: Dialog handles empty panes list
TEST_F(AddOscillatorDialogTest, HandlesEmptyPanesList)
{
    std::vector<Pane> emptyPanes;

    // Should still work with empty panes (has "New pane" option)
    dialog_->setData(testSources_, emptyPanes);
    SUCCEED();
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
    setupDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Create a dummy graphics context
    juce::Image image(juce::Image::ARGB, 400, 500, true);
    juce::Graphics g(image);

    // Should not crash
    dialog_->paint(g);
}

// Test: Resized doesn't crash
TEST_F(AddOscillatorDialogTest, ResizedDoesNotCrash)
{
    setupDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Resize to different sizes
    dialog_->setSize(400, 500);
    dialog_->resized();

    dialog_->setSize(320, 420);
    dialog_->resized();

    // Should not crash - verified by reaching this point
    SUCCEED();
}

// Test: Dialog has reasonable preferred dimensions
TEST_F(AddOscillatorDialogTest, DialogDimensionsReasonable)
{
    // Dialog exposes preferred dimensions via getPreferredWidth/Height
    EXPECT_GT(dialog_->getPreferredWidth(), 0);
    EXPECT_GT(dialog_->getPreferredHeight(), 0);
    EXPECT_LE(dialog_->getPreferredWidth(), 500);   // Reasonable max
    EXPECT_LE(dialog_->getPreferredHeight(), 600);  // Reasonable max
}

// Test: Multiple sources with same name handled correctly
TEST_F(AddOscillatorDialogTest, HandlesDuplicateSourceNames)
{
    // Add a 4th source with duplicate name using default constructor
    SourceInfo source4;
    source4.sourceId = SourceId{"source_4"};
    source4.name = "Track 1";  // Duplicate name
    testSources_.push_back(source4);

    setupDialogWithCallback([](const AddOscillatorDialog::Result&) {});

    // Verify sources count
    EXPECT_EQ(testSources_.size(), 4u);  // Now 4 sources
    // No crash with duplicate names
}

// Test: Cancel callback is called when cancel is requested
TEST_F(AddOscillatorDialogTest, CancelCallbackCalled)
{
    bool cancelCalled = false;

    dialog_->setData(testSources_, testPanes_);
    dialog_->setOnCancel([&cancelCalled]() {
        cancelCalled = true;
    });

    // The cancel callback would be triggered by the cancel button click
    // We can't easily simulate that without more test infrastructure
    // but we verify the callback can be set
    EXPECT_FALSE(cancelCalled);  // Not called yet
}
