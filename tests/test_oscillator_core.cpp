/*
    Oscil - Oscillator Core Tests
*/

#include "core/OscilState.h"
#include "core/Oscillator.h"

#include <gtest/gtest.h>

using namespace oscil;

class OscillatorCoreTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Oscillator ID generation
TEST_F(OscillatorCoreTest, IdGeneration)
{
    Oscillator osc1;
    Oscillator osc2;

    EXPECT_TRUE(osc1.getId().isValid());
    EXPECT_TRUE(osc2.getId().isValid());
    EXPECT_NE(osc1.getId(), osc2.getId());
}

// Test: Default values
TEST_F(OscillatorCoreTest, DefaultValues)
{
    Oscillator osc;

    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::FullStereo);
    EXPECT_TRUE(osc.isVisible());
    EXPECT_EQ(osc.getOpacity(), 1.0f);
    EXPECT_EQ(osc.getOrderIndex(), 0);
    EXPECT_EQ(osc.getShaderId(), "basic");
    EXPECT_EQ(osc.getVisualPresetId(), "default");
}

// Test: Setters and getters
TEST_F(OscillatorCoreTest, SettersAndGetters)
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
TEST_F(OscillatorCoreTest, OpacityClamping)
{
    Oscillator osc;

    osc.setOpacity(-0.5f);
    EXPECT_EQ(osc.getOpacity(), 0.0f);

    osc.setOpacity(1.5f);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
}

// Test: Effective colour (with opacity)
TEST_F(OscillatorCoreTest, EffectiveColour)
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

// Test: Serialization round-trip
TEST_F(OscillatorCoreTest, SerializationRoundTrip)
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

// Test: Schema version migration
TEST_F(OscillatorCoreTest, SchemaV1Migration)
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

TEST_F(OscillatorCoreTest, SchemaV1MigrationNoSource)
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
TEST_F(OscillatorCoreTest, DeserializeWrongType)
{
    juce::ValueTree wrongType("WrongType");
    wrongType.setProperty("id", "test-id", nullptr);

    Oscillator osc;
    OscillatorId originalId = osc.getId();
    osc.fromValueTree(wrongType);

    // Should not load from wrong type
    EXPECT_EQ(osc.getId(), originalId);
}

TEST_F(OscillatorCoreTest, DeserializeWithMissingProperties)
{
    juce::ValueTree minimal("Oscillator");
    // Only has the type, no properties

    Oscillator osc(minimal);

    // Should have defaults
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::FullStereo);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
    EXPECT_TRUE(osc.isVisible());
}

TEST_F(OscillatorCoreTest, DeserializeWithOutOfRangeValues)
{
    juce::ValueTree state("Oscillator");
    state.setProperty("lineWidth", 1000.0f, nullptr);
    state.setProperty("opacity", 5.0f, nullptr);

    Oscillator osc(state);

    // Values should be clamped
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
    EXPECT_EQ(osc.getOpacity(), 1.0f);
}

// Test: Extreme order indices
TEST_F(OscillatorCoreTest, ExtremeOrderIndexNegative)
{
    Oscillator osc;
    osc.setOrderIndex(-100);
    EXPECT_EQ(osc.getOrderIndex(), -100);
}

TEST_F(OscillatorCoreTest, ExtremeOrderIndexLarge)
{
    Oscillator osc;
    osc.setOrderIndex(INT_MAX);
    EXPECT_EQ(osc.getOrderIndex(), INT_MAX);
}

// Test: Invalid OscillatorId handling
TEST_F(OscillatorCoreTest, InvalidOscillatorId)
{
    OscillatorId invalid = OscillatorId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST_F(OscillatorCoreTest, OscillatorIdEquality)
{
    OscillatorId id1 = OscillatorId::generate();
    OscillatorId id2 = OscillatorId::generate();
    OscillatorId id1Copy;
    id1Copy.id = id1.id;

    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1Copy);
}

// Test: Name length edge cases
TEST_F(OscillatorCoreTest, NameMaxLength)
{
    Oscillator osc;

    // Create a name longer than MAX_NAME_LENGTH (32)
    juce::String longName;
    for (int i = 0; i < 50; ++i)
        longName += "X";

    osc.setName(longName);
    EXPECT_EQ(osc.getName().length(), Oscillator::MAX_NAME_LENGTH);
}

TEST_F(OscillatorCoreTest, NameMinLength)
{
    Oscillator osc;
    osc.setName("A");
    EXPECT_EQ(osc.getName(), "A");
    EXPECT_TRUE(Oscillator::isValidName("A"));
}

TEST_F(OscillatorCoreTest, NameEmptyIsInvalid)
{
    EXPECT_FALSE(Oscillator::isValidName(""));
}

TEST_F(OscillatorCoreTest, NameExactlyMaxLength)
{
    juce::String exactName;
    for (int i = 0; i < Oscillator::MAX_NAME_LENGTH; ++i)
        exactName += "X";

    EXPECT_TRUE(Oscillator::isValidName(exactName));

    Oscillator osc;
    osc.setName(exactName);
    EXPECT_EQ(osc.getName().length(), Oscillator::MAX_NAME_LENGTH);
}

// Test: TimeWindow optional handling
TEST_F(OscillatorCoreTest, TimeWindowDefault)
{
    Oscillator osc;
    EXPECT_FALSE(osc.getTimeWindow().has_value());
}

TEST_F(OscillatorCoreTest, TimeWindowSetValue)
{
    Oscillator osc;
    osc.setTimeWindow(0.5f);
    EXPECT_TRUE(osc.getTimeWindow().has_value());
    EXPECT_FLOAT_EQ(osc.getTimeWindow().value(), 0.5f);
}

TEST_F(OscillatorCoreTest, TimeWindowClear)
{
    Oscillator osc;
    osc.setTimeWindow(1.0f);
    EXPECT_TRUE(osc.getTimeWindow().has_value());

    osc.setTimeWindow(std::nullopt);
    EXPECT_FALSE(osc.getTimeWindow().has_value());
}

TEST_F(OscillatorCoreTest, TimeWindowSerializationRoundTrip)
{
    Oscillator original;
    original.setTimeWindow(2.5f);

    auto valueTree = original.toValueTree();
    Oscillator restored(valueTree);

    EXPECT_TRUE(restored.getTimeWindow().has_value());
    EXPECT_FLOAT_EQ(restored.getTimeWindow().value(), 2.5f);
}

TEST_F(OscillatorCoreTest, TimeWindowNullSerializationRoundTrip)
{
    Oscillator original;
    // TimeWindow is not set (nullopt)

    auto valueTree = original.toValueTree();
    Oscillator restored(valueTree);

    EXPECT_FALSE(restored.getTimeWindow().has_value());
}

// Test: Source state transitions
TEST_F(OscillatorCoreTest, ClearSource)
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

TEST_F(OscillatorCoreTest, SetSourceIdTransitionsState)
{
    Oscillator osc;
    EXPECT_EQ(osc.getState(), OscillatorState::NO_SOURCE);

    osc.setSourceId(SourceId::generate());
    EXPECT_EQ(osc.getState(), OscillatorState::ACTIVE);

    osc.setSourceId(SourceId::invalid());
    EXPECT_EQ(osc.getState(), OscillatorState::NO_SOURCE);
}

TEST_F(OscillatorCoreTest, ClearSourcePreservesConfiguration)
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

// Test: Hash functions for IDs
TEST_F(OscillatorCoreTest, OscillatorIdHash)
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
