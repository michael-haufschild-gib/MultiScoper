#include "core/WaveformColorPalette.h"
#include "ui/dialogs/OscillatorColorDialog.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

namespace oscil
{

class OscillatorColorDialogTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
        dialog = std::make_unique<OscillatorColorDialog>(getThemeManager());
    }

    void TearDown() override
    {
        dialog.reset();
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

    OscilButton* findButtonWithText(const juce::String& text)
    {
        for (auto* child : dialog->getChildren())
        {
            if (auto* button = dynamic_cast<OscilButton*>(child))
            {
                if (button->getText() == text)
                    return button;
            }
        }
        return nullptr;
    }

    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<OscillatorColorDialog> dialog;
};

TEST_F(OscillatorColorDialogTest, InitialState)
{
    EXPECT_NE(dialog->getPreferredWidth(), 0);
    EXPECT_NE(dialog->getPreferredHeight(), 0);
}

TEST_F(OscillatorColorDialogTest, SetColors)
{
    auto colors = WaveformColorPalette::getAllColors();
    ASSERT_FALSE(colors.empty());

    dialog->setColors(colors);
    // Dialog should still report valid dimensions after setting colors
    EXPECT_GT(dialog->getPreferredWidth(), 0);
    EXPECT_GT(dialog->getPreferredHeight(), 0);
}

TEST_F(OscillatorColorDialogTest, CallbackTriggered)
{
    auto colors = WaveformColorPalette::getAllColors();
    ASSERT_FALSE(colors.empty());
    dialog->setColors(colors);
    dialog->setSelectedColor(colors[2]);

    bool callbackCalled = false;
    juce::Colour selectedColor;

    dialog->setOnColorSelected([&](juce::Colour c) {
        callbackCalled = true;
        selectedColor = c;
    });

    auto* okButton = findButtonWithText("OK");
    ASSERT_NE(okButton, nullptr);
    okButton->triggerClick();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(selectedColor.getARGB(), colors[2].getARGB());
}

TEST_F(OscillatorColorDialogTest, NonPaletteInitialColorFallsBackToFirstPaletteColorOnConfirm)
{
    auto colors = WaveformColorPalette::getAllColors();
    ASSERT_FALSE(colors.empty());
    dialog->setColors(colors);

    // Simulates restoring/importing a custom color that's not in the fixed palette.
    dialog->setSelectedColor(juce::Colour(0xFF123456));

    bool callbackCalled = false;
    juce::Colour selectedColor;
    dialog->setOnColorSelected([&](juce::Colour c) {
        callbackCalled = true;
        selectedColor = c;
    });

    auto* okButton = findButtonWithText("OK");
    ASSERT_NE(okButton, nullptr);
    okButton->triggerClick();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(selectedColor.getARGB(), WaveformColorPalette::getColor(0).getARGB());
}

TEST_F(OscillatorColorDialogTest, NonPaletteColorDoesNotReusePreviousSelection)
{
    auto colors = WaveformColorPalette::getAllColors();
    ASSERT_GE(colors.size(), 6u);
    dialog->setColors(colors);

    // Simulate a previous dialog invocation selecting a palette entry.
    dialog->setSelectedColor(colors[5]);
    // New invocation with a non-palette color should not keep the old swatch.
    dialog->setSelectedColor(juce::Colour(0xFF654321));

    bool callbackCalled = false;
    juce::Colour selectedColor;
    dialog->setOnColorSelected([&](juce::Colour c) {
        callbackCalled = true;
        selectedColor = c;
    });

    auto* okButton = findButtonWithText("OK");
    ASSERT_NE(okButton, nullptr);
    okButton->triggerClick();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(selectedColor.getARGB(), WaveformColorPalette::getColor(0).getARGB());
}

} // namespace oscil
