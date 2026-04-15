/*
    Benchmarks for oscil::TimingEngine per-block update paths.

    Two hot paths exercised together in processBlock:
      1. updateHostInfo(PositionInfo) — SeqLock-published host state
      2. processBlock(AudioBuffer) — trigger detection over the block

    Swept block sizes match common DAW choices (64, 128, 256, 512, 1024).
*/

#include "core/dsp/TimingEngine.h"
#include "core/dsp/TimingEngineTypes.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <benchmark/benchmark.h>
#include <cstddef>

namespace
{

juce::AudioPlayHead::PositionInfo makePositionInfo(int64_t samplePos, double bpm)
{
    juce::AudioPlayHead::PositionInfo info;
    info.setIsPlaying(true);
    info.setBpm(bpm);
    info.setTimeInSamples(samplePos);
    juce::AudioPlayHead::TimeSignature ts;
    ts.numerator = 4;
    ts.denominator = 4;
    info.setTimeSignature(ts);
    return info;
}

void fillDeterministic(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const int idx = i + ch * 17;
            data[i] = static_cast<float>((idx * 1103515245 + 12345) & 0xFFFF) / 65536.0f - 0.5f;
        }
    }
}

} // namespace

//==============================================================================
// Per-block update: exercises updateHostInfo + processBlock + sample-index
// propagation for a trigger-enabled configuration.
//==============================================================================
static void BM_TimingEngine_Update(benchmark::State& state)
{
    const int numSamples = static_cast<int>(state.range(0));

    oscil::TimingEngine engine;
    engine.setSampleRate(48000.0);
    engine.setTimingMode(oscil::TimingMode::TIME);
    engine.setTimeIntervalMs(500.0f);
    // Exercise the trigger path so processBlock does useful work rather than
    // short-circuiting in WaveformTriggerMode::None.
    engine.setWaveformTriggerMode(oscil::WaveformTriggerMode::RisingEdge);
    engine.setTriggerThreshold(0.05f);
    engine.setTriggerChannel(0);
    engine.setTriggerHysteresis(0.01f);

    juce::AudioBuffer<float> buffer(2, numSamples);
    fillDeterministic(buffer);

    int64_t samplePos = 0;
    double bpm = 120.0;

    for (auto _ : state)
    {
        auto info = makePositionInfo(samplePos, bpm);
        engine.updateHostInfo(info);
        const bool triggered = engine.processBlock(buffer);
        benchmark::DoNotOptimize(triggered);

        samplePos += numSamples;
        benchmark::ClobberMemory();
    }

    const int64_t totalSamples = state.iterations() * numSamples;
    state.SetItemsProcessed(totalSamples);
    state.SetBytesProcessed(totalSamples * static_cast<int64_t>(sizeof(float)) * 2);
}
BENCHMARK(BM_TimingEngine_Update)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);
