# ADR-004: Lock Strategy by Thread Domain

## Status
Accepted

## Context
Oscil has three thread domains with different latency requirements:
1. **Audio thread** -- must never block. Budget: ~1ms per block at 48kHz/512 samples.
2. **Message (UI) thread** -- can block briefly. Budget: ~16ms per frame at 60fps.
3. **OpenGL thread** -- can block briefly but contention causes frame drops.

## Decision

| Primitive | Where Used | Why |
|-----------|-----------|-----|
| `SeqLock<T>` | `TimingEngine` config/host info, `SharedCaptureBuffer` metadata | Zero-allocation, zero-blocking for audio thread writer. Reader spins briefly (~ns). |
| `juce::SpinLock` | `RenderEngine::waveformStatesMutex_`, `DecimatingCaptureBuffer::bufferSwapLock_`, `PluginProcessor::stateLock_`, `PluginProcessor::captureConfigLock_` | Short critical sections (<1us). No syscall overhead. Used between message and render threads, never contested by audio thread. |
| `std::shared_mutex` | `InstanceRegistry::mutex_` | Read-heavy access pattern (many readers querying sources, few writers registering/unregistering). Registration is infrequent and non-realtime. |
| `std::mutex` | `GlobalPreferences::mutex_`, `MemoryBudgetManager::buffersMutex_` | Infrequent access from message thread only. Simple and correct. |
| `std::atomic<T>` | `Source::state_`, `TimingEngine::triggered_`, `PluginProcessor::cpuUsage_` | Single-word values read/written from different threads. No struct consistency needed. |
| `writeEpoch_` (atomic counter) | `SharedCaptureBuffer` | Torn-read detection for the ring buffer data array (separate from SeqLock for metadata). Reader brackets memcpy with epoch checks, retries on mismatch. |

## Rules
1. Audio thread: never acquire a mutex or SpinLock (use SeqLock, atomics, or tryLock with drop-on-contention).
2. SpinLock: only for critical sections under ~1us. Never for operations that allocate or do I/O.
3. std::mutex: only for message-thread-only operations or infrequent cross-thread access.

## Verification
- RTSan CI job (`OSCIL_ENABLE_RTSAN=ON`) detects any blocking/allocation in `processBlock()`.
- `tests/test_seqlock.cpp`, `tests/test_capture_buffer_threading.cpp`: concurrent correctness tests.
