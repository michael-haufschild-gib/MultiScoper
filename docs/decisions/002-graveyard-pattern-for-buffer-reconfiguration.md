# ADR-002: Graveyard Pattern for Buffer Reconfiguration

## Status

Accepted

## Context
`DecimatingCaptureBuffer` may be reconfigured from the message thread (user changes quality preset) while the audio thread is writing to the current buffer. Reconfiguration replaces the internal `SharedCaptureBuffer` and `ProcessingContext`. The old objects must not be destroyed while the audio thread holds a pointer to them.

## Options Considered

1. **std::atomic<shared_ptr>** -- Not yet portable across all target compilers (C++20 `std::atomic<shared_ptr>` support varies).
2. **Stop audio thread, swap, restart** -- Causes audible glitch. Unacceptable.
3. **Graveyard (deferred destruction)** -- Old buffer and context are moved to a timestamped "graveyard" vector. The message thread periodically calls `cleanUpGarbage()` to destroy items older than a safe threshold.
4. **Read-copy-update (RCU)** -- Correct but adds significant complexity for a single-producer scenario.

## Decision

Graveyard pattern. It is simple, correct, and requires no special library support. The SpinLock on `bufferSwapLock_` serializes pointer swaps (message thread) and pointer reads (UI/render thread). The audio thread uses `tryLock` (a single CAS atomic — not a blocking lock) and skips the write if contended, dropping frames rather than blocking. This preserves the lock-free guarantee on the audio thread: `tryLock` either succeeds immediately or returns false without spinning or waiting.

## Consequences

- Old buffers stay in memory for a short period after reconfiguration.
- `cleanUpGarbage()` must be called periodically from the message thread (currently done in `PluginEditor::timerCallback`).
- Memory usage temporarily doubles during reconfiguration (old + new buffer coexist briefly).

## Verification

- `tests/test_decimating_capture_buffer_concurrent.cpp`: concurrent write during reconfigure.
- `tests/test_decimating_capture_buffer_edge.cpp`: graveyard cleanup tests.
