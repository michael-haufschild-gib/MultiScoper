/*
    Oscil - PluginProcessor getStateInformation / setStateInformation
    race-path coverage.

    Context: some DAWs (Pro Tools, Reaper) call AudioProcessor::getStateInformation
    from the audio thread during autosave while processBlock is running. The
    non-message-thread branch in PluginProcessorState.cpp:14-44 uses a
    SpinLock::ScopedTryLock to grab the cached published bytes without
    allocating. That path currently has zero unit-test coverage — see
    docs/ci_reports/harness_capability_gaps.md items #5 and #9.

    These tests exercise the race shape directly with std::thread:

      * GetStateFromAudioThread_WhileProcessBlockRunning_NoDeadlock
      * GetStateFromAudioThread_TryLockContended_SilentlyDropsFrame
      * ConcurrentGetStateFromMultipleAudioThreads_NoTorn
      * SetStateInformation_BeforePrepareToPlay_SampleRateSnapshot
      * StateRoundTrip_SetThenGet_Deterministic
*/

#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/OscilState.h"
#include "ui/theme/ThemeManager.h"

#include "OscilTestFixtures.h"
#include "OscilTestUtils.h"
#include "plugin/PluginProcessor.h"
#include "rendering/PresetManager.h"
#include "rendering/ShaderRegistry.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace oscil;
using namespace oscil::test;

namespace
{
// Parse the returned blob and assert it is well-formed state XML.
// Returns the oscillator count (for the torn-read check).
int parseBlobAndGetOscillatorCount(const juce::MemoryBlock& block)
{
    if (block.getSize() == 0)
        return -1;

    auto xml = juce::String::createStringFromData(block.getData(), static_cast<int>(block.getSize()));
    OscilState parsed;
    if (!parsed.fromXmlString(xml))
        return -1;
    return parsed.getOscillatorCount();
}
} // namespace

class PluginProcessorStateRaceTest : public ::testing::Test
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

        // Disable GPU so prepareToPlay does not try to attach an OpenGL context
        // on a headless CI box.
        processor->getState().setGpuRenderingEnabled(false);
    }

    void TearDown() override
    {
        // Drain any callAsync()s queued by setStateInformation / deferRegistration
        // before tearing down the processor, otherwise WeakReferences fire post-dtor.
        pumpMessageQueue(100);
        processor.reset();
        pumpMessageQueue(50);
        memoryBudgetManager_.reset();
        presetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }

    // Seed a known state with a few oscillators so every getStateInformation
    // returns a non-trivial payload and the round-trip has something to compare.
    void seedState(int numOscillators = 3)
    {
        auto& state = processor->getState();
        for (int i = 0; i < numOscillators; ++i)
        {
            Oscillator osc;
            osc.setName("Osc " + juce::String(i));
            state.addOscillator(osc);
        }
    }
};

// =============================================================================
// 1. No deadlock: getStateInformation from a non-message thread while
//    processBlock runs on another thread.
// =============================================================================

TEST_F(PluginProcessorStateRaceTest, GetStateFromAudioThread_WhileProcessBlockRunning_NoDeadlock)
{
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(50);
    seedState(5);
    // Prime the published cache on the message thread so the non-MT path
    // has something to hand back.
    {
        juce::MemoryBlock prime;
        processor->getStateInformation(prime);
        ASSERT_GT(prime.getSize(), 0u);
    }

    std::atomic<bool> running{true};
    std::atomic<int> processIters{0};
    std::atomic<int> getStateIters{0};

    std::thread audio([this, &running, &processIters]() {
        juce::AudioBuffer<float> buffer(2, 256);
        juce::MidiBuffer midi;
        while (running.load(std::memory_order_relaxed))
        {
            buffer.clear();
            processor->processBlock(buffer, midi);
            processIters.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::thread autosave([this, &running, &getStateIters]() {
        // This thread is not the message thread, so getStateInformation takes
        // the real-time-safe tryLock branch in PluginProcessorState.cpp:22-31.
        while (running.load(std::memory_order_relaxed))
        {
            juce::MemoryBlock block;
            processor->getStateInformation(block);
            getStateIters.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running.store(false, std::memory_order_relaxed);

    audio.join();
    autosave.join();

    // Both loops must have made real progress — a deadlock would pin one at 0.
    EXPECT_GT(processIters.load(), 100) << "processBlock loop did not progress — likely deadlock";
    EXPECT_GT(getStateIters.load(), 100) << "getStateInformation loop did not progress — likely deadlock";
}

// =============================================================================
// 2. Under contention the non-MT path must never corrupt the destination.
//    The lock can only be held by updateCachedState (message thread). We
//    simulate contention by forcing a burst of message-thread serializations
//    while a worker hammers the non-MT path. The returned buffer must always
//    be either empty (dropped frame) or a fully-formed XML blob — never torn.
// =============================================================================

TEST_F(PluginProcessorStateRaceTest, GetStateFromAudioThread_TryLockContended_SilentlyDropsFrame)
{
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(50);
    seedState(4);
    // Prime.
    {
        juce::MemoryBlock prime;
        processor->getStateInformation(prime);
        ASSERT_GT(prime.getSize(), 0u);
    }

    std::atomic<bool> running{true};
    std::atomic<int> calls{0};
    std::atomic<int> parseFailures{0};
    std::atomic<int> drops{0};

    std::thread worker([this, &running, &calls, &parseFailures, &drops]() {
        while (running.load(std::memory_order_relaxed))
        {
            juce::MemoryBlock block;
            processor->getStateInformation(block);
            calls.fetch_add(1, std::memory_order_relaxed);

            if (block.getSize() == 0)
            {
                // Acceptable: lock was contended or nothing has been published yet.
                drops.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            // Non-empty → must parse as valid XML state. A torn read would fail here.
            auto xml = juce::String::createStringFromData(block.getData(), static_cast<int>(block.getSize()));
            OscilState parsed;
            if (!parsed.fromXmlString(xml))
                parseFailures.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Message thread: pump cached-state updates via repeated getStateInformation
    // calls from the MT path (which calls updateCachedState under the lock).
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(400);
    while (std::chrono::steady_clock::now() < deadline)
    {
        juce::MemoryBlock block;
        processor->getStateInformation(block);
        // Tiny breath to let the worker interleave.
        std::this_thread::yield();
    }
    running.store(false, std::memory_order_relaxed);
    worker.join();

    EXPECT_GT(calls.load(), 50) << "worker did not run enough iterations to exercise contention";
    EXPECT_EQ(parseFailures.load(), 0) << "Non-empty blob returned from non-MT path failed to parse — torn read";
    // drops >= 0 is fine; any drops must be silent (no corruption of destData).
}

// =============================================================================
// 3. Four workers calling the non-MT path in parallel — every non-empty blob
//    must round-trip through fromXmlString with the same oscillator count.
// =============================================================================

TEST_F(PluginProcessorStateRaceTest, ConcurrentGetStateFromMultipleAudioThreads_NoTorn)
{
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(50);
    constexpr int kSeed = 6;
    seedState(kSeed);
    {
        juce::MemoryBlock prime;
        processor->getStateInformation(prime);
        ASSERT_GT(prime.getSize(), 0u);
    }

    constexpr int kNumWorkers = 4;
    std::atomic<bool> running{true};
    std::atomic<int> totalReads{0};
    std::atomic<int> parseFailures{0};
    std::atomic<int> oscMismatches{0};

    std::vector<std::thread> workers;
    workers.reserve(kNumWorkers);
    for (int w = 0; w < kNumWorkers; ++w)
    {
        workers.emplace_back([this, &running, &totalReads, &parseFailures, &oscMismatches]() {
            while (running.load(std::memory_order_relaxed))
            {
                juce::MemoryBlock block;
                processor->getStateInformation(block);
                totalReads.fetch_add(1, std::memory_order_relaxed);
                if (block.getSize() == 0)
                    continue;
                int count = parseBlobAndGetOscillatorCount(block);
                if (count < 0)
                    parseFailures.fetch_add(1, std::memory_order_relaxed);
                else if (count != kSeed)
                    oscMismatches.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers)
        t.join();

    EXPECT_GT(totalReads.load(), 500) << "workers did not iterate enough";
    EXPECT_EQ(parseFailures.load(), 0) << "torn read (non-empty blob failed to parse)";
    EXPECT_EQ(oscMismatches.load(), 0) << "oscillator count diverged from seeded value — torn content";
}

// =============================================================================
// 4. setStateInformation before prepareToPlay — documents the sample-rate
//    snapshot behavior at PluginProcessorState.cpp:81. The snapshot captures
//    currentSampleRate_ at call time (constructor default = 44100). After a
//    later prepareToPlay(48000), the state must still be applied correctly and
//    the processor must report the new rate.
// =============================================================================

TEST_F(PluginProcessorStateRaceTest, SetStateInformation_BeforePrepareToPlay_SampleRateSnapshot)
{
    // Build a known state blob on a separate, prepared processor.
    juce::MemoryBlock savedState;
    {
        auto donor = std::make_unique<OscilPluginProcessor>(*registry_, *themeManager_, *shaderRegistry_,
                                                            *presetManager_, *memoryBudgetManager_);
        donor->getState().setGpuRenderingEnabled(false);
        donor->prepareToPlay(44100.0, 512);
        pumpMessageQueue(50);

        donor->getTimingEngine().setTimingMode(TimingMode::MELODIC);
        donor->getTimingEngine().setTimeIntervalMs(333);
        auto& ds = donor->getState();
        Oscillator osc;
        osc.setName("Pre-prepare oscillator");
        ds.addOscillator(osc);

        donor->getStateInformation(savedState);
        ASSERT_GT(savedState.getSize(), 0u);
        pumpMessageQueue(50);
    }

    // Fresh processor: setStateInformation runs BEFORE prepareToPlay, so the
    // sampleRate snapshot inside setStateInformation is the constructor default.
    ASSERT_DOUBLE_EQ(processor->getSampleRate(), 44100.0);

    processor->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));
    pumpMessageQueue(300);

    // Now prepare at a different rate. The captureBuffer_ must be reconfigured
    // to 48000 via deferRegistration (PluginProcessor.cpp:119-121), and the
    // restored timing state must still be intact.
    processor->prepareToPlay(48000.0, 512);
    pumpMessageQueue(300);

    EXPECT_DOUBLE_EQ(processor->getSampleRate(), 48000.0);

    const auto& cfg = processor->getTimingEngine().getConfig();
    EXPECT_EQ(cfg.timingMode, TimingMode::MELODIC) << "restored timing mode lost across prepareToPlay";
    EXPECT_FLOAT_EQ(cfg.timeIntervalMs, 333.0f) << "restored timing interval lost across prepareToPlay";

    EXPECT_EQ(processor->getState().getOscillatorCount(), 1) << "restored oscillator lost across prepareToPlay";

    // The source must have been (re)registered at the NEW rate — this catches
    // the regression where setStateInformation's sample-rate snapshot leaks
    // into the long-lived source metadata (docs/ci_reports/harness_capability_gaps.md #14).
    ASSERT_TRUE(processor->getSourceId().isValid());
    auto info = registry_->getSource(processor->getSourceId());
    ASSERT_TRUE(info.has_value());
    EXPECT_DOUBLE_EQ(info->sampleRate, 48000.0) << "source registered at stale 44100 rate after prepareToPlay(48000)";
}

// =============================================================================
// 5. Round-trip: set → get must produce XML parseable into an equivalent
//    state. We compare structural invariants (oscillator count, timing mode)
//    rather than byte-for-byte XML, since ValueTree serialization is not
//    guaranteed to be canonical across attribute ordering / whitespace.
// =============================================================================

TEST_F(PluginProcessorStateRaceTest, StateRoundTrip_SetThenGet_Deterministic)
{
    processor->prepareToPlay(44100.0, 512);
    pumpMessageQueue(50);

    processor->getTimingEngine().setTimingMode(TimingMode::TIME);
    processor->getTimingEngine().setTimeIntervalMs(250);
    processor->getTimingEngine().setHostSyncEnabled(false);

    Oscillator osc;
    osc.setName("Deterministic Round-Trip Osc");
    processor->getState().addOscillator(osc);

    // First serialization.
    juce::MemoryBlock blob1;
    processor->getStateInformation(blob1);
    ASSERT_GT(blob1.getSize(), 0u);

    // Apply it back (a no-op semantically).
    processor->setStateInformation(blob1.getData(), static_cast<int>(blob1.getSize()));
    pumpMessageQueue(200);

    // Second serialization after the round-trip.
    juce::MemoryBlock blob2;
    processor->getStateInformation(blob2);
    ASSERT_GT(blob2.getSize(), 0u);

    // Structural equivalence (stronger than "sizes match" — we actually parse).
    OscilState parsed1;
    OscilState parsed2;
    auto xml1 = juce::String::createStringFromData(blob1.getData(), static_cast<int>(blob1.getSize()));
    auto xml2 = juce::String::createStringFromData(blob2.getData(), static_cast<int>(blob2.getSize()));
    ASSERT_TRUE(parsed1.fromXmlString(xml1));
    ASSERT_TRUE(parsed2.fromXmlString(xml2));

    EXPECT_EQ(parsed1.getOscillatorCount(), parsed2.getOscillatorCount());
    EXPECT_EQ(parsed1.getOscillatorCount(), 1);

    // And the live processor's timing must still reflect what we set.
    const auto& cfg = processor->getTimingEngine().getConfig();
    EXPECT_EQ(cfg.timingMode, TimingMode::TIME);
    EXPECT_FLOAT_EQ(cfg.timeIntervalMs, 250.0f);
    EXPECT_FALSE(cfg.hostSyncEnabled);
}
