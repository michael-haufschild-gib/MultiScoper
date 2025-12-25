/*
    Oscil - Code Review Fixes Tests
    Tests for issues identified and fixed during comprehensive code review
*/

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"
#include "core/OscilState.h"
#include "rendering/RenderCommon.h"

using namespace oscil;

// ============================================================================
// SeqLock Retry Behavior Tests
// ============================================================================

class SeqLockTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SeqLockTest, ReadSucceedsUnderNormalConditions)
{
    AtomicMetadata metadata;
    
    // Write some data
    CaptureFrameMetadata writeData;
    writeData.sampleRate = 48000.0;
    writeData.numChannels = 2;
    writeData.timestamp = 12345;
    writeData.isPlaying = true;
    writeData.bpm = 140.0;
    
    metadata.write(writeData);
    
    // Read should succeed
    auto result = metadata.read();
    
    EXPECT_DOUBLE_EQ(result.sampleRate, 48000.0);
    EXPECT_EQ(result.numChannels, 2);
    EXPECT_EQ(result.timestamp, 12345);
    EXPECT_TRUE(result.isPlaying);
    EXPECT_DOUBLE_EQ(result.bpm, 140.0);
}

TEST_F(SeqLockTest, ConcurrentReadWriteDoesNotCrash)
{
    AtomicMetadata metadata;
    std::atomic<bool> stop{false};
    std::atomic<int> readCount{0};
    std::atomic<int> writeCount{0};
    
    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 1000 && !stop; ++i)
        {
            CaptureFrameMetadata data;
            data.sampleRate = 44100.0 + i;
            data.numChannels = 2;
            data.timestamp = i;
            metadata.write(data);
            ++writeCount;
        }
    });
    
    // Multiple reader threads
    std::vector<std::thread> readers;
    for (int r = 0; r < 4; ++r)
    {
        readers.emplace_back([&]() {
            while (!stop && writeCount < 1000)
            {
                auto result = metadata.read();
                // Just check read doesn't crash
                EXPECT_GE(result.sampleRate, 0.0);
                ++readCount;
            }
        });
    }
    
    writer.join();
    stop = true;
    for (auto& t : readers) t.join();
    
    EXPECT_EQ(writeCount.load(), 1000);
    EXPECT_GT(readCount.load(), 0);
}

// ============================================================================
// InstanceRegistry Thread Safety Tests
// ============================================================================

class InstanceRegistryThreadSafetyTest : public ::testing::Test
{
protected:
    std::mutex dispatcherMutex;
    
    static InstanceRegistry& getRegistry() { return InstanceRegistry::getInstance(); }
    
    void SetUp() override
    {
        // Synchronous dispatch for tests
        getRegistry().setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });
        
        // Clear existing registrations
        auto sources = getRegistry().getAllSources();
        for (const auto& source : sources)
        {
            getRegistry().unregisterInstance(source.sourceId);
        }
    }
    
    void TearDown() override
    {
        // Clean up
        auto sources = getRegistry().getAllSources();
        for (const auto& source : sources)
        {
            getRegistry().unregisterInstance(source.sourceId);
        }
        
        // Reset dispatcher
        getRegistry().setDispatcher([](std::function<void()> f) {
            juce::MessageManager::callAsync(std::move(f));
        });
    }
};

// The runtime guard test - listener operations should silently fail from wrong thread
// (Cannot test this directly without message manager, but we can verify the guard exists)

// ============================================================================
// OscilState Const-Correctness Tests
// ============================================================================

class OscilStateConstCorrectnessTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(OscilStateConstCorrectnessTest, ToXmlStringIsNonConst)
{
    OscilState state;
    
    // Add an oscillator
    Oscillator osc;
    osc.setName("Test Osc");
    state.addOscillator(osc);
    
    // toXmlString should work (it's non-const now)
    juce::String xml = state.toXmlString();
    EXPECT_TRUE(xml.isNotEmpty());
    EXPECT_TRUE(xml.contains("Test Osc"));
}

TEST_F(OscilStateConstCorrectnessTest, GetStateRemainsConst)
{
    const OscilState constState;
    
    // const getState() should still work
    const juce::ValueTree& tree = constState.getState();
    EXPECT_TRUE(tree.isValid());
}

// ============================================================================
// GlobalPreferences Async Save Tests
// ============================================================================

class GlobalPreferencesAsyncTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GlobalPreferencesAsyncTest, SettingPreferenceDoesNotBlock)
{
    GlobalPreferences prefs;
    
    auto start = std::chrono::steady_clock::now();
    
    // Set multiple preferences rapidly - should not block on each save
    for (int i = 0; i < 10; ++i)
    {
        prefs.setDefaultColumnLayout(i % 3 + 1);
    }
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should complete quickly (debounced async saves)
    // Allow 100ms which is generous - should be much faster
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 100);
}

// ============================================================================
// sRGB to Linear Color Conversion Tests
// ============================================================================

class ColorConversionTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ColorConversionTest, SRGBToLinearConversion)
{
    // Pure white should remain close to white
    auto white = sRGBToLinear(juce::Colours::white);
    EXPECT_NEAR(white.getFloatRed(), 1.0f, 0.01f);
    EXPECT_NEAR(white.getFloatGreen(), 1.0f, 0.01f);
    EXPECT_NEAR(white.getFloatBlue(), 1.0f, 0.01f);
    
    // Pure black should remain black
    auto black = sRGBToLinear(juce::Colours::black);
    EXPECT_NEAR(black.getFloatRed(), 0.0f, 0.01f);
    EXPECT_NEAR(black.getFloatGreen(), 0.0f, 0.01f);
    EXPECT_NEAR(black.getFloatBlue(), 0.0f, 0.01f);
    
    // Mid-grey (0.5) in sRGB should be darker in linear (~0.21)
    auto grey = sRGBToLinear(juce::Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 1.0f));
    EXPECT_LT(grey.getFloatRed(), 0.3f);  // Should be around 0.21
    EXPECT_GT(grey.getFloatRed(), 0.1f);
}

TEST_F(ColorConversionTest, LinearToSRGBConversion)
{
    // Round-trip conversion should be identity
    auto original = juce::Colour::fromFloatRGBA(0.7f, 0.3f, 0.5f, 1.0f);
    auto linear = sRGBToLinear(original);
    auto back = linearToSRGB(linear);
    
    EXPECT_NEAR(back.getFloatRed(), original.getFloatRed(), 0.01f);
    EXPECT_NEAR(back.getFloatGreen(), original.getFloatGreen(), 0.01f);
    EXPECT_NEAR(back.getFloatBlue(), original.getFloatBlue(), 0.01f);
}

TEST_F(ColorConversionTest, AlphaPreservedInConversion)
{
    auto withAlpha = juce::Colour::fromFloatRGBA(1.0f, 0.0f, 0.0f, 0.5f);
    auto linear = sRGBToLinear(withAlpha);
    
    EXPECT_NEAR(linear.getFloatAlpha(), 0.5f, 0.01f);
}

// ============================================================================
// Framebuffer Leak Counter Tests
// ============================================================================

#if OSCIL_ENABLE_OPENGL
#include "rendering/Framebuffer.h"

class FramebufferLeakCounterTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FramebufferLeakCounterTest, LeakCounterTracksAllocations)
{
    int initialCount = Framebuffer::getLeakCount();
    
    {
        Framebuffer fb1;
        EXPECT_EQ(Framebuffer::getLeakCount(), initialCount + 1);
        
        {
            Framebuffer fb2;
            EXPECT_EQ(Framebuffer::getLeakCount(), initialCount + 2);
        }
        
        EXPECT_EQ(Framebuffer::getLeakCount(), initialCount + 1);
    }
    
    EXPECT_EQ(Framebuffer::getLeakCount(), initialCount);
}
#endif


