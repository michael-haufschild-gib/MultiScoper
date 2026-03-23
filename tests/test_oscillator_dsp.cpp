/*
    Oscil - Oscillator DSP-Related Tests
    Tests for processing mode semantics, trace behavior, string conversion,
    line width constraints, and interaction between mode and rendering state.
*/

#include "core/Oscillator.h"

#include <gtest/gtest.h>
#include <limits>

using namespace oscil;

class OscillatorDspTest : public ::testing::Test
{
};

// =============================================================================
// Single-trace detection: determines stereo vs mono rendering path
// =============================================================================

// Bug caught: isSingleTrace returns wrong value for a mode, causing stereo
// waveform to render as mono or vice versa — user sees half the data.
TEST_F(OscillatorDspTest, FullStereoIsNotSingleTrace)
{
    Oscillator osc;
    osc.setProcessingMode(ProcessingMode::FullStereo);
    EXPECT_FALSE(osc.isSingleTrace());
}

TEST_F(OscillatorDspTest, AllNonStereoModesAreSingleTrace)
{
    Oscillator osc;

    const ProcessingMode singleTraceModes[] = {ProcessingMode::Mono, ProcessingMode::Mid, ProcessingMode::Side,
                                               ProcessingMode::Left, ProcessingMode::Right};

    for (auto mode : singleTraceModes)
    {
        osc.setProcessingMode(mode);
        EXPECT_TRUE(osc.isSingleTrace()) << processingModeToString(mode).toStdString() << " should be single-trace";
    }
}

// =============================================================================
// Processing mode string conversion: used in serialization and UI
// =============================================================================

// Bug caught: round-trip through string/enum fails for a mode, causing
// deserialization to silently default to FullStereo. User loads a project
// and their Mid/Side oscillators are gone.
TEST_F(OscillatorDspTest, ProcessingModeStringRoundTripForAllModes)
{
    const ProcessingMode allModes[] = {ProcessingMode::FullStereo, ProcessingMode::Mono, ProcessingMode::Mid,
                                       ProcessingMode::Side,       ProcessingMode::Left, ProcessingMode::Right};

    for (auto mode : allModes)
    {
        juce::String str = processingModeToString(mode);
        EXPECT_FALSE(str.isEmpty()) << "Mode " << static_cast<int>(mode) << " has empty string representation";

        ProcessingMode roundTripped = stringToProcessingMode(str);
        EXPECT_EQ(roundTripped, mode) << "Round-trip failed for '" << str.toStdString() << "'";
    }
}

// Bug caught: unknown string doesn't default safely, returns uninitialized
// enum value that happens to work in debug builds but crashes in release.
TEST_F(OscillatorDspTest, UnknownStringDefaultsToFullStereo)
{
    EXPECT_EQ(stringToProcessingMode("Unknown"), ProcessingMode::FullStereo);
    EXPECT_EQ(stringToProcessingMode(""), ProcessingMode::FullStereo);
    EXPECT_EQ(stringToProcessingMode("fullstereo"), ProcessingMode::FullStereo); // case mismatch
}

// =============================================================================
// Line width constraints: rendering uses line width directly in GL calls
// =============================================================================

// Bug caught: negative line width passed to OpenGL causes rendering artifacts
// or GPU driver crash.
TEST_F(OscillatorDspTest, LineWidthClampedToValidRange)
{
    Oscillator osc;

    osc.setLineWidth(-5.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);

    osc.setLineWidth(0.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);

    osc.setLineWidth(100.0f);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);

    osc.setLineWidth(Oscillator::MIN_LINE_WIDTH);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);

    osc.setLineWidth(Oscillator::MAX_LINE_WIDTH);
    EXPECT_EQ(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
}

// Bug caught: NaN line width bypasses clamping and propagates to shader
// uniforms, causing all waveforms to disappear.
TEST_F(OscillatorDspTest, NaNLineWidthFallsBackToDefault)
{
    Oscillator osc;
    osc.setLineWidth(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(osc.getLineWidth(), Oscillator::DEFAULT_LINE_WIDTH);
}

// Bug caught: Infinity line width clamped but the clamping logic uses
// comparison that doesn't handle infinity correctly.
TEST_F(OscillatorDspTest, InfinityLineWidthClamped)
{
    Oscillator osc;
    osc.setLineWidth(std::numeric_limits<float>::infinity());
    EXPECT_LE(osc.getLineWidth(), Oscillator::MAX_LINE_WIDTH);
    EXPECT_TRUE(std::isfinite(osc.getLineWidth()));

    osc.setLineWidth(-std::numeric_limits<float>::infinity());
    EXPECT_GE(osc.getLineWidth(), Oscillator::MIN_LINE_WIDTH);
    EXPECT_TRUE(std::isfinite(osc.getLineWidth()));
}

// =============================================================================
// Processing mode serialization: must survive round-trip through ValueTree
// =============================================================================

// Bug caught: processing mode serialized as integer but deserialized as
// string (or vice versa), causing silent mode change on project reload.
TEST_F(OscillatorDspTest, ProcessingModeSerializationRoundTrip)
{
    const ProcessingMode allModes[] = {ProcessingMode::FullStereo, ProcessingMode::Mono, ProcessingMode::Mid,
                                       ProcessingMode::Side,       ProcessingMode::Left, ProcessingMode::Right};

    for (auto mode : allModes)
    {
        Oscillator original;
        original.setProcessingMode(mode);

        auto valueTree = original.toValueTree();
        Oscillator restored(valueTree);

        EXPECT_EQ(restored.getProcessingMode(), mode)
            << "Processing mode " << processingModeToString(mode).toStdString()
            << " did not survive serialization round-trip";
    }
}

// =============================================================================
// Processing mode + visibility interaction
// =============================================================================

// Bug caught: hidden oscillator still considered for rendering allocation,
// wasting GPU resources. Verify that visibility is independent of mode.
TEST_F(OscillatorDspTest, VisibilityIndependentOfProcessingMode)
{
    Oscillator osc;

    osc.setProcessingMode(ProcessingMode::Mid);
    osc.setVisible(false);
    EXPECT_FALSE(osc.isVisible());
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mid);

    osc.setVisible(true);
    EXPECT_TRUE(osc.isVisible());
    EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mid);
}

// Bug caught: changing processing mode resets visibility to default.
TEST_F(OscillatorDspTest, ProcessingModeChangePreservesVisibility)
{
    Oscillator osc;
    osc.setVisible(false);

    osc.setProcessingMode(ProcessingMode::Side);
    EXPECT_FALSE(osc.isVisible()) << "Mode change should not reset visibility";

    osc.setProcessingMode(ProcessingMode::FullStereo);
    EXPECT_FALSE(osc.isVisible()) << "Mode change should not reset visibility";
}

// Bug caught: changing processing mode resets line width or opacity to defaults.
TEST_F(OscillatorDspTest, ProcessingModeChangePreservesDisplayProperties)
{
    Oscillator osc;
    osc.setLineWidth(3.0f);
    osc.setOpacity(0.6f);
    osc.setColour(juce::Colours::cyan);

    osc.setProcessingMode(ProcessingMode::Mid);

    EXPECT_FLOAT_EQ(osc.getLineWidth(), 3.0f);
    EXPECT_FLOAT_EQ(osc.getOpacity(), 0.6f);
    EXPECT_EQ(osc.getColour(), juce::Colours::cyan);
}
