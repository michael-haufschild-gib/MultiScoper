/*
    Oscil - SeqLock
    Generic lock-free single-producer / multiple-consumer publish primitive
*/

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
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
 *
 * Uses memcpy for the data store/load, which is safe for any trivially
 * copyable type regardless of size. The sequence counter (not the data
 * atomicity) provides the torn-read protection.
 */
template <typename T>
struct SeqLock
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "SeqLock<T> requires T to be trivially copyable");

    void write(const T& value)
    {
        sequence_.fetch_add(1, std::memory_order_release);   // odd → write in progress
        std::memcpy(&data_, &value, sizeof(T));
        sequence_.fetch_add(1, std::memory_order_release);   // even → write complete
    }

    T read() const
    {
        static constexpr int MAX_RETRIES = 1000;
        int retries = 0;
        T snapshot;
        for (;;)
        {
            uint32_t seq1 = sequence_.load(std::memory_order_acquire);
            if (seq1 & 1)
            {
                if (++retries > MAX_RETRIES)
                {
                    // Writer appears stuck mid-write. Return best-effort data.
                    // This fires a debug breakpoint in development builds.
                    jassertfalse;
                    std::memcpy(&snapshot, &data_, sizeof(T));
                    return snapshot;
                }
                continue;
            }
            std::memcpy(&snapshot, &data_, sizeof(T));
            uint32_t seq2 = sequence_.load(std::memory_order_acquire);
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
    std::atomic<uint32_t> sequence_{0};
    alignas(T) T data_{};
};

} // namespace oscil
