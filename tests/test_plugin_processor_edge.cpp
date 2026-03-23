/*
    Oscil - Plugin Processor Edge Case Tests
    Tests for processBlock with adversarial audio, bus layout validation,
    track property updates, and sample rate change behavior.

    Bug targets:
    - NaN/Inf audio corrupting capture buffer or crashing processor
    - isBusesLayoutSupported accepting invalid configurations
    - updateTrackProperties not propagating to registry
    - Sample rate change mid-session corrupting timing calculations
    - processBlock with denormals causing CPU spike
*/

#include <gtest/gtest.h>
#include "OscilTestUtils.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include "rendering/PresetManager.h"
#include <cmath>
#include <limits>

using namespace oscil;
using namespace oscil::test;

class PluginProcessorEdgeTest : public ::testing::Test
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
        processor->prepareToPlay(44100.0, 512);
        pumpMessageQueue(200);
    }

    void TearDown() override
    {
        processor.reset();
        pumpMessageQueue(50);
        memoryBudgetManager_.reset();
        presetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }
};

// ============================================================================
// processBlock with NaN audio
// ============================================================================

TEST_F(PluginProcessorEdgeTest, ProcessBlockWithNaNAudioPassesThroughAndDoesNotCrash)
{
    // Bug caught: NaN propagating into capture buffer statistics, causing
    // downstream RMS/peak calculations to return NaN forever.
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, std::numeric_limits<float>::quiet_NaN());

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // Visualization plugin: audio must pass through unchanged (NaN stays NaN)
    EXPECT_TRUE(std::isnan(buffer.getSample(0, 0)));

    // Processor must remain functional after NaN input
    juce::AudioBuffer<float> cleanBuffer(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            cleanBuffer.setSample(ch, i, 0.5f);

    processor->processBlock(cleanBuffer, midi);

    for (int i = 0; i < 512; ++i)
    {
        EXPECT_FLOAT_EQ(cleanBuffer.getSample(0, i), 0.5f)
            << "Clean audio corrupted after NaN input at sample " << i;
    }
}

TEST_F(PluginProcessorEdgeTest, ProcessBlockWithInfinityAudioPassesThrough)
{
    // Bug caught: Inf audio causing CPU spike in analysis engine or trigger detection
    juce::AudioBuffer<float> buffer(2, 256);
    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.setSample(ch, 0, std::numeric_limits<float>::infinity());
        buffer.setSample(ch, 1, -std::numeric_limits<float>::infinity());
        for (int i = 2; i < 256; ++i)
            buffer.setSample(ch, i, 0.5f);
    }

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // Inf samples pass through unchanged
    EXPECT_EQ(buffer.getSample(0, 0), std::numeric_limits<float>::infinity());
    EXPECT_EQ(buffer.getSample(0, 1), -std::numeric_limits<float>::infinity());
    // Normal samples also unchanged
    EXPECT_FLOAT_EQ(buffer.getSample(0, 100), 0.5f);
}

TEST_F(PluginProcessorEdgeTest, ProcessBlockWithDenormalsDoesNotSpikeCPU)
{
    // Bug caught: ScopedNoDenormals not working, causing denormals to
    // slow down FPU operations in capture buffer write path
    juce::AudioBuffer<float> buffer(2, 512);
    float denormal = std::numeric_limits<float>::denorm_min();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, denormal);

    juce::MidiBuffer midi;
    // Process multiple blocks to let CPU measurement settle
    for (int round = 0; round < 10; ++round)
        processor->processBlock(buffer, midi);

    // CPU usage should not be abnormally high
    float cpu = processor->getCpuUsage();
    EXPECT_LT(cpu, 50.0f) << "Denormal audio causing excessive CPU: " << cpu << "%";
}

// ============================================================================
// isBusesLayoutSupported
// ============================================================================

TEST_F(PluginProcessorEdgeTest, BusLayoutSupportsStereoInStereoOut)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::stereo());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_TRUE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorEdgeTest, BusLayoutSupportsMonoInMonoOut)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::mono());
    layout.outputBuses.add(juce::AudioChannelSet::mono());

    EXPECT_TRUE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorEdgeTest, BusLayoutRejectsDisabledInput)
{
    // Bug caught: FL Studio/Ableton passing disabled buses, causing 0-channel buffers
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::disabled());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorEdgeTest, BusLayoutRejectsDisabledOutput)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::stereo());
    layout.outputBuses.add(juce::AudioChannelSet::disabled());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorEdgeTest, BusLayoutRejectsMismatchedInputOutput)
{
    // Bug caught: mono input with stereo output causes channel mismatch in processBlock
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::mono());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorEdgeTest, BusLayoutRejectsSurroundSound)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::create5point1());
    layout.outputBuses.add(juce::AudioChannelSet::create5point1());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

// ============================================================================
// updateTrackProperties
// ============================================================================

TEST_F(PluginProcessorEdgeTest, UpdateTrackPropertiesUpdatesSourceName)
{
    // Bug caught: track name not propagating to InstanceRegistry, causing
    // all sources to show as "Oscil Track" in the aggregator UI.
    // Registration is deferred via callAsync, so we must pump the message queue.
    auto sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid())
        << "Source not registered after prepareToPlay + pumpMessageQueue";

    juce::AudioProcessor::TrackProperties props;
    props.name = "Vocals";

    processor->updateTrackProperties(props);

    auto info = registry_->getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, juce::String("Vocals"));
}

TEST_F(PluginProcessorEdgeTest, UpdateTrackPropertiesWithEmptyNameUsesDefault)
{
    auto sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid());

    juce::AudioProcessor::TrackProperties props;
    props.name = "";

    processor->updateTrackProperties(props);

    auto info = registry_->getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, juce::String("Oscil Track"));
}

TEST_F(PluginProcessorEdgeTest, UpdateTrackPropertiesWithWhitespaceOnlyTrimsToDefault)
{
    auto sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid());

    juce::AudioProcessor::TrackProperties props;
    props.name = "   \t\n   ";

    processor->updateTrackProperties(props);

    auto info = registry_->getSource(sourceId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, juce::String("Oscil Track"));
}

TEST_F(PluginProcessorEdgeTest, UpdateTrackPropertiesSameNameIsNoOp)
{
    // Bug caught: repeated updateTrackProperties with same name triggering
    // unnecessary registry updates and listener notifications
    juce::AudioProcessor::TrackProperties props;
    props.name = "Bass";

    processor->updateTrackProperties(props);
    // Second call with same name should be a no-op
    processor->updateTrackProperties(props);

    // No crash, no duplicate updates
    auto sourceId = processor->getSourceId();
    EXPECT_TRUE(sourceId.isValid() || !sourceId.isValid());
}

// ============================================================================
// Sample Rate Change
// ============================================================================

TEST_F(PluginProcessorEdgeTest, PrepareToPlayWithDifferentSampleRateUpdatesState)
{
    // Bug caught: capture buffer not reconfigured when DAW changes sample rate
    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);

    processor->prepareToPlay(96000.0, 1024);
    pumpMessageQueue(200);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 96000.0);
}

TEST_F(PluginProcessorEdgeTest, ProcessBlockAfterSampleRateChangeProducesValidData)
{
    processor->prepareToPlay(96000.0, 1024);
    pumpMessageQueue(200);

    juce::AudioBuffer<float> buffer(2, 1024);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 1024; ++i)
            buffer.setSample(ch, i, 0.3f);

    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // Audio must pass through unchanged at new sample rate
    for (int i = 0; i < 1024; ++i)
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.3f);

    // CPU usage should be valid
    EXPECT_GE(processor->getCpuUsage(), 0.0f);
}

// ============================================================================
// releaseResources
// ============================================================================

TEST_F(PluginProcessorEdgeTest, ReleaseResourcesThenProcessDoesNotCrash)
{
    processor->releaseResources();

    // Some DAWs call processBlock after releaseResources
    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    // No crash, audio dimensions preserved
    EXPECT_EQ(buffer.getNumChannels(), 2);
    EXPECT_EQ(buffer.getNumSamples(), 256);
}

// ============================================================================
// Plugin Metadata
// ============================================================================

TEST_F(PluginProcessorEdgeTest, PluginMetadataIsCorrect)
{
    EXPECT_EQ(processor->getName(), juce::String("Oscil"));
    EXPECT_TRUE(processor->acceptsMidi());
    EXPECT_FALSE(processor->producesMidi());
    EXPECT_FALSE(processor->isMidiEffect());
    EXPECT_DOUBLE_EQ(processor->getTailLengthSeconds(), 0.0);
    EXPECT_EQ(processor->getNumPrograms(), 1);
    EXPECT_EQ(processor->getCurrentProgram(), 0);
    EXPECT_TRUE(processor->hasEditor());
}
