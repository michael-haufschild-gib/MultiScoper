/*
    Benchmarks for UI-thread analysis helpers reached on every repaint:
    SignalProcessor::calculatePeak, calculateRMS, calculateCorrelation, and
    AdaptiveDecimator::processWithEnvelope.

    These are called per visible oscillator per VBlank tick, so their cost
    scales with screen refresh rate × number of active oscillators. They
    sit on the UI (message) thread, not the audio thread — but high cost
    here starves repaints and eats the GL thread budget.

    Block sizes mirror common displaySamples_ values (256..4096).
*/

#include "core/dsp/SignalProcessor.h"

#include <benchmark/benchmark.h>
#include <cstddef>
#include <span>
#include <vector>

namespace
{

void fillDeterministic(std::vector<float>& buf, int offset)
{
    for (size_t i = 0; i < buf.size(); ++i)
    {
        const int idx = static_cast<int>(i) + offset;
        buf[i] = static_cast<float>((idx * 1103515245 + 12345) & 0xFFFF) / 65536.0f - 0.5f;
    }
}

} // namespace

static void BM_CalculatePeak(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    std::vector<float> buf(static_cast<size_t>(n));
    fillDeterministic(buf, 0);
    const std::span<const float> span(buf);

    for (auto _ : state)
    {
        float peak = oscil::SignalProcessor::calculatePeak(span);
        benchmark::DoNotOptimize(peak);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_CalculatePeak)->Arg(256)->Arg(1024)->Arg(2048)->Arg(4096);

static void BM_CalculateRMS(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    std::vector<float> buf(static_cast<size_t>(n));
    fillDeterministic(buf, 0);
    const std::span<const float> span(buf);

    for (auto _ : state)
    {
        float rms = oscil::SignalProcessor::calculateRMS(span);
        benchmark::DoNotOptimize(rms);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_CalculateRMS)->Arg(256)->Arg(1024)->Arg(2048)->Arg(4096);

static void BM_CalculateCorrelation(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    std::vector<float> left(static_cast<size_t>(n));
    std::vector<float> right(static_cast<size_t>(n));
    fillDeterministic(left, 0);
    fillDeterministic(right, 7);
    const std::span<const float> l(left);
    const std::span<const float> r(right);

    for (auto _ : state)
    {
        float c = oscil::SignalProcessor::calculateCorrelation(l, r);
        benchmark::DoNotOptimize(c);
    }

    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_CalculateCorrelation)->Arg(256)->Arg(1024)->Arg(2048)->Arg(4096);

//==============================================================================
// AdaptiveDecimator::processWithEnvelope — one of the heaviest per-repaint
// consumers: min/max reduction over `samplesPerPixel` wide windows for every
// pixel of the waveform display (displayWidth pixels × tens to hundreds of
// samples per pixel).
//==============================================================================
static void BM_EnvelopeDecimator(benchmark::State& state)
{
    const int inputSamples = static_cast<int>(state.range(0));
    const int displayWidth = static_cast<int>(state.range(1));

    std::vector<float> input(static_cast<size_t>(inputSamples));
    fillDeterministic(input, 0);

    oscil::AdaptiveDecimator decimator;
    decimator.setDisplayWidth(displayWidth);

    std::vector<float> minEnv, maxEnv;
    minEnv.reserve(static_cast<size_t>(displayWidth));
    maxEnv.reserve(static_cast<size_t>(displayWidth));

    const std::span<const float> span(input);

    for (auto _ : state)
    {
        decimator.processWithEnvelope(span, minEnv, maxEnv);
        benchmark::DoNotOptimize(minEnv.data());
        benchmark::DoNotOptimize(maxEnv.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * inputSamples);
    state.SetBytesProcessed(state.iterations() * inputSamples * static_cast<int64_t>(sizeof(float)));
    state.counters["samplesPerPixel"] = static_cast<double>(inputSamples) / displayWidth;
}
// (inputSamples, displayWidth) — realistic combinations.
//   2048 samples /  800 px  ≈ 2.56 spp  (common)
//   4096 samples /  800 px  ≈ 5.12 spp
//   8192 samples / 1600 px  ≈ 5.12 spp  (high-res)
//  16384 samples / 1600 px  ≈ 10.24 spp
BENCHMARK(BM_EnvelopeDecimator)->ArgsProduct({{2048, 4096, 8192, 16384}, {800, 1600}});

//==============================================================================
// AdaptiveDecimator::process — simpler single-value-per-pixel peak-preserving
// decimator used by the GPU path (LineRenderer).
//==============================================================================
static void BM_PeakDecimator(benchmark::State& state)
{
    const int inputSamples = static_cast<int>(state.range(0));
    const int displayWidth = static_cast<int>(state.range(1));

    std::vector<float> input(static_cast<size_t>(inputSamples));
    fillDeterministic(input, 0);

    oscil::AdaptiveDecimator decimator;
    decimator.setDisplayWidth(displayWidth);

    std::vector<float> out;
    out.reserve(static_cast<size_t>(displayWidth * 2));

    const std::span<const float> span(input);

    for (auto _ : state)
    {
        decimator.process(span, out);
        benchmark::DoNotOptimize(out.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * inputSamples);
    state.SetBytesProcessed(state.iterations() * inputSamples * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_PeakDecimator)->ArgsProduct({{2048, 4096, 8192, 16384}, {800, 1600}});
