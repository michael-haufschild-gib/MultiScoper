/*
    Oscil - Timing Engine Tests (Listeners & Thread Safety)
*/

#include "test_timing_engine_fixture.h"

class TestTimingListener : public TimingEngine::Listener
{
public:
    int modeChangedCount = 0;
    int intervalChangedCount = 0;
    int bpmChangedCount = 0;
    int syncStateChangedCount = 0;
    int timeSignatureChangedCount = 0;

    TimingMode lastMode = TimingMode::TIME;
    float lastInterval = 0.0f;
    float lastBPM = 0.0f;
    bool lastSyncState = false;
    int lastNumerator = 4;
    int lastDenominator = 4;

    void timingModeChanged(TimingMode mode) override
    {
        modeChangedCount++;
        lastMode = mode;
    }

    void intervalChanged(float actualIntervalMs) override
    {
        intervalChangedCount++;
        lastInterval = actualIntervalMs;
    }

    void hostBPMChanged(float bpm) override
    {
        bpmChangedCount++;
        lastBPM = bpm;
    }

    void hostSyncStateChanged(bool enabled) override
    {
        syncStateChangedCount++;
        lastSyncState = enabled;
    }

    void timeSignatureChanged(int numerator, int denominator) override
    {
        timeSignatureChangedCount++;
        lastNumerator = numerator;
        lastDenominator = denominator;
    }
};

TEST_F(TimingEngineTest, Listener_DeterministicNotification)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // 1. Change mode
    engine.setTimingMode(TimingMode::MELODIC);
    
    // Verify listener NOT called yet (it's pending)
    EXPECT_EQ(listener.modeChangedCount, 0);
    
    // Dispatch updates
    engine.dispatchPendingUpdates();
    
    // Verify listener CALLED
    EXPECT_EQ(listener.modeChangedCount, 1);
    EXPECT_EQ(listener.lastMode, TimingMode::MELODIC);

    // 2. Change Interval
    // Switch back to TIME mode first so timeIntervalMs affects actualIntervalMs
    engine.setTimingMode(TimingMode::TIME);
    engine.dispatchPendingUpdates(); // Dispatch mode change and potential interval change (if switching to TIME changed it)
    
    // Reset counts for clarity or just check increment
    int prevIntervalCount = listener.intervalChangedCount;
    
    engine.setTimeIntervalMs(123.4f);
    // Should be pending
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount);
    
    engine.dispatchPendingUpdates();
    // Should have incremented
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount + 1);
    
    engine.setTimeIntervalMs(250.0f);
    engine.dispatchPendingUpdates();
    EXPECT_EQ(listener.intervalChangedCount, prevIntervalCount + 2);
    EXPECT_FLOAT_EQ(listener.lastInterval, 250.0f);

    engine.removeListener(&listener);
}

TEST_F(TimingEngineTest, Listener_AddAndRemove)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // Change mode
    engine.setTimingMode(TimingMode::MELODIC);
    engine.dispatchPendingUpdates();
    EXPECT_EQ(listener.modeChangedCount, 1);

    engine.removeListener(&listener);

    // Change mode again
    engine.setTimingMode(TimingMode::TIME);
    engine.dispatchPendingUpdates();
    
    // Should NOT have incremented
    EXPECT_EQ(listener.modeChangedCount, 1);
}

TEST_F(TimingEngineTest, FromValueTree_ClearsPendingNotificationFlags)
{
    TestTimingListener listener;
    engine.addListener(&listener);

    // Queue all pending notification flags while returning to persisted defaults.
    engine.setTimingMode(TimingMode::MELODIC);
    engine.setTimingMode(TimingMode::TIME);
    engine.setTimeIntervalMs(700.0f);
    engine.setTimeIntervalMs(500.0f);
    engine.setHostSyncEnabled(true);
    engine.setHostSyncEnabled(false);

    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(147.0);
    posInfo.setTimeSignature(juce::AudioPlayHead::TimeSignature{ 7, 8 });
    engine.updateHostInfo(posInfo);

    EXPECT_EQ(listener.modeChangedCount, 0);
    EXPECT_EQ(listener.intervalChangedCount, 0);
    EXPECT_EQ(listener.bpmChangedCount, 0);
    EXPECT_EQ(listener.syncStateChangedCount, 0);
    EXPECT_EQ(listener.timeSignatureChangedCount, 0);

    auto persisted = engine.toValueTree();
    engine.fromValueTree(persisted);
    engine.dispatchPendingUpdates();

    EXPECT_EQ(listener.modeChangedCount, 0);
    EXPECT_EQ(listener.intervalChangedCount, 0);
    EXPECT_EQ(listener.bpmChangedCount, 0);
    EXPECT_EQ(listener.syncStateChangedCount, 0);
    EXPECT_EQ(listener.timeSignatureChangedCount, 0);

    engine.removeListener(&listener);
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_F(TimingEngineTest, ThreadSafety_ConcurrentConfigChanges)
{
    std::atomic<bool> running{true};
    std::atomic<int> iterations{0};

    // Thread that changes timing mode
    std::thread modeChanger([this, &running, &iterations]()
    {
        while (running)
        {
            engine.setTimingMode(TimingMode::TIME);
            engine.setTimingMode(TimingMode::MELODIC);
            iterations++;
        }
    });

    // Thread that reads config
    std::thread configReader([this, &running]()
    {
        while (running)
        {
            auto config = engine.getConfig();
            auto interval = engine.getActualIntervalMs();
            (void)config;
            (void)interval;
        }
    });

    // Thread that changes interval
    std::thread intervalChanger([this, &running]()
    {
        int i = 0;
        while (running)
        {
            engine.setTimeIntervalMs(100 + (i % 1000));
            i++;
        }
    });

    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    modeChanger.join();
    configReader.join();
    intervalChanger.join();

    // Should have completed many iterations without crashing
    EXPECT_GT(iterations.load(), 10);
}

TEST_F(TimingEngineTest, ThreadSafety_ManualTriggerAtomic)
{
    engine.setWaveformTriggerMode(WaveformTriggerMode::Manual);

    std::atomic<int> triggerCount{0};
    std::atomic<bool> running{true};

    // Thread that requests triggers
    std::thread requester([this, &running]()
    {
        while (running)
        {
            engine.requestManualTrigger();
        }
    });

    // Thread that checks/clears triggers
    std::thread checker([this, &running, &triggerCount]()
    {
        while (running)
        {
            if (engine.checkAndClearManualTrigger())
            {
                triggerCount++;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;

    requester.join();
    checker.join();

    // Should have detected some triggers
    EXPECT_GT(triggerCount.load(), 0);
}
