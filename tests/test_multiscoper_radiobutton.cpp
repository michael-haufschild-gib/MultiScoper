/*
    MultiScoper - RadioButton Component Tests
    Tests for MultiScoperRadioButton and MultiScoperRadioGroup UI components
*/

#include "ui/components/MultiScoperRadioButton.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

// =============================================================================
// MultiScoperRadioButton Tests
// =============================================================================

class MultiScoperRadioButtonTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(MultiScoperRadioButtonTest, DefaultConstruction)
{
    MultiScoperRadioButton radio(getThemeManager());

    EXPECT_FALSE(radio.isSelected());
    EXPECT_TRUE(radio.isEnabled());
    EXPECT_TRUE(radio.getLabel().isEmpty());
}

TEST_F(MultiScoperRadioButtonTest, ConstructionWithLabel)
{
    MultiScoperRadioButton radio(getThemeManager(), "Option A");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
    EXPECT_FALSE(radio.isSelected());
}

TEST_F(MultiScoperRadioButtonTest, ConstructionWithLabelAndTestId)
{
    MultiScoperRadioButton radio(getThemeManager(), "Option A", "radio-1");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
}

TEST_F(MultiScoperRadioButtonTest, SetSelected)
{
    MultiScoperRadioButton radio(getThemeManager());

    radio.setSelected(true, false);
    EXPECT_TRUE(radio.isSelected());

    radio.setSelected(false, false);
    EXPECT_FALSE(radio.isSelected());
}

TEST_F(MultiScoperRadioButtonTest, SetLabel)
{
    MultiScoperRadioButton radio(getThemeManager());
    radio.setLabel("Option A");

    EXPECT_EQ(radio.getLabel(), juce::String("Option A"));
}

TEST_F(MultiScoperRadioButtonTest, DefaultLabelOnRight)
{
    MultiScoperRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.isLabelOnRight());
}

TEST_F(MultiScoperRadioButtonTest, SetLabelOnRight)
{
    MultiScoperRadioButton radio(getThemeManager());

    radio.setLabelOnRight(false);
    EXPECT_FALSE(radio.isLabelOnRight());

    radio.setLabelOnRight(true);
    EXPECT_TRUE(radio.isLabelOnRight());
}

TEST_F(MultiScoperRadioButtonTest, EnabledState)
{
    MultiScoperRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.isEnabled());

    radio.setEnabled(false);
    EXPECT_FALSE(radio.isEnabled());

    radio.setEnabled(true);
    EXPECT_TRUE(radio.isEnabled());
}

TEST_F(MultiScoperRadioButtonTest, OnSelectedCallback)
{
    MultiScoperRadioButton radio(getThemeManager());
    int selectCount = 0;

    radio.onSelected = [&selectCount]() { selectCount++; };

    radio.setSelected(true, true);
    EXPECT_EQ(selectCount, 1);
}

TEST_F(MultiScoperRadioButtonTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperRadioButton radio(getThemeManager());
    int selectCount = 0;

    radio.onSelected = [&selectCount]() { selectCount++; };

    radio.setSelected(true, false);
    EXPECT_EQ(selectCount, 0);
}

TEST_F(MultiScoperRadioButtonTest, PreferredWidthPositive)
{
    MultiScoperRadioButton radio(getThemeManager());
    radio.setLabel("Test Option");

    int width = radio.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(MultiScoperRadioButtonTest, PreferredHeightPositive)
{
    MultiScoperRadioButton radio(getThemeManager());

    int height = radio.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(MultiScoperRadioButtonTest, ThemeChangeDoesNotThrow)
{
    MultiScoperRadioButton radio(getThemeManager());
    radio.setSelected(true, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    radio.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(radio.isSelected());
}

TEST_F(MultiScoperRadioButtonTest, WantsKeyboardFocus)
{
    MultiScoperRadioButton radio(getThemeManager());

    EXPECT_TRUE(radio.getWantsKeyboardFocus());
}

// =============================================================================
// MultiScoperRadioGroup Tests
// =============================================================================

class MultiScoperRadioGroupTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }
    IThemeService& getThemeService() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(MultiScoperRadioGroupTest, DefaultConstruction)
{
    MultiScoperRadioGroup group(getThemeService());

    EXPECT_EQ(group.getNumOptions(), 0);
    EXPECT_EQ(group.getSelectedIndex(), -1);
    EXPECT_TRUE(group.getSelectedLabel().isEmpty());
}

TEST_F(MultiScoperRadioGroupTest, ConstructionWithOrientation)
{
    MultiScoperRadioGroup group(getThemeService(), MultiScoperRadioGroup::Orientation::Horizontal);

    EXPECT_EQ(group.getOrientation(), MultiScoperRadioGroup::Orientation::Horizontal);
}

TEST_F(MultiScoperRadioGroupTest, AddOption)
{
    MultiScoperRadioGroup group(getThemeService());

    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    EXPECT_EQ(group.getNumOptions(), 3);
}

TEST_F(MultiScoperRadioGroupTest, AddOptionsInitializerList)
{
    MultiScoperRadioGroup group(getThemeService());

    group.addOptions({"Option A", "Option B", "Option C"});

    EXPECT_EQ(group.getNumOptions(), 3);
}

TEST_F(MultiScoperRadioGroupTest, ClearOptions)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    group.clearOptions();
    EXPECT_EQ(group.getNumOptions(), 0);
}

TEST_F(MultiScoperRadioGroupTest, SetSelectedIndex)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    group.setSelectedIndex(1, false);
    EXPECT_EQ(group.getSelectedIndex(), 1);
}

TEST_F(MultiScoperRadioGroupTest, GetSelectedLabel)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    group.setSelectedIndex(1, false);
    EXPECT_EQ(group.getSelectedLabel(), juce::String("Option B"));
}

TEST_F(MultiScoperRadioGroupTest, MutualExclusivity)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");
    group.addOption("Option C");

    group.setSelectedIndex(0, false);
    EXPECT_EQ(group.getSelectedIndex(), 0);

    group.setSelectedIndex(2, false);
    EXPECT_EQ(group.getSelectedIndex(), 2);
    // Only one should be selected at a time
}

TEST_F(MultiScoperRadioGroupTest, OnSelectionChangedCallback)
{
    MultiScoperRadioGroup group(getThemeService());
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

TEST_F(MultiScoperRadioGroupTest, OnSelectionChangedLabelCallback)
{
    MultiScoperRadioGroup group(getThemeService());
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

TEST_F(MultiScoperRadioGroupTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int changeCount = 0;

    group.onSelectionChanged = [&changeCount](int) { changeCount++; };

    group.setSelectedIndex(1, false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(MultiScoperRadioGroupTest, SetOrientationVertical)
{
    MultiScoperRadioGroup group(getThemeService());

    group.setOrientation(MultiScoperRadioGroup::Orientation::Vertical);
    EXPECT_EQ(group.getOrientation(), MultiScoperRadioGroup::Orientation::Vertical);
}

TEST_F(MultiScoperRadioGroupTest, SetOrientationHorizontal)
{
    MultiScoperRadioGroup group(getThemeService());

    group.setOrientation(MultiScoperRadioGroup::Orientation::Horizontal);
    EXPECT_EQ(group.getOrientation(), MultiScoperRadioGroup::Orientation::Horizontal);
}

TEST_F(MultiScoperRadioGroupTest, SetSpacing)
{
    MultiScoperRadioGroup group(getThemeService());

    group.setSpacing(20);
    EXPECT_EQ(group.getSpacing(), 20);
}

TEST_F(MultiScoperRadioGroupTest, SetEnabled)
{
    MultiScoperRadioGroup group(getThemeService());

    group.setEnabled(false);
    EXPECT_FALSE(group.isEnabled());

    group.setEnabled(true);
    EXPECT_TRUE(group.isEnabled());
}

TEST_F(MultiScoperRadioGroupTest, InvalidIndexHandling)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.setSelectedIndex(0, false);

    group.setSelectedIndex(999, false);
    // Invalid index should not change the valid selection
    EXPECT_EQ(group.getSelectedIndex(), 0);
}

TEST_F(MultiScoperRadioGroupTest, GetButton)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    auto* button = group.getButton(1);
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->getLabel(), juce::String("Option B"));
}

TEST_F(MultiScoperRadioGroupTest, GetButtonInvalidIndex)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");

    auto* button = group.getButton(999);
    EXPECT_EQ(button, nullptr);
}

TEST_F(MultiScoperRadioGroupTest, PreferredWidthPositive)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int width = group.getPreferredWidth();
    EXPECT_GT(width, 0);
}

TEST_F(MultiScoperRadioGroupTest, PreferredHeightPositive)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.addOption("Option B");

    int height = group.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(MultiScoperRadioGroupTest, ThemeChangeDoesNotThrow)
{
    MultiScoperRadioGroup group(getThemeService());
    group.addOption("Option A");
    group.setSelectedIndex(0, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    group.themeChanged(newTheme);

    // Selection should be preserved
    EXPECT_EQ(group.getSelectedIndex(), 0);
}
