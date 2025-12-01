# Oscil Codebase – Concurrency and Memory Review

_Date: 2025-12-01_

This document summarizes a repository-wide review of **thread safety** and **memory/lifetime** risks in the Oscil codebase. It focuses on likely hotspots rather than exhaustively listing every line.

For each area, we list:
- What the code does
- Potential issues or assumptions
- Suggested mitigations / things to harden

---

## 1. Audio / DSP / Core Concurrency

### 1.1 `SharedCaptureBuffer`

**Files**: `include/core/SharedCaptureBuffer.h`, `src/core/SharedCaptureBuffer.cpp`

**Role**: Lock-free ring buffer for real-time audio capture and visualization (audio thread → UI thread).

**Key patterns**:
- Metadata is stored in `AtomicMetadata` using multiple `std::atomic<...>` fields.
- Readers use a seqlock-style `read()` that retries while a write is in progress (`sequence` odd).
- After `MAX_RETRIES` iterations, reader falls back to best-effort relaxed loads.

**Risks / assumptions**:
- After exceeding `MAX_RETRIES`, metadata can be internally inconsistent (e.g., `numChannels`, `numSamples`, and offsets may not correspond perfectly).
- Any code that assumes metadata is *always* fully consistent across fields may index into buffers out of bounds or make incorrect layout assumptions.
- All cross-thread metadata must live in `AtomicMetadata` and be atomic; any related non-atomic field accessed from multiple threads would be a data race.

**Mitigations**:
- Ensure all metadata used by readers for buffer indexing (e.g. channel count, sample count, write indices) is clamped and validated before use.
- Avoid relying on exact equality or strict invariants per read; treat metadata as advisory in UI.
- Confirm no additional non-atomic shared metadata exists outside `AtomicMetadata` that is touched on both audio and UI threads.

---

### 1.2 `InstanceRegistry`

**Files**: `include/core/InstanceRegistry.h`, `src/core/InstanceRegistry.cpp`

**Role**: Singleton registry managing sources and shared capture buffers; used for multi-instance coordination and UI access.

**Key patterns**:
- Uses `std::mutex` / `std::shared_mutex` to guard internal maps.
- Exposes `std::shared_ptr<SharedCaptureBuffer>` via `getCaptureBuffer(...)`.
- Listener API (`InstanceRegistryListener`) for UI or coordination logic.

**Risks / assumptions**:
- **Mutex usage from audio thread**: any call into `InstanceRegistry` from `processBlock()` (or functions it calls) that acquires a mutex is **not RT-safe**. This is mainly a concern if the registry is ever used directly on audio path.
- **Listener callbacks under a lock**: if listener notifications are made while holding the registry mutex, and listeners re-enter `InstanceRegistry` or take other locks, potential for deadlock exists.
- **Shared ownership**: `std::shared_ptr<SharedCaptureBuffer>` is safe for lifetime but may encourage holding buffers longer than expected (buffers might outlive source removal if other holders exist).

**Mitigations**:
- Treat `InstanceRegistry` as **message/background-thread-only**. Avoid using it from the audio thread entirely. If unavoidable, ensure such code does not take locks (e.g. only reading atomics or pre-published pointers).
- Verify the implementation ensures:
  - `addListener` / `removeListener` lock `listener`-related state.
  - Listener notifications copy the listener set under a lock, release the lock, then invoke callbacks outside any mutex.
- Document clearly which public methods are safe from which threads.
- Optionally, add runtime asserts in debug builds (e.g. detecting audio-thread access).

---

### 1.3 `Source`

**Files**: `include/core/Source.h`, `src/core/Source.cpp`, tests in `tests/test_source.cpp`

**Role**: Represents a DAW track/source, holds signal and correlation metrics, timestamps, and state.

**Key patterns**:
- Tests such as `ConcurrentUpdateLastAudioTime` and `ConcurrentMetricsUpdate` exercise concurrent API usage from multiple threads.
- Metrics and internal state are accessed in both readers and writers concurrently.

**Risks / assumptions**:
- If internal fields storing metrics or timestamps are plain (non-atomic) values and are written/read from multiple threads without synchronization, this is a **data race** even if tests “do not crash”.
- Returning composite metric structs by value that are built from multiple atomics must be done carefully to avoid partial/torn reads across fields.

**Mitigations**:
- Ensure that **every** field potentially read/written from multiple threads is either:
  - `std::atomic<...>` with appropriate memory ordering, or
  - Protected by a mutex with consistent locking discipline.
- For composite structs returned by value (e.g. `getCorrelationMetrics`, `getSignalMetrics`):
  - Construct them locally from atomics in a single function, then return that local copy.
  - Avoid exposing references or pointers to internal mutable state.

---

### 1.4 `TimingEngine`

**Files**: `include/dsp/TimingEngine.h`, `src/dsp/TimingEngine.cpp`, `tests/test_timing_engine.cpp`

**Role**: Synchronization/timing engine (TIME/MELODIC modes, triggers, DAW sync).

**Key patterns**:
- UI thread writes configuration via setters → values stored in `std::atomic` members.
- Audio thread reads from atomics in `processBlock`/update functions.
- `listeners_` is `juce::ListenerList<Listener>`; `listenerMutex_` (std::mutex) is documented as protecting add/remove.
- Tests stress concurrent config changes, reads, and manual triggers.

**Risks / assumptions**:
- **Listener locking**:
  - `listenerMutex_` is intended for add/remove only; `ListenerList::call()` is assumed thread-safe. However, improper usage (e.g. calling `listeners_.call(...)` under `listenerMutex_`) can deadlock if listeners call back into `addListener`/`removeListener` or other locked methods.
- **Non-atomic fields**:
  - Fields like `previousSample_`, `previousTriggerState_`, `previousBPM_` are plain members. If any of these are written on audio thread and read on UI thread (or vice versa), they introduce a data race.
  - From context, they appear intended to be **audio-thread-only**. That must remain true.
- **Dispatch on wrong thread**:
  - `dispatchPendingUpdates()` uses atomics and then calls `listeners_.call(...)`. If this is ever called from the audio thread and listeners do UI work (e.g. repaint, access Components), it violates thread-domain rules.

**Mitigations**:
- Enforce and document that:
  - All listener notifications (`listeners_.call`) run on the **message thread** (or a dedicated background thread), never on the audio thread.
  - `addListener`/`removeListener` take `listenerMutex_`; `listeners_.call` runs without that mutex held.
- Confirm that `previousSample_`, `previousTriggerState_`, `previousBPM_` are **only** touched on a single thread. If any cross-thread access happens, convert them to `std::atomic` or protect with a mutex.

---

### 1.5 Test Audio Generator

**Files**: `test_harness/include/TestAudioGenerator.h`, `test_harness/src/TestAudioGenerator.cpp`

**Role**: Generates test audio waveforms for E2E testing; documented as RT-safe.

**Key patterns**:
- Uses `std::atomic<float> amplitude_`, `std::atomic<int> burstSamples_`, `std::atomic<bool> generating_` for cross-thread configuration.
- Uses `std::mt19937 rng_` and `std::uniform_real_distribution<float> noiseDist_` for noise.
- `sampleRate_`, `phase_`, `phaseIncrement_`, `burstCounter_` are plain fields.

**Risks / assumptions**:
- `rng_` and `noiseDist_` are not thread-safe if accessed from multiple threads.
- If `configure`-style functions can be called from UI/background threads while audio thread is generating, then `sampleRate_`, `phase_`, etc. can be subject to data races.

**Mitigations**:
- Treat `TestAudioGenerator` as **single-threaded to audio thread** for non-atomic members and RNG (document this explicitly).
- If configuration from non-audio threads is required, either:
  - Use atomics for all cross-thread fields, or
  - Provide an explicit synchronization mechanism (e.g. double-buffered config struct with atomic swap).

---

## 2. UI, OpenGL, and Thread Domains

### 2.1 `WaveformGLRenderer`

**Files**: `include/rendering/WaveformGLRenderer.h`, `src/rendering/WaveformGLRenderer.cpp`

**Role**: `juce::OpenGLRenderer` implementation for waveform visualizations.

**Key patterns**:
- Commented thread model:
  - `updateWaveform()` called on **message thread**.
  - `renderOpenGL()` called on **OpenGL render thread**.
  - Uses a `SpinLock` (per comments) for synchronization.
- Holds `std::unique_ptr<RenderEngine> renderEngine_`, `std::atomic<bool> contextReady_`, and various GL state.

**Risks / assumptions**:
- All shared mutable state between message thread and GL thread must be either immutable after initialization or protected by the same `SpinLock`.
- `renderEngine_` lifetime and access must be single-threaded to GL thread or properly synchronized. Creating/destroying it on message thread while the GL thread is rendering can cause use-after-free.

**Mitigations**:
- Audit all fields that are accessed from both `updateWaveform()` and `renderOpenGL()`:
  - Ensure every mutable shared field (buffers, config, flags such as `useRenderEngine_`) is accessed only under a `juce::SpinLock` (or equivalent) or via atomics.
  - Treat GL objects and `RenderEngine` as owned by the GL thread; create/destroy them via `OpenGLContext::executeOnGLThread` or only when context is inactive.
- Ensure that teardown of `WaveformGLRenderer` and `RenderEngine` happens only after the OpenGL context has been detached and GL thread is no longer calling `renderOpenGL()`.

---

### 2.2 `RenderEngine`

**Files**: `include/rendering/RenderEngine.h`, `src/rendering/RenderEngine.cpp`

**Role**: Central orchestrator for advanced rendering, framebuffer pooling, shaders, particles, 3D visualization.

**Key patterns**:
- Uses `std::unique_ptr` and `std::unordered_map` to manage framebuffers/shaders.
- Primarily used from OpenGL render thread (via `WaveformGLRenderer`).

**Risks / assumptions**:
- Any method that mutates shared containers (`unordered_map`s, pools) must be called only from the GL thread.
- If configuration methods are callable from message/background threads, they must not race with render calls.

**Mitigations**:
- Treat `RenderEngine` as **GL-thread-only** and enforce this in code comments and usage.
- If cross-thread configuration is required, use GL-thread queuing (e.g. `OpenGLContext::executeOnGLThread` or an internal command queue) to serialize all mutations on the GL thread.

---

### 2.3 `PaneComponent`

**Files**: `include/ui/PaneComponent.h`, `src/ui/PaneComponent.cpp`

**Role**: UI component containing one or more oscillator `WaveformComponent`s stacked vertically; supports drag-and-drop.

**Key patterns**:
- Uses `InstanceRegistry::getInstance().getCaptureBuffer(...)` to wire `WaveformComponent` to a `SharedCaptureBuffer`.
- Maintains a `std::vector<OscillatorEntry> waveforms_` (each entry has an `Oscillator` value and `std::unique_ptr<WaveformComponent>`).
- Paint/layout occur on message thread.

**Risks / assumptions**:
- `PaneComponent` is expected to be **message-thread-only**; as long as that holds, `waveforms_` (std::vector) is safe.
- Indirect dependence on `InstanceRegistry` singleton: misusing `InstanceRegistry` from non-UI threads elsewhere can still introduce issues, but `PaneComponent` itself appears UI-only.

**Mitigations**:
- Keep all `PaneComponent` usage on the message thread.
- Ensure that any background worker never touches `waveforms_` directly (e.g., no background thread calling `addOscillator`, `updateOscillator`, etc.).

---

### 2.4 `OscilModal`

**Files**: `include/ui/components/OscilModal.h`, `src/ui/components/OscilModal.cpp`

**Role**: Animated modal dialog component with theme integration, accessibility, focus trap.

**Key patterns**:
- In constructor: `ThemeManager::getInstance().addListener(this);`
- In destructor: removes itself as theme listener.
- Uses `juce::Timer`, `ThemeManagerListener`, `FocusChangeListener`. Accessibility handler is returned by `createAccessibilityHandler()`.

**Risks / assumptions**:
- Components and `ThemeManager` are assumed to exist on the **message thread**.
- If `OscilModal` is ever destroyed on a non-message thread, listener removal may race with theme notifications.
- `OscilModalAccessibilityHandler` likely holds on to references/pointers back to `OscilModal`; must not outlive the component.

**Mitigations**:
- Ensure `OscilModal` construction and destruction only occur on the message thread.
- Confirm that JUCE’s accessibility ownership model guarantees the handler is destroyed with the Component (which is the typical case).

---

### 2.5 `PluginTestServer::runOnMessageThread`

**File**: `src/ui/PluginTestServer.cpp`

**Role**: Utility for running a functor on the JUCE message thread; blocks caller until completion.

**Key patterns**:
- If `isThisTheMessageThread()` is true, executes `func()` directly.
- Otherwise, uses `juce::MessageManager::callAsync(...)` with a `juce::WaitableEvent` and waits using `done.wait()`.

**Risks / assumptions**:
- **Deadlock potential** if misused:
  - If `runOnMessageThread` is called from a non-message thread that indirectly depends on the same thread being free to service `callAsync` tasks (e.g. nested waits, complicated test runners), a deadlock can occur.
  - Re-entrancy from the message thread is guarded by the `isThisTheMessageThread()` check, so that specific scenario is safe.
- It assumes the JUCE message loop is actively processing events while the non-message thread is blocked.

**Mitigations**:
- Clearly document that `runOnMessageThread` is **not** to be called from the message thread (beyond the existing runtime check) and should be used only in test/harness contexts.
- Optionally, add a timeout or debugging aids around `done.wait()` to catch potential stalls in CI.

---

## 3. Test Harness and Registries

### 3.1 `TestElementRegistry`

**Files**: `test_harness/include/TestElementRegistry.h`, `test_harness/src/TestElementRegistry.cpp`

**Role**: Global registry mapping `testId → juce::Component*` for E2E testing.

**Key patterns**:
- Uses `std::mutex` and `std::lock_guard` around a `std::unordered_map<juce::String, juce::Component*>`.
- Components register/unregister themselves via RAII (`TestRegistration`, `TestableComponentMixin`).

**Risks / assumptions**:
- Map values are **raw pointers**; lifetime depends on components correctly calling `unregisterElement`.
- If a component is destroyed without unregistering (e.g. early exit, test failure, exception), the registry will hold a **dangling pointer**.
- Intended for test harness; leaks/dangling pointers are limited to tests, but can cause sporadic crashes or invalid accesses.

**Mitigations**:
- As this is test-only infrastructure, current design may be acceptable.
- To harden further:
  - Use `std::weak_ptr<juce::Component>` via a controlled factory or wrapper.
  - Add assertions in `unregisterElement` when an unknown component is removed.
  - Optionally, add a `validate()` method (debug-only) to detect inconsistencies in tests.

---

### 3.2 `TestMetrics`

**Files**: `test_harness/include/TestMetrics.h`, `test_harness/src/TestMetrics.cpp`

**Role**: Collects FPS, CPU usage, memory usage, and other metrics; uses `juce::Timer` to sample periodically.

**Key patterns**:
- Uses `std::mutex` for `frameTimes_` and `snapshots_`.
- FPS calculation traverses `frameTimes_` from newest to oldest with a mutex held.
- Used in test harness only.

**Risks / assumptions**:
- Mutexes are used in simple, non-nested ways; low risk of deadlock.
- CPU/memory measurement calls likely use system APIs; not RT-safe, but this is not on the audio thread.

**Mitigations**:
- Ensure `TestMetrics` is not used from audio thread.
- No major concurrency or leak concerns beyond that.

---

## 4. Memory Management / Leak Risks

### 4.1 JUCE Components and Leak Detectors

**Files**: `include/ui/**`, `include/ui/components/**`, `src/ui/**`

**Positive patterns**:
- Most components end with `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)`, enabling debug leak detection.
- Child components are generally held as `std::unique_ptr` and added via `addAndMakeVisible`.
- Listeners (e.g. `ThemeManagerListener`, `InstanceRegistryListener`) are explicitly removed in destructors in several places.

**Things to double-check**:
- **LookAndFeel**: any component that calls `setLookAndFeel(&someLookAndFeel)` must call `setLookAndFeel(nullptr)` in its destructor; otherwise JUCE might hold references to a destroyed LookAndFeel.
- **Timers**: components using `startTimer`/`startTimerHz` should always `stopTimer()` in the destructor. Some classes, like `OscilTransportSync`, already do this correctly.
- **Manual `new` usage**:
  - For example, in `AddOscillatorDialog::setupComponents()`:
    - `auto* selector = new juce::ColourSelector(...);` – ensure the selector is always passed into a `CallOutBox` or dialog helper that takes ownership and eventually deletes it.
    - Check for early-return or error paths where the raw pointer might not be adopted.

**Mitigations**:
- Run a quick grep for `setLookAndFeel(` and verify:
  - Each call is paired with `setLookAndFeel(nullptr)` in the owning class’s destructor.
- Grep for `new juce::` and validate that each occurrence’s ownership is clear (scoped smart pointer, or JUCE dialog helper that takes ownership).

---

### 4.2 OpenGL Resources (Framebuffer, Pooling, Shaders)

**Files**: `include/rendering/Framebuffer.h`, `src/rendering/Framebuffer.cpp`, `include/rendering/FramebufferPool.h`, `src/rendering/FramebufferPool.cpp`, shader-related classes

**Role**: Manage OpenGL framebuffers, textures, renderbuffers, and shader programs.

**Risks / assumptions**:
- If `Framebuffer::destroy()` or equivalent cleanup function is not called when contexts are resized or torn down, GPU objects can be leaked for the plugin’s lifetime.
- However, OpenGL drivers typically free these resources when the context or process dies; leaks are mostly localized to plugin runtime.

**Mitigations**:
- Ensure that `Framebuffer` destruction or explicit `destroy()` is called:
  - On `RenderEngine` destruction.
  - When resizing or recreating framebuffers.
- Avoid making GL calls after context detachment (e.g. guard with context checks where needed).

---

### 4.3 Singleton Lifetimes and `SharedCaptureBuffer`

**Files**: `include/core/InstanceRegistry.h`, `src/core/InstanceRegistry.cpp`, `include/core/SharedCaptureBuffer.h`

**Role**: Long-lived singletons manage shared buffers across plugin instances.

**Risks / assumptions**:
- Using `std::shared_ptr<SharedCaptureBuffer>` with a singleton may make it easy to “leak by design”: buffers might remain allocated until process termination if held by a long-lived registry.
- In plugin environments, module unloading and host behavior may interact with static singletons in non-trivial ways.

**Mitigations**:
- Provide a `shutdown()` or `reset()` method on `InstanceRegistry` to clear internal state and release `shared_ptr`s when the plugin is being unloaded.
- Call this from a well-defined place (e.g. `OscilPluginProcessor` shutdown / global deinitialization path).

---

## 5. Tests and Concurrency Expectations

**Relevant tests**:
- `tests/test_shared_capture_buffer.cpp::ThreadSafetyConcurrentAccess`
- `tests/test_plugin_processor.cpp::ConcurrentProcessing`
- `tests/test_animation_settings.cpp::ConcurrentReadWhileWriting`
- `tests/test_timing_engine.cpp::ThreadSafety_*`
- `tests/test_source.cpp::Concurrent*`

**Observations**:
- Tests demonstrate deliberate, high-contention concurrent access to shared structures.
- `ConcurrentProcessing` test starts **two threads** calling `processBlock` on the same processor instance:
  - Real DAWs generally guarantee per-instance `processBlock` is single-threaded.
  - This test is therefore more of a **stress test** than a host model.

**Risks / assumptions**:
- If the design intentionally assumes single-threaded `processBlock` per instance, then truly making it thread-safe for concurrent calls is *not* a requirement.
- However, tests that run concurrent `processBlock` may hide real races (undefined behavior may not manifest in those specific runs).

**Mitigations**:
- Reaffirm and document the invariants for `OscilPluginProcessor`:
  - If `processBlock` is designed for single-threaded usage, call that out and consider relaxing or rephrasing the `ConcurrentProcessing` test as a stress/fuzz test only, not an expectation for host behavior.
- If the goal is full thread safety for concurrent `processBlock`, audit all shared state inside the processor for proper synchronization.

---

## 6. Recommended Next Steps

1. **Document thread domains per class**
   - For `InstanceRegistry`, `TimingEngine`, `RenderEngine`, `WaveformGLRenderer`, `SharedCaptureBuffer`, and `Source`, add short comments in headers clarifying:
     - Which methods are audio-thread-only, message-thread-only, or multi-threaded.
     - Which synchronization mechanism is used (atomics, spin lock, mutex, seqlock).

2. **Audit and tighten synchronization in key areas**
   - `InstanceRegistry` implementation:
     - Ensure listener notifications are not under locks.
     - Enforce message/background-thread-only usage.
   - `TimingEngine`:
     - Confirm all non-atomic state is single-threaded.
     - Keep listener callbacks off the audio thread.
   - `WaveformGLRenderer` / `RenderEngine`:
     - Verify all cross-thread mutable state is covered by `SpinLock` or single-threaded to GL thread.

3. **Memory and leak hygiene pass**
   - Search for `setLookAndFeel(` and verify matching `setLookAndFeel(nullptr)` in destructors.
   - Search for `new juce::` and ensure each allocation is clearly owned by a smart pointer or a JUCE helper that frees it.
   - Confirm OpenGL resources are released when contexts are destroyed.

4. **Lifecycle management for singletons**
   - Introduce explicit shutdown/reset for `InstanceRegistry` (and any other long-lived singletons) to release shared resources when the plugin is unloaded.

