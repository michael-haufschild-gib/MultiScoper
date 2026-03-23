/*
    Oscil - SeqLock
    Generic lock-free single-producer / multiple-consumer publish primitive
*/

#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>
#include <type_traits>

namespace oscil
{

/**
 * Generic SeqLock for publishing a plain struct from one writer thread
 * to many reader threads without locks.
 *
 * Single-producer / multiple-consumer. The writer increments a sequence
 * counter to odd before writing, then to even after. Readers spin until
 * they observe two identical even sequence values bracketing their read.
 *
 * Uses memcpy for the data store/load, which is safe for any trivially
 * copyable type regardless of size. The sequence counter (not the data
 * atomicity) provides the torn-read protection.
 */
template <typename T>
struct SeqLock
{
    static_assert(std::is_trivially_copyable_v<T>, "SeqLock<T> requires T to be trivially copyable");

    void write(const T& value)
    {
        sequence_.fetch_add(1, std::memory_order_acq_rel); // odd → write in progress
        std::memcpy(&data_, &value, sizeof(T));
        std::atomic_thread_fence(std::memory_order_release); // ensure memcpy visible before seq
        sequence_.fetch_add(1, std::memory_order_release);   // even → write complete
    }

    T read() const
    {
        T snapshot;
        for (;;)
        {
            uint32_t seq1 = sequence_.load(std::memory_order_acquire);
            if (seq1 & 1)
            {
                std::this_thread::yield();
                continue;
            }
            std::atomic_thread_fence(std::memory_order_acquire); // ensure data_ loads happen after seq1
            std::memcpy(&snapshot, &data_, sizeof(T));
            std::atomic_thread_fence(std::memory_order_acquire); // ensure data_ loads complete before seq2
            uint32_t seq2 = sequence_.load(std::memory_order_acquire);
            if (seq1 == seq2)
                return snapshot;
        }
    }

private:
    std::atomic<uint32_t> sequence_{0};
    alignas(T) T data_{};
};

} // namespace oscil
