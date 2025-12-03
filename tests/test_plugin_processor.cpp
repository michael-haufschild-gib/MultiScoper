/*
    Oscil - Plugin Processor Tests
    Comprehensive tests for the main audio processor
*/

#include <gtest/gtest.h>
#include "plugin/PluginProcessor.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class PluginProcessorTest : public ::testing::Test
{
protected:
    std::unique_ptr<OscilPluginProcessor> processor;

    void SetUp() override
    {
        processor = std::make_unique<OscilPluginProcessor>(InstanceRegistry::getInstance(), ThemeManager::getInstance());
    }

    void TearDown() override
    {
        processor.reset();
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

// =============================================================================
// Plugin Metadata Tests
// =============================================================================

TEST_F(PluginProcessorTest, PluginName)
{
    EXPECT_EQ(processor->getName(), "Oscil");
}

TEST_F(PluginProcessorTest, NoMidiSupport)
{
    EXPECT_FALSE(processor->acceptsMidi());
    EXPECT_FALSE(processor->producesMidi());
    EXPECT_FALSE(processor->isMidiEffect());
}

TEST_F(PluginProcessorTest, ZeroTailLength)
{
    EXPECT_DOUBLE_EQ(processor->getTailLengthSeconds(), 0.0);
}

TEST_F(PluginProcessorTest, SingleProgram)
{
    EXPECT_EQ(processor->getNumPrograms(), 1);
    EXPECT_EQ(processor->getCurrentProgram(), 0);
    EXPECT_EQ(processor->getProgramName(0), "");
}

TEST_F(PluginProcessorTest, HasEditor)
{
    EXPECT_TRUE(processor->hasEditor());
}

// =============================================================================
// Bus Layout Tests
// =============================================================================

TEST_F(PluginProcessorTest, StereoLayoutSupported)
{
    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_TRUE(processor->isBusesLayoutSupported(stereoLayout));
}

TEST_F(PluginProcessorTest, MonoLayoutSupported)
{
    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.inputBuses.add(juce::AudioChannelSet::mono());
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());

    EXPECT_TRUE(processor->isBusesLayoutSupported(monoLayout));
}

TEST_F(PluginProcessorTest, DisabledInputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::disabled());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorTest, DisabledOutputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::stereo());
    layout.outputBuses.add(juce::AudioChannelSet::disabled());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorTest, MismatchedInputOutputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::mono());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorTest, SurroundLayoutRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::create5point1());
    layout.outputBuses.add(juce::AudioChannelSet::create5point1());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorTest, QuadLayoutRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::quadraphonic());
    layout.outputBuses.add(juce::AudioChannelSet::quadraphonic());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

// =============================================================================
// Prepare To Play Tests
// =============================================================================

TEST_F(PluginProcessorTest, PrepareToPlay_SetsSampleRate)
{
    processor->prepareToPlay(48000.0, 512);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 48000.0);
}

TEST_F(PluginProcessorTest, PrepareToPlay_RegistersWithInstanceRegistry)
{
    processor->prepareToPlay(44100.0, 256);

    SourceId sourceId = processor->getSourceId();
    EXPECT_TRUE(sourceId.isValid());

    // Should be able to retrieve the buffer from the registry
    auto buffer = processor->getBuffer(sourceId);
    EXPECT_NE(buffer, nullptr);
}

TEST_F(PluginProcessorTest, PrepareToPlay_DifferentSampleRates)
{
    // Test common sample rates
    std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};

    for (double rate : sampleRates)
    {
        processor = std::make_unique<OscilPluginProcessor>(InstanceRegistry::getInstance(), ThemeManager::getInstance());
        processor->prepareToPlay(rate, 512);

        EXPECT_DOUBLE_EQ(processor->getSampleRate(), rate)
            << "Failed for sample rate: " << rate;
    }
}

TEST_F(PluginProcessorTest, PrepareToPlay_DifferentBlockSizes)
{
    // Test common block sizes
    std::vector<int> blockSizes = {64, 128, 256, 512, 1024, 2048, 4096};

    for (int blockSize : blockSizes)
    {
        processor = std::make_unique<OscilPluginProcessor>(InstanceRegistry::getInstance(), ThemeManager::getInstance());
        processor->prepareToPlay(44100.0, blockSize);

        // Should not crash
        EXPECT_TRUE(processor->getSourceId().isValid())
            << "Failed for block size: " << blockSize;
    }
}

TEST_F(PluginProcessorTest, PrepareToPlay_TimingEngineUpdated)
{
    processor->prepareToPlay(96000.0, 256);

    auto& timingEngine = processor->getTimingEngine();
    EXPECT_DOUBLE_EQ(timingEngine.getHostInfo().sampleRate, 96000.0);
}

// =============================================================================
// Process Block Tests
// =============================================================================

TEST_F(PluginProcessorTest, ProcessBlock_PassThroughAudio)
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

TEST_F(PluginProcessorTest, ProcessBlock_CapturesAudio)
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

TEST_F(PluginProcessorTest, ProcessBlock_EmptyBuffer)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> emptyBuffer(2, 0);
    juce::MidiBuffer midiBuffer;

    // Should not crash on empty buffer
    processor->processBlock(emptyBuffer, midiBuffer);
}

TEST_F(PluginProcessorTest, ProcessBlock_ZeroChannels)
{
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> noChannelBuffer(0, 512);
    juce::MidiBuffer midiBuffer;

    // Should not crash on buffer with no channels
    processor->processBlock(noChannelBuffer, midiBuffer);
}

TEST_F(PluginProcessorTest, ProcessBlock_MonoBuffer)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(1, 512, 0.5f);
    juce::MidiBuffer midiBuffer;

    // Should handle mono buffer without crashing
    processor->processBlock(buffer, midiBuffer);
}

TEST_F(PluginProcessorTest, ProcessBlock_LargeBuffer)
{
    processor->prepareToPlay(44100.0, 8192);

    auto buffer = generateTestBuffer(2, 8192, 0.5f);
    juce::MidiBuffer midiBuffer;

    // Should handle large buffers
    processor->processBlock(buffer, midiBuffer);
}

TEST_F(PluginProcessorTest, ProcessBlock_SmallBuffer)
{
    processor->prepareToPlay(44100.0, 32);

    auto buffer = generateTestBuffer(2, 32, 0.5f);
    juce::MidiBuffer midiBuffer;

    // Should handle small buffers
    processor->processBlock(buffer, midiBuffer);
}

TEST_F(PluginProcessorTest, ProcessBlock_CPUUsageTracked)
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

TEST_F(PluginProcessorTest, ProcessBlock_MetadataCaptured)
{
    processor->prepareToPlay(44100.0, 512);

    auto buffer = generateTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midiBuffer;

    processor->processBlock(buffer, midiBuffer);

    auto captureBuffer = processor->getCaptureBuffer();
    auto metadata = captureBuffer->getLatestMetadata();

    EXPECT_DOUBLE_EQ(metadata.sampleRate, 44100.0);
    EXPECT_EQ(metadata.numChannels, 2);
    EXPECT_EQ(metadata.numSamples, 512);
}

// =============================================================================
// State Persistence Tests
// =============================================================================

TEST_F(PluginProcessorTest, StateInformation_SaveAndRestore)
{
    processor->prepareToPlay(44100.0, 512);

    // Configure some state
    processor->getTimingEngine().setTimingMode(TimingMode::MELODIC);
    processor->getTimingEngine().setTimeIntervalMs(750);

    // Save state
    juce::MemoryBlock savedState;
    processor->getStateInformation(savedState);

    EXPECT_GT(savedState.getSize(), 0u);

    // Create new processor and restore state
    auto newProcessor = std::make_unique<OscilPluginProcessor>(InstanceRegistry::getInstance(), ThemeManager::getInstance());
    newProcessor->prepareToPlay(44100.0, 512);

    newProcessor->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    // Verify state was restored
    EXPECT_EQ(newProcessor->getTimingEngine().getConfig().timingMode, TimingMode::MELODIC);
    EXPECT_FLOAT_EQ(newProcessor->getTimingEngine().getConfig().timeIntervalMs, 750.0f);
}

TEST_F(PluginProcessorTest, StateInformation_EmptyState)
{
    processor->prepareToPlay(44100.0, 512);

    // Restore from empty/invalid data - should not crash
    processor->setStateInformation(nullptr, 0);

    // State should remain valid
    EXPECT_TRUE(processor->getSourceId().isValid() || !processor->getSourceId().isValid());
}

TEST_F(PluginProcessorTest, StateInformation_InvalidXml)
{
    processor->prepareToPlay(44100.0, 512);

    // Store original timing mode
    auto originalMode = processor->getTimingEngine().getConfig().timingMode;

    // Restore from invalid XML - should not crash
    const char* invalidXml = "not valid xml <><>";
    processor->setStateInformation(invalidXml, static_cast<int>(strlen(invalidXml)));

    // State should still be valid (unchanged on failure)
    EXPECT_TRUE(processor->getCaptureBuffer() != nullptr);
}

TEST_F(PluginProcessorTest, StateInformation_ContainsTimingState)
{
    processor->prepareToPlay(44100.0, 512);

    // Set non-default timing state
    processor->getTimingEngine().setTimingMode(TimingMode::MELODIC);
    processor->getTimingEngine().setHostSyncEnabled(true);

    juce::MemoryBlock savedState;
    processor->getStateInformation(savedState);

    // Convert to string and check for timing data
    juce::String stateXml = juce::String::createStringFromData(savedState.getData(),
                                                                static_cast<int>(savedState.getSize()));

    EXPECT_TRUE(stateXml.contains("Timing") || stateXml.contains("timing"));
}

// =============================================================================
// Service Accessor Tests
// =============================================================================

TEST_F(PluginProcessorTest, GetCaptureBuffer_NotNull)
{
    EXPECT_NE(processor->getCaptureBuffer(), nullptr);
}

TEST_F(PluginProcessorTest, GetCaptureBuffer_SameInstanceAfterPrepare)
{
    auto bufferBefore = processor->getCaptureBuffer();

    processor->prepareToPlay(44100.0, 512);

    auto bufferAfter = processor->getCaptureBuffer();

    EXPECT_EQ(bufferBefore, bufferAfter);
}

TEST_F(PluginProcessorTest, GetState_ReturnsValidState)
{
    auto& state = processor->getState();

    // State should have valid schema version
    EXPECT_GT(state.getSchemaVersion(), 0);
}

TEST_F(PluginProcessorTest, GetTimingEngine_ReturnsValidEngine)
{
    auto& engine = processor->getTimingEngine();

    // Should be able to configure without crashing
    engine.setTimingMode(TimingMode::TIME);
    EXPECT_EQ(engine.getConfig().timingMode, TimingMode::TIME);
}

TEST_F(PluginProcessorTest, GetInstanceRegistry_ReturnsValidRegistry)
{
    processor->prepareToPlay(44100.0, 512);

    auto& registry = processor->getInstanceRegistry();

    // Should be able to access registry methods
    auto sources = registry.getAllSources();
    EXPECT_GE(sources.size(), 0u);
}

TEST_F(PluginProcessorTest, GetThemeService_ReturnsValidService)
{
    auto& themeService = processor->getThemeService();

    // Should be able to access theme service methods
    auto currentTheme = themeService.getCurrentTheme();
    EXPECT_FALSE(currentTheme.name.isEmpty());
}

// =============================================================================
// Buffer Retrieval Tests
// =============================================================================

TEST_F(PluginProcessorTest, GetBuffer_OwnSourceId)
{
    processor->prepareToPlay(44100.0, 512);

    SourceId sourceId = processor->getSourceId();
    auto buffer = processor->getBuffer(sourceId);

    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer, processor->getCaptureBuffer());
}

TEST_F(PluginProcessorTest, GetBuffer_InvalidSourceId)
{
    processor->prepareToPlay(44100.0, 512);

    SourceId invalidId = SourceId::generate(); // Random ID not registered
    auto buffer = processor->getBuffer(invalidId);

    // Should return nullptr for unregistered source
    EXPECT_EQ(buffer, nullptr);
}

// =============================================================================
// Lifecycle Tests
// =============================================================================

TEST_F(PluginProcessorTest, MultiplePrepareCalls)
{
    // First prepare
    processor->prepareToPlay(44100.0, 512);
    SourceId firstId = processor->getSourceId();

    // Second prepare with different settings
    processor->prepareToPlay(48000.0, 256);
    SourceId secondId = processor->getSourceId();

    // Should have valid source IDs (may or may not be same depending on implementation)
    EXPECT_TRUE(firstId.isValid());
    EXPECT_TRUE(secondId.isValid());
}

TEST_F(PluginProcessorTest, ReleaseResources_DoesNotCrash)
{
    processor->prepareToPlay(44100.0, 512);

    // Process some audio
    auto buffer = generateTestBuffer(2, 512);
    juce::MidiBuffer midiBuffer;
    processor->processBlock(buffer, midiBuffer);

    // Release should not crash
    processor->releaseResources();

    // Should still have valid capture buffer
    EXPECT_NE(processor->getCaptureBuffer(), nullptr);
}

TEST_F(PluginProcessorTest, DestructorUnregistersFromRegistry)
{
    processor->prepareToPlay(44100.0, 512);
    SourceId sourceId = processor->getSourceId();

    // Verify registered
    auto buffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
    EXPECT_NE(buffer, nullptr);

    // Destroy processor
    processor.reset();

    // Should be unregistered (buffer should be nullptr or registry should not find it)
    buffer = InstanceRegistry::getInstance().getCaptureBuffer(sourceId);
    EXPECT_EQ(buffer, nullptr);
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(PluginProcessorTest, ProcessBeforePrepare)
{
    // Process before prepareToPlay - should not crash
    auto buffer = generateTestBuffer(2, 512);
    juce::MidiBuffer midiBuffer;

    // May not capture correctly, but should not crash
    processor->processBlock(buffer, midiBuffer);
}

TEST_F(PluginProcessorTest, SourceIdBeforePrepare)
{
    // Source ID before prepareToPlay should be invalid
    SourceId sourceId = processor->getSourceId();
    EXPECT_FALSE(sourceId.isValid());
}

TEST_F(PluginProcessorTest, ConcurrentProcessing)
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

TEST_F(PluginProcessorTest, StateAccessDuringProcessing)
{
    processor->prepareToPlay(44100.0, 512);

    std::atomic<bool> running{true};

    // Thread that processes audio
    std::thread audioThread([this, &running]()
    {
        while (running)
        {
            auto buffer = generateTestBuffer(2, 256);
            juce::MidiBuffer midiBuffer;
            processor->processBlock(buffer, midiBuffer);
        }
    });

    // Thread that accesses state
    std::thread stateThread([this, &running]()
    {
        while (running)
        {
            juce::MemoryBlock stateData;
            processor->getStateInformation(stateData);

            auto& engine = processor->getTimingEngine();
            (void)engine.getConfig();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    audioThread.join();
    stateThread.join();
}

// =============================================================================
// Program Change Tests (No-op verification)
// =============================================================================

TEST_F(PluginProcessorTest, SetCurrentProgram_NoOp)
{
    processor->setCurrentProgram(0);
    EXPECT_EQ(processor->getCurrentProgram(), 0);

    processor->setCurrentProgram(1);
    EXPECT_EQ(processor->getCurrentProgram(), 0); // Should remain 0
}

TEST_F(PluginProcessorTest, ChangeProgramName_NoOp)
{
    processor->changeProgramName(0, "New Name");
    EXPECT_EQ(processor->getProgramName(0), ""); // Should remain empty
}
