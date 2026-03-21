/*
    Oscil - SeqLock
    Generic lock-free single-producer / multiple-consumer publish primitive
*/

#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>
#include <juce_core/juce_core.h>

namespace oscil
{

/**
 * Generic SeqLock for publishing a plain struct from one writer thread
 * to many reader threads without locks.
 *
 * Single-producer / multiple-consumer. The writer increments a sequence
 * counter to odd before writing, then to even after. Readers spin until
 * they observe two identical even sequence values bracketing their read.
 */
template <typename T>
struct SeqLock
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "SeqLock<T> requires T to be trivially copyable");

    void write(const T& value)
    {
        sequence.fetch_add(1, std::memory_order_release);   // odd → write in progress
        data.store(value, std::memory_order_relaxed);
        sequence.fetch_add(1, std::memory_order_release);   // even → write complete
    }

    T read() const
    {
        static constexpr int MAX_RETRIES = 1000;
        int retries = 0;
        for (;;)
        {
            uint32_t seq1 = sequence.load(std::memory_order_acquire);
            if (seq1 & 1)
            {
                if (++retries > MAX_RETRIES)
                {
                    // Writer appears stuck mid-write. Return best-effort data.
                    // This fires a debug breakpoint in development builds.
                    jassertfalse;
                    return data.load(std::memory_order_relaxed);
                }
                continue;
            }
            T snapshot = data.load(std::memory_order_relaxed);
            uint32_t seq2 = sequence.load(std::memory_order_acquire);
            if (seq1 == seq2)
                return snapshot;
            if (++retries > MAX_RETRIES)
            {
                // Sequence changed during read and retries exhausted. Return best-effort snapshot.
                jassertfalse;
                return snapshot;
            }
        }
    }

private:
    std::atomic<uint32_t> sequence{0};
    // std::atomic<T> requires T to be trivially copyable.
    std::atomic<T> data{T{}};
};

} // namespace oscil
