/*
    Oscil - Instance Registry Dispatcher Tests
    Behavioural coverage for InstanceRegistry::setDispatcher:
      * empty-callable rejection (preserves prior dispatcher),
      * thread-safety of the dispatcher snapshot against concurrent notifications.
*/

#include "core/InstanceRegistry.h"
#include "core/SharedCaptureBuffer.h"

#include "Oscil.h"

#include <juce_events/juce_events.h>

#include <atomic>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

namespace oscil
{

class InstanceRegistryDispatcherTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::mutex dispatcherMutex;

    InstanceRegistry& getRegistry() { return *registry_; }

    void SetUp() override
    {
        registry_ = std::make_unique<InstanceRegistry>();

        // Synchronous dispatch for deterministic assertions.
        getRegistry().setDispatcher([this](std::function<void()> f) {
            std::scoped_lock lock(dispatcherMutex);
            f();
        });
    }

    void TearDown() override { registry_.reset(); }
};

class CountingRegistryListener final : public InstanceRegistryListener
{
public:
    void sourceAdded(const SourceId&) override { ++addedCount; }
    void sourceRemoved(const SourceId&) override { ++removedCount; }
    void sourceUpdated(const SourceId&) override { ++updatedCount; }

    int addedCount = 0;
    int removedCount = 0;
    int updatedCount = 0;
};

TEST_F(InstanceRegistryDispatcherTest, RejectsEmptyCallable)
{
    // Passing an empty std::function must not brick future notifications.
    // After a no-op attempt the previously installed dispatcher must still
    // handle events — otherwise dispatchNotification would throw
    // std::bad_function_call and terminate.
    CountingRegistryListener listener;
    getRegistry().addListener(&listener);

    getRegistry().setDispatcher({});

    auto buffer = std::make_shared<SharedCaptureBuffer>();
    auto id = getRegistry().registerInstance("dispatcher_guard", buffer, "Dispatcher Guard");
    EXPECT_TRUE(id.isValid());

    EXPECT_EQ(listener.addedCount, 1) << "setDispatcher({}) must leave the previously installed dispatcher in place";

    getRegistry().removeListener(&listener);
}

TEST_F(InstanceRegistryDispatcherTest, IsThreadSafeAgainstConcurrentNotifications)
{
    // Regression test for the std::function assignment race in dispatchNotification.
    // Without the dispatcherMutex_ snapshot in dispatchNotification, the main
    // (message) thread replacing dispatcher_ while a worker thread reads it
    // during registerInstance is a data race (std::function has no thread-safe
    // copy guarantee).
    //
    // Concurrency model:
    //   main thread   : setDispatcher — satisfies the message-thread jassert
    //                   because GTest main IS the message thread under the
    //                   JUCE harness.
    //   worker thread : registerInstance — triggers dispatchNotification,
    //                   which snapshots dispatcher_ under dispatcherMutex_.
    //
    // Under tsan the race would be reported. In a race-free build the test
    // verifies (a) no crash, (b) no deadlock, (c) every registration triggers
    // at least one dispatcher invocation.
    constexpr int kIterations = 200;

    std::atomic<int> dispatchCountA{0};
    std::atomic<int> dispatchCountB{0};

    auto dispatcherA = [&dispatchCountA](std::function<void()> f) {
        dispatchCountA.fetch_add(1, std::memory_order_relaxed);
        if (f)
            f();
    };
    auto dispatcherB = [&dispatchCountB](std::function<void()> f) {
        dispatchCountB.fetch_add(1, std::memory_order_relaxed);
        if (f)
            f();
    };

    getRegistry().setDispatcher(dispatcherA);

    std::atomic<int> registrationsDone{0};
    std::thread worker([&]() {
        // Reuse the same trackIdentifier so dedup treats each call as an
        // update (dispatchNotification path) rather than consuming a slot in
        // the bounded registry.
        auto buffer = std::make_shared<SharedCaptureBuffer>();
        const juce::String stableTrackId = "dispatcher_race_stable";
        for (int i = 0; i < kIterations; ++i)
        {
            auto id = getRegistry().registerInstance(stableTrackId, buffer, "Track " + juce::String(i));
            EXPECT_TRUE(id.isValid());
            registrationsDone.fetch_add(1, std::memory_order_release);
        }
    });

    while (registrationsDone.load(std::memory_order_acquire) < kIterations)
    {
        getRegistry().setDispatcher(dispatcherB);
        getRegistry().setDispatcher(dispatcherA);
    }

    worker.join();

    const int totalDispatches = dispatchCountA.load() + dispatchCountB.load();
    EXPECT_GE(totalDispatches, kIterations) << "Every registerInstance must trigger at least one dispatcher invocation";
}

} // namespace oscil
