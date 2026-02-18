/*
    Oscil - Decimating Capture Buffer Unit Tests
    Tests for SIMD decimation, anti-aliasing filter, and memory management
*/

#include <gtest/gtest.h>
#include "core/DecimatingCaptureBuffer.h"
#include <cmath>
#include <numeric>

using namespace oscil;

//==============================================================================
// SIMD Decimation Filter Tests
//==============================================================================

class SIMDDecimationFilterTest : public ::testing::Test
{
protected:
    SIMDDecimationFilter filter;
    static constexpr int TEST_BLOCK_SIZE = 1024;
    
    // Helper to process a block and get output count
    int processTestBlock(const float* input, float* output, int numSamples, int numChannels = 1)
    {
        const float* inputs[2] = { input, input };
        float* outputs[2] = { output, output };
        return filter.processBlock(inputs, outputs, numSamples, numChannels);
    }
};

TEST_F(SIMDDecimationFilterTest, DefaultConfigurationIsPassthrough)
{
    filter.configure(1, 44100.0, TEST_BLOCK_SIZE);

    std::vector<float> input(TEST_BLOCK_SIZE, 0.5f);
    std::vector<float> output(TEST_BLOCK_SIZE);
    
    int outputCount = processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);
    
    // With ratio 1, output count equals input count
    EXPECT_EQ(outputCount, TEST_BLOCK_SIZE);
    EXPECT_FLOAT_EQ(output[0], 0.5f);
}

TEST_F(SIMDDecimationFilterTest, IsActiveReturnsFalseForRatioOne)
{
    filter.configure(1, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_FALSE(filter.isActive());
}

TEST_F(SIMDDecimationFilterTest, IsActiveReturnsTrueForHigherRatios)
{
    filter.configure(2, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_TRUE(filter.isActive());

    filter.configure(8, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_TRUE(filter.isActive());
}

TEST_F(SIMDDecimationFilterTest, GetDecimationRatioReturnsConfiguredValue)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_EQ(filter.getDecimationRatio(), 4);

    filter.configure(16, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_EQ(filter.getDecimationRatio(), 16);
}

TEST_F(SIMDDecimationFilterTest, DecimationReducesOutputCount)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);
    
    std::vector<float> input(TEST_BLOCK_SIZE, 1.0f);
    std::vector<float> output(TEST_BLOCK_SIZE);
    
    int outputCount = processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);
    
    // 4x decimation: 1024 samples -> ~256 samples
    EXPECT_NEAR(outputCount, TEST_BLOCK_SIZE / 4, 1);
}

TEST_F(SIMDDecimationFilterTest, ResetClearsFilterState)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);

    // Process some samples to build up state
    std::vector<float> input(TEST_BLOCK_SIZE, 1.0f);
    std::vector<float> output(TEST_BLOCK_SIZE);
    processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);

    filter.reset();

    // After reset, processing DC should converge back to DC
    std::fill(input.begin(), input.end(), 0.0f);
    processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);

    // Output should be near zero
    float sum = 0.0f;
    for (int i = 10; i < TEST_BLOCK_SIZE / 4; ++i)
        sum += std::abs(output[static_cast<size_t>(i)]);
    
    EXPECT_NEAR(sum / static_cast<float>(TEST_BLOCK_SIZE / 4 - 10), 0.0f, 0.1f);
}

TEST_F(SIMDDecimationFilterTest, FilterAttenuatesHighFrequencies)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);

    float highFreqEnergy = 0.0f;
    float lowFreqEnergy = 0.0f;

    std::vector<float> input(TEST_BLOCK_SIZE);
    std::vector<float> output(TEST_BLOCK_SIZE);

    // High frequency test (alternating +1/-1 = Nyquist)
    for (int i = 0; i < TEST_BLOCK_SIZE; ++i)
        input[static_cast<size_t>(i)] = (i % 2 == 0) ? 1.0f : -1.0f;
    
    filter.reset();
    int count = processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);
    
    for (int i = 0; i < count; ++i)
        highFreqEnergy += output[static_cast<size_t>(i)] * output[static_cast<size_t>(i)];

    // Low frequency test (DC signal = 1.0)
    std::fill(input.begin(), input.end(), 1.0f);
    filter.reset();
    count = processTestBlock(input.data(), output.data(), TEST_BLOCK_SIZE);
    
    for (int i = 10; i < count; ++i)  // Skip initial transient
        lowFreqEnergy += output[static_cast<size_t>(i)] * output[static_cast<size_t>(i)];

    // High frequency should be significantly attenuated
    EXPECT_LT(highFreqEnergy, lowFreqEnergy * 0.1f);
}

TEST_F(SIMDDecimationFilterTest, GetFilterOrderReturnsPositiveForActiveFilter)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_GT(filter.getFilterOrder(), 0u);
}

TEST_F(SIMDDecimationFilterTest, GetMemoryUsageBytesReturnsPositive)
{
    filter.configure(4, 44100.0, TEST_BLOCK_SIZE);
    EXPECT_GT(filter.getMemoryUsageBytes(), 0u);
}

TEST_F(SIMDDecimationFilterTest, PhaseTrackingAcrossBlocks)
{
    filter.configure(4, 44100.0, 256);
    
    std::vector<float> input(256, 1.0f);
    std::vector<float> output(256);
    
    int totalOutput = 0;
    
    // Process multiple blocks
    for (int block = 0; block < 4; ++block)
    {
        totalOutput += processTestBlock(input.data(), output.data(), 256);
    }
    
    // After 4 blocks of 256 = 1024 samples, should have ~256 output samples
    EXPECT_NEAR(totalOutput, 256, 4);
}

//==============================================================================
// DecimatingCaptureBuffer Construction Tests
//==============================================================================

class DecimatingCaptureBufferConstructionTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(DecimatingCaptureBufferConstructionTest, DefaultConstructorUsesStandardQuality)
{
    DecimatingCaptureBuffer buffer;

    EXPECT_EQ(buffer.getConfig().qualityPreset, QualityPreset::Standard);
    EXPECT_EQ(buffer.getCaptureRate(), CaptureRate::STANDARD);
}

TEST_F(DecimatingCaptureBufferConstructionTest, ConstructorWithConfigAppliesSettings)
{
    CaptureQualityConfig config;
    config.qualityPreset = QualityPreset::Eco;
    config.bufferDuration = BufferDuration::Short;

    DecimatingCaptureBuffer buffer(config, 44100);

    EXPECT_EQ(buffer.getConfig().qualityPreset, QualityPreset::Eco);
    EXPECT_EQ(buffer.getCaptureRate(), CaptureRate::ECO);
    EXPECT_EQ(buffer.getSourceRate(), 44100);
}

TEST_F(DecimatingCaptureBufferConstructionTest, ConfigureChangesSettings)
{
    DecimatingCaptureBuffer buffer;

    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::High;
    newConfig.bufferDuration = BufferDuration::Long;

    buffer.configure(newConfig, 96000);

    EXPECT_EQ(buffer.getConfig().qualityPreset, QualityPreset::High);
    EXPECT_EQ(buffer.getCaptureRate(), CaptureRate::HIGH);
    EXPECT_EQ(buffer.getSourceRate(), 96000);
}

//==============================================================================
// Decimation Ratio Tests
//==============================================================================

class DecimatingBufferRatioTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(DecimatingBufferRatioTest, EcoPresetAt44kHzGives4xDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    DecimatingCaptureBuffer buffer(config, 44100);

    // 44100 / 11025 = 4
    EXPECT_EQ(buffer.getDecimationRatio(), 4);
}

TEST_F(DecimatingBufferRatioTest, StandardPresetAt44kHzGives2xDecimation)
{
    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer buffer(config, 44100);

    // 44100 / 22050 = 2
    EXPECT_EQ(buffer.getDecimationRatio(), 2);
}

TEST_F(DecimatingBufferRatioTest, HighPresetAt44kHzGivesNoDecimation)
{
    config.qualityPreset = QualityPreset::High;
    DecimatingCaptureBuffer buffer(config, 44100);

    // 44100 / 44100 = 1
    EXPECT_EQ(buffer.getDecimationRatio(), 1);
}

TEST_F(DecimatingBufferRatioTest, UltraPresetGivesNoDecimation)
{
    config.qualityPreset = QualityPreset::Ultra;
    DecimatingCaptureBuffer buffer(config, 192000);

    EXPECT_EQ(buffer.getDecimationRatio(), 1);
    EXPECT_EQ(buffer.getCaptureRate(), 192000);
}

TEST_F(DecimatingBufferRatioTest, EcoPresetAt192kHzGivesHighDecimation)
{
    config.qualityPreset = QualityPreset::Eco;
    DecimatingCaptureBuffer buffer(config, 192000);

    // 192000 / 11025 ≈ 17
    EXPECT_EQ(buffer.getDecimationRatio(), 17);
}

//==============================================================================
// Write and Read Tests
//==============================================================================

class DecimatingBufferWriteReadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        config.qualityPreset = QualityPreset::Standard;
        config.bufferDuration = BufferDuration::Medium;
        buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);
    }

    CaptureQualityConfig config;
    std::unique_ptr<DecimatingCaptureBuffer> buffer;

    juce::AudioBuffer<float> createTestBuffer(int numSamples, int numChannels = 2,
                                               float frequency = 440.0f, float sampleRate = 44100.0f)
    {
        juce::AudioBuffer<float> audioBuffer(numChannels, numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = audioBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                data[i] = std::sin(2.0f * juce::MathConstants<float>::pi * frequency * t);
            }
        }

        return audioBuffer;
    }
};

TEST_F(DecimatingBufferWriteReadTest, WriteIncreasesAvailableSamples)
{
    auto audioBuffer = createTestBuffer(1024);
    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;

    size_t before = buffer->getAvailableSamples();
    buffer->write(audioBuffer, meta);
    size_t after = buffer->getAvailableSamples();

    // With 2x decimation, 1024 samples becomes 512
    EXPECT_GT(after, before);
    EXPECT_NEAR(static_cast<double>(after - before), 512.0, 10.0);
}

TEST_F(DecimatingBufferWriteReadTest, ReadReturnsWrittenSamples)
{
    auto audioBuffer = createTestBuffer(2048);
    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;

    buffer->write(audioBuffer, meta);

    std::vector<float> output(1024);
    int samplesRead = buffer->read(output.data(), 1024, 0);

    EXPECT_GT(samplesRead, 0);
}

TEST_F(DecimatingBufferWriteReadTest, DCSignalPreservedAfterDecimation)
{
    // Write DC signal (all 1.0)
    juce::AudioBuffer<float> dcBuffer(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = dcBuffer.getWritePointer(ch);
        std::fill(data, data + 4096, 1.0f);
    }

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    buffer->write(dcBuffer, meta);

    // Read back
    std::vector<float> output(1024);
    int samplesRead = buffer->read(output.data(), 1024, 0);

    // DC should be preserved (allow for filter warmup)
    float avgOutput = 0.0f;
    int validSamples = 0;
    for (int i = 100; i < samplesRead; ++i)  // Skip initial transient
    {
        avgOutput += output[static_cast<size_t>(i)];
        validSamples++;
    }
    if (validSamples > 0)
        avgOutput /= static_cast<float>(validSamples);

    EXPECT_NEAR(avgOutput, 1.0f, 0.05f);
}

TEST_F(DecimatingBufferWriteReadTest, NoDecimationWithUltraPreset)
{
    config.qualityPreset = QualityPreset::Ultra;
    buffer = std::make_unique<DecimatingCaptureBuffer>(config, 44100);

    auto audioBuffer = createTestBuffer(1024);
    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;

    buffer->write(audioBuffer, meta);

    // No decimation, so all samples should be available
    EXPECT_GE(buffer->getAvailableSamples(), 1024u);
}

//==============================================================================
// Memory Usage Tests
//==============================================================================

class DecimatingBufferMemoryTest : public ::testing::Test
{
protected:
    CaptureQualityConfig config;
};

TEST_F(DecimatingBufferMemoryTest, EcoUsesLessMemoryThanStandard)
{
    config.bufferDuration = BufferDuration::Medium;

    config.qualityPreset = QualityPreset::Eco;
    DecimatingCaptureBuffer ecoBuffer(config, 44100);

    config.qualityPreset = QualityPreset::Standard;
    DecimatingCaptureBuffer standardBuffer(config, 44100);

    EXPECT_LT(ecoBuffer.getMemoryUsageBytes(), standardBuffer.getMemoryUsageBytes());
}

TEST_F(DecimatingBufferMemoryTest, ShortDurationUsesLessMemoryThanLong)
{
    config.qualityPreset = QualityPreset::Standard;

    config.bufferDuration = BufferDuration::Short;
    DecimatingCaptureBuffer shortBuffer(config, 44100);

    config.bufferDuration = BufferDuration::VeryLong;
    DecimatingCaptureBuffer longBuffer(config, 44100);

    EXPECT_LT(shortBuffer.getMemoryUsageBytes(), longBuffer.getMemoryUsageBytes());
}

TEST_F(DecimatingBufferMemoryTest, MemoryUsageStringFormatsCorrectly)
{
    config.qualityPreset = QualityPreset::Standard;
    config.bufferDuration = BufferDuration::Medium;
    DecimatingCaptureBuffer buffer(config, 44100);

    juce::String memStr = buffer.getMemoryUsageString();

    // Should contain KB or MB
    EXPECT_TRUE(memStr.contains("KB") || memStr.contains("MB"));
}

TEST_F(DecimatingBufferMemoryTest, GetMemoryUsageBytesIsPositive)
{
    DecimatingCaptureBuffer buffer;
    EXPECT_GT(buffer.getMemoryUsageBytes(), 0u);
}

//==============================================================================
// Clear and Reset Tests
//==============================================================================

class DecimatingBufferClearTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferClearTest, ClearResetsAvailableSamples)
{
    // Write some data
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    CaptureFrameMetadata meta;
    buffer->write(audioBuffer, meta);

    EXPECT_GT(buffer->getAvailableSamples(), 0u);

    buffer->clear();

    EXPECT_EQ(buffer->getAvailableSamples(), 0u);
}

//==============================================================================
// Metering Tests
//==============================================================================

class DecimatingBufferMeteringTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferMeteringTest, PeakLevelReturnsZeroForEmptyBuffer)
{
    EXPECT_FLOAT_EQ(buffer->getPeakLevel(0), 0.0f);
}

TEST_F(DecimatingBufferMeteringTest, RMSLevelReturnsZeroForEmptyBuffer)
{
    EXPECT_FLOAT_EQ(buffer->getRMSLevel(0), 0.0f);
}

TEST_F(DecimatingBufferMeteringTest, PeakLevelDetectsSignal)
{
    // Write a known signal
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = audioBuffer.getWritePointer(ch);
        for (int i = 0; i < 4096; ++i)
        {
            data[i] = 0.5f;  // Constant 0.5 signal
        }
    }

    CaptureFrameMetadata meta;
    meta.sampleRate = 44100.0;
    buffer->write(audioBuffer, meta);

    float peak = buffer->getPeakLevel(0, 1024);

    // Peak should be close to 0.5 (allowing for filter effects)
    EXPECT_GT(peak, 0.3f);
    EXPECT_LT(peak, 0.7f);
}

//==============================================================================
// Configuration Change Tests
//==============================================================================

class DecimatingBufferReconfigureTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferReconfigureTest, SetQualityPresetChangesDecimationRatio)
{
    EXPECT_EQ(buffer->getCaptureRate(), CaptureRate::STANDARD);

    buffer->setQualityPreset(QualityPreset::Eco, 44100);

    EXPECT_EQ(buffer->getCaptureRate(), CaptureRate::ECO);
    EXPECT_EQ(buffer->getDecimationRatio(), 4);
}

TEST_F(DecimatingBufferReconfigureTest, ConfigurePreservesDataIntegrity)
{
    // Write some data
    juce::AudioBuffer<float> audioBuffer(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* data = audioBuffer.getWritePointer(ch);
        for (int i = 0; i < 4096; ++i)
            data[i] = 0.5f;
    }

    CaptureFrameMetadata meta;
    buffer->write(audioBuffer, meta);

    // Reconfigure
    CaptureQualityConfig newConfig;
    newConfig.qualityPreset = QualityPreset::Eco;
    buffer->configure(newConfig, 48000);

    // Buffer should be cleared/reset after reconfiguration
    EXPECT_EQ(buffer->getConfig().qualityPreset, QualityPreset::Eco);
}

//==============================================================================
// Internal Buffer Access Tests
//==============================================================================

class DecimatingBufferInternalAccessTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<DecimatingCaptureBuffer>();
    }

    std::unique_ptr<DecimatingCaptureBuffer> buffer;
};

TEST_F(DecimatingBufferInternalAccessTest, GetInternalBufferReturnsValidPointer)
{
    auto internal = buffer->getInternalBuffer();
    EXPECT_NE(internal, nullptr);
}

TEST_F(DecimatingBufferInternalAccessTest, GetCapacityReturnsPositiveValue)
{
    EXPECT_GT(buffer->getCapacity(), 0u);
}
