# Multi-Process Harness Feasibility Spike

Read-only investigation per Stream 2.8. Question: should we rework the Oscil test harness so each `TestTrack` runs in its own OS process to reproduce Logic AU's sandbox isolation?

## TL;DR — Drop

Multi-process harness is ~4–6 engineer-weeks. It catches approximately zero failure modes that a real Logic-AU + Reaper cross-instance scenario (Stream 3.3 extended to AU) does not already catch better. Do the 1-day "per-track registry injection" substitute instead (see Alternative).

## Cost estimate (4–6 engineer-weeks)

| Component | Estimate |
|---|---|
| IPC layer (shm ring + control socket, framing format) | 1.5 w |
| Child lifecycle (spawn/kill/crash-recovery/zombies, macOS codesign for AU) | 1 w |
| Audio dispatch via IPC (clock in parent, block push/ack per child) | 1 w |
| HTTP routing fan-out to right child PID | 0.5 w |
| Windows parity (named pipes vs unix sockets, job objects for cleanup) | 1 w |
| Debugger/log aggregation + CI wiring | 0.5 w |

## Coverage uniquely added vs Stream 3.3 + auval

Effectively none.

- **AU sandbox isolation behavior** — Logic under AU exercises the real sandbox; a multi-process harness only simulates it.
- **Process-boundary state serialization** — not a shipping concern; hosts handle it, not us.
- **`auval` is NOT a cross-instance substitute.** It loads one plugin at a time per invocation and cannot host two simultaneously in one process. `[UNVERIFIED — based on auval's documented contract; would need to check Apple Core Audio Utility tool docs before relying on this claim.]`

`PluginFactory.cpp:40-48` and `TestTrack.cpp:47-49` confirm the current shared-singleton model. The *absence* of sharing is trivially reproducible without spawning processes — see Alternative below.

## Risks of building it

- CI flakiness from process-spawn timing
- macOS entitlements / codesign overhead for AU child processes
- Windows job-object cleanup leaks
- Debugger attach across N children
- Duplicated harness code drift against the real plugin path
- High maintenance burden, low signal

## Alternative path (1 day) — per-track `InstanceRegistry` injection

`PluginFactory` already supports DI via `PluginProcessorConfig` (`PluginFactory.cpp:56-64`) and `setInstance()` (`PluginFactory.cpp:50-54`). Add a mode flag to `TestDAW` that constructs a fresh `InstanceRegistry` per `TestTrack` and injects it instead of the shared one.

**This reproduces** the gap-#7 failure mode — cross-instance discovery silently not working under isolation — without any process overhead.

**This does NOT reproduce**: separate OS heaps per instance, separate OpenGL contexts, separate `createPluginFilter()` static-init ordering. None of those are listed as suspected root causes in `harness_capability_gaps.md`. The substitute is valid for the stated goal.

Coverage that remains unaddressed by the substitute:
- Behavior under real AU sandbox (covered by Stream 3.4 Reaper-AU scenario)
- Cross-process IPC crash recovery (not a product requirement)

## Files reviewed

- `src/plugin/PluginFactory.cpp` / `.h`
- `include/core/InstanceRegistry.h` + impl
- `test_harness/include/TestDAW.h` + `TestTrack.h`
- `test_harness/src/TestTrack.cpp` (lines 41-54 confirm shared factory use)
- `test_harness/src/TestHttpServer*.cpp` (3 call sites to `getInstance().getInstanceRegistry()`)
- `docs/decisions/003-single-singleton-composition-root.md`
- `docs/ci_reports/harness_capability_gaps.md` (gap #7)

## Recommendation

1. Drop the multi-process harness from scope.
2. Add a new task for the per-track registry injection mode (1-day implementation).
3. Extend Stream 3.4 (Reaper AU scenario) to explicitly cover 2-instance AU cross-discovery on Logic/Reaper — this is where the real sandbox lives.
