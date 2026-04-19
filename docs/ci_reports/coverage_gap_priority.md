# Coverage Gap Priority Synthesis

Consolidation of four audit reports into a prioritized gap list driving Stream 2 (harness primitives) and Stream 3 (Reaper on-demand) scope.

Sources:
- `e2e_audit.md` — what 27 existing E2E test files cover and miss
- `harness_api_audit.md` — 73 HTTP endpoints, thread correctness, missing endpoints
- `scenario_coverage_matrix.md` — 70 TC-* scenarios: 27 covered / 17 partial / 26 absent
- `harness_capability_gaps.md` — 18 DAW behaviors the harness cannot reproduce

## Headline numbers

- **Zero CI coverage for E2E.** The 27-file Python suite runs on-demand only.
- **26 of 70 documented user scenarios have no test evidence.** 17 more are partial.
- **18 distinct DAW behaviors are physically unreproducible in the current harness.** Most are structural — one process, fixed sample rate, fixed buffer size, no bus renegotiation.
- **Harness HTTP worker-thread races — resolved in this PR.** MT-dispatch migration (ADR-016) plus a `forbidden_patterns_lint` rule now force every mutable-state HTTP handler through a MessageManager hop. No endpoints remain in the original 9-endpoint gap.

## P0 — Ship blockers / likely causes of the instability testers report

These are the ones that best explain "plugin is unstable in different DAWs."

| Gap | Evidence | Addressed by |
|---|---|---|
| **AU-sandbox singleton divergence.** All "multi-instance" tests share one `PluginFactory::getInstance()`; Logic AU runs each plugin out-of-process so the registry is not shared. Headline product feature (cross-instance discovery) is *structurally untested*. | `PluginFactory.cpp:40-48`, `TestTrack.cpp:47-49`; harness gap #7 | Stream 3.3 (Reaper 2-track cross-discovery) + Stream 2.8 spike (multi-process harness feasibility) |
| **`prepareToPlay` on audio thread** — Pro Tools / Reaper path. Defensive code at `PluginProcessor.cpp:134-137` is never exercised. | harness gap #4 | Stream 2.4 |
| **`getStateInformation` on audio thread during playback** — RT-safe tryLock branch has zero callers. Pro Tools autosave hits this. | `PluginProcessorState.cpp:22-31`; harness gap #5, #9 | Stream 2.6 |
| **Sample-rate change mid-session** — `prepareToPlay` runs once per track; users commonly open sessions at 44.1k then swap devices to 48k. Every `currentSampleRate_` reader could be stale during reconfigure. | `TestTrack.cpp:76`; harness gap #1 | Stream 2.1 + Stream 3.5 |
| **Buffer-size change mid-session** — same class, different parameter. | harness gap #2 | Stream 2.2 + Stream 3.5 |
| **Editor detach/reattach without destroying the processor** — harness tears down editor fully; DAWs reparent. `parentHierarchyChanged` reattach branch + OpenGL re-attach + `testServer_` double-start on port 8765/9876 are blind spots. | `PluginEditor.cpp:118-123, 247-256`; harness gap #8 | Stream 2.5 |

## P1 — Important coverage gaps the existing harness CAN reach

Not structural; the harness can hit these, tests just don't.

| Gap | Evidence | Addressed by |
|---|---|---|
| ~~**Harness HTTP endpoints read state from worker thread**~~ — `/state/oscillators`, `/state/panes`, `/waveform/state`, `/diagnostic/snapshot`, `/daw/tracks`, `/track/{id}/info`, most of `/track/{id}/audio`, `/track/{id}/burst`. Torn reads masquerade as flake. | harness_api_audit § "Fire-and-forget / race risk" (9 endpoints) | **Resolved in this PR** — ADR-016 MT-dispatch + `forbidden_patterns_lint` gate |
| **Plugin scan cycle (instantiate → query → destroy, repeat)** — no coverage; DAWs do this on startup and a crash here fails plugin validation silently. | harness gap #6 | Stream 2.7 |
| **Bus-layout renegotiation (stereo↔mono, disabled bus)** — `isBusesLayoutSupported` never called by the harness. FL Studio/Ableton exercise the "reject disabled bus" branch. | `PluginProcessor.cpp:183-198`; harness gap #3 | Stream 2.3 + Stream 3.5 |
| **Transport backwards / loop wrap** — `TestTransport::advancePosition` only increments; `isLooping` hard-coded false. Loop-wrap triggers never reach the trigger-timestamp math. | `TestTransport.cpp:35-41, 60`; harness gap #10 | Separate P1 task — extend `TestTransport` |
| **26 absent + 17 partial user scenarios** — top concrete misses: TC-LAY-006 (pane drag edge cases), TC-OSC-002 / TC-SRC-004 (Add-to-Pane flow), TC-CNF-001 (delete confirmation), TC-TRG-001/002 (trigger mode), TC-DIS-003 (hold), TC-MC-001 (timebase slider), TC-KEY-003/005/006 (keyboard nav). | scenario_coverage_matrix.md § Top 10 | Separate P1 tasks per scenario family — out of current streams |
| **State-restore ordering — `setStateInformation` before `prepareToPlay`** — Cubase/Live use this order; harness always does prepare first. `sampleRate` snapshot at `PluginProcessorState.cpp:81` is wrong in that case. | harness gap #14 | Stream 2.6 addendum |
| **Rename (`updateTrackProperties`) timing vs prepare** — early-returns if sourceId invalid; rename-before-prepare silently drops name. | `PluginProcessor.cpp:159-176`; harness gap #13, #18 | Separate P1 task (small) |

## P2 — Non-critical gaps noted for the backlog

- Sustained soak test > 10s — leak detection horizon too short (`test_performance.py`).
- No visual/pixel regression for waveform / themes / 3D shaders — `verify_element_color` tests API contract only.
- No fuzz over operation sequences (add/delete/move/reorder/save/load with shrinking).
- Missing OpenGL driver-loss / context-lost simulation.
- Missing clipboard, context menu, and drag-and-drop-container E2E coverage.
- Missing Retina/HiDPI scaling tests; the harness uses a fixed-size window.
- 16-instance test claims cover scale but all in one process — same singleton problem as P0 #1.
- `MidiBuffer` never carries real events in harness — MIDI-trigger waveform restart path is dead code under test.
- `releaseResources` contract untested (no-op today, but regression risk if body gains logic).

## Stream routing summary

| Stream 2 task | P0/P1 gaps addressed |
|---|---|
| 2.1 Sample-rate | P0: sample-rate change |
| 2.2 Buffer-size | P0: buffer-size change |
| 2.3 Bus layout | P1: bus renegotiation |
| 2.4 Audio-thread prepareToPlay | P0: Pro Tools/Reaper prepare path |
| 2.5 Editor detach/reattach | P0: parentHierarchyChanged + GL re-attach |
| 2.6 State-save race | P0: RT-safe getStateInformation + P1: set-state ordering |
| 2.7 Scan cycle | P1: plugin scanner lifecycle |
| 2.8 Multi-process harness spike | P0: AU sandbox singleton (spike: is it feasible or do we rely on Stream 3.3?) |

| Stream 3 scenario | P0/P1 gaps addressed |
|---|---|
| 3.2 Save/reopen | P1: full state round-trip under a real host |
| 3.3 Cross-instance discovery | P0: AU sandbox divergence under Logic; VST3 cross-discovery under Reaper |
| 3.4 AU variant | P0: AU-specific bugs pluginval misses |
| 3.5 Buffer/SR under real host | P0: SR + buffer-size change, but driven by actual DAW audio device |
| 3.6 CLAP variant | P1: CLAP integration (no validator in CI) |

## Gaps that no current stream addresses — file separate tasks

1. **P1 scenario backlog** — TC-LAY-006, TC-OSC-002, TC-SRC-004, TC-CNF-001, TC-TRG-001/002, TC-DIS-003, TC-MC-001, TC-KEY-003/005/006. Each a small E2E PR.
2. **Transport backwards/loop wrap** — extend `TestTransport`.
3. **`updateTrackProperties` ordering** — small defensive-code test.

(Formerly listed: "Harness HTTP thread-safety pass — 9 endpoints need MessageManager hops." Resolved in this PR via the ADR-016 MT-dispatch migration and `forbidden_patterns_lint` enforcement.)

(Formerly listed: `PluginTestServer` compile-time gate. Resolved — sources
are now gated behind `MULTISCOPER_ENABLE_TEST_SERVER`, off for shipping builds.)

## Exit criteria for "tests now match reality"

- P0 gaps all have at least one covering test (Stream 2 primitive **or** Stream 3 scenario).
- The next externally-reported DAW crash is reproducible via one of the new tests.
- No harness HTTP endpoint reads mutable plugin state from a worker thread without a MessageManager hop. **Met in this PR** — enforced by `forbidden_patterns_lint`.
