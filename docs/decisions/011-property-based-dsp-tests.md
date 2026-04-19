# ADR-011: Property-Based Tests for DSP Modules

## Status

Accepted

## Context

MultiScoper's DSP modules (`SignalProcessor`, `TimingEngine`, `DecimatingCaptureBuffer`, `SharedCaptureBuffer`) are currently exercised by a broad suite of hand-written GoogleTest unit tests. Those tests check specific, hand-picked inputs: "process a 1024-sample sine wave through Mono mode and verify the average," "write 2048 samples then read back 1024," and so on. Hand-picked inputs catch the bugs the author thought to look for. They miss:

- Interactions between unrelated configuration axes (quality preset × source rate × duration).
- Boundary conditions around power-of-two rounding, wrap-around indices, and FP epsilon.
- Permutation and symmetry invariants that must hold across *all* legal inputs, not just the examples on hand.

Property-based tests assert invariants over *generated* inputs and shrink on failure to a minimal counterexample. They complement hand-written tests; they do not replace them.

## Decision

Adopt [rapidcheck](https://github.com/emil-e/rapidcheck) (with its GoogleTest adapter) as a **test-only** dependency. Rapidcheck is fetched via `CPMAddPackage` in `cmake/Tests.cmake` and linked only into `MultiScoperTests`. It is not a dependency of any plugin artifact (VST3 / AU / CLAP / Standalone), nor of the fuzzer targets, nor of any release binary.

### Scope of properties added

| Module                     | Properties asserted                                                                                 |
|----------------------------|-----------------------------------------------------------------------------------------------------|
| `SignalProcessor`          | Output magnitude bounded by input magnitude (all `ProcessingMode`s); mono-mix L/R-swap invariance; Mid/Side round-trip; empty-input safety. |
| `TimingEngine`             | Monotonicity of `getDisplaySampleCount` in interval; sample-rate proportionality (doubling rate doubles sample count); zero sample rate → zero sample count. |
| `DecimatingCaptureBuffer`  | Quality preset → decimation ratio correctness for ratios {1, 2, 4, 8}; `configure()` idempotence (calling twice leaves observable state identical to calling once). |
| `SharedCaptureBuffer`      | Read-after-write: N writes followed by read(N) returns the written sequence in order; wrap-around: writing 2×capacity and reading `capacity` returns the *last* `capacity` samples; multi-channel independence. |

Each property is expressed via `RC_GTEST_PROP(Suite, Name, (args))` and counted as a single ctest case. Rapidcheck runs the default 100 iterations per property on each ctest invocation. Counter-examples are shrunk and printed with a seed for reproducibility.

### How to add a new property

1. Pick the module and identify an invariant that must hold for all legal inputs.
2. Add the property to the relevant `tests/test_*_properties.cpp` file (or create a new one and register it in `cmake/Tests.cmake::MULTISCOPER_TEST_SOURCES`).
3. Use rapidcheck generators (`rc::gen::inRange`, `rc::gen::container`, `rc::gen::arbitrary`) bounded to realistic audio ranges (samples in `[-1, 1]`, buffer sizes ≤ 64 for tight shrinks, sample rates in `{22050, 44100, 48000, 88200, 96000, 192000}`).
4. Express the assertion with `RC_ASSERT` or standard `EXPECT_*`. GoogleTest failure handlers interoperate with rapidcheck's shrinker.
5. If the property fails with a real minimal counterexample, do not weaken the property — investigate whether production code has a bug and fix it, or add a task to the task list.

### Pin policy

Rapidcheck does not publish semver tags. Per ADR-006, dependencies without stable tags are pinned to a 40-character commit SHA with a dated rationale comment. The initial pin is `ff6af6fc683159deb51c543b065eba14dfcf329b` (September 2024 HEAD). Review cadence: quarterly, alongside the other SHA-pinned dependencies listed in ADR-006.

## Consequences

- DSP regressions that only surface under specific generator-seeded inputs are caught early with a minimal counter-example rather than appearing as flaky user reports.
- Plugin artifacts do not gain a runtime dependency — rapidcheck headers and code are compiled into `MultiScoperTests` only. Release and fuzzer builds are unaffected.
- Property tests run on the same `ctest --preset dev` invocation developers already use, with a default 100 iterations each. CI time cost is minimal (single-digit seconds).
- A new ADR is added to the dependency-pinning bookkeeping in ADR-006; rapidcheck must be included in the quarterly pin review.

## Production defects discovered

None at introduction.

One API subtlety was surfaced and documented rather than changed: for `DecimatingCaptureBuffer`, the stored `getCaptureRate()` returns the *integer-exact* post-decimation rate (`sourceRate / decimationRatio`), which may differ from the preset's *nominal* rate reported by `CaptureQualityConfig::getCaptureRate`. Example: at `sourceRate=192000` with `QualityPreset::Standard` (nominal 22050 Hz), the decimation ratio truncates to `192000 / 22050 = 8`, so the buffer stores `192000 / 8 = 24000`. This is the rate the decimator actually produces, and is what subsequent consumers should use. The property test asserts the integer-exact contract (`sourceRate / ratio`) rather than the nominal preset rate. Callers relying on `buffer.getCaptureRate()` to round-trip with `config.getCaptureRate(sr)` should switch to the integer-exact relation; the nominal preset rate is only a target, not a stored value.

If future property runs surface defects, they are to be documented here with a link to the fix commit and (if tracked) the task ID.

## Alternatives rejected

- **Hypothesis via FFI or a Python harness.** Introduces a second toolchain and a serialization boundary; too heavy for in-process DSP invariants.
- **Expand hand-written tests further.** Hand-written tests cannot cover the combinatorial space; they complement but do not replace property tests.
- **Add fuzzers (libFuzzer) for these modules.** Covered separately in ADR-010; fuzzers prioritize crash discovery over invariant assertions. Property tests are the right tool for mathematical invariants like `(L+R)/2 == (R+L)/2`.
