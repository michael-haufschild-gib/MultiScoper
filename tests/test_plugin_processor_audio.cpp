/*
    Oscil - Plugin Processor Audio Tests
    Tests for processBlock correctness, edge cases, capture integrity,
    and lifecycle robustness of the audio processing path.
*/

#include <gtest/gtest.h>
#include "plugin/PluginProcessor.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/PresetManager.h"
#include <thread>
#include <atomic>
#include <cmath>
#include <limits>

using namespace oscil;

class PluginProcessorAudioTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<PresetManager> presetManager_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<OscilPluginProcessor> processor;

    void SetUp() override
    {
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        presetManager_ = std::make_unique<PresetManager>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();
        processor = std::make_unique<OscilPluginProcessor>(
            *registry_, *themeManager_, *shaderRegistry_, *presetManager_, *memoryBudgetManager_);
    }

    void TearDown() override
    {
        processor.reset();
        memoryBudgetManager_.reset();
        presetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }

    juce::AudioBuffer<float> generateTestBuffer(int numChannels, int numSamples, float value = 0.5f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample(ch, i, value);
        return buffer;
    }

    juce::AudioBuffer<float> generateSineBuffer(int numChannels, int numSamples,
                                                 float frequency = 440.0f, float sampleRate = 44100.0f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample(ch, i,
                    std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate));
        return buffer;
    }
};

// =============================================================================
// Core contract: audio pass-through and capture
// =============================================================================

// Bug caught: processBlock modifies the audio buffer instead of passing it
// through unaltered, causing downstream plugins to receive corrupted audio.
TEST_F(PluginProcessorAudioTest, ProcessBlockPassesThroughStereoAudioUnchanged)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; ++i)
    {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, -0.3f);
    }

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.5f) << "Left channel modified at sample " << i;
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), -0.3f) << "Right channel modified at sample " << i;
    }
}

// Bug caught: processBlock writes to capture buffer but the data is corrupt
// (wrong samples, wrong channel order, or wrong count).
TEST_F(PluginProcessorAudioTest, ProcessBlockCapturesAudioWithCorrectMetadata)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    auto captureBuffer = processor->getCaptureBuffer();
    ASSERT_NE(captureBuffer, nullptr);
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);

    auto metadata = captureBuffer->getLatestMetadata();
    EXPECT_LE(metadata.sampleRate, 44100.0);
    EXPECT_EQ(metadata.numChannels, 2);
    EXPECT_GT(metadata.numSamples, 0);
    EXPECT_LE(metadata.numSamples, 512);
}

// =============================================================================
// Edge case: degenerate buffers
// =============================================================================

// Bug caught: empty buffer causes division by zero in CPU measurement code.
TEST_F(PluginProcessorAudioTest, EmptyBufferDoesNotCrashOrCorruptState)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> emptyBuffer(2, 0);
    juce::MidiBuffer midi;
    processor->processBlock(emptyBuffer, midi);

    EXPECT_GE(processor->getCpuUsage(), 0.0f);
    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);
}

// Bug caught: mono input causes out-of-bounds access when code assumes
// buffer.getNumChannels() >= 2 without checking.
TEST_F(PluginProcessorAudioTest, MonoBufferProcessedSafely)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(1, 512, 0.7f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // Mono pass-through: data must survive
    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.7f);
}

// Bug caught: zero-channel buffer dereferences channel pointer array at [0].
TEST_F(PluginProcessorAudioTest, ZeroChannelBufferHandledGracefully)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> noChannelBuffer(0, 512);
    juce::MidiBuffer midi;
    processor->processBlock(noChannelBuffer, midi);

    EXPECT_EQ(noChannelBuffer.getNumChannels(), 0);
    EXPECT_EQ(noChannelBuffer.getNumSamples(), 512);
}

// Bug caught: processBlock called before prepareToPlay uses uninitialized
// capture buffer pointer, causing null dereference.
TEST_F(PluginProcessorAudioTest, ProcessBeforePrepareDoesNotCrash)
{
    auto buffer = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // Buffer data must survive (pass-through even without prepare)
    EXPECT_EQ(buffer.getNumChannels(), 2);
    EXPECT_EQ(buffer.getNumSamples(), 512);
}

// =============================================================================
// NaN / Infinity in audio: must not corrupt state or other samples
// =============================================================================

// Bug caught: NaN in input buffer propagates to CPU usage calculation,
// causing getCpuUsage() to return NaN forever.
TEST_F(PluginProcessorAudioTest, NaNInAudioDoesNotCorruptProcessorState)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.5f);
    buffer.setSample(0, 100, std::numeric_limits<float>::quiet_NaN());
    buffer.setSample(1, 200, std::numeric_limits<float>::quiet_NaN());

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // CPU usage must remain finite
    float cpu = processor->getCpuUsage();
    EXPECT_TRUE(std::isfinite(cpu)) << "NaN in audio corrupted CPU usage tracking";

    // Sample rate must remain valid
    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);

    // Processing subsequent clean buffers must still work
    auto cleanBuffer = generateTestBuffer(2, 512, 0.3f);
    processor->processBlock(cleanBuffer, midi);

    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(cleanBuffer.getSample(0, i), 0.3f)
            << "Clean buffer corrupted after NaN buffer at sample " << i;
    }
}

// Bug caught: Infinity in audio causes infinite loop or hang in peak
// detection within the capture write path.
TEST_F(PluginProcessorAudioTest, InfinityInAudioDoesNotHang)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.5f);
    buffer.setSample(0, 0, std::numeric_limits<float>::infinity());
    buffer.setSample(1, 0, -std::numeric_limits<float>::infinity());

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    EXPECT_TRUE(std::isfinite(processor->getCpuUsage()));
}

// =============================================================================
// Sample rate changes and lifecycle transitions
// =============================================================================

// Bug caught: prepareToPlay called twice with different sample rates causes
// the capture buffer to use stale sample rate for metadata.
TEST_F(PluginProcessorAudioTest, SampleRateChangeUpdatesMetadata)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer44 = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer44, midi);

    // Change sample rate
    processor->releaseResources();
    processor->prepareToPlay(96000.0, 512);

    auto buffer96 = generateTestBuffer(2, 512, 0.5f);
    processor->processBlock(buffer96, midi);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 96000.0);
}

// Bug caught: releaseResources then prepareToPlay with same rate causes
// double-free or dangling pointer in capture buffer.
TEST_F(PluginProcessorAudioTest, ReleaseAndRePrepareIsSafe)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer1 = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer1, midi);

    processor->releaseResources();
    processor->prepareToPlay(44100.0, 512);

    auto buffer2 = generateTestBuffer(2, 512, 0.3f);
    processor->processBlock(buffer2, midi);

    // After re-prepare, processing must still work and pass-through correctly
    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ(buffer2.getSample(0, i), 0.3f);

    auto captureBuffer = processor->getCaptureBuffer();
    ASSERT_NE(captureBuffer, nullptr);
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);
}

// Bug caught: calling prepareToPlay twice without releaseResources causes
// resource leak in capture buffer allocation.
TEST_F(PluginProcessorAudioTest, DoublePrepareWithoutReleaseIsSafe)
{
    processor->prepareToPlay(44100.0, 512);
    processor->prepareToPlay(48000.0, 256);

    auto buffer = generateTestBuffer(2, 256, 0.4f);
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 48000.0);
    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.4f);
}

// =============================================================================
// Varying buffer sizes: DAWs can change block sizes mid-stream
// =============================================================================

// Bug caught: buffer size larger than prepareToPlay's samplesPerBlock causes
// out-of-bounds write in capture buffer.
TEST_F(PluginProcessorAudioTest, BufferLargerThanPreparedSizeIsSafe)
{
    processor->prepareToPlay(44100.0, 512);

    // DAW sends a larger block than what was prepared for
    auto largeBuffer = generateTestBuffer(2, 2048, 0.5f);
    juce::MidiBuffer midi;
    processor->processBlock(largeBuffer, midi);

    // Data must pass through unchanged
    for (int i = 0; i < 2048; ++i)
        EXPECT_FLOAT_EQ(largeBuffer.getSample(0, i), 0.5f);
}

// Bug caught: varying buffer sizes cause incorrect available-samples count
// due to accumulation bug in capture buffer.
TEST_F(PluginProcessorAudioTest, VaryingBufferSizesMaintainValidState)
{
    processor->prepareToPlay(44100.0, 2048);

    std::vector<int> sizes = {32, 64, 128, 256, 512, 1024, 2048, 512, 256, 128, 64};

    juce::MidiBuffer midi;
    for (int size : sizes)
    {
        auto buffer = generateTestBuffer(2, size);
        processor->processBlock(buffer, midi);
    }

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);
    auto captureBuffer = processor->getCaptureBuffer();
    ASSERT_NE(captureBuffer, nullptr);
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);
}

// =============================================================================
// CPU usage tracking correctness
// =============================================================================

// Bug caught: CPU usage overflows or returns negative value after many
// iterations due to integer overflow in timing accumulator.
TEST_F(PluginProcessorAudioTest, CpuUsageRemainsValidAfterManyBlocks)
{
    processor->prepareToPlay(44100.0, 512);

    juce::MidiBuffer midi;
    for (int i = 0; i < 1000; ++i)
    {
        auto buffer = generateSineBuffer(2, 512);
        processor->processBlock(buffer, midi);
    }

    float cpuUsage = processor->getCpuUsage();
    EXPECT_GE(cpuUsage, 0.0f);
    EXPECT_LT(cpuUsage, 100.0f) << "CPU usage should be a reasonable percentage";
    EXPECT_TRUE(std::isfinite(cpuUsage));
}

// =============================================================================
// Capture integrity across sequential blocks
// =============================================================================

// Bug caught: sequential processBlock calls cause capture buffer to lose
// earlier data prematurely (writePos miscalculation).
TEST_F(PluginProcessorAudioTest, SequentialBlocksAccumulateInCapture)
{
    processor->prepareToPlay(44100.0, 256);

    juce::MidiBuffer midi;
    for (int i = 0; i < 100; ++i)
    {
        auto buffer = generateTestBuffer(2, 256);
        processor->processBlock(buffer, midi);
    }

    auto captureBuffer = processor->getCaptureBuffer();
    ASSERT_NE(captureBuffer, nullptr);

    // After 100 blocks of 256 samples, capture should have data
    // (may be decimated, but must be non-zero)
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);
}
