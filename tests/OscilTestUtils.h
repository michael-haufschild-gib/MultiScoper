/*
    Oscil - Test Utilities

    Message queue pumping helper for tests that rely on async JUCE callbacks.
    (Test-server handlers use a separate, templated runOnMessageThread in
    include/tools/test_server/TestServerHandlerBase.h.)
*/

#pragma once

#include <juce_events/juce_events.h>

#include <chrono>

namespace oscil::test
{

/**
 * Pumps the JUCE message queue to process pending async operations.
 *
 * Use this when tests trigger callAsync() operations that need to complete
 * before assertions.
 *
 * On platforms with JUCE_MODAL_LOOPS_PERMITTED and when invoked from the
 * message thread, this uses runDispatchLoopUntil to actively process pending
 * messages. Otherwise it falls back to Thread::sleep, which still gives the
 * message thread a chance to run between sleep intervals.
 *
 * @param maxWaitMs Maximum time to pump messages (default 100ms)
 * @return Always true (signature kept bool for backward compatibility).
 */
inline bool pumpMessageQueue(int maxWaitMs = 100)
{
    auto const endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(maxWaitMs);
    constexpr int chunkMs = 10;

    while (std::chrono::steady_clock::now() < endTime)
    {
#if JUCE_MODAL_LOOPS_PERMITTED
        auto* mm = juce::MessageManager::getInstanceWithoutCreating();
        if (mm && mm->isThisTheMessageThread())
            mm->runDispatchLoopUntil(chunkMs);
        else
            juce::Thread::sleep(chunkMs);
#else
        juce::Thread::sleep(chunkMs);
#endif
    }

    return true;
}

} // namespace oscil::test
