/*
    MultiScoper - SharedCaptureBuffer Property-Based Tests (rapidcheck)

    Ring-buffer correctness invariants.
    See docs/decisions/011-property-based-dsp-tests.md.
*/

#include "core/SharedCaptureBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include <vector>

using namespace multiscoper;

namespace
{

// Generator: a bounded audio sample in [-1, 1].
rc::Gen<float> genSample()
{
    return rc::gen::map(rc::gen::inRange(-1000, 1001), [](int value) { return static_cast<float>(value) / 1000.0f; });
}

// Generator: a power-of-two capacity in {64, 128, 256, 512, 1024}.
// SharedCaptureBuffer rounds up to the next power of two, so starting from
// one keeps test reasoning tight.
rc::Gen<std::size_t> genCapacityPow2()
{
    return rc::gen::map(rc::gen::inRange<int>(6, 11), // 2^6 = 64 .. 2^10 = 1024
                        [](int e) { return std::size_t{1} << static_cast<unsigned>(e); });
}

// Build an AudioBuffer containing the given per-channel sample sequences.
// Caller guarantees all channels have equal length.
juce::AudioBuffer<float> buildBuffer(const std::vector<std::vector<float>>& channels)
{
    int const numChannels = static_cast<int>(channels.size());
    int const numSamples = channels.empty() ? 0 : static_cast<int>(channels[0].size());
    juce::AudioBuffer<float> buf(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dest = buf.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            dest[i] = channels[static_cast<std::size_t>(ch)][static_cast<std::size_t>(i)];
    }
    return buf;
}

} // namespace

// ---------------------------------------------------------------------------
// Property: Read-after-write returns the written sequence in order.
// For any N <= capacity, writing N samples then read(N) yields the same
// values at the same positions.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SharedCaptureBufferProperties, ReadAfterWritePreservesOrder, ())
{
    std::size_t const capacity = *genCapacityPow2();
    std::size_t const n = *rc::gen::inRange<std::size_t>(1, capacity + 1);

    auto const leftSeq = *rc::gen::container<std::vector<float>>(n, genSample());
    auto const rightSeq = *rc::gen::container<std::vector<float>>(n, genSample());

    SharedCaptureBuffer buffer(capacity);

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    meta.numSamples = static_cast<int>(n);

    auto const audio = buildBuffer({leftSeq, rightSeq});
    buffer.write(audio, meta);

    std::vector<float> outL(n, 0.0f);
    std::vector<float> outR(n, 0.0f);
    int const gotL = buffer.readBlocking(outL.data(), static_cast<int>(n), 0);
    int const gotR = buffer.readBlocking(outR.data(), static_cast<int>(n), 1);

    RC_ASSERT(static_cast<std::size_t>(gotL) == n);
    RC_ASSERT(static_cast<std::size_t>(gotR) == n);
    for (std::size_t i = 0; i < n; ++i)
    {
        RC_ASSERT(outL[i] == leftSeq[i]);
        RC_ASSERT(outR[i] == rightSeq[i]);
    }
}

// ---------------------------------------------------------------------------
// Property: Wrap-around returns the most recently written samples.
// Writing 2 * capacity samples and reading `capacity` returns the
// last `capacity` samples written (not the first).
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SharedCaptureBufferProperties, WrapAroundReturnsLatestSamples, ())
{
    std::size_t const capacity = *genCapacityPow2();
    std::size_t const total = capacity * 2;

    auto const seq = *rc::gen::container<std::vector<float>>(total, genSample());
    // Zero right channel to keep the assertion simple; channel independence is
    // asserted in its own property below.
    std::vector<float> const zeros(total, 0.0f);

    SharedCaptureBuffer buffer(capacity);

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    meta.numSamples = static_cast<int>(total);

    auto const audio = buildBuffer({seq, zeros});
    buffer.write(audio, meta);

    std::vector<float> out(capacity, 0.0f);
    int const got = buffer.readBlocking(out.data(), static_cast<int>(capacity), 0);
    RC_ASSERT(static_cast<std::size_t>(got) == capacity);

    // Expect the last `capacity` samples of `seq`.
    for (std::size_t i = 0; i < capacity; ++i)
    {
        RC_ASSERT(out[i] == seq[capacity + i]);
    }
}

// ---------------------------------------------------------------------------
// Property: Channels are independent.
// Different sequences on L and R come back per-channel unmixed.
// ---------------------------------------------------------------------------

RC_GTEST_PROP(SharedCaptureBufferProperties, ChannelsAreIndependent, ())
{
    std::size_t const capacity = *genCapacityPow2();
    std::size_t const n = *rc::gen::inRange<std::size_t>(1, capacity + 1);

    auto const leftSeq = *rc::gen::container<std::vector<float>>(n, genSample());
    auto rightSeq = *rc::gen::container<std::vector<float>>(n, genSample());
    // Force L[i] != R[i] for every i by perturbing when equal — guarantees
    // the property actually detects cross-channel contamination.
    for (std::size_t i = 0; i < n; ++i)
    {
        if (rightSeq[i] == leftSeq[i])
            rightSeq[i] = leftSeq[i] + 0.5f;
    }

    SharedCaptureBuffer buffer(capacity);

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    meta.numSamples = static_cast<int>(n);

    auto const audio = buildBuffer({leftSeq, rightSeq});
    buffer.write(audio, meta);

    std::vector<float> outL(n, 0.0f);
    std::vector<float> outR(n, 0.0f);
    (void) buffer.readBlocking(outL.data(), static_cast<int>(n), 0);
    (void) buffer.readBlocking(outR.data(), static_cast<int>(n), 1);

    for (std::size_t i = 0; i < n; ++i)
    {
        RC_ASSERT(outL[i] == leftSeq[i]);
        RC_ASSERT(outR[i] == rightSeq[i]);
        RC_ASSERT(outL[i] != outR[i]);
    }
}
