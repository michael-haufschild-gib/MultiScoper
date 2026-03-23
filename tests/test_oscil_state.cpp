/*
    Oscil - OscilState Unit Tests
    Tests for the central state management class: oscillator CRUD,
    property getters/setters, reorder operations, and edge cases.

    Bug targets:
    - removeOscillator not renumbering remaining orderIndex values
    - updateOscillator silently dropping VisualOverrides children
    - reorderOscillators with equal indices returning silently but not preserving state
    - getOscillator returning stale data after updateOscillator
    - getColumnLayout returning out-of-range enum on corrupt state
    - getCaptureQualityConfig returning wrong defaults when node absent
    - setGainDb storing truncated float precision
    - fromXmlString succeeding on non-OscilState root element
*/

#include <gtest/gtest.h>
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "helpers/OscillatorBuilder.h"

using namespace oscil;
using namespace oscil::test;

class OscilStateTest : public ::testing::Test
{
protected:
    std::unique_ptr<OscilState> state;

    void SetUp() override
    {
        state = std::make_unique<OscilState>();
    }

    OscillatorId addOscillator(const juce::String& name, int orderIndex = 0)
    {
        auto osc = OscillatorBuilder()
            .withName(name)
            .withSourceId(SourceId::generate())
            .withOrderIndex(orderIndex)
            .build();
        state->addOscillator(osc);
        return osc.getId();
    }
};

// ============================================================================
// Default State
// ============================================================================

TEST_F(OscilStateTest, DefaultStateHasCurrentSchemaVersion)
{
    EXPECT_EQ(state->getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}

TEST_F(OscilStateTest, DefaultStateHasNoOscillators)
{
    EXPECT_EQ(state->getOscillators().size(), 0);
}

TEST_F(OscilStateTest, DefaultStateHasNoPanes)
{
    EXPECT_EQ(state->getLayoutManager().getPaneCount(), 0);
}

TEST_F(OscilStateTest, DefaultThemeIsDarkProfessional)
{
    EXPECT_EQ(state->getThemeName(), "Dark Professional");
}

TEST_F(OscilStateTest, DefaultColumnLayoutIsSingle)
{
    EXPECT_EQ(state->getColumnLayout(), ColumnLayout::Single);
}

TEST_F(OscilStateTest, DefaultSidebarWidth)
{
    EXPECT_EQ(state->getSidebarWidth(), 300);
}

TEST_F(OscilStateTest, DefaultSidebarNotCollapsed)
{
    EXPECT_FALSE(state->isSidebarCollapsed());
}

TEST_F(OscilStateTest, DefaultStatusBarVisible)
{
    EXPECT_TRUE(state->isStatusBarVisible());
}

TEST_F(OscilStateTest, DefaultShowGridEnabled)
{
    EXPECT_TRUE(state->isShowGridEnabled());
}

TEST_F(OscilStateTest, DefaultAutoScaleEnabled)
{
    EXPECT_TRUE(state->isAutoScaleEnabled());
}

TEST_F(OscilStateTest, DefaultGainDbIsZero)
{
    EXPECT_FLOAT_EQ(state->getGainDb(), 0.0f);
}

TEST_F(OscilStateTest, DefaultCaptureQualityIsStandard)
{
    auto config = state->getCaptureQualityConfig();
    EXPECT_EQ(config.qualityPreset, QualityPreset::Standard);
    EXPECT_EQ(config.bufferDuration, BufferDuration::Medium);
    EXPECT_TRUE(config.autoAdjustQuality);
}

// ============================================================================
// Oscillator CRUD
// ============================================================================

TEST_F(OscilStateTest, AddOscillatorIncreasesCount)
{
    addOscillator("Osc 1");
    EXPECT_EQ(state->getOscillators().size(), 1);

    addOscillator("Osc 2");
    EXPECT_EQ(state->getOscillators().size(), 2);
}

TEST_F(OscilStateTest, GetOscillatorByIdReturnsCorrectOscillator)
{
    auto id = addOscillator("Target Osc");

    auto retrieved = state->getOscillator(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->getName(), "Target Osc");
    EXPECT_EQ(retrieved->getId(), id);
}

TEST_F(OscilStateTest, GetOscillatorWithInvalidIdReturnsNullopt)
{
    addOscillator("Existing");
    auto result = state->getOscillator(OscillatorId::generate());
    EXPECT_FALSE(result.has_value());
}

TEST_F(OscilStateTest, RemoveOscillatorDecreasesCount)
{
    auto id1 = addOscillator("Osc 1", 0);
    auto id2 = addOscillator("Osc 2", 1);

    state->removeOscillator(id1);
    EXPECT_EQ(state->getOscillators().size(), 1);

    auto remaining = state->getOscillator(id2);
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining->getName(), "Osc 2");
}

TEST_F(OscilStateTest, RemoveOscillatorRenumbersOrderIndices)
{
    // Bug caught: after removing oscillator at index 0, remaining oscillators
    // keep their original order indices, leaving a gap (1,2 instead of 0,1)
    auto id0 = addOscillator("Osc 0", 0);
    addOscillator("Osc 1", 1);
    addOscillator("Osc 2", 2);

    state->removeOscillator(id0);

    auto oscillators = state->getOscillators();
    ASSERT_EQ(oscillators.size(), 2);

    // Remaining oscillators should have contiguous indices starting from 0
    std::vector<int> indices;
    for (const auto& osc : oscillators)
        indices.push_back(osc.getOrderIndex());
    std::sort(indices.begin(), indices.end());

    EXPECT_EQ(indices[0], 0);
    EXPECT_EQ(indices[1], 1);
}

TEST_F(OscilStateTest, RemoveNonexistentOscillatorIsNoOp)
{
    addOscillator("Existing");
    state->removeOscillator(OscillatorId::generate());
    EXPECT_EQ(state->getOscillators().size(), 1);
}

TEST_F(OscilStateTest, UpdateOscillatorPersistsChanges)
{
    auto id = addOscillator("Original Name");

    auto osc = state->getOscillator(id);
    ASSERT_TRUE(osc.has_value());

    osc->setName("Updated Name");
    osc->setProcessingMode(ProcessingMode::Side);
    osc->setOpacity(0.42f);
    state->updateOscillator(*osc);

    auto updated = state->getOscillator(id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->getName(), "Updated Name");
    EXPECT_EQ(updated->getProcessingMode(), ProcessingMode::Side);
    EXPECT_FLOAT_EQ(updated->getOpacity(), 0.42f);
}

TEST_F(OscilStateTest, UpdateOscillatorPreservesVisualOverrides)
{
    // Bug caught: updateOscillator using copyPropertiesFrom drops child
    // ValueTree nodes (VisualOverrides) because that method only copies properties
    auto id = addOscillator("Styled Osc");

    auto osc = state->getOscillator(id);
    ASSERT_TRUE(osc.has_value());

    osc->setVisualOverride("glow", 0.75f);
    osc->setVisualOverride("thickness", 2.0f);
    state->updateOscillator(*osc);

    auto updated = state->getOscillator(id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_FLOAT_EQ(static_cast<float>(updated->getVisualOverride("glow")), 0.75f);
    EXPECT_FLOAT_EQ(static_cast<float>(updated->getVisualOverride("thickness")), 2.0f);
}

// ============================================================================
// Reorder Operations
// ============================================================================

TEST_F(OscilStateTest, ReorderOscillatorsSameIndexIsNoOp)
{
    addOscillator("A", 0);
    addOscillator("B", 1);

    state->reorderOscillators(0, 0);

    auto oscillators = state->getOscillators();
    std::sort(oscillators.begin(), oscillators.end(),
              [](const Oscillator& a, const Oscillator& b) {
                  return a.getOrderIndex() < b.getOrderIndex();
              });
    EXPECT_EQ(oscillators[0].getName(), "A");
    EXPECT_EQ(oscillators[1].getName(), "B");
}

TEST_F(OscilStateTest, ReorderOscillatorsOutOfBoundsIsNoOp)
{
    addOscillator("A", 0);

    state->reorderOscillators(0, 5);
    state->reorderOscillators(-1, 0);

    EXPECT_EQ(state->getOscillators().size(), 1);
}

TEST_F(OscilStateTest, ReorderOscillatorsMovesForward)
{
    addOscillator("A", 0);
    addOscillator("B", 1);
    addOscillator("C", 2);

    // Move A from index 0 to index 2
    state->reorderOscillators(0, 2);

    auto oscillators = state->getOscillators();
    std::sort(oscillators.begin(), oscillators.end(),
              [](const Oscillator& a, const Oscillator& b) {
                  return a.getOrderIndex() < b.getOrderIndex();
              });

    EXPECT_EQ(oscillators[0].getName(), "B");
    EXPECT_EQ(oscillators[1].getName(), "C");
    EXPECT_EQ(oscillators[2].getName(), "A");
}

TEST_F(OscilStateTest, ReorderOscillatorsMovesBackward)
{
    addOscillator("A", 0);
    addOscillator("B", 1);
    addOscillator("C", 2);

    // Move C from index 2 to index 0
    state->reorderOscillators(2, 0);

    auto oscillators = state->getOscillators();
    std::sort(oscillators.begin(), oscillators.end(),
              [](const Oscillator& a, const Oscillator& b) {
                  return a.getOrderIndex() < b.getOrderIndex();
              });

    EXPECT_EQ(oscillators[0].getName(), "C");
    EXPECT_EQ(oscillators[1].getName(), "A");
    EXPECT_EQ(oscillators[2].getName(), "B");
}

// ============================================================================
// Property Setters and Getters
// ============================================================================

TEST_F(OscilStateTest, SetThemeNamePersists)
{
    state->setThemeName("Custom Theme");
    EXPECT_EQ(state->getThemeName(), "Custom Theme");
}

TEST_F(OscilStateTest, SetColumnLayoutPersists)
{
    state->setColumnLayout(ColumnLayout::Triple);
    EXPECT_EQ(state->getColumnLayout(), ColumnLayout::Triple);
}

TEST_F(OscilStateTest, SetSidebarWidthPersists)
{
    state->setSidebarWidth(450);
    EXPECT_EQ(state->getSidebarWidth(), 450);
}

TEST_F(OscilStateTest, SetSidebarCollapsedPersists)
{
    state->setSidebarCollapsed(true);
    EXPECT_TRUE(state->isSidebarCollapsed());

    state->setSidebarCollapsed(false);
    EXPECT_FALSE(state->isSidebarCollapsed());
}

TEST_F(OscilStateTest, SetStatusBarVisiblePersists)
{
    state->setStatusBarVisible(false);
    EXPECT_FALSE(state->isStatusBarVisible());
}

TEST_F(OscilStateTest, SetShowGridEnabledPersists)
{
    state->setShowGridEnabled(false);
    EXPECT_FALSE(state->isShowGridEnabled());
}

TEST_F(OscilStateTest, SetAutoScaleEnabledPersists)
{
    state->setAutoScaleEnabled(false);
    EXPECT_FALSE(state->isAutoScaleEnabled());
}

TEST_F(OscilStateTest, SetGainDbPreservesFloatPrecision)
{
    // Bug caught: getGainDb truncating float precision by storing as double
    // and casting back to float
    state->setGainDb(-6.125f);
    EXPECT_FLOAT_EQ(state->getGainDb(), -6.125f);

    state->setGainDb(12.5f);
    EXPECT_FLOAT_EQ(state->getGainDb(), 12.5f);
}

TEST_F(OscilStateTest, SetCaptureQualityConfigPersists)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::High;
    config.bufferDuration = BufferDuration::Long;
    config.autoAdjustQuality = false;
    config.memoryBudget.totalBudgetBytes = 1024 * 1024 * 64; // 64MB

    state->setCaptureQualityConfig(config);

    auto retrieved = state->getCaptureQualityConfig();
    EXPECT_EQ(retrieved.qualityPreset, QualityPreset::High);
    EXPECT_EQ(retrieved.bufferDuration, BufferDuration::Long);
    EXPECT_FALSE(retrieved.autoAdjustQuality);
    EXPECT_EQ(retrieved.memoryBudget.totalBudgetBytes, 1024u * 1024 * 64);
}

// ============================================================================
// Serialization Edge Cases
// ============================================================================

TEST_F(OscilStateTest, FromXmlStringRejectsEmptyString)
{
    EXPECT_FALSE(state->fromXmlString(""));
}

TEST_F(OscilStateTest, FromXmlStringRejectsMalformedXml)
{
    EXPECT_FALSE(state->fromXmlString("<not <valid xml"));
}

TEST_F(OscilStateTest, FromXmlStringRejectsWrongRootElement)
{
    // Bug caught: fromXmlString accepting XML with wrong root element,
    // silently replacing state with unrelated data
    EXPECT_FALSE(state->fromXmlString("<WrongRoot version=\"2\"/>"));
}

TEST_F(OscilStateTest, ToXmlStringProducesNonEmptyOutput)
{
    juce::String xml = state->toXmlString();
    EXPECT_FALSE(xml.isEmpty());
}

TEST_F(OscilStateTest, ValueTreeListenerReceivesPropertyChanges)
{
    // Bug caught: addListener/removeListener not forwarding to ValueTree
    struct TestListener : juce::ValueTree::Listener
    {
        int changeCount = 0;
        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
        {
            changeCount++;
        }
    };

    TestListener listener;
    state->addListener(&listener);

    state->setShowGridEnabled(false);
    EXPECT_GT(listener.changeCount, 0);

    state->removeListener(&listener);
    int countAfterRemove = listener.changeCount;
    state->setShowGridEnabled(true);
    EXPECT_EQ(listener.changeCount, countAfterRemove);
}

// ============================================================================
// Constructor with XML
// ============================================================================

TEST_F(OscilStateTest, ConstructFromValidXmlRestoresState)
{
    state->setThemeName("Neon");
    state->setGainDb(-3.0f);
    state->setShowGridEnabled(false);

    juce::String xml = state->toXmlString();

    OscilState restored(xml);
    EXPECT_EQ(restored.getThemeName(), "Neon");
    EXPECT_FLOAT_EQ(restored.getGainDb(), -3.0f);
    EXPECT_FALSE(restored.isShowGridEnabled());
}

TEST_F(OscilStateTest, ConstructFromInvalidXmlFallsBackToDefaults)
{
    OscilState restored("garbage data");

    // Should have valid defaults
    EXPECT_EQ(restored.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(restored.getThemeName(), "Dark Professional");
}

TEST_F(OscilStateTest, ConstructFromEmptyXmlFallsBackToDefaults)
{
    OscilState restored("");

    EXPECT_EQ(restored.getSchemaVersion(), OscilState::CURRENT_SCHEMA_VERSION);
}
