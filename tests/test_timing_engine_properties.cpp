/*
    Oscil - TimingEngine Property-Based Tests (rapidcheck)

    Invariants for TimingEngine::getDisplaySampleCount and related accessors.
    See docs/decisions/011-property-based-dsp-tests.md.

    Note: TimingEngine does not expose a direct "time -> sample index" API.
    It exposes getDisplaySampleCount(sampleRate), which multiplies the
    configured window size (derived from timeIntervalMs in TIME mode) by
    the sample rate. The properties below target that composite behavior,
    which is the user-facing invariant that matters for waveform display.
*/

#include "core/dsp/TimingEngine.h"
#include "core/dsp/TimingEngineTypes.h"

#include <cmath>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

using namespace oscil;

namespace
{

// Generator: a legal time interval in milliseconds, bounded by
// TimingEngine's clamping window [MIN_TIME_INTERVAL_MS, MAX_TIME_INTERVAL_MS].
rc::Gen<float> genIntervalMs()
{
    // Integer grid keeps shrinks deterministic; map to float in [0.1, 4000].
    return rc::gen::map(rc::gen::inRange(1, 40000), [](int tenths) { return static_cast<float>(tenths) / 10.0f; });
}

// Generator: a realistic audio sample rate.
rc::Gen<double> genSampleRate()
{
    return rc::gen::element<double>(22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0);
}

} // namespace

// ---------------------------------------------------------------------------
// Property: Monotonicity in interval.
// If a <= b and both are within the clamp window, sample count also satisfies
// getDisplaySampleCount(a) <= getDisplaySampleCount(b).
// ---------------------------------------------------------------------------

RC_GTEST_PROP(TimingEngineProperties, SampleCountMonotoneInInterval, ())
{
    float const a = *genIntervalMs();
    float const b = *genIntervalMs();
    double const sr = *genSampleRate();

    float const lo = std::min(a, b);
    float const hi = std::max(a, b);

    TimingEngine engine;
    engine.setTimeIntervalMs(lo);
    int const nLo = engine.getDisplaySampleCount(sr);

    engine.setTimeIntervalMs(hi);
    int const nHi = engine.getDisplaySampleCount(sr);

    RC_ASSERT(nLo <= nHi);
}

// ---------------------------------------------------------------------------
// Property: Sample-rate proportionality.
// Doubling the sample rate approximately doubles the displayed sample count,
// modulo the integer truncation that getDisplaySampleCount performs.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(TimingEngineProperties, SampleCountProportionalToSampleRate, ())
{
    float const ms = *genIntervalMs();
    double const baseRate = *rc::gen::element<double>(22050.0, 44100.0, 48000.0, 96000.0);

    TimingEngine engine;
    engine.setTimeIntervalMs(ms);

    int const nBase = engine.getDisplaySampleCount(baseRate);
    int const nDouble = engine.getDisplaySampleCount(baseRate * 2.0);

    // nDouble should be within 1 of 2 * nBase (integer truncation on each).
    // The window is (ms / 1000.0), so n = int(ms * rate / 1000.0).
    // int(2x) - 2*int(x) is in {-1, 0, 1} for non-negative real x.
    long const diff = static_cast<long>(nDouble) - 2L * static_cast<long>(nBase);
    RC_ASSERT(diff >= -1 && diff <= 1);
}

// ---------------------------------------------------------------------------
// Property: Zero sample rate yields zero displayed sample count.
// (The "zero-time -> zero-index" analog: with no samples per second,
// no samples fit into any finite window.)
// ---------------------------------------------------------------------------

RC_GTEST_PROP(TimingEngineProperties, ZeroSampleRateYieldsZeroSamples, ())
{
    float const ms = *genIntervalMs();

    TimingEngine engine;
    engine.setTimeIntervalMs(ms);

    RC_ASSERT(engine.getDisplaySampleCount(0.0) == 0);
}
