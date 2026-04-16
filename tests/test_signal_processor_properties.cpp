/*
    Oscil - SignalProcessor Property-Based Tests (rapidcheck)

    Invariants asserted here complement the hand-written tests in
    test_signal_processor.cpp by exercising generated inputs across
    all ProcessingMode values. See docs/decisions/011-property-based-dsp-tests.md.
*/

#include "core/dsp/SignalProcessor.h"

#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <rapidcheck/gtest.h>
#include <utility>
#include <vector>

namespace
{

// Generator: a bounded audio sample in [-1, 1].
// Using a fixed resolution grid keeps shrinking deterministic.
rc::Gen<float> genSample()
{
    return rc::gen::map(rc::gen::inRange(-1000, 1001), [](int value) { return static_cast<float>(value) / 1000.0f; });
}

// Generator: a pair of equal-length buffers in [-1, 1], size in [1, 64].
// Equal length is a hard requirement for all SignalProcessor modes here.
rc::Gen<std::pair<std::vector<float>, std::vector<float>>> genStereoBuffer()
{
    return rc::gen::mapcat(rc::gen::inRange<std::size_t>(1, 65), [](std::size_t n) {
        return rc::gen::pair(rc::gen::container<std::vector<float>>(n, genSample()),
                             rc::gen::container<std::vector<float>>(n, genSample()));
    });
}

float maxAbs(const std::vector<float>& v)
{
    float m = 0.0f;
    for (float const s : v)
        m = std::max(m, std::abs(s));
    return m;
}

// Epsilon tolerance for |(l +/- r) * 0.5|: the intermediate sum can round up
// by up to a couple of ULPs before the multiply. Scale by the input magnitude.
float boundedMagnitudeEpsilon(float inputBound)
{
    return std::max(1.0e-6f, inputBound * 4.0f * std::numeric_limits<float>::epsilon());
}

} // namespace

using namespace oscil;

// ---------------------------------------------------------------------------
// Property: Output magnitude is bounded by input magnitude (all modes).
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SignalProcessorProperties, OutputBoundedByInputFullStereo, ())
{
    auto const stereo = *genStereoBuffer();
    const auto& [left, right] = stereo;

    const SignalProcessor proc;
    ProcessedSignal out;
    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::FullStereo, out);

    float const bound = std::max(maxAbs(left), maxAbs(right));
    float const eps = boundedMagnitudeEpsilon(bound);
    RC_ASSERT(maxAbs(out.channel1) <= bound + eps);
    RC_ASSERT(maxAbs(out.channel2) <= bound + eps);
}

RC_GTEST_PROP(SignalProcessorProperties, OutputBoundedByInputMonoMidSide, ())
{
    auto const stereo = *genStereoBuffer();
    const auto& [left, right] = stereo;

    const SignalProcessor proc;
    ProcessedSignal out;
    float const bound = std::max(maxAbs(left), maxAbs(right));
    float const eps = boundedMagnitudeEpsilon(bound);

    for (auto const mode : {ProcessingMode::Mono, ProcessingMode::Mid, ProcessingMode::Side})
    {
        proc.process(std::span<const float>(left), std::span<const float>(right), mode, out);
        // For Mono/Mid: |(L+R)/2| <= (|L|+|R|)/2 <= max(|L|,|R|).
        // For Side:     |(L-R)/2| <= (|L|+|R|)/2 <= max(|L|,|R|).
        RC_ASSERT(maxAbs(out.channel1) <= bound + eps);
    }
}

RC_GTEST_PROP(SignalProcessorProperties, OutputBoundedByInputLeftRight, ())
{
    auto const stereo = *genStereoBuffer();
    const auto& [left, right] = stereo;

    const SignalProcessor proc;
    ProcessedSignal out;

    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::Left, out);
    RC_ASSERT(maxAbs(out.channel1) <= maxAbs(left));

    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::Right, out);
    // The processor emits |R| (or |L| when right is empty). Bound covers both.
    RC_ASSERT(maxAbs(out.channel1) <= std::max(maxAbs(left), maxAbs(right)));
}

// ---------------------------------------------------------------------------
// Property: Mono-mix is invariant under L/R permutation.
// (L + R) * 0.5f == (R + L) * 0.5f bitwise — IEEE 754 addition commutes.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SignalProcessorProperties, MonoMixSwapInvariant, ())
{
    auto const stereo = *genStereoBuffer();
    const auto& [left, right] = stereo;

    const SignalProcessor proc;
    ProcessedSignal out1;
    ProcessedSignal out2;

    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::Mono, out1);
    proc.process(std::span<const float>(right), std::span<const float>(left), ProcessingMode::Mono, out2);

    RC_ASSERT(out1.numSamples == out2.numSamples);
    for (std::size_t i = 0; i < out1.channel1.size(); ++i)
    {
        RC_ASSERT(out1.channel1[i] == out2.channel1[i]);
    }
}

// ---------------------------------------------------------------------------
// Property: Mid + Side reconstructs L; Mid - Side reconstructs R.
// Mid = (L+R)/2, Side = (L-R)/2 ⇒ L = Mid+Side, R = Mid-Side.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SignalProcessorProperties, MidSideRoundTrip, ())
{
    auto const stereo = *genStereoBuffer();
    const auto& [left, right] = stereo;

    const SignalProcessor proc;
    ProcessedSignal mid;
    ProcessedSignal side;

    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::Mid, mid);
    proc.process(std::span<const float>(left), std::span<const float>(right), ProcessingMode::Side, side);

    float const bound = std::max(maxAbs(left), maxAbs(right));
    float const eps = boundedMagnitudeEpsilon(bound) * 2.0f; // two FP ops: div then add
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        float const reconstructedL = mid.channel1[i] + side.channel1[i];
        float const reconstructedR = mid.channel1[i] - side.channel1[i];
        RC_ASSERT(std::abs(reconstructedL - left[i]) <= eps);
        RC_ASSERT(std::abs(reconstructedR - right[i]) <= eps);
    }
}

// ---------------------------------------------------------------------------
// Property: Empty input produces empty output for every mode (no crash).
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SignalProcessorProperties, EmptyInputProducesEmptyOutput, ())
{
    const SignalProcessor proc;
    ProcessedSignal out;
    std::span<const float> const empty;

    for (auto const mode : {ProcessingMode::FullStereo, ProcessingMode::Mono, ProcessingMode::Mid, ProcessingMode::Side,
                            ProcessingMode::Left, ProcessingMode::Right})
    {
        proc.process(empty, empty, mode, out);
        RC_ASSERT(out.numSamples == 0);
        RC_ASSERT(out.channel1.empty());
        RC_ASSERT(out.channel2.empty());
    }
}
