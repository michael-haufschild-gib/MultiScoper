/*
    Oscil - Test Utilities

    Provides common utilities for unit testing JUCE-based components:
    - Message queue pumping for async operations
    - Timeout helpers
    - Mock service factories
*/

#pragma once

#include <juce_events/juce_events.h>

#include <chrono>
#include <functional>
#include <gtest/gtest.h>

namespace oscil::test
{

/**
 * Pumps the JUCE message queue to process pending async operations.
 *
 * Use this when tests trigger callAsync() operations that need to complete
 * before assertions.
 *
 * Note: On platforms without JUCE_MODAL_LOOPS_PERMITTED, this uses Thread::sleep
 * which allows the message thread to process messages between sleep intervals.
 *
 * @param maxWaitMs Maximum time to pump messages (default 100ms)
 * @param condition Optional condition to check - returns early when true
 * @return true if condition was met (or no condition specified), false if timeout
 */
inline bool pumpMessageQueue(int maxWaitMs = 100, std::function<bool()> condition = nullptr)
{
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::milliseconds(maxWaitMs);

    // Process in small chunks to allow condition checking
    const int chunkMs = 10;

    while (std::chrono::steady_clock::now() < endTime)
    {
        // Check condition first
        if (condition && condition())
            return true;

#if JUCE_MODAL_LOOPS_PERMITTED
        // When modal loops are permitted, we can use runDispatchLoopUntil
        // to actually process pending messages
        auto* mm = juce::MessageManager::getInstanceWithoutCreating();
        if (mm && mm->isThisTheMessageThread())
        {
            mm->runDispatchLoopUntil(chunkMs);
        }
        else
        {
            juce::Thread::sleep(chunkMs);
        }
#else
        // Fall back to Thread::sleep, which gives the message thread
        // time to process async callbacks between iterations
        juce::Thread::sleep(chunkMs);
#endif
    }

    return condition ? condition() : true;
}

/**
 * Waits for a condition to become true, pumping messages while waiting.
 *
 * @param condition Function that returns true when condition is met
 * @param maxWaitMs Maximum time to wait
 * @return true if condition was met, false if timeout
 */
inline bool waitForCondition(std::function<bool()> condition, int maxWaitMs = 1000)
{
    return pumpMessageQueue(maxWaitMs, condition);
}

/**
 * Runs a lambda on the message thread and waits for completion.
 * Useful for tests that need to interact with UI components.
 *
 * @param func Function to execute on message thread
 * @param maxWaitMs Maximum time to wait for completion
 * @return true if function completed, false if timeout
 */
inline bool runOnMessageThread(std::function<void()> func, int maxWaitMs = 1000)
{
    auto* mm = juce::MessageManager::getInstanceWithoutCreating();
    if (!mm)
    {
        func();
        return true;
    }

    if (mm->isThisTheMessageThread())
    {
        func();
        return true;
    }

    std::atomic<bool> completed{false};

    juce::MessageManager::callAsync([&func, &completed]() {
        func();
        completed.store(true, std::memory_order_release);
    });

    return waitForCondition([&completed]() { return completed.load(std::memory_order_acquire); }, maxWaitMs);
}

/**
 * RAII guard that temporarily stops and restarts a timer.
 * Useful for tests that don't want timer callbacks firing.
 */
class ScopedTimerStop
{
public:
    explicit ScopedTimerStop(juce::Timer& timer) : timer_(timer), wasRunning_(timer.isTimerRunning())
    {
        if (wasRunning_)
            timer_.stopTimer();
    }

    ~ScopedTimerStop()
    {
        if (wasRunning_)
            timer_.startTimerHz(60); // Default refresh rate
    }

private:
    juce::Timer& timer_;
    bool wasRunning_;
};

} // namespace oscil::test
