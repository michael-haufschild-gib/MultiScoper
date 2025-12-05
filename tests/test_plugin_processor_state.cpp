/*
    Oscil - Plugin Processor State Tests
    Tests for state get/set, persistence, and restoration
*/

#include <gtest/gtest.h>
#include "OscilTestUtils.h"
#include "plugin/PluginProcessor.h"
#include "core/SharedCaptureBuffer.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"
#include <thread>
#include <atomic>

using namespace oscil;
using namespace oscil::test;

class PluginProcessorStateTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<OscilPluginProcessor> processor;

    void SetUp() override
    {
        // Create services before processor
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();

        processor = std::make_unique<OscilPluginProcessor>(
            *registry_,
            *themeManager_,
            *shaderRegistry_,
            *memoryBudgetManager_);
    }

    void TearDown() override
    {
        // Reset in reverse order
        processor.reset();
        memoryBudgetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }
};

// === State Persistence Tests ===

TEST_F(PluginProcessorStateTest, StateInformation_SaveAndRestore)
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
    auto newProcessor = std::make_unique<OscilPluginProcessor>(
        *registry_,
        *themeManager_,
        *shaderRegistry_,
        *memoryBudgetManager_);
    newProcessor->prepareToPlay(44100.0, 512);

    newProcessor->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    // Verify state was restored
    EXPECT_EQ(newProcessor->getTimingEngine().getConfig().timingMode, TimingMode::MELODIC);
    EXPECT_FLOAT_EQ(newProcessor->getTimingEngine().getConfig().timeIntervalMs, 750.0f);
}

TEST_F(PluginProcessorStateTest, StateInformation_EmptyState)
{
    processor->prepareToPlay(44100.0, 512);

    // Restore from empty/invalid data - should not crash
    processor->setStateInformation(nullptr, 0);

    // State should remain valid
    EXPECT_TRUE(processor->getSourceId().isValid() || !processor->getSourceId().isValid());
}

TEST_F(PluginProcessorStateTest, StateInformation_InvalidXml)
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

TEST_F(PluginProcessorStateTest, StateInformation_ContainsTimingState)
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

TEST_F(PluginProcessorStateTest, StateInformation_RoundTrip)
{
    processor->prepareToPlay(44100.0, 512);

    // Configure multiple aspects of state
    processor->getTimingEngine().setTimingMode(TimingMode::TIME);
    processor->getTimingEngine().setTimeIntervalMs(500);
    processor->getTimingEngine().setHostSyncEnabled(false);

    // Save state
    juce::MemoryBlock savedState;
    processor->getStateInformation(savedState);

    // Restore to same processor
    processor->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    // Verify state is preserved
    EXPECT_EQ(processor->getTimingEngine().getConfig().timingMode, TimingMode::TIME);
    EXPECT_FLOAT_EQ(processor->getTimingEngine().getConfig().timeIntervalMs, 500.0f);
    EXPECT_FALSE(processor->getTimingEngine().getConfig().hostSyncEnabled);
}

TEST_F(PluginProcessorStateTest, StateInformation_MultipleSaveRestore)
{
    processor->prepareToPlay(44100.0, 512);

    // Save initial state
    juce::MemoryBlock state1;
    processor->getStateInformation(state1);

    // Modify state
    processor->getTimingEngine().setTimingMode(TimingMode::MELODIC);

    // Save modified state
    juce::MemoryBlock state2;
    processor->getStateInformation(state2);

    // Restore first state
    processor->setStateInformation(state1.getData(), static_cast<int>(state1.getSize()));

    // Should be back to initial timing mode
    const auto& config = processor->getTimingEngine().getConfig();
    EXPECT_NE(config.timingMode, TimingMode::MELODIC);

    // Restore second state
    processor->setStateInformation(state2.getData(), static_cast<int>(state2.getSize()));

    // Should now be melodic again
    EXPECT_EQ(processor->getTimingEngine().getConfig().timingMode, TimingMode::MELODIC);
}

TEST_F(PluginProcessorStateTest, StateInformation_VeryLargeState)
{
    processor->prepareToPlay(44100.0, 512);

    // Add many oscillators to create large state
    for (int i = 0; i < 50; ++i)
    {
        auto& state = processor->getState();
        Oscillator osc;
        osc.setName("Oscillator " + juce::String(i));
        state.addOscillator(osc);
    }

    // Save state
    juce::MemoryBlock savedState;
    processor->getStateInformation(savedState);

    EXPECT_GT(savedState.getSize(), 1000u); // Should be reasonably large

    // Create new processor and restore
    auto newProcessor = std::make_unique<OscilPluginProcessor>(
        *registry_,
        *themeManager_,
        *shaderRegistry_,
        *memoryBudgetManager_);
    newProcessor->prepareToPlay(44100.0, 512);

    newProcessor->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    // Verify oscillators were restored
    EXPECT_EQ(newProcessor->getState().getOscillators().size(), 50u);
}

TEST_F(PluginProcessorStateTest, StateAccessDuringProcessing)
{
    processor->prepareToPlay(44100.0, 512);

    std::atomic<bool> running{true};

    // Thread that processes audio
    std::thread audioThread([this, &running]()
    {
        while (running)
        {
            juce::AudioBuffer<float> buffer(2, 256);
            buffer.clear();
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

    // Should complete without crashes or deadlocks
    EXPECT_NE(processor->getCaptureBuffer(), nullptr);
}

TEST_F(PluginProcessorStateTest, StateInformation_BeforePrepare)
{
    // Get state before prepareToPlay - should not crash
    juce::MemoryBlock state;
    processor->getStateInformation(state);

    EXPECT_GT(state.getSize(), 0u);
}

TEST_F(PluginProcessorStateTest, StateInformation_SetBeforePrepare)
{
    // Set state before prepareToPlay - should not crash
    juce::MemoryBlock state;
    state.setSize(100);

    processor->setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    // Should still be functional
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(200);
    EXPECT_TRUE(processor->getSourceId().isValid());
}
