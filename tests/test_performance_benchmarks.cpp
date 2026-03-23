/*
    Oscil - Performance Benchmarks
    Regression detection for render-loop-critical data paths.

    Threshold policy: each threshold is ~3-5x the typical measured time
    on an Apple M-series CPU. This catches significant regressions while
    allowing CI runners (which are slower) to pass without flakes.
    If a threshold trips on CI, measure the actual time before raising it.
*/

#include "core/Oscillator.h"
#include "core/SeqLock.h"
#include "core/SharedCaptureBuffer.h"

#include "rendering/VisualConfiguration.h"

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>

using namespace oscil;

// ============================================================================
// SharedCaptureBuffer Throughput
// ============================================================================

TEST(PerformanceBenchmarks, SharedCaptureBufferWriteThroughput)
{
    SharedCaptureBuffer buffer(1048576);
    constexpr int kNumChannels = 2;
    constexpr int kBlockSize = 512;
    constexpr int kIterations = 10000;

    juce::AudioBuffer<float> testBuf(kNumChannels, kBlockSize);
    for (int ch = 0; ch < kNumChannels; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            testBuf.setSample(ch, i, std::sin(static_cast<float>(i) * 0.01f));

    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = kNumChannels;
    meta.numSamples = kBlockSize;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
        buffer.write(testBuf, meta);
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "[PERF] SharedCaptureBuffer write: " << kIterations << " x " << kBlockSize << " samples in " << us
              << " us (" << (us / static_cast<double>(kIterations)) << " us/block)" << std::endl;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(ms, 10) << "Write throughput too slow: " << ms << " ms for " << kIterations << " blocks";
}

TEST(PerformanceBenchmarks, SharedCaptureBufferReadThroughput)
{
    SharedCaptureBuffer buffer(1048576);
    constexpr int kNumChannels = 2;
    constexpr int kBlockSize = 512;
    constexpr int kReadSize = 2048;
    constexpr int kIterations = 10000;

    // Pre-fill the buffer with enough data for reads
    juce::AudioBuffer<float> writeBuf(kNumChannels, kBlockSize);
    for (int ch = 0; ch < kNumChannels; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            writeBuf.setSample(ch, i, std::sin(static_cast<float>(i) * 0.01f));

    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = kNumChannels;
    meta.numSamples = kBlockSize;

    // Write enough data so reads have material to return
    for (int i = 0; i < 100; ++i)
        buffer.write(writeBuf, meta);

    std::vector<float> readBuf(kReadSize);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
        buffer.read(readBuf.data(), kReadSize, 0);
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "[PERF] SharedCaptureBuffer read: " << kIterations << " x " << kReadSize << " samples in " << us
              << " us (" << (us / static_cast<double>(kIterations)) << " us/block)" << std::endl;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(ms, 20) << "Read throughput too slow: " << ms << " ms for " << kIterations << " blocks";
}

// ============================================================================
// VisualConfiguration Serialization
// ============================================================================

TEST(PerformanceBenchmarks, VisualConfigurationSerializationRoundTrip)
{
    constexpr int kIterations = 1000;

    // Configure a non-trivial config (multiple effects enabled)
    VisualConfiguration config;
    config.shaderType = ShaderType::NeonGlow;
    config.bloom.enabled = true;
    config.bloom.intensity = 1.5f;
    config.bloom.iterations = 6;
    config.trails.enabled = true;
    config.trails.decay = 0.15f;
    config.colorGrade.enabled = true;
    config.colorGrade.contrast = 1.3f;
    config.scanlines.enabled = true;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        juce::ValueTree tree = config.toValueTree();
        VisualConfiguration restored = VisualConfiguration::fromValueTree(tree);
        // Prevent compiler from optimizing away the round-trip
        if (restored.bloom.intensity < -1.0f)
            std::abort();
    }
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "[PERF] VisualConfiguration round-trip: " << kIterations << " iterations in " << us << " us ("
              << (us / static_cast<double>(kIterations)) << " us/iter)" << std::endl;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(ms, 500) << "Serialization too slow: " << ms << " ms for " << kIterations << " round-trips";
}

// ============================================================================
// Oscillator ValueTree Serialization
// ============================================================================

TEST(PerformanceBenchmarks, OscillatorSerializationRoundTrip)
{
    constexpr int kIterations = 100;

    // Create oscillators with varied configurations
    std::vector<Oscillator> oscillators;
    oscillators.reserve(kIterations);
    for (int i = 0; i < kIterations; ++i)
    {
        Oscillator osc;
        osc.setName("Benchmark_" + juce::String(i));
        osc.setProcessingMode(static_cast<ProcessingMode>(i % 6));
        osc.setColour(juce::Colour::fromHSV(static_cast<float>(i) / kIterations, 0.8f, 0.9f, 1.0f));
        osc.setLineWidth(1.0f + (i % 10) * 0.4f);
        osc.setOpacity(0.5f + (i % 5) * 0.1f);
        osc.setVisible(i % 3 != 0);
        osc.setShaderId(i % 2 == 0 ? "basic" : "neon_glow");
        oscillators.push_back(std::move(osc));
    }

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        juce::ValueTree tree = oscillators[i].toValueTree();
        Oscillator restored(tree);
        // Prevent compiler from optimizing away
        if (restored.getName().length() > 10000)
            std::abort();
    }
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "[PERF] Oscillator round-trip: " << kIterations << " iterations in " << us << " us ("
              << (us / static_cast<double>(kIterations)) << " us/iter)" << std::endl;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(ms, 20) << "Oscillator serialization too slow: " << ms << " ms for " << kIterations << " round-trips";
}

// ============================================================================
// SeqLock Throughput
// ============================================================================

TEST(PerformanceBenchmarks, SeqLockWriteReadThroughput)
{
    constexpr int kIterations = 100000;

    SeqLock<CaptureFrameMetadata> seqLock;
    CaptureFrameMetadata meta{};
    meta.sampleRate = 44100.0;
    meta.numChannels = 2;
    meta.isPlaying = true;
    meta.bpm = 120.0;

    // Measure write throughput
    auto startWrite = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        meta.timestamp = i;
        meta.numSamples = 512 + (i % 256);
        seqLock.write(meta);
    }
    auto elapsedWrite = std::chrono::steady_clock::now() - startWrite;

    auto writeUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsedWrite).count();
    std::cout << "[PERF] SeqLock write: " << kIterations << " iterations in " << writeUs << " us ("
              << (writeUs / static_cast<double>(kIterations) * 1000.0) << " ns/write)" << std::endl;

    // Measure read throughput
    auto startRead = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        CaptureFrameMetadata snapshot = seqLock.read();
        // Prevent compiler from optimizing away
        if (snapshot.sampleRate < 0.0)
            std::abort();
    }
    auto elapsedRead = std::chrono::steady_clock::now() - startRead;

    auto readUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsedRead).count();
    std::cout << "[PERF] SeqLock read: " << kIterations << " iterations in " << readUs << " us ("
              << (readUs / static_cast<double>(kIterations) * 1000.0) << " ns/read)" << std::endl;

    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedWrite + elapsedRead).count();
    EXPECT_LT(totalMs, 10) << "SeqLock throughput too slow: " << totalMs << " ms for " << kIterations
                           << " write+read cycles";
}
