/*
    Oscil - StatusBarComponent Tests
    Verifies the two-zone layout's hint-text API, elision, and separator gating.
*/

#include "ui/panels/StatusBarComponent.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class StatusBarComponentTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(StatusBarComponentTest, DefaultHintIsEmpty)
{
    StatusBarComponent bar(getThemeManager());
    EXPECT_TRUE(bar.getHintText().isEmpty());
}

TEST_F(StatusBarComponentTest, HintTextRoundTrip)
{
    StatusBarComponent bar(getThemeManager());
    bar.setHintText("Hovering Oscillator 1");
    EXPECT_EQ(bar.getHintText(), juce::String("Hovering Oscillator 1"));

    bar.setHintText("Different status");
    EXPECT_EQ(bar.getHintText(), juce::String("Different status"));
}

TEST_F(StatusBarComponentTest, ClearHintBySettingEmpty)
{
    StatusBarComponent bar(getThemeManager());
    bar.setHintText("Something");
    EXPECT_FALSE(bar.getHintText().isEmpty());

    bar.setHintText({});
    EXPECT_TRUE(bar.getHintText().isEmpty());
}

TEST_F(StatusBarComponentTest, EmptyHintDoesNotDrawSeparator)
{
    StatusBarComponent bar(getThemeManager());
    // Wide enough that a hint would have space; assert only the empty-hint gate.
    bar.setSize(1200, 24);
    EXPECT_FALSE(bar.shouldDrawSeparator());
}

TEST_F(StatusBarComponentTest, NonEmptyHintWithRoomEnablesSeparator)
{
    StatusBarComponent bar(getThemeManager());
    bar.setSize(1200, 24);
    bar.setHintText("Ready");
    EXPECT_TRUE(bar.shouldDrawSeparator());
}

TEST_F(StatusBarComponentTest, NonEmptyHintWithoutRoomSuppressesSeparator)
{
    StatusBarComponent bar(getThemeManager());
    // Too narrow to fit the right-zone plus separator region plus any hint.
    bar.setSize(50, 24);
    bar.setHintText("Ready");
    EXPECT_FALSE(bar.shouldDrawSeparator());
}

TEST_F(StatusBarComponentTest, LongHintIsElidedAndEndsWithEllipsis)
{
    StatusBarComponent bar(getThemeManager());
    const juce::String longText = juce::String::repeatedString("abcdefghij ", 50);
    bar.setHintText(longText);

    // A narrow available width forces elision. The returned string must fit and
    // end with the ellipsis marker used by the implementation.
    const auto elided = bar.getElidedHintText(100.0f);
    EXPECT_TRUE(elided.endsWith("..."));
    EXPECT_LT(elided.length(), longText.length());
}

TEST_F(StatusBarComponentTest, ShortHintIsNotElided)
{
    StatusBarComponent bar(getThemeManager());
    const juce::String text = "OK";
    bar.setHintText(text);
    const auto elided = bar.getElidedHintText(500.0f);
    EXPECT_EQ(elided, text);
}

TEST_F(StatusBarComponentTest, ZeroAvailableWidthReturnsEmpty)
{
    StatusBarComponent bar(getThemeManager());
    bar.setHintText("Anything");
    EXPECT_TRUE(bar.getElidedHintText(0.0f).isEmpty());
}

TEST_F(StatusBarComponentTest, RenderingModeIsPreservedAcrossSetters)
{
    StatusBarComponent bar(getThemeManager());
    const auto initial = bar.getRenderingMode();
    bar.setFps(60.0f);
    bar.setCpuUsage(5.0f);
    bar.setMemoryUsage(32.0f);
    bar.setOscillatorCount(3);
    bar.setSourceCount(2);
    EXPECT_EQ(bar.getRenderingMode(), initial);
}
