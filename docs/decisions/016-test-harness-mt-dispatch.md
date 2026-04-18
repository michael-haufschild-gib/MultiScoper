# ADR 016: Test Harness HTTP Handlers Dispatch onto the Message Thread

Date: 2026-04-18
Status: Accepted

## Context

The in-process test harness (`test_harness/`) serves an HTTP API on port 8765.
Every HTTP request is handled on an httplib worker thread — an arbitrary
`std::thread` with no relationship to the JUCE message thread (MT). The
plugin code under test (OscilPluginProcessor, OscilPluginEditor, OscilState,
TestTrack) assumes message-thread affinity: structural edits, editor ops,
parameter changes, state mutation, and JUCE Component queries are only
legal on the MT.

Historically, many handlers took shortcuts and read plugin state directly
from the HTTP worker thread. That produced two classes of failure under
load:

1. **Use-after-free on `TestTrack::sourceIdMutex_`.** `TestDAW::getTrack(i)`
   returned a raw pointer read from `tracks_` without any lock. If a
   concurrent `addTrack`/`removeTrack` (scheduled via `callAsync`) reset the
   slot on the MT between the HTTP worker's null-check and its next
   dereference, `track->getSourceId()` locked a mutex inside destroyed
   memory. `pthread_mutex_lock` returned `EINVAL`, libc++ threw
   `std::system_error("mutex lock failed: Invalid argument")`, nothing
   caught it, `std::terminate()` aborted the harness. This matched issue
   #24 exactly and reproduced at the 16-instance × 192 kHz × 8192-sample
   stress level.

2. **Data races on plugin internals.** Even without UAF, reading an
   `OscilState` oscillator list or a `DecimatingCaptureBuffer`'s peak/RMS
   stats from the HTTP worker while the MT mutated them was torn-read
   territory — invisible under light load, flaky under stress.

The `harness_api_audit.md` flagged 14+ endpoints reading `tracks_` on the
HTTP thread and 9 reading `OscilState` / `InstanceRegistry` /
`CaptureBuffer` / `TimingEngine` without any MT hop.

## Decision

**Every HTTP handler that touches plugin or track state dispatches its
work onto the message thread and blocks until it completes (bounded
timeout).**

This matches the threading topology a real DAW uses. Logic / Reaper /
Live / Cubase do not have a "worker thread" that reaches into a plugin
slot to sample its source ID while the graph rebuilds. UI events go
through the main thread. Automation flows through the audio thread.
Nothing else exists.

### Helpers

Two helpers on `TestHttpServer` encode the pattern:

```cpp
enum class TrackCallResult { Ok, NotFound, Timeout };

TrackCallResult runOnTrackSync(int trackId,
                               std::function<void(TestTrack&)> fn,
                               int timeoutMs = 3000);

bool runOnMessageThreadBlocking(std::function<void()> fn,
                                int timeoutMs,
                                const char* label);
```

Both heap-own their internal state (`shared_ptr<State>` holding `fn`,
`juce::WaitableEvent`, and the result), so a timeout-return is safe even
when the MT lambda eventually fires. The track lookup happens inside the
MT lambda — that way `addTrack` / `removeTrack` are serialized with the
dispatch, and the pointer is guaranteed live for the lifetime of `fn`.

`respondIfTrackCallFailed(result, res, notFoundMsg, timeoutMsg)` writes
the matching 404 / 504 response and tells the handler to early-return,
keeping handler bodies short.

### Handler contract

Every lambda passed to the helpers — or to raw
`juce::MessageManager::callAsync` — must capture its outputs via
`std::shared_ptr<T>` by value. **No `[&]` captures. No raw references to
caller stack.** If the helper times out and the handler returns, the
lambda may still fire later; a stack-reference capture would turn that
into a use-after-free.

Value captures of `this` (process-lifetime), scalar inputs, `std::string`,
`juce::String`, `PaneId` / `OscillatorId` / `SourceId`, and
`juce::Component::SafePointer<T>` are safe.

Canonical handler shape:

```cpp
void TestHttpServer::handleX(const httplib::Request& req, httplib::Response& res)
{
    const int trackId = resolveTrackId(req);
    auto data = std::make_shared<json>();
    auto missing = std::make_shared<bool>(false);

    const auto result = runOnTrackSync(trackId, [data, missing](TestTrack& track) {
        auto& state = track.getProcessor().getState();
        if (!state.hasX())        { *missing = true; return; }
        (*data)["x"] = state.getX();
    }, 5000);

    if (respondIfTrackCallFailed(result, res, "Track not found", "Timeout reading X"))
        return;
    if (*missing) { res.set_content(errorResponse("X not found").dump(), "application/json"); return; }
    res.set_content(successResponse(*data).dump(), "application/json");
}
```

### Legacy `resolveTrack(req)` helper

`resolveTrack(req)` / `resolveTrackFromBody(body)` return a raw
`TestTrack*`. These are **legacy, MT-only**. The header warns against
calling them from the HTTP worker. Prefer `resolveTrackId(req)` +
`runOnTrackSync`. The legacy variants remain for code that already runs
on the MT and wants a one-liner.

## Consequences

+ **Issue #24 closed.** No more `pthread_mutex_lock EINVAL` terminate;
  the full E2E suite runs without TERMINATE / mutex / Segfault markers.
+ **Deterministic threading.** Any crash the harness can hit is a crash a
  real DAW could hit — the topology matches. This was not true before.
+ **Backpressure surfaced.** If the MT genuinely stalls, handlers return
  504 instead of silently returning torn data. That makes MT-hotspot
  regressions visible in test output.

− **Every state-touching endpoint pays one MT round-trip.** Under heavy
  automation this serializes through the message thread. Bounded
  timeouts (3–5 s) keep one slow operation from cascading.
− **Handler boilerplate grew.** The `shared_ptr<T>` output-capture pattern
  is mechanical but not trivially expressible in C++. The
  `respondIfTrackCallFailed` helper offsets most of the growth.

## Non-goals

- We did not refactor `TestDAW::getTrack` itself to take the dispatch
  mutex. That would be option A — invent an RW lock and teach HTTP
  readers to hold it. It works, but it simulates a threading model no
  real DAW uses. Rejected in favor of the MT-dispatch approach.
- We did not introduce a per-track mutex. Access is serialized through
  the MT; that's sufficient and matches production.

## Enforcement

Any new HTTP handler that takes a `TestTrack*` outside an MT lambda, or
that passes `[&]` captures into `runOnTrackSync` /
`runOnMessageThreadBlocking` / `callAsync`, is a regression. Reviewers
should flag both patterns on sight. A future lint rule could grep for
the combination, but nothing exists today.
