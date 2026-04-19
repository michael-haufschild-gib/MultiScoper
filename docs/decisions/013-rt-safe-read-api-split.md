# ADR 013: Split IAudioBuffer / SeqLock read API into Blocking and Snapshot Variants

**Status**: Accepted

**Date**: 2026-04-14

## Context

`IAudioBuffer::read(...)` and `SeqLock<T>::read()` both previously exposed a
single "blocks until consistent" read contract. Implementations spin on a
SeqLock-style epoch counter and fall back to `std::this_thread::yield()` when
they observe a writer in progress.

`std::this_thread::yield()` is a syscall. It is NOT real-time safe and must
not appear on any audio-thread code path. The single-API design hid this
constraint at the call site — a consumer could not tell from the signature
whether it was allowed to call `read()` from the audio thread.

Today every MultiScoper consumer of `IAudioBuffer::read` is on the UI/message
thread (WaveformPresenter, tests, etc.), so the current behavior is
accidentally correct. As new features (real-time analysis, RT transients
metering, RTSan-enforced builds) are added, an audio-thread call to
`read()` would silently regress real-time safety.

## Decision

Split each read API into two variants with names that make the thread
domain explicit at the call site:

### IAudioBuffer

- `int readBlocking(float* output, int numSamples, int channel = 0) const`
- `int readBlocking(juce::AudioBuffer<float>& output, int numSamples) const`
  - Spins with `std::this_thread::yield()` until a consistent snapshot is
    observed. UI/message thread only. NOT real-time safe.

- `std::optional<int> readSnapshot(float* output, int numSamples, int channel = 0) const`
- `std::optional<int> readSnapshot(juce::AudioBuffer<float>& output, int numSamples) const`
  - Single epoch-bracketed attempt. Returns `std::nullopt` on torn read or
    writer-in-progress observation. Never yields. Safe from any thread
    including the audio thread. Callers must handle the `nullopt` case
    (typical policy: skip frame, reuse last good result).

### SeqLock<T>

- `T readBlocking() const` — the previous `read()` semantics, renamed.
- `std::optional<T> tryRead() const` — single-pass, real-time safe.

### Concrete Buffers

- `SharedCaptureBuffer` implements both variants directly on its epoch
  counter. The core copy logic is factored into file-local helpers so
  `readBlocking` and `readSnapshot` share identical torn-read semantics;
  only the retry policy (loop+yield vs. return nullopt) differs.
- `DecimatingCaptureBuffer::readBlocking` continues to take its
  `bufferSwapLock_` SpinLock (safe on the UI thread). Its
  `readSnapshot` variants bypass that lock and use the already-present
  lock-free `publishedBuffer_` atomic (kept alive for ~2s by the
  graveyard pattern from ADR 002), making the snapshot path also safe
  against concurrent `reconfigure()` calls from the message thread.

## Consequences

- **New consumers can be safely placed on the audio thread.** Any
  audio-thread read site simply uses `readSnapshot` / `tryRead` and
  falls back to the last cached value on `nullopt`.

- **Existing callers retain their behavior** via the mechanical
  `read` → `readBlocking` rename.

- **TimingEngine pre-existing issue not resolved by this ADR.**
  `TimingEngine::processBlock`, `processMidi`, `updateHostBPM`, and
  `detectTrigger` are all on the audio thread and call
  `configLock_.readBlocking()` / `hostInfoLock_.readBlocking()`. Those
  are renames, not behavioral migrations. The pre-existing yield-on-
  contention is extremely rare (config writes only happen from user UI
  interaction), but a follow-up should migrate those sites to
  `tryRead()` + a cached fallback before RTSan enforcement is turned on.

- **No migration burden beyond renaming for today's codebase.** A grep
  of production code before this ADR showed exactly one non-self
  consumer of `IAudioBuffer::read` (`WaveformPresenter::readAndPadSamples`,
  UI thread) and one of `SeqLock::read` (TimingEngine, as above). Both
  are renamed to `readBlocking`.

- **Test coverage added.** `tests/test_audio_buffer_thread_safety.cpp`
  exercises `readSnapshot` under writer contention for both
  `SharedCaptureBuffer` and `DecimatingCaptureBuffer`, asserting that
  every non-nullopt return is self-consistent (no torn data) and that
  per-call wall-clock time stays bounded (proxy for "no yield syscall
  on torn contention"). `tests/test_seqlock.cpp` adds `tryRead`
  coverage for the lock-free single-pass path.

## Migration Guidance

When adding a new consumer of `IAudioBuffer` or a new `SeqLock<T>`
reader:

1. **Identify the thread domain** of the caller.
2. **UI/message thread** (paint, timer, menu handler, pluginEditor
   handler): use `readBlocking` / `readBlocking()`.
3. **Audio thread** (`processBlock`, anything invoked from it): use
   `readSnapshot` / `tryRead()`. Maintain a cached last-good value in
   the consumer and use it when the call returns `nullopt`.
4. **Worker thread with RT requirements** (e.g. GL render thread if
   treated as RT): same as audio thread.

Never introduce a new call to `readBlocking` / `readBlocking()` from
the audio thread.
