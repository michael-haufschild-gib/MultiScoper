/*
    Benchmarks for multiscoper::SignalProcessor::process — one benchmark per
    ProcessingMode, sweeping numSamples ∈ {64, 256, 1024, 4096}.

    SignalProcessor is stateless and invoked on the UI (message) thread.
    ProcessedSignal::resize() explicitly gates its vector allocation on
    isThisTheMessageThread() (see include/core/dsp/SignalProcessor.h); the
    benchmark main installs a MessageManager on this thread so that gate
    passes.
*/

#include "core/Oscillator.h"
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

/// Shared body: build L/R spans and run process() in a loop. Preallocates
/// the ProcessedSignal output buffer on the first call (permitted by the
/// message-thread gate) so subsequent iterations avoid allocation.
void runProcessBenchmark(benchmark::State& state, multiscoper::ProcessingMode mode)
{
    const int numSamples = static_cast<int>(state.range(0));

    std::vector<float> left(static_cast<size_t>(numSamples));
    std::vector<float> right(static_cast<size_t>(numSamples));
    fillDeterministic(left, 0);
    fillDeterministic(right, 5);

    const std::span<const float> leftSpan(left);
    const std::span<const float> rightSpan(right);

    multiscoper::SignalProcessor processor;
    multiscoper::ProcessedSignal output;

    // Pre-size output so steady-state iterations do not reallocate. The
    // first process() call would resize anyway; doing it up front keeps the
    // measurement focused on the processing path rather than on one-shot
    // allocation.
    const bool isStereo = (mode == multiscoper::ProcessingMode::FullStereo);
    output.resize(numSamples, isStereo);

    for (auto _ : state)
    {
        processor.process(leftSpan, rightSpan, mode, output);
        benchmark::DoNotOptimize(output.channel1.data());
        if (isStereo)
            benchmark::DoNotOptimize(output.channel2.data());
        benchmark::ClobberMemory();
    }

    // SignalProcessor reads both channels and writes one or two.
    const int64_t inputSamples = state.iterations() * numSamples * 2;
    state.SetItemsProcessed(inputSamples);
    state.SetBytesProcessed(inputSamples * static_cast<int64_t>(sizeof(float)));
}

} // namespace

static void BM_SignalProcessor_FullStereo(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::FullStereo);
}
BENCHMARK(BM_SignalProcessor_FullStereo)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_SignalProcessor_Mono(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::Mono);
}
BENCHMARK(BM_SignalProcessor_Mono)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_SignalProcessor_Mid(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::Mid);
}
BENCHMARK(BM_SignalProcessor_Mid)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_SignalProcessor_Side(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::Side);
}
BENCHMARK(BM_SignalProcessor_Side)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_SignalProcessor_Left(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::Left);
}
BENCHMARK(BM_SignalProcessor_Left)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);

static void BM_SignalProcessor_Right(benchmark::State& state)
{
    runProcessBenchmark(state, multiscoper::ProcessingMode::Right);
}
BENCHMARK(BM_SignalProcessor_Right)->Arg(64)->Arg(256)->Arg(1024)->Arg(4096);
