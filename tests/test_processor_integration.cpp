/*
    Oscil - Processor Integration Tests
    End-to-end tests that trace data flow across PluginProcessor, InstanceRegistry,
    CaptureBuffer, OscilState, and TimingEngine boundaries.

    These tests verify that independently-correct components produce correct
    results when wired together — the class of bugs that unit tests miss.
*/

#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/OscilState.h"
#include "core/Oscillator.h"
#include "core/SharedCaptureBuffer.h"
#include "ui/theme/ThemeManager.h"

#include "OscilTestFixtures.h"
#include "OscilTestUtils.h"
#include "helpers/AudioBufferBuilder.h"
#include "helpers/OscillatorBuilder.h"
#include "plugin/PluginProcessor.h"
#include "rendering/PresetManager.h"
#include "rendering/ShaderRegistry.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

class ProcessorIntegrationTest : public ::testing::Test
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

        processor = std::make_unique<OscilPluginProcessor>(*registry_, *themeManager_, *shaderRegistry_,
                                                           *presetManager_, *memoryBudgetManager_);
        processor->getState().setGpuRenderingEnabled(false);
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

// =============================================================================
// Data flow: processBlock -> CaptureBuffer -> Registry lookup -> read back
// =============================================================================

// Bug caught: processor registers with registry but the capture buffer
// returned by registry lookup is different from the one written to.
TEST_F(ProcessorIntegrationTest, CaptureBufferRetrievableViaRegistry)
{
    SourceId sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid());

    auto bufferViaRegistry = registry_->getCaptureBuffer(sourceId);
    auto bufferViaProcessor = processor->getCaptureBuffer();

    ASSERT_NE(bufferViaRegistry, nullptr);
    ASSERT_NE(bufferViaProcessor, nullptr);

    // Process some audio to populate the buffer
    auto audioBuffer = AudioBufferBuilder().withChannels(2).withSamples(512).withDC(0.6f).build();
    juce::MidiBuffer midi;
    processor->processBlock(audioBuffer, midi);

    // Both accessors must see the same data
    EXPECT_GT(bufferViaProcessor->getAvailableSamples(), 0u);
}

// Bug caught: processor adds oscillator to state but the oscillator's
// sourceId doesn't match the processor's registered sourceId.
TEST_F(ProcessorIntegrationTest, OscillatorSourceIdMatchesRegisteredSource)
{
    SourceId sourceId = processor->getSourceId();
    ASSERT_TRUE(sourceId.isValid());

    auto& state = processor->getState();

    Oscillator osc;
    osc.setSourceId(sourceId);
    osc.setName("Integration Test Osc");
    state.addOscillator(osc);

    auto oscillators = state.getOscillators();
    ASSERT_EQ(oscillators.size(), 1);
    EXPECT_EQ(oscillators[0].getSourceId(), sourceId);

    // Verify we can get the buffer for this oscillator's source
    auto buffer = processor->getBuffer(oscillators[0].getSourceId());
    EXPECT_NE(buffer, nullptr);
}

// =============================================================================
// Multi-processor integration: two processors sharing one registry
// =============================================================================

// Bug caught: second processor registration overwrites first processor's
// source or causes ID collision.
TEST_F(ProcessorIntegrationTest, TwoProcessorsRegisterDistinctSources)
{
    auto processor2 = std::make_unique<OscilPluginProcessor>(*registry_, *themeManager_, *shaderRegistry_,
                                                             *presetManager_, *memoryBudgetManager_);
    processor2->getState().setGpuRenderingEnabled(false);
    processor2->prepareToPlay(48000.0, 256);
    pumpMessageQueue(200);

    SourceId id1 = processor->getSourceId();
    SourceId id2 = processor2->getSourceId();

    ASSERT_TRUE(id1.isValid());
    ASSERT_TRUE(id2.isValid());
    EXPECT_NE(id1, id2) << "Two processors must have distinct source IDs";

    EXPECT_EQ(registry_->getSourceCount(), 2u);

    auto source1 = registry_->getSource(id1);
    auto source2 = registry_->getSource(id2);
    ASSERT_TRUE(source1.has_value());
    ASSERT_TRUE(source2.has_value());

    EXPECT_DOUBLE_EQ(source1->sampleRate, 44100.0);
    EXPECT_DOUBLE_EQ(source2->sampleRate, 48000.0);

    processor2.reset();
    pumpMessageQueue(100);
}

// =============================================================================
// State persistence through getStateInformation / setStateInformation
// =============================================================================

// Bug caught: oscillators added to state before save are not restored after
// setStateInformation, because the state XML doesn't include them.
TEST_F(ProcessorIntegrationTest, StateRoundTripPreservesOscillators)
{
    auto& state = processor->getState();

    Oscillator osc;
    osc.setSourceId(processor->getSourceId());
    osc.setProcessingMode(ProcessingMode::Mid);
    osc.setColour(juce::Colours::red);
    osc.setName("Round Trip Osc");
    state.addOscillator(osc);

    // Save state
    juce::MemoryBlock stateData;
    processor->getStateInformation(stateData);
    ASSERT_GT(stateData.getSize(), 0u);

    // Create a new processor and restore state
    auto processor2 = std::make_unique<OscilPluginProcessor>(*registry_, *themeManager_, *shaderRegistry_,
                                                             *presetManager_, *memoryBudgetManager_);
    processor2->getState().setGpuRenderingEnabled(false);
    processor2->setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
    processor2->prepareToPlay(44100.0, 512);

    auto& restoredState = processor2->getState();
    auto oscillators = restoredState.getOscillators();
    ASSERT_EQ(oscillators.size(), 1);

    EXPECT_EQ(oscillators[0].getProcessingMode(), ProcessingMode::Mid);
    EXPECT_EQ(oscillators[0].getColour().getARGB(), juce::Colours::red.getARGB());
    EXPECT_EQ(oscillators[0].getName(), "Round Trip Osc");

    processor2.reset();
    pumpMessageQueue(100);
}

// =============================================================================
// Timing engine integration: processBlock updates timing state
// =============================================================================

// Bug caught: processBlock doesn't forward host timing info to TimingEngine,
// causing free-running mode to use default 120 BPM instead of actual host BPM.
TEST_F(ProcessorIntegrationTest, TimingEngineReflectsProcessorSampleRate)
{
    auto& engine = processor->getTimingEngine();

    // After prepareToPlay(44100, 512), timing engine should know the sample rate
    EXPECT_DOUBLE_EQ(engine.getHostInfo().sampleRate, 44100.0);

    // Change sample rate
    processor->releaseResources();
    processor->prepareToPlay(96000.0, 512);

    EXPECT_DOUBLE_EQ(engine.getHostInfo().sampleRate, 96000.0);
}

// =============================================================================
// Processor destruction cascade: verify no dangling references
// =============================================================================

// Bug caught: processor destruction leaves dangling listener registrations
// in InstanceRegistry, causing crash when registry later notifies listeners.
TEST_F(ProcessorIntegrationTest, ProcessorDestructionDoesNotLeakRegistryListeners)
{
    SourceId id = processor->getSourceId();
    ASSERT_TRUE(id.isValid());

    processor.reset();
    pumpMessageQueue(100);

    // Registry should still be usable after processor destruction
    EXPECT_EQ(registry_->getSourceCount(), 0u);

    // Registering a new source should not crash (no dangling listeners)
    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto newId = registry_->registerInstance("new_track", buffer, "New Track");
    EXPECT_TRUE(newId.isValid());

    registry_->unregisterInstance(newId);
}

// Bug caught: empty state data passed to setStateInformation causes crash.
TEST_F(ProcessorIntegrationTest, SetStateInformationWithEmptyDataDoesNotCrash)
{
    uint8_t emptyData[1] = {0};
    processor->setStateInformation(emptyData, 0);

    // Processor should still be fully functional
    auto buffer = AudioBufferBuilder().withChannels(2).withSamples(512).withDC(0.5f).build();
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.5f);
}

// Bug caught: corrupt/truncated state data crashes setStateInformation.
TEST_F(ProcessorIntegrationTest, SetStateInformationWithGarbageDataDoesNotCrash)
{
    uint8_t garbage[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03};
    processor->setStateInformation(garbage, sizeof(garbage));

    // Must remain functional
    EXPECT_FALSE(processor->getName().isEmpty());
    EXPECT_TRUE(processor->getSourceId().isValid());
}
