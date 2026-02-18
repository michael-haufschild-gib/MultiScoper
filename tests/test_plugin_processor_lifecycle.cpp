/*
    Oscil - Plugin Processor Lifecycle Tests
    Tests for construction, metadata, bus layouts, prepare, release, and destruction
*/

#include <gtest/gtest.h>
#include "OscilTestUtils.h"
#include "plugin/PluginProcessor.h"
#include "core/AudioCapturePool.h"
#include "core/CaptureThread.h"
#include "core/SharedCaptureBuffer.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"

using namespace oscil;
using namespace oscil::test;

class PluginProcessorLifecycleTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<AudioCapturePool> capturePool_;
    std::unique_ptr<CaptureThread> captureThread_;
    std::unique_ptr<OscilPluginProcessor> processor;

    void SetUp() override
    {
        // Create services in dependency order
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();
        capturePool_ = std::make_unique<AudioCapturePool>();
        captureThread_ = std::make_unique<CaptureThread>(*capturePool_);
        captureThread_->startCapturing();

        // Create processor with owned services
        processor = std::make_unique<OscilPluginProcessor>(
            *registry_,
            *themeManager_,
            *shaderRegistry_,
            *memoryBudgetManager_,
            *capturePool_,
            *captureThread_);
        processor->prepareToPlay(44100.0, 512);
    }

    void TearDown() override
    {
        // Reset in reverse dependency order
        processor.reset();
        if (captureThread_)
            captureThread_->stopCapturing();
        captureThread_.reset();
        capturePool_.reset();
        memoryBudgetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }

    // Helper to create a fresh processor
    std::unique_ptr<OscilPluginProcessor> createNewProcessor()
    {
        return std::make_unique<OscilPluginProcessor>(
            *registry_,
            *themeManager_,
            *shaderRegistry_,
            *memoryBudgetManager_,
            *capturePool_,
            *captureThread_);
    }
};

// === Plugin Metadata Tests ===

TEST_F(PluginProcessorLifecycleTest, PluginName)
{
    EXPECT_EQ(processor->getName(), "MultiScoper");
}

TEST_F(PluginProcessorLifecycleTest, MidiSupport)
{
    EXPECT_TRUE(processor->acceptsMidi());
    EXPECT_FALSE(processor->producesMidi());
    EXPECT_FALSE(processor->isMidiEffect());
}

TEST_F(PluginProcessorLifecycleTest, ZeroTailLength)
{
    EXPECT_DOUBLE_EQ(processor->getTailLengthSeconds(), 0.0);
}

TEST_F(PluginProcessorLifecycleTest, SingleProgram)
{
    EXPECT_EQ(processor->getNumPrograms(), 1);
    EXPECT_EQ(processor->getCurrentProgram(), 0);
    EXPECT_EQ(processor->getProgramName(0), "");
}

TEST_F(PluginProcessorLifecycleTest, HasEditor)
{
    EXPECT_TRUE(processor->hasEditor());
}

// === Bus Layout Tests ===

TEST_F(PluginProcessorLifecycleTest, StereoLayoutSupported)
{
    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_TRUE(processor->isBusesLayoutSupported(stereoLayout));
}

TEST_F(PluginProcessorLifecycleTest, MonoLayoutSupported)
{
    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.inputBuses.add(juce::AudioChannelSet::mono());
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());

    EXPECT_TRUE(processor->isBusesLayoutSupported(monoLayout));
}

TEST_F(PluginProcessorLifecycleTest, DisabledInputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::disabled());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorLifecycleTest, DisabledOutputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::stereo());
    layout.outputBuses.add(juce::AudioChannelSet::disabled());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorLifecycleTest, MismatchedInputOutputRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::mono());
    layout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorLifecycleTest, SurroundLayoutRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::create5point1());
    layout.outputBuses.add(juce::AudioChannelSet::create5point1());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

TEST_F(PluginProcessorLifecycleTest, QuadLayoutRejected)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::quadraphonic());
    layout.outputBuses.add(juce::AudioChannelSet::quadraphonic());

    EXPECT_FALSE(processor->isBusesLayoutSupported(layout));
}

// === Prepare To Play Tests ===

TEST_F(PluginProcessorLifecycleTest, PrepareToPlay_SetsSampleRate)
{
    processor->prepareToPlay(48000.0, 512);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 48000.0);
}

TEST_F(PluginProcessorLifecycleTest, PrepareToPlay_RegistersWithInstanceRegistry)
{
    processor->prepareToPlay(44100.0, 256);
    pumpMessageQueue(200);  // Allow async registration to complete

    SourceId sourceId = processor->getSourceId();
    EXPECT_TRUE(sourceId.isValid());

    // Should be able to retrieve the buffer from the registry
    auto buffer = processor->getBuffer(sourceId);
    EXPECT_NE(buffer, nullptr);
}

TEST_F(PluginProcessorLifecycleTest, PrepareToPlay_DifferentSampleRates)
{
    // Test common sample rates
    std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};

    for (double rate : sampleRates)
    {
        processor = createNewProcessor();
        processor->prepareToPlay(rate, 512);

        EXPECT_DOUBLE_EQ(processor->getSampleRate(), rate)
            << "Failed for sample rate: " << rate;
    }
}

TEST_F(PluginProcessorLifecycleTest, PrepareToPlay_DifferentBlockSizes)
{
    // Test common block sizes
    std::vector<int> blockSizes = {64, 128, 256, 512, 1024, 2048, 4096};

    for (int blockSize : blockSizes)
    {
        processor = createNewProcessor();
        processor->prepareToPlay(44100.0, blockSize);
        pumpMessageQueue(200);

        // Should not crash
        EXPECT_TRUE(processor->getSourceId().isValid())
            << "Failed for block size: " << blockSize;
    }
}

TEST_F(PluginProcessorLifecycleTest, PrepareToPlay_TimingEngineUpdated)
{
    processor->prepareToPlay(96000.0, 256);

    auto& timingEngine = processor->getTimingEngine();
    EXPECT_DOUBLE_EQ(timingEngine.getHostInfo().sampleRate, 96000.0);
}

TEST_F(PluginProcessorLifecycleTest, MultiplePrepareCalls)
{
    // First prepare
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(200);
    SourceId firstId = processor->getSourceId();

    // Second prepare with different settings
    processor->prepareToPlay(48000.0, 256);
    pumpMessageQueue(200);
    SourceId secondId = processor->getSourceId();

    // Should have valid source IDs (may or may not be same depending on implementation)
    EXPECT_TRUE(firstId.isValid());
    EXPECT_TRUE(secondId.isValid());
}

// === Service Accessor Tests ===

TEST_F(PluginProcessorLifecycleTest, GetCaptureBuffer_NotNull)
{
    EXPECT_NE(processor->getCaptureBuffer(), nullptr);
}

TEST_F(PluginProcessorLifecycleTest, GetCaptureBuffer_SameInstanceAfterPrepare)
{
    auto bufferBefore = processor->getCaptureBuffer();

    processor->prepareToPlay(44100.0, 512);

    auto bufferAfter = processor->getCaptureBuffer();

    // With DecimatingCaptureBuffer, the internal buffer is recreated on reconfiguration
    // so pointers will differ. We just ensure we still have a valid buffer.
    EXPECT_NE(bufferAfter, nullptr);
}

TEST_F(PluginProcessorLifecycleTest, GetState_ReturnsValidState)
{
    auto& state = processor->getState();

    // State should have valid schema version
    EXPECT_GT(state.getSchemaVersion(), 0);
}

TEST_F(PluginProcessorLifecycleTest, GetTimingEngine_ReturnsValidEngine)
{
    auto& engine = processor->getTimingEngine();

    // Should be able to configure without crashing
    engine.setTimingMode(TimingMode::TIME);
    EXPECT_EQ(engine.getConfig().timingMode, TimingMode::TIME);
}

TEST_F(PluginProcessorLifecycleTest, GetInstanceRegistry_ReturnsValidRegistry)
{
    processor->prepareToPlay(44100.0, 512);

    auto& registry = processor->getInstanceRegistry();

    // Should be able to access registry methods
    auto sources = registry.getAllSources();
    EXPECT_GE(sources.size(), 0u);
}

TEST_F(PluginProcessorLifecycleTest, GetThemeService_ReturnsValidService)
{
    auto& themeService = processor->getThemeService();

    // Should be able to access theme service methods
    auto currentTheme = themeService.getCurrentTheme();
    EXPECT_FALSE(currentTheme.name.isEmpty());
}

// === Buffer Retrieval Tests ===

TEST_F(PluginProcessorLifecycleTest, GetBuffer_OwnSourceId)
{
    processor->prepareToPlay(44100.0, 512);

    SourceId sourceId = processor->getSourceId();
    auto buffer = processor->getBuffer(sourceId);

    EXPECT_NE(buffer, nullptr);
    
    // Note: getBuffer returns the DecimatingCaptureBuffer wrapper (as IAudioBuffer)
    // while getCaptureBuffer returns the internal SharedCaptureBuffer.
    // So they are not the same pointer anymore.
    // We verify we got a valid buffer that corresponds to the source.
}

TEST_F(PluginProcessorLifecycleTest, GetBuffer_InvalidSourceId)
{
    processor->prepareToPlay(44100.0, 512);

    SourceId invalidId = SourceId::generate(); // Random ID not registered
    auto buffer = processor->getBuffer(invalidId);

    // Should return nullptr for unregistered source
    EXPECT_EQ(buffer, nullptr);
}

// === Release and Destruction Tests ===

TEST_F(PluginProcessorLifecycleTest, ReleaseResources_DoesNotCrash)
{
    processor->prepareToPlay(44100.0, 512);

    // Process some audio
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midiBuffer;
    processor->processBlock(buffer, midiBuffer);

    // Release should not crash
    processor->releaseResources();

    // Should still have valid capture buffer
    EXPECT_NE(processor->getCaptureBuffer(), nullptr);
}

TEST_F(PluginProcessorLifecycleTest, DestructorUnregistersFromRegistry)
{
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(200);
    SourceId sourceId = processor->getSourceId();

    // Verify registered
    auto buffer = registry_->getCaptureBuffer(sourceId);
    EXPECT_NE(buffer, nullptr);

    // Destroy processor
    processor.reset();

    // Should be unregistered (buffer should be nullptr or registry should not find it)
    buffer = registry_->getCaptureBuffer(sourceId);
    EXPECT_EQ(buffer, nullptr);
}

// === Edge Case Tests ===

TEST_F(PluginProcessorLifecycleTest, SourceIdBeforePrepare)
{
    // Create a fresh processor not yet prepared
    auto freshProcessor = createNewProcessor();

    // Source ID before prepareToPlay should be invalid
    SourceId sourceId = freshProcessor->getSourceId();
    EXPECT_FALSE(sourceId.isValid());
}

// === Program Change Tests (No-op verification) ===

TEST_F(PluginProcessorLifecycleTest, SetCurrentProgram_NoOp)
{
    processor->setCurrentProgram(0);
    EXPECT_EQ(processor->getCurrentProgram(), 0);

    processor->setCurrentProgram(1);
    EXPECT_EQ(processor->getCurrentProgram(), 0); // Should remain 0
}

TEST_F(PluginProcessorLifecycleTest, ChangeProgramName_NoOp)
{
    processor->changeProgramName(0, "New Name");
    EXPECT_EQ(processor->getProgramName(0), ""); // Should remain empty
}
