# Reaper On-Demand Integration Tests

DAW host integration tests for Oscil, driven by Reaper (ReaScript/Lua). Run manually, not in CI.

## Prerequisites

- macOS with Reaper installed at `/Applications/REAPER.app` (override with `REAPER_PATH` env var).
- Oscil built locally. Default plugin path: `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3`
  (override the parent dir with `OSCIL_PLUGIN_DIR`).
- Reaper must have scanned the plugin at least once — open Reaper, re-scan plugins
  (Preferences -> Plug-ins -> VST -> Re-scan) so `oscil4` is known to the host.
- Python-ReaScript is NOT required; these tests are Lua.

## Running

```bash
# Run full suite
tests/reaper/run_reaper_tests.sh

# Run a single scenario by name (without .lua extension)
tests/reaper/run_reaper_tests.sh load_vst3_smoke
```

The script launches Reaper headlessly-ish (`-nonewinst -new`), Reaper auto-runs
`lib/run_all.lua`, scenarios write results to `/tmp/oscil_reaper_results.json`,
the script reads that file and reports pass/fail. Non-zero exit on any failure.

## Layout

```
tests/reaper/
  README.md                      # this file
  run_reaper_tests.sh            # shell entrypoint (chmod +x)
  lib/
    reaper_test_lib.lua          # shared helpers (asserts, plugin load, project lifecycle)
    run_all.lua                  # driver Reaper runs; discovers scenarios/*.lua
  scenarios/
    *.lua                        # one scenario per file (added in separate PRs)
```

## Writing a Scenario

Create `scenarios/<name>.lua`:

```lua
local T = require("reaper_test_lib")
local function run()
  T.load_vst3("oscil4", 0)
  T.play_seconds(1.0)
  T.assert_true(reaper.CountTracks(0) == 1, "expected 1 track")
end
return { name = "my_scenario", run = run }
```

`run_all.lua` wraps `run()` in pcall and records pass/fail + error detail.

## Relationship to In-Process Harness

The in-process `test_harness/` harness covers plugin UI and logic without a real DAW.
This Reaper suite covers things only a real host exercises: VST3/AU/CLAP load paths,
save/reload state round-trips, DAW automation, transport-driven processBlock cadence.
On-demand because Reaper GUI automation is slow and environment-sensitive.

## Troubleshooting

- "Reaper not found": install Reaper or set `REAPER_PATH=/path/to/REAPER.app/Contents/MacOS/REAPER`.
- "Plugin not found": build Oscil first (`cmake --build --preset dev`) or set
  `OSCIL_PLUGIN_DIR=/abs/path/to/vst3/dir`.
- Results file missing or stale: check Reaper's stdout; ReaScript errors surface there.
  Delete `/tmp/oscil_reaper_results.json` and re-run.
- Plugin not loading: confirm Reaper has scanned it (see Prerequisites).
