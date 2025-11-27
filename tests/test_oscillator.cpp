/*
    Oscil - Oscillator Model Tests
*/

#include <gtest/gtest.h>
#include "core/Oscillator.h"
#include "core/Pane.h"
#include "core/OscilState.h"

using namespace oscil;

class OscillatorTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Oscillator ID generation
TEST_F(OscillatorTest, IdGeneration)
{
    Oscillator osc1;
    Oscillator osc2;

    EXPECT_TRUE(osc1.getId().isValid());
    EXPECT_TRUE(osc2.getId().isValid());
    EXPECT_NE(osc1.getId(), osc2.getId());
}

// Test: Default values
TEST_F(OscillatorTest, DefaultValues)
{
    Oscillator osc;

    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::FullStereo);
    EXPECT_TRUE(osc.isVisible());
    EXPECT_EQ(osc.getOpacity(), 1.0f);
    EXPECT_EQ(osc.getOrderIndex(), 0);
}

// Test: Setters and getters
TEST_F(OscillatorTest, SettersAndGetters)
{
    Oscillator osc;

    SourceId sourceId = SourceId::generate();
    osc.setSourceId(sourceId);
    EXPECT_EQ(osc.getSourceId(), sourceId);

    osc.setProcessingMode(ProcessingMode::Mid);
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mid);

    osc.setColour(juce::Colours::red);
    EXPECT_EQ(osc.getColour(), juce::Colours::red);

    osc.setOpacity(0.5f);
    EXPECT_EQ(osc.getOpacity(), 0.5f);

    osc.setVisible(false);
    EXPECT_FALSE(osc.isVisible());

    osc.setName("Test Oscillator");
    EXPECT_EQ(osc.getName(), "Test Oscillator");

    PaneId paneId = PaneId::generate();
    osc.setPaneId(paneId);
    EXPECT_EQ(osc.getPaneId(), paneId);

    osc.setOrderIndex(5);
    EXPECT_EQ(osc.getOrderIndex(), 5);
}

// Test: Opacity clamping
TEST_F(OscillatorTest, OpacityClamping)
{
    Oscillator osc;

    osc.setOpacity(-0.5f);
    EXPECT_EQ(osc.getOpacity(), 0.0f);

    osc.setOpacity(1.5f);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
}

// Test: Effective colour (with opacity)
TEST_F(OscillatorTest, EffectiveColour)
{
    Oscillator osc;
    osc.setColour(juce::Colour(0xFF00FF00));
    osc.setOpacity(0.5f);

    auto effective = osc.getEffectiveColour();
    EXPECT_EQ(effective.getRed(), 0);
    EXPECT_EQ(effective.getGreen(), 255);
    EXPECT_EQ(effective.getBlue(), 0);
    EXPECT_NEAR(effective.getFloatAlpha(), 0.5f, 0.01f);
}

// Test: Single trace detection
TEST_F(OscillatorTest, SingleTraceDetection)
{
    Oscillator osc;

    osc.setProcessingMode(ProcessingMode::FullStereo);
    EXPECT_FALSE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Mono);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Mid);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Side);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Left);
    EXPECT_TRUE(osc.isSingleTrace());

    osc.setProcessingMode(ProcessingMode::Right);
    EXPECT_TRUE(osc.isSingleTrace());
}

// Test: Serialization round-trip
TEST_F(OscillatorTest, SerializationRoundTrip)
{
    Oscillator original;
    original.setSourceId(SourceId::generate());
    original.setProcessingMode(ProcessingMode::Side);
    original.setColour(juce::Colour(0xFF123456));
    original.setOpacity(0.75f);
    original.setPaneId(PaneId::generate());
    original.setOrderIndex(3);
    original.setVisible(false);
    original.setName("Test");

    auto valueTree = original.toValueTree();

    Oscillator restored(valueTree);

    EXPECT_EQ(restored.getSourceId(), original.getSourceId());
    EXPECT_EQ(restored.getProcessingMode(), original.getProcessingMode());
    EXPECT_EQ(restored.getColour().getARGB(), original.getColour().getARGB());
    EXPECT_EQ(restored.getOpacity(), original.getOpacity());
    EXPECT_EQ(restored.getPaneId(), original.getPaneId());
    EXPECT_EQ(restored.getOrderIndex(), original.getOrderIndex());
    EXPECT_EQ(restored.isVisible(), original.isVisible());
    EXPECT_EQ(restored.getName(), original.getName());
}

// Test: Processing mode string conversion
TEST_F(OscillatorTest, ProcessingModeStringConversion)
{
    EXPECT_EQ(processingModeToString(ProcessingMode::FullStereo), "FullStereo");
    EXPECT_EQ(processingModeToString(ProcessingMode::Mono), "Mono");
    EXPECT_EQ(processingModeToString(ProcessingMode::Mid), "Mid");
    EXPECT_EQ(processingModeToString(ProcessingMode::Side), "Side");
    EXPECT_EQ(processingModeToString(ProcessingMode::Left), "Left");
    EXPECT_EQ(processingModeToString(ProcessingMode::Right), "Right");

    EXPECT_EQ(stringToProcessingMode("FullStereo"), ProcessingMode::FullStereo);
    EXPECT_EQ(stringToProcessingMode("Mono"), ProcessingMode::Mono);
    EXPECT_EQ(stringToProcessingMode("Mid"), ProcessingMode::Mid);
    EXPECT_EQ(stringToProcessingMode("Side"), ProcessingMode::Side);
    EXPECT_EQ(stringToProcessingMode("Left"), ProcessingMode::Left);
    EXPECT_EQ(stringToProcessingMode("Right"), ProcessingMode::Right);

    // Unknown string should default to FullStereo
    EXPECT_EQ(stringToProcessingMode("Unknown"), ProcessingMode::FullStereo);
}

// Pane Tests

class PaneTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Pane ID generation
TEST_F(PaneTest, IdGeneration)
{
    Pane pane1;
    Pane pane2;

    EXPECT_TRUE(pane1.getId().isValid());
    EXPECT_TRUE(pane2.getId().isValid());
    EXPECT_NE(pane1.getId(), pane2.getId());
}

// Test: Pane default values
TEST_F(PaneTest, DefaultValues)
{
    Pane pane;

    EXPECT_FALSE(pane.isCollapsed());
    EXPECT_EQ(pane.getOrderIndex(), 0);
}

// Test: Pane serialization
TEST_F(PaneTest, SerializationRoundTrip)
{
    Pane original;
    original.setOrderIndex(5);
    original.setCollapsed(true);
    original.setName("Test Pane");

    auto valueTree = original.toValueTree();

    Pane restored(valueTree);

    EXPECT_EQ(restored.getOrderIndex(), original.getOrderIndex());
    EXPECT_EQ(restored.isCollapsed(), original.isCollapsed());
    EXPECT_EQ(restored.getName(), original.getName());
}

// PaneLayoutManager Tests

class PaneLayoutManagerTest : public ::testing::Test
{
protected:
    PaneLayoutManager manager;

    void SetUp() override {}
};

// Test: Add and remove panes
TEST_F(PaneLayoutManagerTest, AddRemovePanes)
{
    EXPECT_EQ(manager.getPaneCount(), 0);

    Pane pane1;
    pane1.setOrderIndex(0);
    manager.addPane(pane1);
    EXPECT_EQ(manager.getPaneCount(), 1);

    Pane pane2;
    pane2.setOrderIndex(1);
    manager.addPane(pane2);
    EXPECT_EQ(manager.getPaneCount(), 2);

    manager.removePane(pane1.getId());
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Column layout
TEST_F(PaneLayoutManagerTest, ColumnLayout)
{
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Single);
    EXPECT_EQ(manager.getColumnCount(), 1);

    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnCount(), 2);

    manager.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnCount(), 3);
}

// Test: Pane column assignment (row-major)
TEST_F(PaneLayoutManagerTest, PaneColumnAssignment)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Row-major: 0->col0, 1->col1, 2->col2, 3->col0, etc.
    EXPECT_EQ(manager.getColumnForPane(0), 0);
    EXPECT_EQ(manager.getColumnForPane(1), 1);
    EXPECT_EQ(manager.getColumnForPane(2), 2);
    EXPECT_EQ(manager.getColumnForPane(3), 0);
    EXPECT_EQ(manager.getColumnForPane(4), 1);
    EXPECT_EQ(manager.getColumnForPane(5), 2);
}

// Test: Pane bounds calculation
TEST_F(PaneLayoutManagerTest, PaneBoundsCalculation)
{
    manager.setColumnLayout(ColumnLayout::Double);

    for (int i = 0; i < 4; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    juce::Rectangle<int> availableArea(0, 0, 800, 600);

    // 4 panes in 2 columns = 2 rows
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);
    auto bounds2 = manager.getPaneBounds(2, availableArea);
    auto bounds3 = manager.getPaneBounds(3, availableArea);

    // First row: panes 0 and 1
    EXPECT_LT(bounds0.getX(), bounds1.getX());
    EXPECT_EQ(bounds0.getY(), bounds1.getY());

    // Second row: panes 2 and 3
    EXPECT_LT(bounds2.getX(), bounds3.getX());
    EXPECT_EQ(bounds2.getY(), bounds3.getY());

    // Row separation
    EXPECT_GT(bounds2.getY(), bounds0.getY());
}

// Test: Move pane
TEST_F(PaneLayoutManagerTest, MovePane)
{
    Pane pane0, pane1, pane2;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    pane2.setOrderIndex(2);
    pane2.setName("C");

    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);

    // Move first pane to end
    manager.movePane(pane0.getId(), 2);

    const auto& panes = manager.getPanes();
    EXPECT_EQ(panes[0].getName(), "B");
    EXPECT_EQ(panes[1].getName(), "C");
    EXPECT_EQ(panes[2].getName(), "A");
}

// Test: Single column pane width - should use full available width
TEST_F(PaneLayoutManagerTest, SingleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Single);

    Pane pane;
    pane.setOrderIndex(0);
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds = manager.getPaneBounds(0, availableArea);

    // In single column, pane width should be close to full width (minus 2*margin=4)
    const int margin = 2;
    int expectedWidth = availableArea.getWidth() - 2 * margin;
    EXPECT_EQ(bounds.getWidth(), expectedWidth);
    EXPECT_EQ(bounds.getX(), margin);
}

// Test: Double column pane width - each pane should be half width
TEST_F(PaneLayoutManagerTest, DoubleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Double);

    // Add 2 panes (one per column after redistribution)
    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane1.setOrderIndex(1);
    manager.addPane(pane0);
    manager.addPane(pane1);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);

    // In double column, each pane width should be half (minus margins)
    const int margin = 2;
    int columnWidth = availableArea.getWidth() / 2;
    int expectedPaneWidth = columnWidth - 2 * margin;

    EXPECT_EQ(bounds0.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds1.getWidth(), expectedPaneWidth);

    // First pane in first column, second pane in second column
    EXPECT_EQ(bounds0.getX(), margin);
    EXPECT_EQ(bounds1.getX(), columnWidth + margin);
}

// Test: Triple column pane width - each pane should be one-third width
TEST_F(PaneLayoutManagerTest, TripleColumnPaneWidth)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Add 3 panes (one per column after redistribution)
    Pane pane0, pane1, pane2;
    pane0.setOrderIndex(0);
    pane1.setOrderIndex(1);
    pane2.setOrderIndex(2);
    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);

    juce::Rectangle<int> availableArea(0, 0, 900, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);
    auto bounds2 = manager.getPaneBounds(2, availableArea);

    // In triple column, each pane width should be one-third (minus margins)
    const int margin = 2;
    int columnWidth = availableArea.getWidth() / 3;  // 300 pixels each
    int expectedPaneWidth = columnWidth - 2 * margin;

    EXPECT_EQ(bounds0.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds1.getWidth(), expectedPaneWidth);
    EXPECT_EQ(bounds2.getWidth(), expectedPaneWidth);

    // Each pane should start at correct column position
    EXPECT_EQ(bounds0.getX(), margin);
    EXPECT_EQ(bounds1.getX(), columnWidth + margin);
    EXPECT_EQ(bounds2.getX(), 2 * columnWidth + margin);
}

// Test: Move pane between columns
TEST_F(PaneLayoutManagerTest, MovePaneBetweenColumns)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    manager.addPane(pane0);
    manager.addPane(pane1);

    // Initially pane0 should be in column 0, pane1 in column 1
    auto* p0 = manager.getPane(pane0.getId());
    auto* p1 = manager.getPane(pane1.getId());
    EXPECT_EQ(p0->getColumnIndex(), 0);
    EXPECT_EQ(p1->getColumnIndex(), 1);

    // Move pane0 to column 1
    manager.movePaneToColumn(pane0.getId(), 1, 0);

    // Now pane0 should be in column 1
    EXPECT_EQ(p0->getColumnIndex(), 1);
}

// Test: Redistribute panes when column layout changes
TEST_F(PaneLayoutManagerTest, RedistributePanesOnLayoutChange)
{
    // Start with 4 panes in single column
    manager.setColumnLayout(ColumnLayout::Single);

    for (int i = 0; i < 4; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    // All panes should be in column 0
    for (const auto& pane : manager.getPanes())
    {
        EXPECT_EQ(pane.getColumnIndex(), 0);
    }

    // Change to double column
    manager.setColumnLayout(ColumnLayout::Double);

    // Panes should be redistributed round-robin: 0,2 in col0, 1,3 in col1
    const auto& panes = manager.getPanes();
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 0);
    EXPECT_EQ(panes[3].getColumnIndex(), 1);

    // Change to triple column
    manager.setColumnLayout(ColumnLayout::Triple);

    // Panes should be redistributed: 0,3 in col0, 1 in col1, 2 in col2
    EXPECT_EQ(panes[0].getColumnIndex(), 0);
    EXPECT_EQ(panes[1].getColumnIndex(), 1);
    EXPECT_EQ(panes[2].getColumnIndex(), 2);
    EXPECT_EQ(panes[3].getColumnIndex(), 0);
}

// Test: Pane count per column
TEST_F(PaneLayoutManagerTest, PaneCountPerColumn)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // Add 5 panes
    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    // Distribution: col0=2, col1=2, col2=1 (round-robin: 0->col0, 1->col1, 2->col2, 3->col0, 4->col1)
    EXPECT_EQ(manager.getPaneCountInColumn(0), 2);
    EXPECT_EQ(manager.getPaneCountInColumn(1), 2);
    EXPECT_EQ(manager.getPaneCountInColumn(2), 1);
}

// Test: Get panes in specific column
TEST_F(PaneLayoutManagerTest, GetPanesInColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane0, pane1, pane2, pane3;
    pane0.setOrderIndex(0);
    pane0.setName("A");
    pane1.setOrderIndex(1);
    pane1.setName("B");
    pane2.setOrderIndex(2);
    pane2.setName("C");
    pane3.setOrderIndex(3);
    pane3.setName("D");

    manager.addPane(pane0);
    manager.addPane(pane1);
    manager.addPane(pane2);
    manager.addPane(pane3);

    // Column 0: A, C (indices 0, 2)
    // Column 1: B, D (indices 1, 3)
    auto col0Panes = manager.getPanesInColumn(0);
    auto col1Panes = manager.getPanesInColumn(1);

    ASSERT_EQ(col0Panes.size(), 2);
    ASSERT_EQ(col1Panes.size(), 2);

    EXPECT_EQ(col0Panes[0]->getName(), "A");
    EXPECT_EQ(col0Panes[1]->getName(), "C");
    EXPECT_EQ(col1Panes[0]->getName(), "B");
    EXPECT_EQ(col1Panes[1]->getName(), "D");
}

// Test: Pane height distribution within a column
TEST_F(PaneLayoutManagerTest, PaneHeightDistribution)
{
    manager.setColumnLayout(ColumnLayout::Single);

    // Add 2 panes with equal height ratio
    Pane pane0, pane1;
    pane0.setOrderIndex(0);
    pane0.setHeightRatio(0.5f);
    pane1.setOrderIndex(1);
    pane1.setHeightRatio(0.5f);
    manager.addPane(pane0);
    manager.addPane(pane1);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds0 = manager.getPaneBounds(0, availableArea);
    auto bounds1 = manager.getPaneBounds(1, availableArea);

    // Each pane should get approximately half the height
    // Note: there are margins, so heights won't be exactly 300
    EXPECT_GT(bounds0.getHeight(), 250);
    EXPECT_GT(bounds1.getHeight(), 250);

    // Pane1 should be below pane0
    EXPECT_GT(bounds1.getY(), bounds0.getY());
}
