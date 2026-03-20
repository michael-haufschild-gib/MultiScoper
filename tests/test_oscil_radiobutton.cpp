/*
    Oscil - RadioButton Component Tests
    Tests for OscilRadioButton and OscilRadioGroup UI components
*/

#include <gtest/gtest.h>
#include "ui/components/OscilRadioButton.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

// =============================================================================
// OscilRadioButton Tests
// =============================================================================

class OscilRadioButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(OscilRadioButtonTest, DefaultConstruction)
{
    OscilRadioButton radio(getThemeManager());

    EXPECT_FALSE(radio.isSelected());
    EXPECT_TRUE(radio.isEnabled());
    EXPECT_TRUE(radio.getLabel().isEmpty());
}

TEST_F(OscilRadioButtonTest, ConstructionWithLabel)
{
    OscilRadioButton radio(getThemeManager(), "Option A");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
    EXPECT_FALSE(radio.isSelected());
}

TEST_F(OscilRadioButtonTest, ConstructionWithLabelAndTestId)
{
    OscilRadioButton radio(getThemeManager(), "Option A", "radio-1");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
}

TEST_F(OscilRadioButtonTest, SetSelected)
{
    OscilRadioButton radio(getThemeManager());

    radio.setSelected(true, false);
    EXPECT_TRUE(radio.isSelected());

    radio.setSelected(false, false);
    EXPECT_FALSE(radio.isSelected());
}

TEST_F(OscilRadioButtonTest, SetLabel)
{
    OscilRadioButton radio(getThemeManager());
    radio.setLabel("Option A");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
}

TEST_F(OscilRadioButtonTest, DefaultLabelOnRight)
{
    OscilRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.isLabelOnRight());
}

TEST_F(OscilRadioButtonTest, SetLabelOnRight)
{
    OscilRadioButton radio(getThemeManager());

    radio.setLabelOnRight(false);
    EXPECT_FALSE(radio.isLabelOnRight());

    radio.setLabelOnRight(true);
    EXPECT_TRUE(radio.isLabelOnRight());
}

TEST_F(OscilRadioButtonTest, EnabledState)
{
    OscilRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.isEnabled());

    radio.setEnabled(false);
    EXPECT_FALSE(radio.isEnabled());

    radio.setEnabled(true);
    EXPECT_TRUE(radio.isEnabled());
}

TEST_F(OscilRadioButtonTest, OnSelectedCallback)
{
    OscilRadioButton radio(getThemeManager());
    int selectCount = 0;

    radio.onSelected = [&selectCount]() {
        selectCount++;
    };

    radio.setSelected(true, true);
    EXPECT_EQ(selectCount, 1);
}

TEST_F(OscilRadioButtonTest, NoCallbackWhenNotifyFalse)
{
    OscilRadioButton radio(getThemeManager());
    int selectCount = 0;

    radio.onSelected = [&selectCount]() {
        selectCount++;
    };

    radio.setSelected(true, false);
    EXPECT_EQ(selectCount, 0);
}

TEST_F(OscilRadioButtonTest, PreferredWidthPositive)
{
    OscilRadioButton radio(getThemeManager());
    radio.setLabel("Test Option");

    int width = radio.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilRadioButtonTest, PreferredHeightPositive)
{
    OscilRadioButton radio(getThemeManager());

    int height = radio.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilRadioButtonTest, ThemeChangeDoesNotThrow)
{
    OscilRadioButton radio(getThemeManager());
    radio.setSelected(true, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    radio.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(radio.isSelected());
}

TEST_F(OscilRadioButtonTest, WantsKeyboardFocus)
{
    OscilRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.getWantsKeyboardFocus());
}

// =============================================================================
// OscilRadioGroup Tests
// =============================================================================

class OscilRadioGroupTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    void TearDown() override
    {
        themeManager_.reset();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }
    IThemeService& getThemeService() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(OscilRadioGroupTest, DefaultConstruction)
{
    OscilRadioGroup group(getThemeService());

    EXPECT_EQ(group.getNumOptions(), 0);
    EXPECT_EQ(group.getSelectedIndex(), -1);
    EXPECT_TRUE(group.getSelectedLabel().isEmpty());
}

TEST_F(OscilRadioGroupTest, ConstructionWithOrientation)
{
    OscilRadioGroup group(getThemeService(), OscilRadioGroup::Orientation::Horizontal);

    EXPECT_EQ(group.getOrientation(), OscilRadioGroup::Orientation::Horizontal);
}

TEST_F(OscilRadioGroupTest, AddOption)
{
    OscilRadioGroup group(getThemeService());

    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    EXPECT_EQ(group.getNumOptions(), 3);
}

TEST_F(OscilRadioGroupTest, AddOptionsInitializerList)
{
    OscilRadioGroup group(getThemeService());

    group.addOptions({"Option A", "Option B", "Option C"});

    EXPECT_EQ(group.getNumOptions(), 3);
}

TEST_F(OscilRadioGroupTest, ClearOptions)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    group.clearOptions();
    EXPECT_EQ(group.getNumOptions(), 0);
}

TEST_F(OscilRadioGroupTest, SetSelectedIndex)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    group.setSelectedIndex(1, false);
    EXPECT_EQ(group.getSelectedIndex(), 1);
}

TEST_F(OscilRadioGroupTest, GetSelectedLabel)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    group.setSelectedIndex(1, false);
    EXPECT_EQ(group.getSelectedLabel(), juce::String("Option B"));
}

TEST_F(OscilRadioGroupTest, MutualExclusivity)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    group.setSelectedIndex(0, false);
    EXPECT_EQ(group.getSelectedIndex(), 0);

    group.setSelectedIndex(2, false);
    EXPECT_EQ(group.getSelectedIndex(), 2);
    // Only one should be selected at a time
}

TEST_F(OscilRadioGroupTest, OnSelectionChangedCallback)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int changeCount = 0;
    int lastIndex = -1;

    group.onSelectionChanged = [&changeCount, &lastIndex](int index) {
        changeCount++;
        lastIndex = index;
    };

    group.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
}

TEST_F(OscilRadioGroupTest, OnSelectionChangedLabelCallback)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int changeCount = 0;
    juce::String lastLabel;

    group.onSelectionChangedLabel = [&changeCount, &lastLabel](const juce::String& label) {
        changeCount++;
        lastLabel = label;
    };

    group.setSelectedIndex(1, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastLabel, juce::String("Option B"));
}

TEST_F(OscilRadioGroupTest, NoCallbackWhenNotifyFalse)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int changeCount = 0;

    group.onSelectionChanged = [&changeCount](int) {
        changeCount++;
    };

    group.setSelectedIndex(1, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(OscilRadioGroupTest, SetOrientationVertical)
{
    OscilRadioGroup group(getThemeService());

    group.setOrientation(OscilRadioGroup::Orientation::Vertical);
    EXPECT_EQ(group.getOrientation(), OscilRadioGroup::Orientation::Vertical);
}

TEST_F(OscilRadioGroupTest, SetOrientationHorizontal)
{
    OscilRadioGroup group(getThemeService());

    group.setOrientation(OscilRadioGroup::Orientation::Horizontal);
    EXPECT_EQ(group.getOrientation(), OscilRadioGroup::Orientation::Horizontal);
}

TEST_F(OscilRadioGroupTest, SetSpacing)
{
    OscilRadioGroup group(getThemeService());

    group.setSpacing(20);
    EXPECT_EQ(group.getSpacing(), 20);
}

TEST_F(OscilRadioGroupTest, SetEnabled)
{
    OscilRadioGroup group(getThemeService());

    group.setEnabled(false);
    EXPECT_FALSE(group.isEnabled());

    group.setEnabled(true);
    EXPECT_TRUE(group.isEnabled());
}

TEST_F(OscilRadioGroupTest, InvalidIndexHandling)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.setSelectedIndex(0, false);

    group.setSelectedIndex(999, false);
    // Invalid index should not change the valid selection
    EXPECT_EQ(group.getSelectedIndex(), 0);
}

TEST_F(OscilRadioGroupTest, GetButton)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    auto* button = group.getButton(1);
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->getLabel(), juce::String("Option B"));
}

TEST_F(OscilRadioGroupTest, GetButtonInvalidIndex)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");

    auto* button = group.getButton(999);
    EXPECT_EQ(button, nullptr);
}

TEST_F(OscilRadioGroupTest, PreferredWidthPositive)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int width = group.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(OscilRadioGroupTest, PreferredHeightPositive)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int height = group.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilRadioGroupTest, ThemeChangeDoesNotThrow)
{
    OscilRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.setSelectedIndex(0, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    group.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(group.getSelectedIndex(), 0);
}
