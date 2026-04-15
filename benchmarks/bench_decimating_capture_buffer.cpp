/*
    Benchmarks for oscil::DecimatingCaptureBuffer.

    Decimation ratio is derived from (QualityPreset, sourceRate):
      Eco(11025)      / 44100  → ratio 4
      Standard(22050) / 44100  → ratio 2
      High(44100)     / 44100  → ratio 1
      Eco(11025)      / 88200  → ratio 8

    We sweep numSamples ∈ {64, 256, 1024, 4096} per ratio. Args() carries
    (numSamples, decimationRatio); the benchmark picks the matching
    (preset, sourceRate) pair.
*/

#include "core/DecimatingCaptureBuffer.h"
#include "core/SharedCaptureBuffer.h"
#include "core/dsp/CaptureQualityConfig.h"
#include "core/dsp/QualityPreset.h"

#include <benchmark/benchmark.h>
#include <cstddef>
#include <utility>
#include <vector>

namespace
{

constexpr int kChannels = 2;

/// Map target decimation ratio to (QualityPreset, sourceRate). Chosen so
/// that sourceRate / captureRate yields exactly the requested ratio.
std::pair<oscil::QualityPreset, int> ratioToConfig(int ratio)
{
    switch (ratio)
    {
        case 1:
            return {oscil::QualityPreset::High, 44100};
        case 2:
            return {oscil::QualityPreset::Standard, 44100};
        case 4:
            return {oscil::QualityPreset::Eco, 44100};
        case 8:
            return {oscil::QualityPreset::Eco, 88200};
        default:
            return {oscil::QualityPreset::Standard, 44100};
    }
}

void fillDeterministic(std::vector<float>& buf, int offset)
{
    for (size_t i = 0; i < buf.size(); ++i)
    {
        const int idx = static_cast<int>(i) + offset;
        buf[i] = static_cast<float>((idx * 1103515245 + 12345) & 0xFFFF) / 65536.0f - 0.5f;
    }
}

oscil::CaptureFrameMetadata makeMetadata(int numSamples, int sourceRate)
{
    oscil::CaptureFrameMetadata meta;
    meta.sampleRate = static_cast<double>(sourceRate);
    meta.numChannels = kChannels;
    meta.timestamp = 0;
    meta.numSamples = numSamples;
    meta.isPlaying = true;
    meta.bpm = 120.0;
    return meta;
}

} // namespace

//==============================================================================
// Write through the decimation pipeline.
//==============================================================================
static void BM_DecimatingCaptureBuffer_Write(benchmark::State& state)
{
    const int numSamples = static_cast<int>(state.range(0));
    const int ratio = static_cast<int>(state.range(1));

    const auto [preset, sourceRate] = ratioToConfig(ratio);

    oscil::CaptureQualityConfig cfg;
    cfg.qualityPreset = preset;
    cfg.bufferDuration = oscil::BufferDuration::Medium;
    cfg.autoAdjustQuality = false; // pin preset regardless of track count

    oscil::DecimatingCaptureBuffer buffer(cfg, sourceRate);

    std::vector<float> left(static_cast<size_t>(numSamples));
    std::vector<float> right(static_cast<size_t>(numSamples));
    fillDeterministic(left, 0);
    fillDeterministic(right, 3);

    const float* channels[kChannels] = {left.data(), right.data()};
    const auto meta = makeMetadata(numSamples, sourceRate);

    for (auto _ : state)
    {
        buffer.write(channels, numSamples, kChannels, meta);
        benchmark::ClobberMemory();
    }

    // Items processed is measured in input samples (pre-decimation) so
    // throughput comparison across ratios is apples-to-apples with the
    // non-decimating buffer.
    const int64_t totalSamples = state.iterations() * numSamples * kChannels;
    state.SetItemsProcessed(totalSamples);
    state.SetBytesProcessed(totalSamples * static_cast<int64_t>(sizeof(float)));

    state.counters["decimationRatio"] = static_cast<double>(ratio);
    state.counters["sourceRate"] = static_cast<double>(sourceRate);
}
BENCHMARK(BM_DecimatingCaptureBuffer_Write)->ArgsProduct({{64, 256, 1024, 4096}, {1, 2, 4, 8}});
