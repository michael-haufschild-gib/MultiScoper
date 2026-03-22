/*
    Oscil - State Integration Tests
    Multi-component data flow tests that verify end-to-end correctness
    across OscilState, Oscillator, Pane, and serialization boundaries.

    Bug targets:
    - State corruption when oscillators reference deleted panes
    - Serialization round-trip losing cross-component relationships
    - Source detachment not propagating through state graph
    - Reorder operations corrupting pane assignments
    - Maximum oscillator limits interacting with serialization
*/

#include <gtest/gtest.h>
#include "helpers/OscillatorBuilder.h"
#include "helpers/StateBuilder.h"
#include "helpers/Fixtures.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/Pane.h"

using namespace oscil;
using namespace oscil::test;

class StateIntegrationTest : public ::testing::Test
{
protected:
    std::unique_ptr<OscilState> state;

    void SetUp() override
    {
        state = std::make_unique<OscilState>();
    }

    // Create a pane, add it to state, and return its ID
    PaneId addPane(const juce::String& name, int orderIndex = 0)
    {
        Pane pane;
        pane.setName(name);
        pane.setOrderIndex(orderIndex);
        state->getLayoutManager().addPane(pane);
        return pane.getId();
    }

    // Create and add an oscillator attached to a pane
    OscillatorId addOscillatorToPane(const juce::String& name, const PaneId& paneId,
                                      const SourceId& sourceId = SourceId::generate())
    {
        auto osc = OscillatorBuilder()
            .withName(name)
            .withSourceId(sourceId)
            .withPaneId(paneId)
            .build();
        state->addOscillator(osc);
        return osc.getId();
    }
};

// ============================================================================
// Multi-Component Serialization Round-Trip
// ============================================================================

TEST_F(StateIntegrationTest, FullStateRoundTripPreservesAllRelationships)
{
    // Bug: serialization loses paneId/sourceId → oscillators orphaned after save/load
    auto p1 = addPane("Pane 1", 0), p2 = addPane("Pane 2", 1);
    auto s1 = SourceId::generate(), s2 = SourceId::generate();
    auto o1 = addOscillatorToPane("Osc 1", p1, s1);
    auto o2 = addOscillatorToPane("Osc 2", p1, s2);
    auto o3 = addOscillatorToPane("Osc 3", p2, s1);

    state->setThemeName("Dark");
    state->setColumnLayout(ColumnLayout::Double);
    state->setSidebarWidth(300);
    state->setShowGridEnabled(true);

    juce::String xml = state->toXmlString();
    ASSERT_FALSE(xml.isEmpty());

    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));
    ASSERT_EQ(restored->getOscillators().size(), 3);

    auto r1 = restored->getOscillator(o1);
    auto r2 = restored->getOscillator(o2);
    auto r3 = restored->getOscillator(o3);
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());

    EXPECT_EQ(r1->getPaneId(), p1);
    EXPECT_EQ(r2->getPaneId(), p1);
    EXPECT_EQ(r3->getPaneId(), p2);
    EXPECT_EQ(r1->getSourceId(), s1);
    EXPECT_EQ(r2->getSourceId(), s2);
    EXPECT_EQ(r3->getSourceId(), s1);
    EXPECT_EQ(r1->getName(), "Osc 1");
    EXPECT_EQ(r2->getName(), "Osc 2");
    EXPECT_EQ(r3->getName(), "Osc 3");

    EXPECT_EQ(restored->getThemeName(), "Dark");
    EXPECT_EQ(restored->getColumnLayout(), ColumnLayout::Double);
    EXPECT_EQ(restored->getSidebarWidth(), 300);
    EXPECT_TRUE(restored->isShowGridEnabled());
}

// ============================================================================
// Source Lifecycle
// ============================================================================

TEST_F(StateIntegrationTest, ClearSourceOnOscillatorPreservesOtherOscillators)
{
    // Bug caught: clearing source on one oscillator corrupts the state
    // of other oscillators sharing the same source.
    auto paneId = addPane("Main Pane");
    auto sharedSource = SourceId::generate();

    auto osc1Id = addOscillatorToPane("Osc 1", paneId, sharedSource);
    auto osc2Id = addOscillatorToPane("Osc 2", paneId, sharedSource);

    // Clear source on osc1
    auto osc1 = state->getOscillator(osc1Id);
    ASSERT_TRUE(osc1.has_value());
    osc1->clearSource();
    state->updateOscillator(*osc1);

    // Osc1 should be NO_SOURCE
    auto updated1 = state->getOscillator(osc1Id);
    ASSERT_TRUE(updated1.has_value());
    EXPECT_EQ(updated1->getState(), OscillatorState::NO_SOURCE);
    EXPECT_FALSE(updated1->hasSource());

    // Osc2 should still have its source
    auto updated2 = state->getOscillator(osc2Id);
    ASSERT_TRUE(updated2.has_value());
    EXPECT_EQ(updated2->getSourceId(), sharedSource);
    EXPECT_EQ(updated2->getState(), OscillatorState::ACTIVE);
}

// ============================================================================
// Pane Deletion Cascade
// ============================================================================

TEST_F(StateIntegrationTest, RemovePaneDoesNotDeleteOscillators)
{
    // Bug caught: removing a pane also deletes its oscillators instead of
    // just detaching them.
    auto paneId = addPane("Doomed Pane");
    auto oscId = addOscillatorToPane("Survivor Osc", paneId);

    state->getLayoutManager().removePane(paneId);

    // Oscillator should still exist in state
    auto osc = state->getOscillator(oscId);
    ASSERT_TRUE(osc.has_value()) << "Oscillator deleted when pane was removed";
    EXPECT_EQ(osc->getName(), "Survivor Osc");
}

// ============================================================================
// Reorder Operations Under Complex State
// ============================================================================

TEST_F(StateIntegrationTest, ReorderOscillatorsPreservesPaneAssignments)
{
    // Bug caught: reorderOscillators swapping orderIndex values but
    // accidentally swapping paneId values too.
    auto pane1 = addPane("Pane 1", 0);
    auto pane2 = addPane("Pane 2", 1);

    auto osc1 = OscillatorBuilder().withName("First").withPaneId(pane1).withOrderIndex(0).build();
    auto osc2 = OscillatorBuilder().withName("Second").withPaneId(pane2).withOrderIndex(1).build();
    auto osc3 = OscillatorBuilder().withName("Third").withPaneId(pane1).withOrderIndex(2).build();
    state->addOscillator(osc1);
    state->addOscillator(osc2);
    state->addOscillator(osc3);

    // Reorder: move "First" (index 0) to index 2
    state->reorderOscillators(0, 2);

    // Verify pane assignments are unchanged
    auto oscillators = state->getOscillators();
    ASSERT_EQ(oscillators.size(), 3);

    auto findByName = [&](const juce::String& name) -> std::optional<Oscillator> {
        for (const auto& o : oscillators)
            if (o.getName() == name) return o;
        return std::nullopt;
    };

    auto first = findByName("First");
    auto second = findByName("Second");
    auto third = findByName("Third");

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(third.has_value());

    // Pane assignments must not change during reorder
    EXPECT_EQ(first->getPaneId(), pane1);
    EXPECT_EQ(second->getPaneId(), pane2);
    EXPECT_EQ(third->getPaneId(), pane1);
}

// ============================================================================
// Maximum Oscillators with Serialization
// ============================================================================

TEST_F(StateIntegrationTest, MaxOscillatorsSerializationRoundTrip)
{
    // Bug caught: serialization truncating oscillator list when count is large,
    // or deserialization failing to reconstruct all oscillators.
    auto paneId = addPane("Main Pane");
    constexpr int kMaxOsc = 32;

    std::vector<OscillatorId> ids;
    for (int i = 0; i < kMaxOsc; ++i)
    {
        auto osc = OscillatorBuilder()
            .withName("Osc_" + juce::String(i))
            .withPaneId(paneId)
            .withSourceId(SourceId::generate())
            .withProcessingMode(static_cast<ProcessingMode>(i % 6))
            .withOrderIndex(i)
            .build();
        state->addOscillator(osc);
        ids.push_back(osc.getId());
    }

    ASSERT_EQ(state->getOscillators().size(), kMaxOsc);

    // Round-trip
    juce::String xml = state->toXmlString();
    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));

    auto restoredOscillators = restored->getOscillators();
    EXPECT_EQ(restoredOscillators.size(), kMaxOsc);

    // Verify each oscillator was preserved
    for (int i = 0; i < kMaxOsc; ++i)
    {
        auto osc = restored->getOscillator(ids[static_cast<size_t>(i)]);
        ASSERT_TRUE(osc.has_value()) << "Oscillator " << i << " lost in serialization";
        EXPECT_EQ(osc->getName(), "Osc_" + juce::String(i));
        EXPECT_EQ(osc->getProcessingMode(), static_cast<ProcessingMode>(i % 6));
    }
}

// ============================================================================
// Double Serialization Idempotency
// ============================================================================

TEST_F(StateIntegrationTest, DoubleSerializationIsIdempotent)
{
    // Bug caught: serialization introducing drift (e.g., float precision loss
    // accumulating, or default values being injected on second round-trip).
    auto paneId = addPane("Test Pane");
    addOscillatorToPane("Test Osc", paneId);
    state->setGainDb(-6.0f);
    state->setShowGridEnabled(true);

    // First round-trip
    juce::String xml1 = state->toXmlString();
    auto state2 = std::make_unique<OscilState>();
    ASSERT_TRUE(state2->fromXmlString(xml1));

    // Second round-trip
    juce::String xml2 = state2->toXmlString();
    auto state3 = std::make_unique<OscilState>();
    ASSERT_TRUE(state3->fromXmlString(xml2));

    // Third round-trip to detect accumulating drift
    juce::String xml3 = state3->toXmlString();

    // XML from second and third round-trips should be identical
    EXPECT_EQ(xml2, xml3) << "Serialization is not idempotent — state drifts across round-trips";
}

// ============================================================================
// Empty State Serialization
// ============================================================================

TEST_F(StateIntegrationTest, EmptyStateSerializationRoundTrip)
{
    // Bug caught: empty state serialization producing invalid XML or
    // deserialization crashing on empty oscillator/pane lists.
    juce::String xml = state->toXmlString();
    ASSERT_FALSE(xml.isEmpty());

    auto restored = std::make_unique<OscilState>();
    bool success = restored->fromXmlString(xml);
    ASSERT_TRUE(success);

    EXPECT_EQ(restored->getOscillators().size(), 0);
    EXPECT_EQ(restored->getLayoutManager().getPaneCount(), 0);
}

// ============================================================================
// Add Oscillator Without Pane
// ============================================================================

TEST_F(StateIntegrationTest, OscillatorWithoutPaneSurvivesRoundTrip)
{
    // Bug caught: oscillators without a pane assignment being dropped
    // during serialization because the pane lookup fails.
    auto osc = OscillatorBuilder()
        .withName("Orphan Osc")
        .withSourceId(SourceId::generate())
        .build();
    // No pane assigned (PaneId is invalid)
    state->addOscillator(osc);

    juce::String xml = state->toXmlString();
    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));

    auto restoredOsc = restored->getOscillator(osc.getId());
    ASSERT_TRUE(restoredOsc.has_value());
    EXPECT_EQ(restoredOsc->getName(), "Orphan Osc");
    EXPECT_FALSE(restoredOsc->getPaneId().isValid());
}

// ============================================================================
// Oscillator Visual Config Survives Round-Trip
// ============================================================================

TEST_F(StateIntegrationTest, OscillatorVisualConfigSurvivesRoundTrip)
{
    // Bug caught: visual config (shader, line width, color) reset to defaults
    // during deserialization because the visual properties are nested.
    auto paneId = addPane("Config Pane");

    auto osc = OscillatorBuilder()
        .withName("Styled Osc")
        .withPaneId(paneId)
        .withSourceId(SourceId::generate())
        .withProcessingMode(ProcessingMode::Mid)
        .build();
    osc.setColour(juce::Colour(0xFFAA5500));
    osc.setLineWidth(3.5f);
    osc.setOpacity(0.65f);
    osc.setShaderId("neon_glow");
    osc.setVisible(false);
    state->addOscillator(osc);

    juce::String xml = state->toXmlString();
    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));

    auto r = restored->getOscillator(osc.getId());
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->getColour().getARGB(), juce::Colour(0xFFAA5500).getARGB());
    EXPECT_FLOAT_EQ(r->getLineWidth(), 3.5f);
    EXPECT_FLOAT_EQ(r->getOpacity(), 0.65f);
    EXPECT_EQ(r->getShaderId(), "neon_glow");
    EXPECT_EQ(r->getProcessingMode(), ProcessingMode::Mid);
    EXPECT_FALSE(r->isVisible());
}

// ============================================================================
// Multiple Panes With Distinct Oscillators After Round-Trip
// ============================================================================

TEST_F(StateIntegrationTest, MultiplePanesOscillatorAssignmentsPreserved)
{
    // Bug caught: all oscillators assigned to the first pane after deserialization
    // because pane IDs are regenerated during load.
    auto p1 = addPane("Pane A", 0);
    auto p2 = addPane("Pane B", 1);
    auto p3 = addPane("Pane C", 2);

    addOscillatorToPane("A1", p1);
    addOscillatorToPane("A2", p1);
    addOscillatorToPane("B1", p2);
    addOscillatorToPane("C1", p3);
    addOscillatorToPane("C2", p3);
    addOscillatorToPane("C3", p3);

    juce::String xml = state->toXmlString();
    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));

    auto oscillators = restored->getOscillators();
    ASSERT_EQ(oscillators.size(), 6);

    // Count oscillators per pane
    std::map<juce::String, int> paneOscCount;
    for (const auto& osc : oscillators)
    {
        auto* pane = restored->getLayoutManager().getPane(osc.getPaneId());
        if (pane != nullptr)
            paneOscCount[pane->getName()]++;
    }

    EXPECT_EQ(paneOscCount["Pane A"], 2);
    EXPECT_EQ(paneOscCount["Pane B"], 1);
    EXPECT_EQ(paneOscCount["Pane C"], 3);
}

// ============================================================================
// Reorder Then Serialize Preserves Order
// ============================================================================

TEST_F(StateIntegrationTest, ReorderThenSerializePreservesOrder)
{
    // Bug caught: orderIndex updated in memory but not in ValueTree,
    // so serialization restores the original order.
    auto paneId = addPane("Main");

    for (int i = 0; i < 5; ++i)
    {
        auto osc = OscillatorBuilder()
            .withName("Osc_" + juce::String(i))
            .withPaneId(paneId)
            .withOrderIndex(i)
            .build();
        state->addOscillator(osc);
    }

    // Move Osc_0 from position 0 to position 4
    state->reorderOscillators(0, 4);

    juce::String xml = state->toXmlString();
    auto restored = std::make_unique<OscilState>();
    ASSERT_TRUE(restored->fromXmlString(xml));

    auto oscillators = restored->getOscillators();
    ASSERT_EQ(oscillators.size(), 5);

    // Sort by orderIndex and verify the reorder was persisted
    std::sort(oscillators.begin(), oscillators.end(),
              [](const Oscillator& a, const Oscillator& b) {
                  return a.getOrderIndex() < b.getOrderIndex();
              });

    // After moving index 0 to 4, the order should be: Osc_1, Osc_2, Osc_3, Osc_4, Osc_0
    EXPECT_EQ(oscillators[0].getName(), "Osc_1");
    EXPECT_EQ(oscillators[4].getName(), "Osc_0");
}
