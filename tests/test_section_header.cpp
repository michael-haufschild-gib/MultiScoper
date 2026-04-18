/*
    Oscil - Section Header Tests

    Covers: title round-trip, chevron hit-testing (left vs right bounds fire
    onPrev/onNext), suppression when chevrons hidden, accent setter contract.
*/

#include "ui/components/SectionHeader.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

namespace
{
juce::MouseEvent makeMouseEvent(juce::Component& component, juce::Point<int> position)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position.toFloat(),
                            juce::ModifierKeys::leftButtonModifier, juce::MouseInputSource::defaultPressure,
                            juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                            juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &component,
                            &component, now, position.toFloat(), now, 1, /*mouseWasDragged*/ false);
}

constexpr int kComponentWidth = 160;
constexpr int kComponentHeight = SectionHeader::PREFERRED_HEIGHT;
} // namespace

class SectionHeaderTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }
    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Title Round Trip
// =============================================================================

TEST_F(SectionHeaderTest, DefaultTitleIsEmpty)
{
    SectionHeader header(getThemeManager());
    EXPECT_TRUE(header.getTitle().isEmpty());
}

TEST_F(SectionHeaderTest, ConstructorTitleRoundTrip)
{
    SectionHeader header(getThemeManager(), "Presets");
    EXPECT_EQ(header.getTitle(), juce::String("Presets"));
}

TEST_F(SectionHeaderTest, SetTitleRoundTrip)
{
    SectionHeader header(getThemeManager());
    header.setTitle("Waveform");
    EXPECT_EQ(header.getTitle(), juce::String("Waveform"));

    header.setTitle("Effects");
    EXPECT_EQ(header.getTitle(), juce::String("Effects"));

    header.setTitle({});
    EXPECT_TRUE(header.getTitle().isEmpty());
}

// =============================================================================
// Chevron Click Dispatch
// =============================================================================

TEST_F(SectionHeaderTest, LeftmostClickFiresOnPrevWhenChevronsVisible)
{
    SectionHeader header(getThemeManager(), "Section");
    header.setChevronsVisible(true);
    header.setSize(kComponentWidth, kComponentHeight);

    int prevCalls = 0;
    int nextCalls = 0;
    header.onPrev = [&]() { ++prevCalls; };
    header.onNext = [&]() { ++nextCalls; };

    // Click inside the leftmost 20px column.
    header.mouseDown(makeMouseEvent(header, {5, kComponentHeight / 2}));
    EXPECT_EQ(prevCalls, 1);
    EXPECT_EQ(nextCalls, 0);
}

TEST_F(SectionHeaderTest, RightmostClickFiresOnNextWhenChevronsVisible)
{
    SectionHeader header(getThemeManager(), "Section");
    header.setChevronsVisible(true);
    header.setSize(kComponentWidth, kComponentHeight);

    int prevCalls = 0;
    int nextCalls = 0;
    header.onPrev = [&]() { ++prevCalls; };
    header.onNext = [&]() { ++nextCalls; };

    // Click inside the rightmost 20px column.
    header.mouseDown(makeMouseEvent(header, {kComponentWidth - 5, kComponentHeight / 2}));
    EXPECT_EQ(prevCalls, 0);
    EXPECT_EQ(nextCalls, 1);
}

TEST_F(SectionHeaderTest, CenterClickDoesNotFireChevronCallbacks)
{
    SectionHeader header(getThemeManager(), "Section");
    header.setChevronsVisible(true);
    header.setSize(kComponentWidth, kComponentHeight);

    int prevCalls = 0;
    int nextCalls = 0;
    header.onPrev = [&]() { ++prevCalls; };
    header.onNext = [&]() { ++nextCalls; };

    header.mouseDown(makeMouseEvent(header, {kComponentWidth / 2, kComponentHeight / 2}));
    EXPECT_EQ(prevCalls, 0);
    EXPECT_EQ(nextCalls, 0);
}

TEST_F(SectionHeaderTest, ClickSuppressedWhenChevronsHidden)
{
    SectionHeader header(getThemeManager(), "Section");
    // Chevrons default to hidden; be explicit for the invariant under test.
    header.setChevronsVisible(false);
    header.setSize(kComponentWidth, kComponentHeight);

    int prevCalls = 0;
    int nextCalls = 0;
    header.onPrev = [&]() { ++prevCalls; };
    header.onNext = [&]() { ++nextCalls; };

    header.mouseDown(makeMouseEvent(header, {5, kComponentHeight / 2}));
    header.mouseDown(makeMouseEvent(header, {kComponentWidth - 5, kComponentHeight / 2}));
    EXPECT_EQ(prevCalls, 0);
    EXPECT_EQ(nextCalls, 0);
}

TEST_F(SectionHeaderTest, NullCallbacksDoNotCrash)
{
    SectionHeader header(getThemeManager(), "Section");
    header.setChevronsVisible(true);
    header.setSize(kComponentWidth, kComponentHeight);

    header.onPrev = nullptr;
    header.onNext = nullptr;

    header.mouseDown(makeMouseEvent(header, {5, kComponentHeight / 2}));
    header.mouseDown(makeMouseEvent(header, {kComponentWidth - 5, kComponentHeight / 2}));
    // Surviving these calls without crashing is the contract.
    SUCCEED();
    EXPECT_TRUE(header.getTitle() == juce::String("Section"));
}

// =============================================================================
// Accent Colour Contract
// =============================================================================

TEST_F(SectionHeaderTest, AccentColourSetterPersistsValue)
{
    SectionHeader header(getThemeManager(), "Section");

    auto const customAccent = juce::Colour(0xFF3AA0FF);
    header.setAccentColour(customAccent);
    EXPECT_EQ(header.getAccentColour(), customAccent);

    // Setting transparent black must overwrite, not silently ignore — the
    // "transparent means use theme defaults" rule lives in paint(), not in
    // the setter. A no-op setter would hide state drift on theme swap.
    header.setAccentColour(juce::Colours::transparentBlack);
    EXPECT_EQ(header.getAccentColour(), juce::Colours::transparentBlack);

    // Title is unchanged as a side-effect guard.
    EXPECT_EQ(header.getTitle(), juce::String("Section"));
}

// =============================================================================
// Preferred Height Invariant
// =============================================================================

TEST_F(SectionHeaderTest, PreferredHeightMatchesHeader) { EXPECT_EQ(SectionHeader::PREFERRED_HEIGHT, 22); }
