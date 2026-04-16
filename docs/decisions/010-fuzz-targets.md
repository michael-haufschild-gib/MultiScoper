# ADR-010: libFuzzer Targets for Untrusted Input Surfaces

## Status

Accepted

## Context

Oscil has two ingress points where it parses structured input that did not
originate inside the plugin's own code:

1. **`oscil::OscilState::fromXmlString`** (`src/core/OscilState.cpp:73`) is the
   single entry point for DAW-persisted plugin state. When a DAW reloads a
   session, the host passes the XML blob it serialized previously — but
   nothing guarantees that blob was written by this build, or even by a
   well-formed writer. Session files travel between users, survive DAW
   upgrades, and routinely end up in version control where they can be
   edited by hand. A crash in `fromXmlString` becomes a DAW crash on project
   open, which is the worst possible failure mode.

2. **The plugin test server's JSON handlers** (`src/tools/test_server/*.cpp`)
   bind to `127.0.0.1` and parse untrusted JSON via `nlohmann::json::parse`
   on every request. Any process on the loopback interface can hit them,
   and the E2E harness and its fixtures are authored outside the main
   plugin build, so the input is not trusted. `StateHandler`,
   `OscillatorHandler`, and `SourceHandler` all call
   `nlohmann::json::parse(req.body)` before any validation runs — a crash
   there would affect every handler at once.

Existing unit tests exercise these code paths with hand-written valid and
invalid inputs, but they cannot generate the kind of adversarial bit
sequences (unbalanced tags, truncated UTF-8, deeply nested structures,
multi-megabyte payloads) that real-world bit-flips and malicious inputs
produce.

## Decision

Two libFuzzer targets are added under `fuzz/`, gated by the
`OSCIL_BUILD_FUZZERS` CMake option (default `OFF`):

* **`fuzz_oscil_state_fromxml`** — drives `OscilState::fromXmlString` with
  arbitrary byte sequences. Built by compiling every source file under
  `src/core/` (the transitive closure of what `fromXmlString` depends on)
  plus the minimum JUCE modules (`juce_core`, `juce_data_structures`,
  `juce_events`, `juce_graphics`, `juce_audio_basics`, `juce_dsp`). No
  rendering, UI, or plugin-layer sources are pulled in, so the fuzzer
  binary is small and does not need OpenGL/X11/GUI init.

* **`fuzz_plugin_test_server_state`** — drives `nlohmann::json::parse` on
  the same empty-body-to-`{}` fallback path the real handlers use,
  swallowing exceptions and touching a handful of `value()` accessors to
  exercise member access. The real handlers are tightly coupled to a live
  `juce::AudioProcessorEditor`; exercising them would require bringing up
  the message manager and a fully constructed editor, which is out of
  scope for a stateless fuzz target. The common attack surface is the
  JSON pre-parse, and that is what we fuzz. Future work can extract the
  handler validators into headless, testable form and grow this target to
  cover them.

Both binaries are compiled and linked with
`-fsanitize=fuzzer,address,undefined`. ASan catches use-after-free, heap
overflow, and UAF; UBSan catches signed overflow, null deref, and
alignment bugs. Together these provide strong memory-safety coverage at
runtime.

**Toolchain scope: Linux + mainline LLVM Clang ≥ 14.** AppleClang does not
ship a working libFuzzer runtime (macOS's system clang replaces the
`fuzzer` runtime with a stub). Windows MSVC `/fsanitize=fuzzer` exists but
is out of scope here. The `fuzz/CMakeLists.txt` guard uses
`CMAKE_CXX_COMPILER_ID STREQUAL "Clang"` so AppleClang hits a clear
`fatal_error` with instructions to install Homebrew LLVM.

**CI cadence: nightly, 5 minutes per target.** See
`.github/workflows/fuzz.yml`. Nightly (not per-PR) because libFuzzer runs
are long-tailed and we do not want to block PRs on new-corpus discovery.
Five minutes per target is enough to re-validate the existing corpus
against new code and to find shallow regressions; deeper campaigns can be
triggered manually via `workflow_dispatch`.

## Corpus Strategy

Seed corpora live under `fuzz/corpus/`. They are hand-crafted minimal
examples designed to give libFuzzer's coverage-guided mutator good
starting points:

* `fuzz/corpus/oscil_state/` — five seeds: empty-valid, realistic
  three-oscillator state, adversarial overlapping/duplicate IDs, malformed
  (missing close tag), and a future-version blob (exercises ADR-009
  fail-closed migration).
* `fuzz/corpus/test_server_state/` — three seeds: a typical
  add-oscillator request, a layout update, and an empty `{}` body.

Discovered interesting inputs accumulate in the CI runner's `build/fuzz`
working directory but are not persisted between runs. This is a
deliberate simplicity trade-off — if a CI run finds a new crash-adjacent
input the fix lands in the seed corpus manually, and the corpus stays
small and readable. If we outgrow this we can persist the generated
corpus to an artifact or to S3.

## Crash Triage

A crash surfaces as a non-zero exit code from the fuzzer binary and a
`fuzz-<hash>.crash` file written under
`build/fuzz/artifacts/<target>/`. The workflow uploads those files as a
GitHub artifact on failure. Triage steps:

1. Download the `fuzz-crashes` artifact from the failed workflow run.
2. Reproduce locally:
   `./fuzz_oscil_state_fromxml fuzz-<hash>.crash`. libFuzzer prints the
   ASan/UBSan stack trace and the minimized input.
3. Add a regression test in `tests/` that feeds the minimized input
   through the affected API.
4. Fix the underlying bug, confirm the regression test passes, and add
   the minimized input to the seed corpus under `fuzz/corpus/`.

## Consequences

* Two new CMake targets behind a default-off option. Main plugin build is
  unaffected — configuring `cmake --preset dev` on macOS continues to
  work and continues to skip the fuzz subtree entirely.
* One new nightly CI job (~10 minutes of runner time per night). Does
  not block PRs.
* New directory `fuzz/` under the repo root. Excluded from
  `source_registry_lint.py` (which only scans `src/`) and from
  `size_lint.py` (which is invoked with explicit paths in
  `CMakeLists.txt` that do not include `fuzz/`).
* Contributors on macOS who want to run fuzzers locally must install
  Homebrew LLVM. `fuzz/README.md` documents the commands.

## Alternatives Rejected

* **Factor a reusable `OscilLib` INTERFACE library out of the main
  CMakeLists.txt.** Considered as a cleaner link surface for both the
  plugin and the fuzz targets, but would require reworking the
  `juce_add_plugin` target's warning/suppression wiring and the
  per-source clang-tidy/thread-safety attribute dance. Scope creep. The
  fuzz target instead directly reuses the `OSCIL_SOURCES` variable from
  `cmake/Sources.cmake` (filtered to `src/core/`), which gives the same
  code-reuse without touching plugin wiring.
* **Fuzz on macOS with AppleClang.** Rejected: AppleClang's fuzzer runtime
  is non-functional. Developers can install Homebrew LLVM if they want to
  fuzz locally, but CI is Linux-only.
* **Run fuzzers per-PR.** Rejected: runtime is variable and crash
  discovery is long-tailed. Per-PR runs would create flaky red builds
  without actionable failures.
* **Fuzz the full `PluginTestServer` HTTP pipeline end-to-end.** Rejected
  for now: requires bringing up a live `juce::AudioProcessorEditor` and
  message manager, which turns the fuzz target into a GUI app and
  destroys iteration throughput. The JSON pre-parse covers the shared
  attack surface; handler-specific validators are a future extension
  once they are extracted into headless form.
