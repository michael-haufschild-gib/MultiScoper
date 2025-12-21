#include "OscilTestFixtures.h"
#include "core/dsp/TimingEngine.h"
#include <gtest/gtest.h>

namespace oscil
{

class TimingEngineDeserializationTest : public ::testing::Test
{
protected:
    TimingEngine engine;
};

TEST_F(TimingEngineDeserializationTest, LoadValidState)
{
    juce::ValueTree state(TimingIds::Timing);
    state.setProperty(TimingIds::TimingMode, static_cast<int>(TimingMode::MELODIC), nullptr);
    state.setProperty(TimingIds::NoteInterval, static_cast<int>(EngineNoteInterval::NOTE_1_8TH), nullptr);

    engine.fromValueTree(state);

    auto config = engine.getConfig();
    EXPECT_EQ(config.timingMode, TimingMode::MELODIC);
    EXPECT_EQ(config.noteInterval, EngineNoteInterval::NOTE_1_8TH);
}

TEST_F(TimingEngineDeserializationTest, LoadInvalidTimingMode)
{
    juce::ValueTree state(TimingIds::Timing);
    // 999 is invalid
    state.setProperty(TimingIds::TimingMode, 999, nullptr);

    engine.fromValueTree(state);

    auto config = engine.getConfig();
    // With current bug, this might be 999 (undefined).
    // We want it to be TIME (default).
    EXPECT_EQ(config.timingMode, TimingMode::TIME);
}

TEST_F(TimingEngineDeserializationTest, LoadInvalidNoteInterval)
{
    juce::ValueTree state(TimingIds::Timing);
    state.setProperty(TimingIds::TimingMode, static_cast<int>(TimingMode::MELODIC), nullptr);
    state.setProperty(TimingIds::NoteInterval, 999, nullptr);

    engine.fromValueTree(state);

    auto config = engine.getConfig();
    // With current bug, this might be 999.
    // We want it to be NOTE_1_4TH (default).
    EXPECT_EQ(config.noteInterval, EngineNoteInterval::NOTE_1_4TH);
}

TEST_F(TimingEngineDeserializationTest, LoadInvalidTriggerMode)
{
    juce::ValueTree state(TimingIds::Timing);
    state.setProperty(TimingIds::TriggerMode, 999, nullptr);

    engine.fromValueTree(state);

    auto config = engine.getConfig();
    // Default is None
    EXPECT_EQ(config.triggerMode, WaveformTriggerMode::None);
}

}
