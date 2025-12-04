/*
    Oscil - Plugin Editor Tests
    Tests for the main plugin editor and its refactored components
*/

#include <gtest/gtest.h>
#include "OscilTestFixtures.h"

using namespace oscil;
using namespace oscil::test;

/**
 * Plugin Editor tests using the standardized test fixture.
 * Uses real singletons because many UI components bypass DI.
 */
class PluginEditorTest : public OscilPluginTestFixture
{
};

TEST_F(PluginEditorTest, Construction)
{
    createEditor();
    EXPECT_NE(editor, nullptr);
}

TEST_F(PluginEditorTest, InitialLayout)
{
    createEditor();
    editor->setSize(800, 600);
    editor->resized();

    // Check if pane components are created
    const auto& panes = editor->getPaneComponents();
    // Verify we can call the method without crashing
    EXPECT_NO_THROW(editor->getPaneComponents());
}

TEST_F(PluginEditorTest, GpuRenderingToggle)
{
    createEditor();

    // Toggle GPU rendering - should not crash
    editor->setGpuRenderingEnabled(true);
    pumpMessageQueue(50);

    editor->setGpuRenderingEnabled(false);
    pumpMessageQueue(50);
}

TEST_F(PluginEditorTest, ResizeHandling)
{
    createEditor();

    // Test various sizes
    editor->setSize(1200, 800);
    pumpMessageQueue(20);
    EXPECT_EQ(editor->getWidth(), 1200);
    EXPECT_EQ(editor->getHeight(), 800);

    editor->setSize(600, 400);
    pumpMessageQueue(20);
    EXPECT_EQ(editor->getWidth(), 600);
    EXPECT_EQ(editor->getHeight(), 400);
}

TEST_F(PluginEditorTest, SidebarToggle)
{
    createEditor();

    // Toggle sidebar should not crash
    EXPECT_NO_THROW(editor->toggleSidebar());
    pumpMessageQueue(50);

    EXPECT_NO_THROW(editor->toggleSidebar());
    pumpMessageQueue(50);
}

TEST_F(PluginEditorTest, DisplayOptionsDoNotCrash)
{
    createEditor();

    // All display options should be callable without crashing
    EXPECT_NO_THROW(editor->setShowGridForAllPanes(true));
    EXPECT_NO_THROW(editor->setAutoScaleForAllPanes(true));
    EXPECT_NO_THROW(editor->setHoldDisplayForAllPanes(true));
    EXPECT_NO_THROW(editor->setGainDbForAllPanes(6.0f));
    EXPECT_NO_THROW(editor->setDisplaySamplesForAllPanes(2048));

    pumpMessageQueue(50);
}

TEST_F(PluginEditorTest, RefreshPanelsDoesNotCrash)
{
    createEditor();

    // Multiple refreshes should be stable
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_NO_THROW(editor->refreshPanels());
        pumpMessageQueue(50);
    }
}
