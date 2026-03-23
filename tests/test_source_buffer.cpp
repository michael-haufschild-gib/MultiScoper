/*
    Oscil - Source Buffer and Data Tests
    Tests for Source serialization, display names, metrics, audio config, and capture buffers
*/

#include "core/SharedCaptureBuffer.h"
#include "core/Source.h"

#include <atomic>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <thread>
#include <vector>

using namespace oscil;

class SourceBufferTest : public ::testing::Test
{
protected:
    std::unique_ptr<Source> source;
    SourceId sourceId;
    InstanceId instanceId;

    void SetUp() override
    {
        sourceId = SourceId::generate();
        instanceId = InstanceId::generate();
        source = std::make_unique<Source>(sourceId);
    }

    void TearDown() override { source.reset(); }
};

// === Serialization Tests ===

TEST_F(SourceBufferTest, SerializationRoundTrip)
{
    source->setDawTrackId("track-123");
    source->setDisplayName("Test Track");
    source->setSampleRate(48000.0f);
    source->setChannelConfig(ChannelConfig::STEREO);
    source->setBufferSize(256);
    EXPECT_TRUE(source->transitionTo(SourceState::ACTIVE));

    auto owner = InstanceId::generate();
    source->setOwningInstanceId(owner);

    auto backup = InstanceId::generate();
    source->addBackupInstance(backup);

    // Serialize
    auto tree = source->toValueTree();

    // Deserialize
    Source restored(tree);

    EXPECT_EQ(restored.getId(), source->getId());
    EXPECT_EQ(restored.getDawTrackId(), source->getDawTrackId());
    EXPECT_EQ(restored.getDisplayName(), source->getDisplayName());
    EXPECT_EQ(restored.getSampleRate(), source->getSampleRate());
    EXPECT_EQ(restored.getChannelConfig(), source->getChannelConfig());
    EXPECT_EQ(restored.getBufferSize(), source->getBufferSize());
    EXPECT_EQ(restored.getState(), source->getState());
    EXPECT_EQ(restored.getOwningInstanceId(), source->getOwningInstanceId());
    EXPECT_EQ(restored.getBackupInstanceIds().size(), source->getBackupInstanceIds().size());
}

TEST_F(SourceBufferTest, DeserializeInvalidValueTreeType)
{
    auto originalId = source->getId();

    // Create ValueTree with wrong type
    juce::ValueTree wrongType("WrongType");
    wrongType.setProperty("id", "new-id", nullptr);

    source->fromValueTree(wrongType);

    // Should not change anything due to type check
    EXPECT_EQ(source->getId(), originalId);
}

TEST_F(SourceBufferTest, DeserializeEmptyValueTree)
{
    auto originalId = source->getId();

    juce::ValueTree empty;
    source->fromValueTree(empty);

    // Should not change anything
    EXPECT_EQ(source->getId(), originalId);
}

TEST_F(SourceBufferTest, DeserializeValueTreeMissingProperties)
{
    // Create a minimal ValueTree with only required type
    juce::ValueTree minimal("Source");

    Source testSource(minimal);

    // Should have default values
    EXPECT_TRUE(testSource.getId().id.isEmpty());
    EXPECT_TRUE(testSource.getDawTrackId().isEmpty());
    EXPECT_FLOAT_EQ(testSource.getSampleRate(), 44100.0f);
    EXPECT_EQ(testSource.getChannelConfig(), ChannelConfig::STEREO);
    EXPECT_EQ(testSource.getBufferSize(), 512);
    EXPECT_EQ(testSource.getState(), SourceState::DISCOVERED);
}

TEST_F(SourceBufferTest, DeserializeValueTreeWithInvalidEnums)
{
    juce::ValueTree tree("Source");
    tree.setProperty("id", "test-id", nullptr);
    tree.setProperty("state", "INVALID_STATE", nullptr);
    tree.setProperty("channelConfig", "INVALID_CONFIG", nullptr);

    Source testSource(tree);

    // Should use defaults for invalid enums
    EXPECT_EQ(testSource.getState(), SourceState::DISCOVERED);
    EXPECT_EQ(testSource.getChannelConfig(), ChannelConfig::STEREO);
}

// === Display Name Tests ===

TEST_F(SourceBufferTest, DisplayNameTruncation)
{
    // Create a string longer than MAX_DISPLAY_NAME_LENGTH (64)
    juce::String longName;
    for (int i = 0; i < 100; ++i)
        longName += "X";

    source->setDisplayName(longName);

    EXPECT_LE(source->getDisplayName().length(), Source::MAX_DISPLAY_NAME_LENGTH);
}

TEST_F(SourceBufferTest, DisplayNameExactlyMaxLength)
{
    juce::String exactLength;
    for (int i = 0; i < Source::MAX_DISPLAY_NAME_LENGTH; ++i)
        exactLength += "X";

    source->setDisplayName(exactLength);

    EXPECT_EQ(source->getDisplayName().length(), Source::MAX_DISPLAY_NAME_LENGTH);
    EXPECT_EQ(source->getDisplayName(), exactLength);
}

TEST_F(SourceBufferTest, DisplayNameEmpty)
{
    source->setDisplayName("");

    EXPECT_TRUE(source->getDisplayName().isEmpty());
}

TEST_F(SourceBufferTest, DisplayNameWithSpecialCharacters)
{
    source->setDisplayName("Track\t\n\r\"<>&'");

    EXPECT_EQ(source->getDisplayName(), "Track\t\n\r\"<>&'");
}

TEST_F(SourceBufferTest, DisplayNameWithUnicode)
{
    source->setDisplayName(juce::CharPointer_UTF8("Track 日本語 🎵"));

    EXPECT_EQ(source->getDisplayName(), juce::CharPointer_UTF8("Track 日本語 🎵"));
}

// === Metrics Tests ===

TEST_F(SourceBufferTest, CorrelationMetrics)
{
    EXPECT_FALSE(source->getCorrelationMetrics().isValid);

    source->updateCorrelationMetrics(0.85f);
    EXPECT_TRUE(source->getCorrelationMetrics().isValid);
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, 0.85f);

    // Test clamping
    source->updateCorrelationMetrics(2.0f);
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, 1.0f);

    source->updateCorrelationMetrics(-2.0f);
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, -1.0f);
}

TEST_F(SourceBufferTest, SignalMetrics)
{
    EXPECT_FALSE(source->getSignalMetrics().hasSignal);

    source->updateSignalMetrics(-10.0f, -5.0f, 0.001f);
    EXPECT_TRUE(source->getSignalMetrics().hasSignal);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().rmsLevel, -10.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().peakLevel, -5.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().dcOffset, 0.001f);

    // Signal below threshold
    source->updateSignalMetrics(-70.0f, -65.0f, 0.0f);
    EXPECT_FALSE(source->getSignalMetrics().hasSignal);
}

// === Metrics with NaN and Infinity ===

TEST_F(SourceBufferTest, CorrelationMetricsWithNaN)
{
    source->updateCorrelationMetrics(std::numeric_limits<float>::quiet_NaN());

    // Non-finite correlation should be sanitized to center (0.0)
    EXPECT_TRUE(std::isfinite(source->getCorrelationMetrics().correlationCoefficient));
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, 0.0f);
    EXPECT_TRUE(source->getCorrelationMetrics().isValid);
}

TEST_F(SourceBufferTest, CorrelationMetricsWithPositiveInfinity)
{
    source->updateCorrelationMetrics(std::numeric_limits<float>::infinity());

    // Should be clamped to 1.0
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, 1.0f);
    EXPECT_TRUE(source->getCorrelationMetrics().isValid);
}

TEST_F(SourceBufferTest, CorrelationMetricsWithNegativeInfinity)
{
    source->updateCorrelationMetrics(-std::numeric_limits<float>::infinity());

    // Should be clamped to -1.0
    EXPECT_FLOAT_EQ(source->getCorrelationMetrics().correlationCoefficient, -1.0f);
    EXPECT_TRUE(source->getCorrelationMetrics().isValid);
}

TEST_F(SourceBufferTest, SignalMetricsWithNaN)
{
    float nan = std::numeric_limits<float>::quiet_NaN();
    source->updateSignalMetrics(nan, nan, nan);

    EXPECT_FLOAT_EQ(source->getSignalMetrics().rmsLevel, -100.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().peakLevel, -100.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().dcOffset, 0.0f);
    EXPECT_FALSE(source->getSignalMetrics().hasSignal);
}

TEST_F(SourceBufferTest, SignalMetricsWithInfinity)
{
    float inf = std::numeric_limits<float>::infinity();
    source->updateSignalMetrics(inf, inf, inf);

    EXPECT_FLOAT_EQ(source->getSignalMetrics().rmsLevel, -100.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().peakLevel, -100.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().dcOffset, 0.0f);
    EXPECT_FALSE(source->getSignalMetrics().hasSignal);
}

TEST_F(SourceBufferTest, SignalMetricsWithMixedValues)
{
    // Mix of normal, NaN, and infinity
    float nan = std::numeric_limits<float>::quiet_NaN();
    float inf = std::numeric_limits<float>::infinity();
    source->updateSignalMetrics(-20.0f, inf, nan);

    EXPECT_FLOAT_EQ(source->getSignalMetrics().rmsLevel, -20.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().peakLevel, -100.0f);
    EXPECT_FLOAT_EQ(source->getSignalMetrics().dcOffset, 0.0f);
    EXPECT_TRUE(source->getSignalMetrics().hasSignal);
}

TEST_F(SourceBufferTest, ConcurrentMetricsUpdate)
{
    std::atomic<bool> done{false};
    std::vector<std::thread> threads;

    // Writer threads for correlation
    for (int i = 0; i < 3; ++i)
    {
        threads.emplace_back([this, &done, i]() {
            while (!done.load())
            {
                source->updateCorrelationMetrics(static_cast<float>(i) * 0.3f);
            }
        });
    }

    // Writer threads for signal metrics
    for (int i = 0; i < 3; ++i)
    {
        threads.emplace_back([this, &done, i]() {
            while (!done.load())
            {
                source->updateSignalMetrics(-20.0f + i, -10.0f + i, 0.01f * i);
            }
        });
    }

    // Reader threads
    for (int i = 0; i < 2; ++i)
    {
        threads.emplace_back([this, &done]() {
            while (!done.load())
            {
                [[maybe_unused]] auto corr = source->getCorrelationMetrics();
                [[maybe_unused]] auto sig = source->getSignalMetrics();
            }
        });
    }

    // Run for a short time
    juce::Thread::sleep(100);
    done.store(true);

    for (auto& t : threads)
        t.join();

    // Just verify we didn't crash - metrics should still be accessible
    EXPECT_TRUE(source->getCorrelationMetrics().isValid);
}

// === Audio Configuration Tests ===

TEST_F(SourceBufferTest, SampleRateZero)
{
    source->setSampleRate(0.0f);

    EXPECT_FLOAT_EQ(source->getSampleRate(), 0.0f);
}

TEST_F(SourceBufferTest, SampleRateNegative)
{
    source->setSampleRate(-44100.0f);

    // No validation - just stores the value
    EXPECT_FLOAT_EQ(source->getSampleRate(), -44100.0f);
}

TEST_F(SourceBufferTest, SampleRateVeryHigh)
{
    source->setSampleRate(384000.0f);

    EXPECT_FLOAT_EQ(source->getSampleRate(), 384000.0f);
}

TEST_F(SourceBufferTest, BufferSizeZero)
{
    source->setBufferSize(0);

    EXPECT_EQ(source->getBufferSize(), 0);
}

TEST_F(SourceBufferTest, BufferSizeNegative)
{
    source->setBufferSize(-512);

    // No validation - just stores the value
    EXPECT_EQ(source->getBufferSize(), -512);
}

// === Capture Buffer Tests ===

TEST_F(SourceBufferTest, CaptureBufferInitiallyNull) { EXPECT_EQ(source->getCaptureBuffer(), nullptr); }

TEST_F(SourceBufferTest, SetAndGetCaptureBuffer)
{
    // Create a mock shared_ptr (we don't need real SharedCaptureBuffer)
    auto buffer = std::make_shared<SharedCaptureBuffer>();

    source->setCaptureBuffer(buffer);

    EXPECT_EQ(source->getCaptureBuffer(), buffer);
}

TEST_F(SourceBufferTest, SetCaptureBufferToNull)
{
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    source->setCaptureBuffer(buffer);

    source->setCaptureBuffer(nullptr);

    EXPECT_EQ(source->getCaptureBuffer(), nullptr);
}
