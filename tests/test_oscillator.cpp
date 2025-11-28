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

// ============================================================================
// Oscillator Edge Case Tests
// ============================================================================

// Test: Name length edge cases
TEST_F(OscillatorTest, NameMaxLength)
{
    Oscillator osc;

    // Create a name longer than MAX_NAME_LENGTH (32)
    juce::String longName;
    for (int i = 0; i < 50; ++i)
        longName += "X";

    osc.setName(longName);
    EXPECT_EQ(osc.getName().length(), Oscillator::MAX_NAME_LENGTH);
}

TEST_F(OscillatorTest, NameMinLength)
{
    Oscillator osc;
    osc.setName("A");
    EXPECT_EQ(osc.getName(), "A");
    EXPECT_TRUE(Oscillator::isValidName("A"));
}

TEST_F(OscillatorTest, NameEmptyIsInvalid)
{
    EXPECT_FALSE(Oscillator::isValidName(""));
}

TEST_F(OscillatorTest, NameExactlyMaxLength)
{
    juce::String exactName;
    for (int i = 0; i < Oscillator::MAX_NAME_LENGTH; ++i)
        exactName += "X";

    EXPECT_TRUE(Oscillator::isValidName(exactName));

    Oscillator osc;
    osc.setName(exactName);
    EXPECT_EQ(osc.getName().length(), Oscillator::MAX_NAME_LENGTH);
}

// Test: Line width clamping
TEST_F(OscillatorTest, LineWidthClampingLow)
{
    Oscillator osc;
    osc.setLineWidth(0.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);
}

TEST_F(OscillatorTest, LineWidthClampingHigh)
{
    Oscillator osc;
    osc.setLineWidth(100.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
}

TEST_F(OscillatorTest, LineWidthNegative)
{
    Oscillator osc;
    osc.setLineWidth(-5.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);
}

TEST_F(OscillatorTest, LineWidthDefault)
{
    Oscillator osc;
    EXPECT_EQ(osc.getLineWidth(), Oscillator::DEFAULT_LINE_WIDTH);
}

// Test: Vertical scale clamping
TEST_F(OscillatorTest, VerticalScaleClampingLow)
{
    Oscillator osc;
    osc.setVerticalScale(0.0f);
    EXPECT_EQ(osc.getVerticalScale(), Oscillator::MIN_VERTICAL_SCALE);
}

TEST_F(OscillatorTest, VerticalScaleClampingHigh)
{
    Oscillator osc;
    osc.setVerticalScale(100.0f);
    EXPECT_EQ(osc.getVerticalScale(), Oscillator::MAX_VERTICAL_SCALE);
}

TEST_F(OscillatorTest, VerticalScaleNegative)
{
    Oscillator osc;
    osc.setVerticalScale(-2.0f);
    EXPECT_EQ(osc.getVerticalScale(), Oscillator::MIN_VERTICAL_SCALE);
}

// Test: Vertical offset clamping
TEST_F(OscillatorTest, VerticalOffsetClampingLow)
{
    Oscillator osc;
    osc.setVerticalOffset(-5.0f);
    EXPECT_EQ(osc.getVerticalOffset(), Oscillator::MIN_VERTICAL_OFFSET);
}

TEST_F(OscillatorTest, VerticalOffsetClampingHigh)
{
    Oscillator osc;
    osc.setVerticalOffset(5.0f);
    EXPECT_EQ(osc.getVerticalOffset(), Oscillator::MAX_VERTICAL_OFFSET);
}

// Test: TimeWindow optional handling
TEST_F(OscillatorTest, TimeWindowDefault)
{
    Oscillator osc;
    EXPECT_FALSE(osc.getTimeWindow().has_value());
}

TEST_F(OscillatorTest, TimeWindowSetValue)
{
    Oscillator osc;
    osc.setTimeWindow(0.5f);
    EXPECT_TRUE(osc.getTimeWindow().has_value());
    EXPECT_FLOAT_EQ(osc.getTimeWindow().value(), 0.5f);
}

TEST_F(OscillatorTest, TimeWindowClear)
{
    Oscillator osc;
    osc.setTimeWindow(1.0f);
    EXPECT_TRUE(osc.getTimeWindow().has_value());

    osc.setTimeWindow(std::nullopt);
    EXPECT_FALSE(osc.getTimeWindow().has_value());
}

TEST_F(OscillatorTest, TimeWindowSerializationRoundTrip)
{
    Oscillator original;
    original.setTimeWindow(2.5f);

    auto valueTree = original.toValueTree();
    Oscillator restored(valueTree);

    EXPECT_TRUE(restored.getTimeWindow().has_value());
    EXPECT_FLOAT_EQ(restored.getTimeWindow().value(), 2.5f);
}

TEST_F(OscillatorTest, TimeWindowNullSerializationRoundTrip)
{
    Oscillator original;
    // TimeWindow is not set (nullopt)

    auto valueTree = original.toValueTree();
    Oscillator restored(valueTree);

    EXPECT_FALSE(restored.getTimeWindow().has_value());
}

// Test: Source state transitions
TEST_F(OscillatorTest, ClearSource)
{
    Oscillator osc;
    SourceId sourceId = SourceId::generate();
    osc.setSourceId(sourceId);

    EXPECT_TRUE(osc.hasSource());
    EXPECT_EQ(osc.getState(), OscillatorState::ACTIVE);

    osc.clearSource();

    EXPECT_FALSE(osc.hasSource());
    EXPECT_TRUE(osc.isNoSource());
    EXPECT_FALSE(osc.getSourceId().isValid());
}

TEST_F(OscillatorTest, SetSourceIdTransitionsState)
{
    Oscillator osc;
    EXPECT_EQ(osc.getState(), OscillatorState::NO_SOURCE);

    osc.setSourceId(SourceId::generate());
    EXPECT_EQ(osc.getState(), OscillatorState::ACTIVE);

    osc.setSourceId(SourceId::invalid());
    EXPECT_EQ(osc.getState(), OscillatorState::NO_SOURCE);
}

TEST_F(OscillatorTest, ClearSourcePreservesConfiguration)
{
    Oscillator osc;
    osc.setSourceId(SourceId::generate());
    osc.setColour(juce::Colours::purple);
    osc.setProcessingMode(ProcessingMode::Mid);
    osc.setOpacity(0.8f);
    osc.setName("Test Config");

    osc.clearSource();

    // Configuration should be preserved
    EXPECT_EQ(osc.getColour(), juce::Colours::purple);
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mid);
    EXPECT_EQ(osc.getOpacity(), 0.8f);
    EXPECT_EQ(osc.getName(), "Test Config");
}

// Test: Schema version migration
TEST_F(OscillatorTest, SchemaV1Migration)
{
    // Simulate v1 state (no OscillatorState property)
    juce::ValueTree v1State("Oscillator");
    v1State.setProperty("schemaVersion", 1, nullptr);
    v1State.setProperty("id", "test-id", nullptr);
    v1State.setProperty("sourceId", "source-123", nullptr);
    v1State.setProperty("processingMode", "Mono", nullptr);

    Oscillator osc(v1State);

    // With valid sourceId, should migrate to ACTIVE state
    EXPECT_EQ(osc.getState(), OscillatorState::ACTIVE);
}

TEST_F(OscillatorTest, SchemaV1MigrationNoSource)
{
    juce::ValueTree v1State("Oscillator");
    v1State.setProperty("schemaVersion", 1, nullptr);
    v1State.setProperty("id", "test-id", nullptr);
    v1State.setProperty("sourceId", "", nullptr);
    v1State.setProperty("processingMode", "Mono", nullptr);

    Oscillator osc(v1State);

    // With empty sourceId, should migrate to NO_SOURCE state
    EXPECT_EQ(osc.getState(), OscillatorState::NO_SOURCE);
}

// Test: Deserialization with invalid data
TEST_F(OscillatorTest, DeserializeWrongType)
{
    juce::ValueTree wrongType("WrongType");
    wrongType.setProperty("id", "test-id", nullptr);

    Oscillator osc;
    OscillatorId originalId = osc.getId();
    osc.fromValueTree(wrongType);

    // Should not load from wrong type
    EXPECT_EQ(osc.getId(), originalId);
}

TEST_F(OscillatorTest, DeserializeWithMissingProperties)
{
    juce::ValueTree minimal("Oscillator");
    // Only has the type, no properties

    Oscillator osc(minimal);

    // Should have defaults
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::FullStereo);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
    EXPECT_TRUE(osc.isVisible());
}

TEST_F(OscillatorTest, DeserializeWithOutOfRangeValues)
{
    juce::ValueTree state("Oscillator");
    state.setProperty("lineWidth", 1000.0f, nullptr);
    state.setProperty("verticalScale", -50.0f, nullptr);
    state.setProperty("verticalOffset", 100.0f, nullptr);
    state.setProperty("opacity", 5.0f, nullptr);

    Oscillator osc(state);

    // Values should be clamped
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
    EXPECT_EQ(osc.getVerticalScale(), Oscillator::MIN_VERTICAL_SCALE);
    EXPECT_EQ(osc.getVerticalOffset(), Oscillator::MAX_VERTICAL_OFFSET);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
}

// Test: Extreme order indices
TEST_F(OscillatorTest, ExtremeOrderIndexNegative)
{
    Oscillator osc;
    osc.setOrderIndex(-100);
    EXPECT_EQ(osc.getOrderIndex(), -100);
}

TEST_F(OscillatorTest, ExtremeOrderIndexLarge)
{
    Oscillator osc;
    osc.setOrderIndex(INT_MAX);
    EXPECT_EQ(osc.getOrderIndex(), INT_MAX);
}

// Test: Invalid OscillatorId handling
TEST_F(OscillatorTest, InvalidOscillatorId)
{
    OscillatorId invalid = OscillatorId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST_F(OscillatorTest, OscillatorIdEquality)
{
    OscillatorId id1 = OscillatorId::generate();
    OscillatorId id2 = OscillatorId::generate();
    OscillatorId id1Copy;
    id1Copy.id = id1.id;

    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1Copy);
}

// ============================================================================
// Pane Edge Case Tests
// ============================================================================

// Test: Height ratio clamping
TEST_F(PaneTest, HeightRatioClampingLow)
{
    Pane pane;
    pane.setHeightRatio(0.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MIN_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioClampingHigh)
{
    Pane pane;
    pane.setHeightRatio(2.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MAX_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioNegative)
{
    Pane pane;
    pane.setHeightRatio(-1.0f);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MIN_HEIGHT_RATIO);
}

TEST_F(PaneTest, HeightRatioDefault)
{
    Pane pane;
    EXPECT_EQ(pane.getHeightRatio(), Pane::DEFAULT_HEIGHT_RATIO);
}

// Test: Column index
TEST_F(PaneTest, ColumnIndexDefault)
{
    Pane pane;
    EXPECT_EQ(pane.getColumnIndex(), 0);
}

TEST_F(PaneTest, ColumnIndexSetter)
{
    Pane pane;
    pane.setColumnIndex(2);
    EXPECT_EQ(pane.getColumnIndex(), 2);
}

// Test: PaneId handling
TEST_F(PaneTest, InvalidPaneId)
{
    PaneId invalid = PaneId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST_F(PaneTest, PaneIdEquality)
{
    PaneId id1 = PaneId::generate();
    PaneId id2 = PaneId::generate();
    PaneId id1Copy;
    id1Copy.id = id1.id;

    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1Copy);
}

// Test: Pane deserialization with out of range values
TEST_F(PaneTest, DeserializeWithOutOfRangeHeightRatio)
{
    juce::ValueTree state("Pane");
    state.setProperty("heightRatio", 5.0f, nullptr);

    Pane pane(state);
    EXPECT_EQ(pane.getHeightRatio(), Pane::MAX_HEIGHT_RATIO);
}

TEST_F(PaneTest, DeserializeWrongType)
{
    juce::ValueTree wrongType("WrongType");
    wrongType.setProperty("id", "test-id", nullptr);

    Pane pane;
    PaneId originalId = pane.getId();
    pane.fromValueTree(wrongType);

    // Should not load from wrong type
    EXPECT_EQ(pane.getId(), originalId);
}

// ============================================================================
// PaneDragState Tests
// ============================================================================

class PaneDragStateTest : public ::testing::Test
{
protected:
    PaneDragState dragState;
};

TEST_F(PaneDragStateTest, InitialState)
{
    EXPECT_FALSE(dragState.isDragging);
    EXPECT_FALSE(dragState.draggedPaneId.isValid());
    EXPECT_EQ(dragState.dragStartIndex, -1);
}

TEST_F(PaneDragStateTest, StartDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {100, 200});

    EXPECT_TRUE(dragState.isDragging);
    EXPECT_EQ(dragState.draggedPaneId, paneId);
    EXPECT_EQ(dragState.dragStartIndex, 2);
    EXPECT_EQ(dragState.dragStartColumn, 1);
    EXPECT_EQ(dragState.dragStartPosition, juce::Point<int>(100, 200));
    EXPECT_EQ(dragState.currentPosition, juce::Point<int>(100, 200));
    EXPECT_FALSE(dragState.isValidDropTarget);
}

TEST_F(PaneDragStateTest, UpdateDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 0, 0, {0, 0});
    dragState.updateDrag({150, 300});

    EXPECT_EQ(dragState.currentPosition, juce::Point<int>(150, 300));
}

TEST_F(PaneDragStateTest, SetDropTarget)
{
    PaneId dragId = PaneId::generate();
    PaneId targetId = PaneId::generate();

    dragState.startDrag(dragId, 0, 0, {0, 0});
    dragState.setDropTarget(targetId, 2, 1, true);

    EXPECT_EQ(dragState.dropTargetPaneId, targetId);
    EXPECT_EQ(dragState.dropTargetIndex, 2);
    EXPECT_EQ(dragState.dropTargetColumn, 1);
    EXPECT_TRUE(dragState.isValidDropTarget);
}

TEST_F(PaneDragStateTest, EndDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 1, 0, {50, 50});
    dragState.endDrag();

    EXPECT_FALSE(dragState.isDragging);
    EXPECT_FALSE(dragState.draggedPaneId.isValid());
    EXPECT_EQ(dragState.dragStartIndex, -1);
    EXPECT_EQ(dragState.dropTargetIndex, -1);
}

TEST_F(PaneDragStateTest, CancelDrag)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 1, 0, {50, 50});
    dragState.cancelDrag();

    EXPECT_FALSE(dragState.isDragging);
}

TEST_F(PaneDragStateTest, IsSamePositionDrop)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {0, 0});
    dragState.setDropTarget(PaneId::generate(), 2, 1, true);

    EXPECT_TRUE(dragState.isSamePositionDrop());
}

TEST_F(PaneDragStateTest, IsDifferentPositionDrop)
{
    PaneId paneId = PaneId::generate();
    dragState.startDrag(paneId, 2, 1, {0, 0});
    dragState.setDropTarget(PaneId::generate(), 3, 1, true);

    EXPECT_FALSE(dragState.isSamePositionDrop());
}

// ============================================================================
// PaneLayoutManager Edge Case Tests
// ============================================================================

// Test: Remove non-existent pane
TEST_F(PaneLayoutManagerTest, RemoveNonExistentPane)
{
    Pane pane;
    manager.addPane(pane);
    EXPECT_EQ(manager.getPaneCount(), 1);

    // Try to remove a pane that doesn't exist
    manager.removePane(PaneId::generate());

    // Should still have 1 pane
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Move non-existent pane
TEST_F(PaneLayoutManagerTest, MoveNonExistentPane)
{
    Pane pane;
    manager.addPane(pane);

    // Try to move a pane that doesn't exist - should not crash
    manager.movePane(PaneId::generate(), 0);

    // Original pane should be unaffected
    EXPECT_EQ(manager.getPaneCount(), 1);
}

// Test: Move pane to invalid column
TEST_F(PaneLayoutManagerTest, MovePaneToInvalidColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane;
    manager.addPane(pane);

    // Try to move to column 10 (only 0-1 are valid)
    manager.movePaneToColumn(pane.getId(), 10, 0);

    // Should be clamped to valid column range (column 1 max for Double)
    const Pane* movedPane = manager.getPane(pane.getId());
    EXPECT_LE(movedPane->getColumnIndex(), 1);
}

// Test: Get pane bounds with empty manager
TEST_F(PaneLayoutManagerTest, GetPaneBoundsEmpty)
{
    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(0, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Get pane bounds with invalid index
TEST_F(PaneLayoutManagerTest, GetPaneBoundsInvalidIndexNegative)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(-1, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

TEST_F(PaneLayoutManagerTest, GetPaneBoundsInvalidIndexTooLarge)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    auto bounds = manager.getPaneBounds(100, availableArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Get pane bounds with 0-size area
TEST_F(PaneLayoutManagerTest, GetPaneBoundsZeroSizeArea)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> zeroArea(0, 0, 0, 0);
    auto bounds = manager.getPaneBounds(0, zeroArea);

    // Should return something (even if small due to margins)
    // The bounds will have negative dimensions due to margin subtraction
    EXPECT_LE(bounds.getWidth(), 0);
}

// Test: Get panes in non-existent column
TEST_F(PaneLayoutManagerTest, GetPanesInNonExistentColumn)
{
    manager.setColumnLayout(ColumnLayout::Single);

    Pane pane;
    manager.addPane(pane);

    // Column 5 doesn't exist
    auto panes = manager.getPanesInColumn(5);
    EXPECT_TRUE(panes.empty());
}

// Test: Get pane by invalid ID
TEST_F(PaneLayoutManagerTest, GetPaneInvalidId)
{
    Pane pane;
    manager.addPane(pane);

    Pane* found = manager.getPane(PaneId::generate());
    EXPECT_EQ(found, nullptr);
}

// Test: Many panes stress test
TEST_F(PaneLayoutManagerTest, ManyPanes)
{
    const int count = 100;

    for (int i = 0; i < count; ++i)
    {
        Pane pane;
        pane.setOrderIndex(i);
        manager.addPane(pane);
    }

    EXPECT_EQ(manager.getPaneCount(), count);

    // All panes should be accessible
    juce::Rectangle<int> availableArea(0, 0, 800, 600);
    for (int i = 0; i < count; ++i)
    {
        auto bounds = manager.getPaneBounds(i, availableArea);
        // Bounds might be very small but should exist
        EXPECT_TRUE(bounds.getWidth() >= -10); // Allow for margin issues
    }
}

// Test: Clear all panes
TEST_F(PaneLayoutManagerTest, ClearPanes)
{
    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        manager.addPane(pane);
    }

    EXPECT_EQ(manager.getPaneCount(), 5);

    manager.clear();

    EXPECT_EQ(manager.getPaneCount(), 0);
}

// Test: Listener notifications
class TestPaneListener : public PaneLayoutManager::Listener
{
public:
    int columnLayoutChangedCount = 0;
    int paneOrderChangedCount = 0;
    int paneAddedCount = 0;
    int paneRemovedCount = 0;

    void columnLayoutChanged(ColumnLayout) override { ++columnLayoutChangedCount; }
    void paneOrderChanged() override { ++paneOrderChangedCount; }
    void paneAdded(const PaneId&) override { ++paneAddedCount; }
    void paneRemoved(const PaneId&) override { ++paneRemovedCount; }
};

TEST_F(PaneLayoutManagerTest, ListenerNotifications)
{
    TestPaneListener listener;
    manager.addListener(&listener);

    // Add pane
    Pane pane;
    manager.addPane(pane);
    EXPECT_EQ(listener.paneAddedCount, 1);

    // Change layout
    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(listener.columnLayoutChangedCount, 1);

    // Move pane
    manager.movePane(pane.getId(), 0);
    EXPECT_GE(listener.paneOrderChangedCount, 1);

    // Remove pane
    manager.removePane(pane.getId());
    EXPECT_EQ(listener.paneRemovedCount, 1);

    manager.removeListener(&listener);
}

TEST_F(PaneLayoutManagerTest, RemoveListenerStopsNotifications)
{
    TestPaneListener listener;
    manager.addListener(&listener);
    manager.removeListener(&listener);

    Pane pane;
    manager.addPane(pane);

    EXPECT_EQ(listener.paneAddedCount, 0);
}

// Test: Serialization with empty panes list
TEST_F(PaneLayoutManagerTest, SerializeEmptyPanes)
{
    auto valueTree = manager.toValueTree();

    EXPECT_TRUE(valueTree.isValid());
    EXPECT_EQ(valueTree.getNumChildren(), 0);
}

TEST_F(PaneLayoutManagerTest, DeserializeEmptyPanes)
{
    juce::ValueTree emptyPanes("Panes");
    emptyPanes.setProperty("columnLayout", 2, nullptr);

    manager.fromValueTree(emptyPanes);

    EXPECT_EQ(manager.getPaneCount(), 0);
    EXPECT_EQ(manager.getColumnLayout(), ColumnLayout::Double);
}

// Test: Serialization round-trip
TEST_F(PaneLayoutManagerTest, SerializationRoundTrip)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    for (int i = 0; i < 5; ++i)
    {
        Pane pane;
        pane.setName(juce::String("Pane ") + juce::String(i));
        pane.setHeightRatio(0.5f);
        manager.addPane(pane);
    }

    auto valueTree = manager.toValueTree();

    PaneLayoutManager restored;
    restored.fromValueTree(valueTree);

    EXPECT_EQ(restored.getColumnLayout(), ColumnLayout::Triple);
    EXPECT_EQ(restored.getPaneCount(), 5);
}

// Test: movePaneToColumn with negative column
TEST_F(PaneLayoutManagerTest, MovePaneToNegativeColumn)
{
    manager.setColumnLayout(ColumnLayout::Double);

    Pane pane;
    manager.addPane(pane);

    manager.movePaneToColumn(pane.getId(), -5, 0);

    // Should be clamped to column 0
    const Pane* movedPane = manager.getPane(pane.getId());
    EXPECT_GE(movedPane->getColumnIndex(), 0);
}

// Test: Get pane bounds in column with invalid pane
TEST_F(PaneLayoutManagerTest, GetPaneBoundsInColumnInvalidPane)
{
    Pane pane;
    manager.addPane(pane);

    juce::Rectangle<int> columnArea(0, 0, 400, 600);
    auto bounds = manager.getPaneBoundsInColumn(PaneId::generate(), columnArea);

    EXPECT_TRUE(bounds.isEmpty());
}

// Test: Column count for different layouts
TEST_F(PaneLayoutManagerTest, ColumnCountForLayouts)
{
    manager.setColumnLayout(ColumnLayout::Single);
    EXPECT_EQ(manager.getColumnCount(), 1);

    manager.setColumnLayout(ColumnLayout::Double);
    EXPECT_EQ(manager.getColumnCount(), 2);

    manager.setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(manager.getColumnCount(), 3);
}

// Test: getColumnForPane with out of range index
TEST_F(PaneLayoutManagerTest, GetColumnForPaneOutOfRange)
{
    manager.setColumnLayout(ColumnLayout::Triple);

    // With no panes, should use fallback row-major
    EXPECT_EQ(manager.getColumnForPane(0), 0);
    EXPECT_EQ(manager.getColumnForPane(1), 1);
    EXPECT_EQ(manager.getColumnForPane(2), 2);
    EXPECT_EQ(manager.getColumnForPane(3), 0);
}

// Test: Hash functions for IDs
TEST_F(OscillatorTest, OscillatorIdHash)
{
    OscillatorIdHash hasher;
    OscillatorId id1 = OscillatorId::generate();
    OscillatorId id2 = OscillatorId::generate();

    size_t hash1 = hasher(id1);
    size_t hash2 = hasher(id2);

    // Hashes should be different (with very high probability)
    EXPECT_NE(hash1, hash2);

    // Same ID should produce same hash
    OscillatorId id1Copy;
    id1Copy.id = id1.id;
    EXPECT_EQ(hasher(id1), hasher(id1Copy));
}

TEST_F(PaneTest, PaneIdHash)
{
    PaneIdHash hasher;
    PaneId id1 = PaneId::generate();
    PaneId id2 = PaneId::generate();

    size_t hash1 = hasher(id1);
    size_t hash2 = hasher(id2);

    // Hashes should be different (with very high probability)
    EXPECT_NE(hash1, hash2);

    // Same ID should produce same hash
    PaneId id1Copy;
    id1Copy.id = id1.id;
    EXPECT_EQ(hasher(id1), hasher(id1Copy));
}
