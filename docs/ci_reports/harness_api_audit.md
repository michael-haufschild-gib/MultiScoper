# HTTP API Audit — MultiScoper Test Harness

Audit scope: `test_harness/src/TestHttpServer*.cpp` and
`test_harness/src/TestUIController*.cpp`.
Base URL: `http://127.0.0.1:8765`.

Last re-audit: 2026-04-18 (after ADR 016 MT-dispatch migration).

## Thread legend

- **HTTP**: runs entirely on the httplib worker thread (no MessageManager
  hop). Safe only when the target state is thread-safe on its own (atomics,
  internally locked containers).
- **MT-sync**: dispatches work via `runOnTrackSync` /
  `runOnMessageThreadBlocking` and blocks on a `juce::WaitableEvent` with
  a bounded timeout. All track pointer derefs happen inside the lambda.
- **MT-async**: scheduled on the message thread with no wait (fire and
  forget). Intentional only for UI-animation tails (e.g. `/ui/hover` exit
  leg); any state mutation here is a race.

## Endpoint inventory

| method | path | purpose | thread |
|---|---|---|---|
| GET  | /health | liveness + pid + track/source counts | HTTP (reads atomics + internally locked registry) |
| POST | /transport/play | start transport | HTTP (TestTransport is atomic-backed) |
| POST | /transport/stop | pause transport | HTTP (atomic) |
| POST | /transport/setBpm | set host+internal BPM, force one block + refreshPanels | MT-sync (runOnTrackSync + runSingleBlockSynchronously) |
| POST | /transport/setPosition | set sample position | HTTP (atomic setter) |
| GET  | /transport/state | read playing/bpm/position | HTTP (atomic reads) |
| POST | /track/{id}/audio | change waveform/freq/amp | MT-sync ✅ |
| POST | /track/{id}/burst | schedule N-sample burst | MT-sync ✅ |
| GET  | /track/{id}/info | read generator state + sourceId | MT-sync ✅ |
| POST | /track/{id}/showEditor | open editor | MT-sync |
| POST | /track/{id}/hideEditor | close editor | MT-sync |
| POST | /track/{id}/detachEditor | editor detach without destroy | MT-sync (callAsync+wait) |
| POST | /track/{id}/reattachEditor | re-attach existing editor | MT-sync (callAsync+wait) |
| GET  | /track/{id}/sources | per-track registry view | MT-sync (callAsync+wait) |
| POST | /track/{id}/channelConfig | set channel layout | MT-sync |
| POST | /daw/track/add | instantiate plugin + prepareToPlay | MT-sync |
| POST | /daw/track/remove | destroy plugin | MT-sync |
| GET  | /daw/tracks | list tracks | MT-sync ✅ (runOnMessageThreadBlocking) |
| POST | /daw/setIsolatedRegistries | toggle per-track registry isolation | HTTP (atomic flag + simple setter) |
| POST | /ui/click | click element | MT-sync |
| POST | /ui/doubleClick | double-click | MT-sync |
| POST | /ui/rightClick | right-click / popup | MT-sync |
| POST | /ui/hover | enter + scheduled exit | MT-sync enter, MT-async exit tail |
| POST | /ui/select | combo/dropdown select | MT-sync |
| POST | /ui/toggle | toggle/button state | MT-sync |
| POST | /ui/slider | set slider value | MT-sync |
| POST | /ui/slider/increment | +1 step | MT-sync |
| POST | /ui/slider/decrement | -1 step | MT-sync |
| POST | /ui/slider/reset | double-click reset | MT-sync |
| POST | /ui/drag | drag A→B (+ reorder fast path) | MT-sync |
| POST | /ui/dragOffset | drag by delta | MT-sync |
| POST | /ui/scroll | mouse wheel | MT-sync |
| POST | /ui/keyPress | synth key event | MT-sync |
| POST | /ui/typeText | set text on field | MT-sync |
| POST | /ui/clearText | clear text | MT-sync |
| POST | /ui/focus | grab focus | MT-sync |
| GET  | /ui/focused | id of focused element | MT-sync |
| POST | /ui/focusNext | Tab | MT-sync |
| POST | /ui/focusPrevious | Shift-Tab | MT-sync |
| POST | /ui/waitForElement | poll registry for id | HTTP polling (internally locked registry) |
| POST | /ui/waitForVisible | poll visibility | MT-sync per poll |
| POST | /ui/waitForEnabled | poll enabled | MT-sync per poll |
| GET  | /ui/state | full UI tree + focused id | MT-sync |
| GET  | /ui/elements | list elements (visible/enabled/bounds) | MT-sync |
| GET  | /ui/element/{id} | single-element info | MT-sync |
| POST | /screenshot | capture window or element PNG | MT-sync (TestScreenshot internals) |
| POST | /screenshot/compare | diff against baseline | HTTP (pure image work) |
| POST | /baseline/save | save baseline PNG | HTTP (filesystem only) |
| POST | /verify/waveform | pixel-level waveform pass/fail | HTTP (pure image) |
| POST | /verify/color | background/contains colour | HTTP |
| POST | /verify/bounds | width/height tolerance | MT-sync (queries bounds) |
| POST | /verify/visible | element visibility | MT-sync |
| GET  | /analyze/waveform | amplitude/activity/zero-crossings | HTTP (pure image) |
| POST | /metrics/start | begin collection timer | HTTP (metrics collector is self-locked) |
| POST | /metrics/stop | stop collection | HTTP |
| GET  | /metrics/current | latest snapshot | HTTP (atomic snapshot) |
| GET  | /metrics/stats | aggregated stats | HTTP |
| POST | /metrics/reset | clear ring buffer | HTTP |
| POST | /metrics/recordFrame | manual frame tick | HTTP |
| POST | /state/reset | clear oscillators/panes + reset audio + options UI + re-sync timing sidebar | MT-sync (oscillator/pane cleanup + timing engine reset + sidebar resync in the same lambda; audio/transport reset via runOnMessageThreadBlocking) |
| POST | /state/save | serialize to XML on disk | MT-sync (XML build on MT, file write on HTTP) |
| POST | /state/load | restore XML + refreshPanels | MT-sync |
| GET  | /state/oscillators | list oscillators | MT-sync ✅ |
| POST | /state/oscillator/add | add oscillator | MT-sync |
| POST | /state/oscillator/update | update visible/name/mode/opacity/lineWidth | MT-sync |
| POST | /state/oscillator/reorder | change order indices | MT-sync |
| POST | /state/oscillator/delete | remove oscillator | MT-sync |
| POST | /state/oscillator/move | change paneId | MT-sync |
| POST | /state/pane/add | add pane | MT-sync |
| POST | /state/pane/remove | remove pane | MT-sync |
| GET  | /state/panes | list panes | MT-sync ✅ |
| GET  | /state/sources | list sources from `InstanceRegistry` | HTTP (registry is internally locked) |
| GET  | /layout | column count + pane count | MT-sync |
| POST | /layout | set column layout + refreshPanels | MT-sync |
| GET  | /panes | per-pane computed screen bounds | MT-sync |
| POST | /pane/move | change pane index | MT-sync |
| GET  | /waveform/state | capture-buffer peak/RMS per pane | MT-sync ✅ |
| GET  | /diagnostic/snapshot | full dump | mixed: GUI + oscillator/pane/transport/timing/sources/generators all MT-sync; metrics/logs HTTP |

## Status vs previous audit

The pre-ADR-016 audit listed 14 pre-refactor HTTP-thread `getTrack` call
sites and 9 unsynchronised state reads. All have been migrated except the
following, each of which is now verified-safe rather than fire-and-forget:

1. **`/transport/state`, `/transport/play|stop|setPosition`** — read/write
   atomics on `TestTransport`. No race with the audio dispatcher.
2. **`/state/sources`** — reads `InstanceRegistry::getAllSources()`, which
   takes its own `ScopedSharedLock`.
3. **`/health`, `/metrics/*`** — atomic counters and self-locked metrics
   collector.

Remaining non-MT writes that mutate shared state from the HTTP thread:
**none** identified in the current codebase.

## Known residual race classes

1. `/ui/hover` exit leg is scheduled on the message thread via
   `juce::Timer::callAfterDelay` after the HTTP response has already been
   sent. Tests that inspect element state during the un-hover window see a
   transient visual state. This is intentional (mirrors real hover timing).
2. Screenshot endpoints delegate MT hops to the screenshot subsystem; any
   add/remove of components during capture is still visible as a torn frame
   — captured-at-instant behaviour rather than a captured-at-quiescence
   behaviour. Tests work around this with settle intervals.

## Capability gaps (realism)

Unchanged from the prior audit — see the `Missing endpoints for realistic
DAW behavior` section in `harness_capability_gaps.md` for the prioritised
list. Top 3: sample-rate change, buffer-size change, editor detach/reattach
(the last was landed; the first two remain open).

## Architecture guardrail

ADR 016 codifies the MT-dispatch pattern. The companion lint rule lives in
`cmake/Tests.cmake` (`multiscoper_harness_mt_lint` target, run as part of
`ctest --preset dev`) and rejects new `[&]`, `[this, &` reference captures
inside `runOnTrackSync` / `runOnMessageThreadBlocking` lambdas. See
`docs/decisions/016-test-harness-mt-dispatch.md` for the shared_ptr
payload pattern every new handler must follow.
