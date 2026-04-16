/*
    Benchmarks for oscil::SharedCaptureBuffer write/read hot paths.

    Write is the audio-thread hot path; read is the UI/render-thread hot
    path. Both sweep numSamples ∈ {64, 256, 1024, 4096} at 2 channels —
    the set of block sizes DAWs routinely hand to processBlock.

    SetBytesProcessed and SetItemsProcessed are set so bench output can be
    converted to bytes/s and items/s for intuitive comparisons.
*/

#include "core/SharedCaptureBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <benchmark/benchmark.h>
#include <cstddef>
#include <vector>

namespace
{

constexpr int kChannels = 2;
constexpr double kSampleRate = 48000.0;

/// Build a deterministic sine-like test signal so benchmarks process
/// realistic-looking data rather than all-zero inputs (which some toolchains
/// would be tempted to elide).
void fillDeterministic(std::vector<float>& buf, int offset)
{
    for (size_t i = 0; i < buf.size(); ++i)
    {
        // Cheap pseudo-waveform — no trig, no allocations.
        const int idx = static_cast<int>(i) + offset;
        buf[i] = static_cast<float>((idx * 1103515245 + 12345) & 0xFFFF) / 65536.0f - 0.5f;
    }
}

oscil::CaptureFrameMetadata makeMetadata(int numSamples)
{
    oscil::CaptureFrameMetadata meta;
    meta.sampleRate = kSampleRate;
    meta.numChannels = kChannels;
    meta.timestamp = 0;
    meta.numSamples = numSamples;
    meta.isPlaying = true;
    meta.bpm = 120.0;
    return meta;
}

} // namespace

//==============================================================================
// Write: audio-thread hot path.
//==============================================================================
static void BM_SharedCaptureBuffer_Write(benchmark::State& state)
{
    const int numSamples = static_cast<int>(state.range(0));

    oscil::SharedCaptureBuffer buffer;

    std::vector<float> left(static_cast<size_t>(numSamples));
    std::vector<float> right(static_cast<size_t>(numSamples));
    fillDeterministic(left, 0);
    fillDeterministic(right, 7);

    const float* channels[kChannels] = {left.data(), right.data()};
    const auto meta = makeMetadata(numSamples);

    for (auto _ : state)
    {
        // tryLock=false to exercise the non-contended blocking path — matches
        // what PluginProcessor::processBlock does when it has exclusive write
        // ownership.
        buffer.write(channels, numSamples, kChannels, meta, /*tryLock=*/false);
        benchmark::ClobberMemory();
    }

    const int64_t totalSamples = state.iterations() * numSamples * kChannels;
    state.SetItemsProcessed(totalSamples);
    state.SetBytesProcessed(totalSamples * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_SharedCaptureBuffer_Write)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

//==============================================================================
// Read: UI/render-thread hot path.
//==============================================================================
static void BM_SharedCaptureBuffer_Read(benchmark::State& state)
{
    const int numSamples = static_cast<int>(state.range(0));

    oscil::SharedCaptureBuffer buffer;

    // Prime the buffer with one block of data so read() has something to copy.
    std::vector<float> seedL(static_cast<size_t>(numSamples));
    std::vector<float> seedR(static_cast<size_t>(numSamples));
    fillDeterministic(seedL, 0);
    fillDeterministic(seedR, 11);
    const float* seedChannels[kChannels] = {seedL.data(), seedR.data()};
    buffer.write(seedChannels, numSamples, kChannels, makeMetadata(numSamples), /*tryLock=*/false);

    juce::AudioBuffer<float> output(kChannels, numSamples);

    for (auto _ : state)
    {
        // readBlocking measures the UI-thread hot path (the read API split landed
        // post-baseline; readSnapshot is benched in a separate target if needed).
        const int read = buffer.readBlocking(output, numSamples);
        benchmark::DoNotOptimize(read);
        benchmark::ClobberMemory();
    }

    const int64_t totalSamples = state.iterations() * numSamples * kChannels;
    state.SetItemsProcessed(totalSamples);
    state.SetBytesProcessed(totalSamples * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_SharedCaptureBuffer_Read)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
