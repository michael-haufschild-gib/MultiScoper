/*
    Oscil - Plugin Processor Audio Tests
    Tests for processBlock, buffer handling, CPU tracking, and audio edge cases
*/

#include <gtest/gtest.h>
#include "plugin/PluginProcessor.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include <thread>
#include <atomic>
#include <cmath>

using namespace oscil;

class PluginProcessorAudioTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<OscilPluginProcessor> processor;

    void SetUp() override
    {
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();
        processor = std::make_unique<OscilPluginProcessor>(
            *registry_, *themeManager_, *shaderRegistry_, *memoryBudgetManager_);
    }

    void TearDown() override
    {
        processor.reset();
        memoryBudgetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }

    // Helper to generate audio buffer with specific values
    juce::AudioBuffer<float> generateTestBuffer(int numChannels, int numSamples, float value = 0.5f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                buffer.setSample(ch, i, value);
            }
        }
        return buffer;
    }

    // Helper to generate sine wave
    juce::AudioBuffer<float> generateSineBuffer(int numChannels, int numSamples,
                                                 float frequency = 440.0f, float sampleRate = 44100.0f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate);
                buffer.setSample(ch, i, sample);
            }
        }
        return buffer;
    }
};

// === Process Block Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_PassThroughAudio)
{
    processor->prepareToPlay(44100.0, 512);

    // Generate a test buffer
    auto buffer = generateTestBuffer(2, 512, 0.75f);
    juce::MidiBuffer midiBuffer;

    // Store original values
    std::vector<float> originalLeft(512), originalRight(512);
    for (int i = 0; i < 512; ++i)
    {
        originalLeft[i] = buffer.getSample(0, i);
        originalRight[i] = buffer.getSample(1, i);
    }

    // Process
    processor->processBlock(buffer, midiBuffer);

    // Audio should pass through unchanged (visualization plugin)
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), originalLeft[i]);
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), originalRight[i]);
    }
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_CapturesAudio)
{
    processor->prepareToPlay(44100.0, 512);

    // Generate a test buffer with known values
    auto buffer = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Verify audio was captured
    auto captureBuffer = processor->getCaptureBuffer();
    EXPECT_NE(captureBuffer, nullptr);
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_MetadataCaptured)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    auto captureBuffer = processor->getCaptureBuffer();
    auto metadata = captureBuffer->getLatestMetadata();

    // DecimatingCaptureBuffer defaults to Standard quality (22050 Hz)
    // So with 44100 Hz input, we expect decimation
    EXPECT_LE(metadata.sampleRate, 44100.0);
    EXPECT_EQ(metadata.numChannels, 2);
    
    // Check that samples were captured (count will be lower due to decimation)
    EXPECT_GT(metadata.numSamples, 0);
    EXPECT_LE(metadata.numSamples, 512);
}

// === Edge Case Buffer Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_EmptyBuffer)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> emptyBuffer(2, 0);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(emptyBuffer, midiBuffer);

    // Processor should remain functional after empty buffer
    EXPECT_GE(processor->getCpuUsage(), 0.0f);
    EXPECT_EQ(emptyBuffer.getNumChannels(), 2);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_ZeroChannels)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> noChannelBuffer(0, 512);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(noChannelBuffer, midiBuffer);

    // Buffer dimensions should be unchanged — processBlock is non-destructive
    EXPECT_EQ(noChannelBuffer.getNumChannels(), 0);
    EXPECT_EQ(noChannelBuffer.getNumSamples(), 512);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_MonoBuffer)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(1, 512, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Mono buffer should retain its dimensions after processing
    EXPECT_EQ(buffer.getNumChannels(), 1);
    EXPECT_EQ(buffer.getNumSamples(), 512);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_LargeBuffer)
{
    processor->prepareToPlay(44100.0, 8192);

    auto buffer = generateTestBuffer(2, 8192, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    EXPECT_EQ(buffer.getNumSamples(), 8192);
    EXPECT_GE(processor->getCpuUsage(), 0.0f);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_SmallBuffer)
{
    processor->prepareToPlay(44100.0, 32);

    auto buffer = generateTestBuffer(2, 32, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    EXPECT_EQ(buffer.getNumSamples(), 32);
    EXPECT_EQ(buffer.getNumChannels(), 2);
}

TEST_F(PluginProcessorAudioTest, ProcessBeforePrepare)
{
    // Process before prepareToPlay
    auto buffer = generateTestBuffer(2, 512);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Buffer should retain its dimensions; processor should not corrupt it
    EXPECT_EQ(buffer.getNumChannels(), 2);
    EXPECT_EQ(buffer.getNumSamples(), 512);
}

// === CPU Usage Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_CPUUsageTracked)
{
    processor->prepareToPlay(44100.0, 512);

    // Process multiple buffers to allow CPU tracking to settle
    for (int i = 0; i < 10; ++i)
    {
        auto buffer = generateSineBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        processor->processBlock(buffer, midiBuffer);
    }

    float cpuUsage = processor->getCpuUsage();
    // CPU usage should be a valid percentage (0-100 typically, but can spike higher)
    EXPECT_GE(cpuUsage, 0.0f);
    EXPECT_LT(cpuUsage, 1000.0f); // Sanity check - should not be absurdly high
}

// === Concurrent Processing Tests ===

/*
TEST_F(PluginProcessorAudioTest, ConcurrentProcessing)
{
    processor->prepareToPlay(44100.0, 512);

    std::atomic<bool> running{true};
    std::atomic<int> processCount{0};

    // Simulate concurrent processing (though in real DAW this shouldn't happen)
    std::thread processor1([this, &running, &processCount]()
    {
        while (running)
        {
            auto buffer = generateTestBuffer(2, 256);
            juce::MidiBuffer midiBuffer;
            processor->processBlock(buffer, midiBuffer);
            processCount++;
        }
    });

    std::thread processor2([this, &running, &processCount]()
    {
        while (running)
        {
            auto buffer = generateTestBuffer(2, 256);
            juce::MidiBuffer midiBuffer;
            processor->processBlock(buffer, midiBuffer);
            processCount++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    processor1.join();
    processor2.join();

    // Should have processed many buffers without crashing
    EXPECT_GT(processCount.load(), 10);
}
*/

// === Stress Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_ManyIterations)
{
    processor->prepareToPlay(44100.0, 512);

    // Process many buffers in succession
    for (int i = 0; i < 1000; ++i)
    {
        auto buffer = generateTestBuffer(2, 512, static_cast<float>(i % 100) / 100.0f);
        juce::MidiBuffer midiBuffer;
        processor->processBlock(buffer, midiBuffer);
    }

    // After 1000 iterations, CPU tracking should report a valid value
    float cpuUsage = processor->getCpuUsage();
    EXPECT_GE(cpuUsage, 0.0f);
    EXPECT_LT(cpuUsage, 1000.0f);
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_VaryingBufferSizes)
{
    processor->prepareToPlay(44100.0, 2048);

    std::vector<int> sizes = {32, 64, 128, 256, 512, 1024, 2048, 512, 256, 128, 64};

    for (int size : sizes)
    {
        auto buffer = generateTestBuffer(2, size);
        juce::MidiBuffer midiBuffer;
        processor->processBlock(buffer, midiBuffer);
    }

    // After varying sizes, processor should still report valid sample rate
    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);
    EXPECT_GE(processor->getCpuUsage(), 0.0f);
}

// === Signal Pattern Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_SineWave)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateSineBuffer(2, 512, 440.0f, 44100.0f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Verify audio passes through
    float firstSample = buffer.getSample(0, 0);
    EXPECT_TRUE(std::isfinite(firstSample));
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_Silence)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.0f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Verify silence passes through
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.0f);
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), 0.0f);
    }
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_FullScale)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 1.0f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Verify full scale passes through
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 1.0f);
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), 1.0f);
    }
}

TEST_F(PluginProcessorAudioTest, ProcessBlock_NegativeFullScale)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, -1.0f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    // Verify negative full scale passes through
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), -1.0f);
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), -1.0f);
    }
}

// === Multi-channel Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_DifferentChannelValues)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);

    // Left channel = 0.5, Right channel = -0.5
    for (int i = 0; i < 512; ++i)
    {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, -0.5f);
    }

    juce::MidiBuffer midiBuffer;
    processor->processBlock(buffer, midiBuffer);

    // Verify channels pass through independently
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.5f);
        EXPECT_FLOAT_EQ(buffer.getSample(1, i), -0.5f);
    }
}

// === Sequential Processing Tests ===

TEST_F(PluginProcessorAudioTest, ProcessBlock_Sequential)
{
    processor->prepareToPlay(44100.0, 256);

    // Process multiple blocks sequentially
    for (int i = 0; i < 100; ++i)
    {
        auto buffer = generateTestBuffer(2, 256);
        juce::MidiBuffer midiBuffer;
        processor->processBlock(buffer, midiBuffer);
    }

    // Verify capture buffer has accumulated samples
    auto captureBuffer = processor->getCaptureBuffer();
    EXPECT_GT(captureBuffer->getAvailableSamples(), 0u);
}
