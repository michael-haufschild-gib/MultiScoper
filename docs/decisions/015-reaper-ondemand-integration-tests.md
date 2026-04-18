# ADR 015: Reaper On-Demand DAW Integration Tests

Date: 2026-04-18
Status: Accepted

## Context

Oscil ships as VST3, AU, and CLAP. The existing test surface is:

1. GoogleTest unit tests (`tests/`) — fast, isolated, no host.
2. In-process `test_harness/` — loads the plugin in a standalone host we control
   and exposes an HTTP API on port 8765 for end-to-end assertions.

Neither exercises a real DAW. Host-specific bugs — VST3/AU/CLAP wrapper
divergence, DAW save/reload state round-trips, transport-driven processBlock
cadence, automation delivery, multi-instance host scenarios — only surface
when a real host loads the binary.

## Decision

Add a third tier: on-demand Reaper-driven integration tests under
`tests/reaper/`. Not run in CI. Invoked manually before releases and when
touching plugin wrapper code, state save/load, or timing.

### Why Reaper

- Scriptable via ReaScript (Lua/Python/EEL) — every test assertion we need is a
  plain API call (`TrackFX_AddByName`, `Main_SaveProjectEx`, transport control,
  state chunk extraction).
- Supports VST3, AU, and CLAP in one host — one suite covers all three formats.
- Cross-platform (macOS/Windows/Linux) if we need to extend later.
- Cheap license, commonly installed on engineer workstations.
- No driver layer to maintain — Reaper IS the driver.

Alternatives considered: Logic Pro (AU-only, not scriptable), Ableton Live
(Max-based scripting, heavy), pluginval (great for validation but not for
behavioral assertions), custom VST3 host (reinvents Reaper).

### Why Lua over Python

Reaper's Lua runtime is built in; Python requires the user to install a
Python-ReaScript runtime and wire it up in Preferences. For an on-demand suite
we want zero extra setup. Lua is sufficient — all Reaper APIs are exposed
identically to both languages.

### Why On-Demand, Not CI

- Reaper GUI automation is slow (seconds per scenario, minutes for a suite)
  and flakier than a pure in-process harness — plugin scan caches, audio
  device selection, and GUI focus all affect runs.
- CI runners don't have Reaper licensed/installed; provisioning adds cost and
  a new point of failure for unrelated PRs.
- The value is pre-release confidence, not per-commit regression catching —
  unit tests + the in-process harness catch regressions cheaply.

### Complementarity

| Surface                         | Covered by                 |
| ------------------------------- | -------------------------- |
| Pure logic, DSP, data structures| GoogleTest unit tests      |
| UI behavior, plugin lifecycle   | in-process test harness    |
| Host wrapper correctness        | Reaper on-demand (new)     |
| Save/reload in a DAW project    | Reaper on-demand (new)     |
| VST3/AU/CLAP format divergence  | Reaper on-demand (new)     |

## Structure

```text
tests/reaper/
  run_reaper_tests.sh     # shell entrypoint; launches Reaper, reads JSON results
  lib/reaper_test_lib.lua # helpers (load_vst3, play_seconds, assertions, ...)
  lib/run_all.lua         # driver Reaper auto-runs; discovers scenarios/*.lua
  scenarios/*.lua         # one scenario per file
```

Results land at `/tmp/oscil_reaper_results.json`; shell script exits non-zero
on any failure.

## Consequences

+ Real-host confidence without CI cost.
+ Isolated infrastructure — failures here don't block unrelated PRs.
+ Scaffolding lives in-tree so scenarios can be added incrementally.
- Relies on the engineer remembering to run it before releases — mitigated by
  a release checklist entry (to be added to `docs/release.md`).
- Ties us to Reaper as a host dependency for this suite specifically; acceptable
  given the alternatives.
