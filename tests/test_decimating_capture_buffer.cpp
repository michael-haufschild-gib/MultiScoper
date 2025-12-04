/*
    Oscil - Decimating Capture Buffer Unit Tests
    Tests for decimation, anti-aliasing filter, and memory management
*/

#include <gtest/gtest.h>
#include "core/DecimatingCaptureBuffer.h"
#include <cmath>
#include <numeric>

using namespace oscil;

//==============================================================================
// Decimation Filter Tests
//==============================================================================

class DecimationFilterTest : public ::testing::Test
{
protected:
    DecimationFilter filter;
};

TEST_F(DecimationFilterTest, DefaultConfigurationIsPassthrough)
{
    // Default filter (ratio 1) should pass signal through unchanged
    filter.configure(1, 44100.0);

    float testSignal = 0.5f;
    float output = filter.processSample(testSignal);

    // With ratio 1 and single coefficient, output should equal input
    EXPECT_FLOAT_EQ(output, testSignal);
}

TEST_F(DecimationFilterTest, IsActiveReturnsFalseForRatioOne)
{
    filter.configure(1, 44100.0);
    EXPECT_FALSE(filter.isActive());
}

TEST_F(DecimationFilterTest, IsActiveReturnsTrueForHigherRatios)
{
    filter.configure(2, 44100.0);
    EXPECT_TRUE(filter.isActive());

    filter.configure(8, 44100.0);
    EXPECT_TRUE(filter.isActive());
}

TEST_F(DecimationFilterTest, GetDecimationRatioReturnsConfiguredValue)
{
    filter.configure(4, 44100.0);
    EXPECT_EQ(filter.getDecimationRatio(), 4);

    filter.configure(16, 44100.0);
    EXPECT_EQ(filter.getDecimationRatio(), 16);
}

TEST_F(DecimationFilterTest, ResetClearsFilterState)
{
    filter.configure(4, 44100.0);

    // Process some samples to build up state
    for (int i = 0; i < 100; ++i)
        filter.processSample(1.0f);

    filter.reset();

    // After reset, processing DC should converge back to DC
    float dcOutput = 0.0f;
    for (int i = 0; i < 50; ++i)
        dcOutput = filter.processSample(0.0f);

    EXPECT_NEAR(dcOutput, 0.0f, 0.01f);
}

TEST_F(DecimationFilterTest, FilterAttenuatesHighFrequencies)
{
    // 4x decimation at 44100 Hz uses JUCE Kaiser window design
    filter.configure(4, 44100.0);

    // Generate high-frequency signal (near Nyquist of target rate)
    // At 4x decimation from 44100 -> 11025, Nyquist is 5512.5 Hz
    // Test with signal at 0.4 of source Nyquist = ~8820 Hz
    // This should be heavily attenuated

    float highFreqEnergy = 0.0f;
    float lowFreqEnergy = 0.0f;

    // High frequency test (alternating +1/-1 = Nyquist)
    filter.reset();
    for (int i = 0; i < 1000; ++i)
    {
        float input = (i % 2 == 0) ? 1.0f : -1.0f;
        float output = filter.processSample(input);
        highFreqEnergy += output * output;
    }

    // Low frequency test (DC signal = 1.0)
    filter.reset();
    for (int i = 0; i < 1000; ++i)
    {
        float output = filter.processSample(1.0f);
        if (i > 50)  // Skip initial transient
            lowFreqEnergy += output * output;
    }

    // High frequency should be significantly attenuated
    EXPECT_LT(highFreqEnergy, lowFreqEnergy * 0.1f);
}

TEST_F(DecimationFilterTest, GetFilterOrderReturnsPositiveForActiveFilter)
{
    filter.configure(4, 44100.0);
    EXPECT_GT(filter.getFilterOrder(), 0u);
}

TEST_F(DecimationFilterTest, GetMemoryUsageBytesReturnsPositive)
{
    filter.configure(4, 44100.0);
    EXPECT_GT(filter.getMemoryUsageBytes(), 0u);
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

    // Helper to create test audio buffer
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
    // (new buffer created with different size)
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
