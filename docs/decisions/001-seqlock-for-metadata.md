# ADR-001: SeqLock for Cross-Thread Metadata

## Status
Accepted

## Context
The audio thread writes `CaptureFrameMetadata` (sample rate, BPM, timestamp, channel count) every processBlock call. The UI thread reads this metadata 60 times per second for display. The audio thread cannot block.

## Options Considered
1. **std::mutex** -- Simple but blocks audio thread on contention. Unacceptable for real-time audio.
2. **std::atomic<shared_ptr<const Metadata>>** -- Lock-free but allocates on every write (heap allocation in audio thread). Not real-time safe.
3. **SeqLock (sequence lock)** -- Writer increments a sequence counter to odd before writing, to even after. Readers retry if they see an odd counter or mismatched pre/post counters. No allocation, no blocking, no lock.
4. **Triple buffering** -- Reader always sees a complete snapshot but adds memory overhead and complexity for small structs.

## Decision
SeqLock. It is the simplest lock-free mechanism for single-producer/multiple-consumer publishing of small trivially-copyable structs. The `CaptureFrameMetadata` struct is 40 bytes -- well within a cache line on modern CPUs.

## Consequences
- Audio thread never blocks.
- Readers may retry (spin) briefly during a write, but writes are fast memcpy operations (~nanoseconds).
- `T` must be trivially copyable (enforced by static_assert in SeqLock template).
- Reader sees either the latest complete value or the previous complete value, never a torn mix.

## Verification
- `tests/test_seqlock.cpp`: concurrent writer/reader stress tests verify zero torn reads across 100K+ iterations.
- `tests/test_capture_buffer_threading.cpp`: production metadata struct tested under concurrent load.
