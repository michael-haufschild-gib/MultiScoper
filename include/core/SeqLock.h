/*
    MultiScoper - SeqLock
    Generic lock-free single-producer / multiple-consumer publish primitive
*/

#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <optional>
#include <thread>
#include <type_traits>

namespace multiscoper
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
 *
 * Two read variants are provided:
 *   - readBlocking() — spins with std::this_thread::yield() until a
 *     consistent snapshot is observed. Correct for UI/message threads
 *     but NOT real-time safe (yield is a syscall).
 *   - tryRead() — single pass; returns std::nullopt if a writer is in
 *     progress or a torn read is detected. Real-time safe; no syscalls.
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

    /**
     * Blocking read — spins with yield until a consistent snapshot is
     * observed. Use from UI/message threads only. Not real-time safe.
     */
    T readBlocking() const
    {
        T snapshot;
        for (;;)
        {
            uint32_t const seq1 = sequence_.load(std::memory_order_acquire);
            if ((seq1 & 1) != 0u)
            {
                std::this_thread::yield();
                continue;
            }
            std::atomic_thread_fence(std::memory_order_acquire); // ensure data_ loads happen after seq1
            std::memcpy(&snapshot, &data_, sizeof(T));
            std::atomic_thread_fence(std::memory_order_acquire); // ensure data_ loads complete before seq2
            uint32_t const seq2 = sequence_.load(std::memory_order_acquire);
            if (seq1 == seq2)
                return snapshot;
        }
    }

    /**
     * Non-blocking, real-time-safe single-pass read. Returns std::nullopt
     * if a writer is currently in progress or a torn read is detected.
     * Callers should have a fallback (e.g. cached last value, skip frame).
     */
    [[nodiscard]] std::optional<T> tryRead() const
    {
        uint32_t const seq1 = sequence_.load(std::memory_order_acquire);
        if ((seq1 & 1) != 0u)
            return std::nullopt;

        T snapshot;
        std::atomic_thread_fence(std::memory_order_acquire);
        std::memcpy(&snapshot, &data_, sizeof(T));
        std::atomic_thread_fence(std::memory_order_acquire);

        uint32_t const seq2 = sequence_.load(std::memory_order_acquire);
        if (seq1 != seq2)
            return std::nullopt;

        return snapshot;
    }

private:
    std::atomic<uint32_t> sequence_{0};
    alignas(T) T data_{};
};

} // namespace multiscoper
