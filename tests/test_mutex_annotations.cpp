/*
    MultiScoper - Mutex wrapper behavioral tests.

    Verifies that multiscoper::Mutex / multiscoper::SharedMutex and their scoped-lock
    RAII wrappers preserve the behavior of std::mutex / std::shared_mutex.
    Clang thread-safety-analysis annotations are compile-time; these tests
    only exercise runtime semantics (mutual exclusion, shared read access,
    try-lock contract, RAII release).
*/

#include "core/Mutex.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

// ============================================================================
// multiscoper::Mutex — exclusive mutual exclusion
// ============================================================================

TEST(MutexAnnotations, ExclusiveMutexSerializesIncrements)
{
    multiscoper::Mutex mutex;
    long long counter = 0;
    constexpr int kThreads = 8;
    constexpr int kIterations = 5000;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        workers.emplace_back([&]() {
            for (int i = 0; i < kIterations; ++i)
            {
                mutex.lock();
                ++counter;
                mutex.unlock();
            }
        });
    }
    for (auto& w : workers)
        w.join();

    // Serialized increments must sum exactly — no lost updates.
    EXPECT_EQ(counter, static_cast<long long>(kThreads) * kIterations);
}

TEST(MutexAnnotations, TryLockSucceedsWhenFreeAndFailsWhenHeld)
{
    multiscoper::Mutex mutex;

    // Free: try_lock must succeed.
    ASSERT_TRUE(mutex.try_lock());
    mutex.unlock();

    // Held by another thread: try_lock from main thread must fail.
    std::atomic<bool> holding{false};
    std::atomic<bool> release{false};
    std::thread holder([&]() {
        mutex.lock();
        holding.store(true);
        while (!release.load())
            std::this_thread::sleep_for(1ms);
        mutex.unlock();
    });

    while (!holding.load())
        std::this_thread::sleep_for(1ms);

    EXPECT_FALSE(mutex.try_lock());

    release.store(true);
    holder.join();

    // After release, try_lock succeeds again.
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}

// ============================================================================
// multiscoper::SharedMutex — shared (reader) access in parallel, exclusive blocks all
// ============================================================================

TEST(MutexAnnotations, SharedMutexAllowsConcurrentReaders)
{
    multiscoper::SharedMutex mutex;
    std::atomic<int> readersInside{0};
    std::atomic<int> peakReadersInside{0};
    constexpr int kReaders = 4;

    auto reader = [&]() {
        mutex.lock_shared();
        const int n = readersInside.fetch_add(1) + 1;
        int prev = peakReadersInside.load();
        while (n > prev && !peakReadersInside.compare_exchange_weak(prev, n))
        {
            // retry
        }
        std::this_thread::sleep_for(20ms);
        readersInside.fetch_sub(1);
        mutex.unlock_shared();
    };

    std::vector<std::thread> threads;
    threads.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i)
        threads.emplace_back(reader);
    for (auto& t : threads)
        t.join();

    // At least 2 readers must have overlapped, proving shared access is not serialized.
    EXPECT_GE(peakReadersInside.load(), 2);
}

TEST(MutexAnnotations, SharedMutexExclusiveWriterExcludesReaders)
{
    multiscoper::SharedMutex mutex;
    std::atomic<bool> writerHolding{false};
    std::atomic<bool> writerRelease{false};
    std::atomic<int> readersAcquiredDuringWrite{0};

    std::thread writer([&]() {
        mutex.lock();
        writerHolding.store(true);
        while (!writerRelease.load())
            std::this_thread::sleep_for(1ms);
        mutex.unlock();
    });

    while (!writerHolding.load())
        std::this_thread::sleep_for(1ms);

    // A reader attempt during exclusive ownership must not succeed via try_lock_shared.
    EXPECT_FALSE(mutex.try_lock_shared());
    readersAcquiredDuringWrite.fetch_add(0); // sanity: no shared acquisition occurred

    writerRelease.store(true);
    writer.join();

    // After writer releases, shared acquire succeeds.
    EXPECT_TRUE(mutex.try_lock_shared());
    mutex.unlock_shared();

    EXPECT_EQ(readersAcquiredDuringWrite.load(), 0);
}

// ============================================================================
// Scoped-lock RAII semantics
// ============================================================================

TEST(MutexAnnotations, ScopedLockReleasesOnScopeExit)
{
    multiscoper::Mutex mutex;
    {
        multiscoper::ScopedLock lock(mutex);
        // Cannot re-acquire while holding exclusive lock in this thread.
        EXPECT_FALSE(mutex.try_lock());
    }
    // After scope: available again.
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}

TEST(MutexAnnotations, ScopedSharedLockAllowsParallelReaders)
{
    multiscoper::SharedMutex mutex;
    std::atomic<int> inside{0};
    std::atomic<int> maxInside{0};
    constexpr int kReaders = 3;

    auto reader = [&]() {
        multiscoper::ScopedSharedLock lock(mutex);
        const int n = inside.fetch_add(1) + 1;
        int prev = maxInside.load();
        while (n > prev && !maxInside.compare_exchange_weak(prev, n))
        {
            // retry
        }
        std::this_thread::sleep_for(15ms);
        inside.fetch_sub(1);
    };

    std::vector<std::thread> threads;
    threads.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i)
        threads.emplace_back(reader);
    for (auto& t : threads)
        t.join();

    EXPECT_GE(maxInside.load(), 2);
    EXPECT_EQ(inside.load(), 0);
}

TEST(MutexAnnotations, ScopedLockOnSharedMutexGivesExclusiveAccess)
{
    multiscoper::SharedMutex mutex;
    long long counter = 0;
    constexpr int kThreads = 6;
    constexpr int kIterations = 2000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < kIterations; ++j)
            {
                multiscoper::ScopedLock lock(mutex);
                ++counter;
            }
        });
    }
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(counter, static_cast<long long>(kThreads) * kIterations);
}

} // namespace
