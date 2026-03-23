/*
    Oscil - Pane Header Tests
*/

#include "ui/layout/pane/PaneHeader.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

namespace
{
juce::MouseEvent makeMouseEvent(juce::Component& component, juce::Point<int> position,
                                juce::Point<int> mouseDownPosition, bool mouseWasDragged)
{
    auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position.toFloat(),
                            juce::ModifierKeys::leftButtonModifier, juce::MouseInputSource::defaultPressure,
                            juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                            juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &component,
                            &component, now, mouseDownPosition.toFloat(), now, 1, mouseWasDragged);
}
} // namespace

class PaneHeaderTest : public ::testing::Test
{
protected:
    ThemeManager themeManager;
    PaneHeader header{themeManager};

    void SetUp() override { header.setBounds(0, 0, 320, PaneHeader::HEIGHT); }
};

TEST_F(PaneHeaderTest, DragStartRequiresThresholdAndFiresOncePerGesture)
{
    int dragStartCalls = 0;
    header.onDragStarted = [&](const juce::MouseEvent&) { dragStartCalls++; };

    const juce::Point<int> downPos{4, PaneHeader::HEIGHT / 2};
    header.mouseDown(makeMouseEvent(header, downPos, downPos, false));

    // Small movement should not trigger drag start.
    header.mouseDrag(makeMouseEvent(header, {6, PaneHeader::HEIGHT / 2}, downPos, true));
    EXPECT_EQ(dragStartCalls, 0);

    // Movement beyond threshold should trigger once.
    header.mouseDrag(makeMouseEvent(header, {14, PaneHeader::HEIGHT / 2}, downPos, true));
    EXPECT_EQ(dragStartCalls, 1);

    // Continued drag in same gesture should not re-trigger.
    header.mouseDrag(makeMouseEvent(header, {18, PaneHeader::HEIGHT / 2}, downPos, true));
    EXPECT_EQ(dragStartCalls, 1);
}

TEST_F(PaneHeaderTest, DragDoesNotStartOutsideDragZone)
{
    int dragStartCalls = 0;
    header.onDragStarted = [&](const juce::MouseEvent&) { dragStartCalls++; };

    const juce::Point<int> downPos{40, PaneHeader::HEIGHT / 2};
    header.mouseDown(makeMouseEvent(header, downPos, downPos, false));
    header.mouseDrag(makeMouseEvent(header, {120, PaneHeader::HEIGHT / 2}, downPos, true));

    EXPECT_EQ(dragStartCalls, 0);
}
