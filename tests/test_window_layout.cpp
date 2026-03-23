#include "ui/layout/WindowLayout.h"

#include <gtest/gtest.h>

using namespace oscil;

class WindowLayoutTest : public ::testing::Test
{
protected:
    WindowLayout layout;
};

// Test: Default values
TEST_F(WindowLayoutTest, DefaultValues)
{
    EXPECT_EQ(layout.getWindowWidth(), WindowLayout::DEFAULT_WINDOW_WIDTH);
    EXPECT_EQ(layout.getWindowHeight(), WindowLayout::DEFAULT_WINDOW_HEIGHT);
    EXPECT_EQ(layout.getSidebarWidth(), WindowLayout::DEFAULT_SIDEBAR_WIDTH);
    EXPECT_FALSE(layout.isSidebarCollapsed());
}

// Test: Set window width with clamping
TEST_F(WindowLayoutTest, SetWindowWidthClamping)
{
    // Below minimum
    layout.setWindowWidth(100);
    EXPECT_EQ(layout.getWindowWidth(), WindowLayout::MIN_WINDOW_WIDTH);

    // Above maximum
    layout.setWindowWidth(10000);
    EXPECT_EQ(layout.getWindowWidth(), WindowLayout::MAX_WINDOW_WIDTH);

    // Valid value
    layout.setWindowWidth(1000);
    EXPECT_EQ(layout.getWindowWidth(), 1000);
}

// Test: Set window height with clamping
TEST_F(WindowLayoutTest, SetWindowHeightClamping)
{
    // Below minimum
    layout.setWindowHeight(100);
    EXPECT_EQ(layout.getWindowHeight(), WindowLayout::MIN_WINDOW_HEIGHT);

    // Above maximum
    layout.setWindowHeight(5000);
    EXPECT_EQ(layout.getWindowHeight(), WindowLayout::MAX_WINDOW_HEIGHT);

    // Valid value
    layout.setWindowHeight(700);
    EXPECT_EQ(layout.getWindowHeight(), 700);
}

// Test: Set window size
TEST_F(WindowLayoutTest, SetWindowSize)
{
    layout.setWindowSize(1024, 768);
    EXPECT_EQ(layout.getWindowWidth(), 1024);
    EXPECT_EQ(layout.getWindowHeight(), 768);
}

// Test: Set sidebar width with clamping
TEST_F(WindowLayoutTest, SetSidebarWidthClamping)
{
    // Below minimum
    layout.setSidebarWidth(50);
    EXPECT_EQ(layout.getSidebarWidth(), WindowLayout::MIN_SIDEBAR_WIDTH);

    // Above maximum
    layout.setSidebarWidth(1000);
    EXPECT_EQ(layout.getSidebarWidth(), WindowLayout::MAX_SIDEBAR_WIDTH);

    // Valid value
    layout.setSidebarWidth(400);
    EXPECT_EQ(layout.getSidebarWidth(), 400);
}

// Test: Sidebar collapse state
TEST_F(WindowLayoutTest, SidebarCollapseState)
{
    EXPECT_FALSE(layout.isSidebarCollapsed());

    layout.setSidebarCollapsed(true);
    EXPECT_TRUE(layout.isSidebarCollapsed());

    layout.setSidebarCollapsed(false);
    EXPECT_FALSE(layout.isSidebarCollapsed());
}

// Test: Toggle sidebar collapsed
TEST_F(WindowLayoutTest, ToggleSidebarCollapsed)
{
    EXPECT_FALSE(layout.isSidebarCollapsed());

    layout.toggleSidebarCollapsed();
    EXPECT_TRUE(layout.isSidebarCollapsed());

    layout.toggleSidebarCollapsed();
    EXPECT_FALSE(layout.isSidebarCollapsed());
}

// Test: Effective sidebar width
TEST_F(WindowLayoutTest, EffectiveSidebarWidth)
{
    layout.setSidebarWidth(350);

    // Not collapsed - returns actual width
    EXPECT_EQ(layout.getEffectiveSidebarWidth(), 350);

    // Collapsed - returns collapsed width
    layout.setSidebarCollapsed(true);
    EXPECT_EQ(layout.getEffectiveSidebarWidth(), WindowLayout::COLLAPSED_SIDEBAR_WIDTH);
}

// Test: Oscilloscope area width
TEST_F(WindowLayoutTest, OscilloscopeAreaWidth)
{
    layout.setWindowWidth(1200);
    layout.setSidebarWidth(300);

    // Not collapsed
    EXPECT_EQ(layout.getOscilloscopeAreaWidth(), 1200 - 300);

    // Collapsed
    layout.setSidebarCollapsed(true);
    EXPECT_EQ(layout.getOscilloscopeAreaWidth(), 1200 - WindowLayout::COLLAPSED_SIDEBAR_WIDTH);
}

// Test: Clamp sidebar width respects oscilloscope minimum
TEST_F(WindowLayoutTest, ClampSidebarWidthRespectsOscilloscopeMinimum)
{
    layout.setWindowWidth(800);

    // Sidebar too wide would make oscilloscope area too small
    int maxAllowedSidebar = 800 - WindowLayout::MIN_OSCILLOSCOPE_WIDTH;
    int clamped = layout.clampSidebarWidth(700);

    EXPECT_LE(clamped, maxAllowedSidebar);
    EXPECT_GE(clamped, WindowLayout::MIN_SIDEBAR_WIDTH);
}

// Test: Valid window size check
TEST_F(WindowLayoutTest, IsValidWindowSize)
{
    EXPECT_TRUE(layout.isValidWindowSize(1024, 768));
    EXPECT_TRUE(layout.isValidWindowSize(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_WINDOW_HEIGHT));
    EXPECT_TRUE(layout.isValidWindowSize(WindowLayout::MAX_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_HEIGHT));

    // Invalid sizes
    EXPECT_FALSE(layout.isValidWindowSize(100, 768));   // Width too small
    EXPECT_FALSE(layout.isValidWindowSize(1024, 100));  // Height too small
    EXPECT_FALSE(layout.isValidWindowSize(5000, 768));  // Width too large
    EXPECT_FALSE(layout.isValidWindowSize(1024, 5000)); // Height too large
}

TEST_F(WindowLayoutTest, SerializationRoundTrip)
{
    layout.setWindowWidth(1400);
    layout.setWindowHeight(900);
    layout.setSidebarWidth(350);
    layout.setSidebarCollapsed(true);

    auto tree = layout.toValueTree();

    WindowLayout restored(tree);

    EXPECT_EQ(restored.getWindowWidth(), 1400);
    EXPECT_EQ(restored.getWindowHeight(), 900);
    EXPECT_EQ(restored.getSidebarWidth(), 350);
    EXPECT_TRUE(restored.isSidebarCollapsed());
}

TEST_F(WindowLayoutTest, DeserializeInvalidValueTree)
{
    WindowLayout restored;

    // Wrong type
    juce::ValueTree wrongType("WrongType");
    restored.fromValueTree(wrongType);

    // Should retain defaults
    EXPECT_EQ(restored.getWindowWidth(), WindowLayout::DEFAULT_WINDOW_WIDTH);
}

TEST_F(WindowLayoutTest, DeserializeEmptyValueTree)
{
    WindowLayout restored;

    juce::ValueTree empty;
    restored.fromValueTree(empty);

    // Should retain defaults
    EXPECT_EQ(restored.getWindowWidth(), WindowLayout::DEFAULT_WINDOW_WIDTH);
}

TEST_F(WindowLayoutTest, DeserializeWithOutOfRangeValues)
{
    juce::ValueTree tree(WindowLayoutIds::WindowLayout);
    tree.setProperty(WindowLayoutIds::WindowWidth, 50, nullptr);  // Too small
    tree.setProperty(WindowLayoutIds::WindowHeight, 50, nullptr); // Too small
    tree.setProperty(WindowLayoutIds::SidebarWidth, 50, nullptr); // Too small

    WindowLayout restored(tree);

    // Should be clamped
    EXPECT_EQ(restored.getWindowWidth(), WindowLayout::MIN_WINDOW_WIDTH);
    EXPECT_EQ(restored.getWindowHeight(), WindowLayout::MIN_WINDOW_HEIGHT);
    EXPECT_EQ(restored.getSidebarWidth(), WindowLayout::MIN_SIDEBAR_WIDTH);
}

TEST_F(WindowLayoutTest, DeserializeClampsSidebarAgainstWindowWidthConstraint)
{
    juce::ValueTree tree(WindowLayoutIds::WindowLayout);
    tree.setProperty(WindowLayoutIds::WindowWidth, WindowLayout::MIN_WINDOW_WIDTH, nullptr);
    tree.setProperty(WindowLayoutIds::WindowHeight, WindowLayout::MIN_WINDOW_HEIGHT, nullptr);
    tree.setProperty(WindowLayoutIds::SidebarWidth, WindowLayout::MAX_SIDEBAR_WIDTH, nullptr);

    WindowLayout restored(tree);

    const int maxAllowedSidebar = WindowLayout::MIN_WINDOW_WIDTH - WindowLayout::MIN_OSCILLOSCOPE_WIDTH;

    EXPECT_LE(restored.getSidebarWidth(), maxAllowedSidebar);
    EXPECT_GE(restored.getOscilloscopeAreaWidth(), WindowLayout::MIN_OSCILLOSCOPE_WIDTH);
}

class TestWindowLayoutListener : public WindowLayout::Listener
{
public:
    int windowSizeChangedCount = 0;
    int sidebarWidthChangedCount = 0;
    int sidebarCollapseChangedCount = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    int lastSidebarWidth = 0;
    bool lastCollapseState = false;

    void windowSizeChanged(int width, int height) override
    {
        ++windowSizeChangedCount;
        lastWidth = width;
        lastHeight = height;
    }

    void sidebarWidthChanged(int width) override
    {
        ++sidebarWidthChangedCount;
        lastSidebarWidth = width;
    }

    void sidebarCollapseStateChanged(bool collapsed) override
    {
        ++sidebarCollapseChangedCount;
        lastCollapseState = collapsed;
    }
};

TEST_F(WindowLayoutTest, ListenerWindowSizeChanged)
{
    TestWindowLayoutListener listener;
    layout.addListener(&listener);

    layout.setWindowWidth(1500);
    EXPECT_EQ(listener.windowSizeChangedCount, 1);
    EXPECT_EQ(listener.lastWidth, 1500);

    layout.setWindowHeight(900);
    EXPECT_EQ(listener.windowSizeChangedCount, 2);
    EXPECT_EQ(listener.lastHeight, 900);

    layout.removeListener(&listener);
}

TEST_F(WindowLayoutTest, ListenerSidebarWidthChanged)
{
    TestWindowLayoutListener listener;
    layout.addListener(&listener);

    layout.setSidebarWidth(400);
    EXPECT_EQ(listener.sidebarWidthChangedCount, 1);
    EXPECT_EQ(listener.lastSidebarWidth, 400);

    layout.removeListener(&listener);
}

TEST_F(WindowLayoutTest, ListenerSidebarCollapseChanged)
{
    TestWindowLayoutListener listener;
    layout.addListener(&listener);

    layout.setSidebarCollapsed(true);
    EXPECT_EQ(listener.sidebarCollapseChangedCount, 1);
    EXPECT_TRUE(listener.lastCollapseState);

    layout.setSidebarCollapsed(false);
    EXPECT_EQ(listener.sidebarCollapseChangedCount, 2);
    EXPECT_FALSE(listener.lastCollapseState);

    layout.removeListener(&listener);
}

TEST_F(WindowLayoutTest, ListenerNotCalledWhenValueUnchanged)
{
    TestWindowLayoutListener listener;
    layout.addListener(&listener);

    int initialWidth = layout.getWindowWidth();
    layout.setWindowWidth(initialWidth); // Same value

    // Should not be called
    EXPECT_EQ(listener.windowSizeChangedCount, 0);

    layout.removeListener(&listener);
}

TEST_F(WindowLayoutTest, RemoveListenerStopsNotifications)
{
    TestWindowLayoutListener listener;
    layout.addListener(&listener);
    layout.removeListener(&listener);

    layout.setWindowWidth(1500);
    EXPECT_EQ(listener.windowSizeChangedCount, 0);
}

TEST_F(WindowLayoutTest, SidebarWidthAdjustsWhenWindowShrinks)
{
    layout.setWindowWidth(1200);
    layout.setSidebarWidth(700);

    // Now shrink window - sidebar should be clamped
    layout.setWindowWidth(900);

    // Sidebar width should now be valid for the new window size
    int maxAllowedSidebar = 900 - WindowLayout::MIN_OSCILLOSCOPE_WIDTH;
    EXPECT_LE(layout.getSidebarWidth(), maxAllowedSidebar);
}

TEST_F(WindowLayoutTest, SetWindowSizeAdjustsSidebarIfNeeded)
{
    layout.setSidebarWidth(600);

    // Set window size that would make sidebar too wide
    layout.setWindowSize(900, 700);

    int maxAllowedSidebar = 900 - WindowLayout::MIN_OSCILLOSCOPE_WIDTH;
    EXPECT_LE(layout.getSidebarWidth(), maxAllowedSidebar);
}

TEST_F(WindowLayoutTest, ConstantsAreConsistent)
{
    // Verify constraints are consistent
    EXPECT_LT(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MAX_WINDOW_WIDTH);
    EXPECT_LT(WindowLayout::MIN_WINDOW_HEIGHT, WindowLayout::MAX_WINDOW_HEIGHT);
    EXPECT_LT(WindowLayout::MIN_SIDEBAR_WIDTH, WindowLayout::MAX_SIDEBAR_WIDTH);
    EXPECT_LT(WindowLayout::COLLAPSED_SIDEBAR_WIDTH, WindowLayout::MIN_SIDEBAR_WIDTH);

    // Minimum window width should accommodate minimum sidebar + minimum oscilloscope
    EXPECT_GE(WindowLayout::MIN_WINDOW_WIDTH, WindowLayout::MIN_SIDEBAR_WIDTH + WindowLayout::MIN_OSCILLOSCOPE_WIDTH);
}

class SidebarResizeStateTest : public ::testing::Test
{
protected:
    SidebarResizeState resizeState;
};

TEST_F(SidebarResizeStateTest, InitialState)
{
    EXPECT_FALSE(resizeState.isResizing);
    EXPECT_EQ(resizeState.dragStartX, 0);
    EXPECT_EQ(resizeState.originalSidebarWidth, 0);
}

TEST_F(SidebarResizeStateTest, StartResize)
{
    resizeState.startResize(500, 300);

    EXPECT_TRUE(resizeState.isResizing);
    EXPECT_EQ(resizeState.dragStartX, 500);
    EXPECT_EQ(resizeState.originalSidebarWidth, 300);
    EXPECT_EQ(resizeState.currentSidebarWidth, 300);
    EXPECT_EQ(resizeState.previewWidth, 300);
}

TEST_F(SidebarResizeStateTest, UpdateResizeIncreasesWidth)
{
    resizeState.startResize(500, 300);

    // Move mouse left (sidebar is on right, so left = increase width)
    resizeState.updateResize(450, 1200);

    // delta = 500 - 450 = 50, so preview = 300 + 50 = 350
    EXPECT_EQ(resizeState.previewWidth, 350);
}

TEST_F(SidebarResizeStateTest, UpdateResizeDecreasesWidth)
{
    resizeState.startResize(500, 300);

    // Move mouse right (sidebar is on right, so right = decrease width)
    resizeState.updateResize(550, 1200);

    // delta = 500 - 550 = -50, so preview = 300 - 50 = 250
    EXPECT_EQ(resizeState.previewWidth, 250);
}

TEST_F(SidebarResizeStateTest, UpdateResizeClampsToMinimum)
{
    resizeState.startResize(500, 250);

    // Move mouse far right
    resizeState.updateResize(700, 1200);

    // Should be clamped to minimum
    EXPECT_EQ(resizeState.previewWidth, WindowLayout::MIN_SIDEBAR_WIDTH);
}

TEST_F(SidebarResizeStateTest, UpdateResizeClampsToMaximum)
{
    resizeState.startResize(500, 400);

    // Move mouse far left
    resizeState.updateResize(0, 1200);

    // Should be clamped to maximum (considering oscilloscope minimum)
    int expectedMax = juce::jmin(WindowLayout::MAX_SIDEBAR_WIDTH, 1200 - WindowLayout::MIN_OSCILLOSCOPE_WIDTH);
    EXPECT_EQ(resizeState.previewWidth, expectedMax);
}

TEST_F(SidebarResizeStateTest, UpdateResizeNotResizing)
{
    // Not started - updateResize should do nothing
    resizeState.updateResize(450, 1200);

    EXPECT_FALSE(resizeState.isResizing);
    EXPECT_EQ(resizeState.previewWidth, 0);
}

TEST_F(SidebarResizeStateTest, EndResize)
{
    resizeState.startResize(500, 300);
    resizeState.updateResize(400, 1200); // Preview = 400

    resizeState.endResize();

    EXPECT_FALSE(resizeState.isResizing);
    EXPECT_EQ(resizeState.currentSidebarWidth, resizeState.previewWidth);
}

TEST_F(SidebarResizeStateTest, CancelResize)
{
    resizeState.startResize(500, 300);
    resizeState.updateResize(400, 1200); // Preview = 400

    resizeState.cancelResize();

    EXPECT_FALSE(resizeState.isResizing);
    EXPECT_EQ(resizeState.previewWidth, 300); // Restored to original
}

TEST_F(SidebarResizeStateTest, EndResizeWhenNotResizing)
{
    // Should not crash
    resizeState.endResize();
    EXPECT_FALSE(resizeState.isResizing);
}

TEST_F(SidebarResizeStateTest, MultipleResizeCycles)
{
    // First resize
    resizeState.startResize(500, 300);
    resizeState.updateResize(450, 1200);
    resizeState.endResize();
    EXPECT_EQ(resizeState.currentSidebarWidth, 350);

    // Second resize
    resizeState.startResize(600, 350);
    resizeState.updateResize(550, 1200);
    resizeState.endResize();
    EXPECT_EQ(resizeState.currentSidebarWidth, 400);
}
