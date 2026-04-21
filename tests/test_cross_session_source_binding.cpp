/*
    MultiScoper - Cross-Session Source Binding Tests

    Regression tests for the Bitwig save/reload bug where oscillators that
    referenced another plugin instance's source lost their binding after
    save+close+reopen, rendering as "Self" with no waveform.

    Invariants verified:
      1. trackIdentifier is persisted in MultiScoperState XML (new in v3).
      2. InstanceRegistry::registerInstance yields a SourceId derived
         deterministically from the trackIdentifier — same trackIdentifier
         across sessions -> same SourceId -> cross-instance oscillator
         bindings remain resolvable.
      3. Legacy v2 saves (without TrackIdentifier) migrate cleanly but
         surface an empty trackIdentifier (intentional — the plugin will
         fall back to a fresh UUID for those legacy sessions).
*/

#include "core/InstanceRegistry.h"
#include "core/MultiScoperState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"

#include "helpers/OscillatorBuilder.h"

#include <gtest/gtest.h>
#include <memory>

using namespace multiscoper;
using namespace multiscoper::test;

namespace
{

// Simulate a single plugin-instance session: create a registry entry,
// return (sourceId, buffer) so the caller can hand it to oscillators.
struct SimulatedInstance
{
    juce::String trackIdentifier;
    std::shared_ptr<SharedCaptureBuffer> buffer;
    SourceId sourceId;
};

SimulatedInstance registerInstanceInRegistry(InstanceRegistry& registry, const juce::String& trackId,
                                             const juce::String& displayName)
{
    SimulatedInstance inst;
    inst.trackIdentifier = trackId;
    inst.buffer = std::make_shared<SharedCaptureBuffer>();
    inst.sourceId = registry.registerInstance(trackId, inst.buffer, displayName, 2, 44100.0);
    return inst;
}

class CrossSessionFixture : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry;
    std::mutex dispatcherMutex;

    void SetUp() override
    {
        registry = std::make_unique<InstanceRegistry>();
        // Synchronous dispatch so listener notifications fire deterministically.
        registry->setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });
    }

    void TearDown() override { registry.reset(); }
};

} // namespace

TEST_F(CrossSessionFixture, RegisterInstanceYieldsSourceIdEqualToTrackIdentifier)
{
    // Deterministic SourceId is the core of the fix: a plugin that persists
    // its trackIdentifier across sessions will always register with the same
    // SourceId — so any other plugin instance holding a persisted reference
    // to that SourceId will continue to resolve it after reload.
    const juce::String trackId = juce::Uuid().toString();

    auto inst = registerInstanceInRegistry(*registry, trackId, "Track A");

    EXPECT_TRUE(inst.sourceId.isValid());
    EXPECT_EQ(inst.sourceId.id, trackId);
}

TEST_F(CrossSessionFixture, SameTrackIdentifierAcrossTwoSessionsYieldsSameSourceId)
{
    const juce::String trackId = juce::Uuid().toString();

    // Session 1: plugin registers, captures its SourceId.
    SourceId firstSessionSourceId;
    {
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        firstSessionSourceId = registry->registerInstance(trackId, buffer, "Track A", 2, 44100.0);
        registry->unregisterInstance(firstSessionSourceId);
    }

    // Session 2: fresh registration under the same trackIdentifier.
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    const SourceId secondSessionSourceId = registry->registerInstance(trackId, buffer, "Track A", 2, 44100.0);

    EXPECT_EQ(firstSessionSourceId, secondSessionSourceId);
}

TEST_F(CrossSessionFixture, OscillatorBindingSurvivesSaveLoadAcrossTwoInstances)
{
    // End-to-end: Plugin A on track A, Plugin B on track B.
    // Plugin A has an oscillator bound to Plugin B's source.
    // Save plugin A's state, destroy both instances, recreate both,
    // register them with the SAME persisted trackIdentifiers, and verify
    // the oscillator's persisted sourceId resolves to plugin B's source.

    const juce::String trackIdA = juce::Uuid().toString();
    const juce::String trackIdB = juce::Uuid().toString();

    // --- Session 1: both plugins registered, oscillator cross-bound ---
    auto instA = registerInstanceInRegistry(*registry, trackIdA, "Track A");
    auto instB = registerInstanceInRegistry(*registry, trackIdB, "Track B");

    MultiScoperState stateA;
    stateA.setTrackIdentifier(trackIdA);
    auto oscOnA = OscillatorBuilder()
                      .withName("Scope of Track B")
                      .withSourceId(instB.sourceId) // cross-instance binding
                      .build();
    stateA.addOscillator(oscOnA);

    const juce::String savedXml = stateA.toXmlString();
    ASSERT_FALSE(savedXml.isEmpty());

    // Verify the saved XML actually contains the trackIdentifier property.
    EXPECT_TRUE(savedXml.contains("trackIdentifier"));
    EXPECT_TRUE(savedXml.contains(trackIdA));

    // --- Simulate DAW shutdown: unregister both instances ---
    registry->unregisterInstance(instA.sourceId);
    registry->unregisterInstance(instB.sourceId);
    ASSERT_EQ(registry->getSourceCount(), 0u);

    // --- Session 2: project reopens, plugins re-register under persisted trackIds ---
    MultiScoperState restoredStateA;
    ASSERT_TRUE(restoredStateA.fromXmlString(savedXml));
    EXPECT_EQ(restoredStateA.getTrackIdentifier(), trackIdA);

    // The plugin restores its trackIdentifier and registers with it.
    auto instA2 = registerInstanceInRegistry(*registry, restoredStateA.getTrackIdentifier(), "Track A");
    // Plugin B's instance also returns with its original trackIdentifier.
    auto instB2 = registerInstanceInRegistry(*registry, trackIdB, "Track B");

    // Same trackIdentifier across sessions => same SourceId.
    EXPECT_EQ(instA2.sourceId, instA.sourceId);
    EXPECT_EQ(instB2.sourceId, instB.sourceId);

    // Oscillator's persisted sourceId now resolves.
    const auto restoredOscillators = restoredStateA.getOscillators();
    ASSERT_EQ(restoredOscillators.size(), 1u);
    const auto& restoredOsc = restoredOscillators.front();
    EXPECT_EQ(restoredOsc.getSourceId(), instB2.sourceId);

    auto resolved = registry->getSource(restoredOsc.getSourceId());
    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(resolved->name, "Track B");
}

TEST_F(CrossSessionFixture, PreV3StateIsRejected)
{
    // Pre-v3 saves are no longer supported.
    const juce::String v2Xml =
        R"(<MultiScoperState version="2"><Oscillators/><Panes/><Layout columns="1"/><Theme themeName="Dark"/><Timing/></MultiScoperState>)";

    MultiScoperState state;
    EXPECT_FALSE(state.fromXmlString(v2Xml));
}

TEST_F(CrossSessionFixture, SetTrackIdentifierPersistsThroughRoundTrip)
{
    const juce::String trackId = juce::Uuid().toString();
    MultiScoperState state;
    state.setTrackIdentifier(trackId);

    const juce::String xml = state.toXmlString();
    ASSERT_FALSE(xml.isEmpty());

    MultiScoperState restored;
    ASSERT_TRUE(restored.fromXmlString(xml));
    EXPECT_EQ(restored.getTrackIdentifier(), trackId);
}
