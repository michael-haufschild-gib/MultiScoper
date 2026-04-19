# ADR-012: Benchmarks Land as Developer Tool and CI Artifact — No Regression Gate

## Status

Accepted

## Context

MultiScoper's audio-thread and UI-thread hot paths (`SharedCaptureBuffer::write` and
`read`, `DecimatingCaptureBuffer::write`, `SignalProcessor::process` for each
`ProcessingMode`, `TimingEngine::updateHostInfo` + `processBlock`) are
performance-sensitive. A regression in any one of them can translate directly
to audio glitches (priority inversion on the audio thread) or UI jank.

Prior to this ADR the project had no systematic way to observe the wall-clock
cost of these paths over time. All performance claims were either anecdotal or
inferred from release builds running in a DAW.

The obvious remedy — microbenchmarks in CI with a regression gate — runs into
a well-documented problem on GitHub-hosted runners:

* Shared virtualized CPU cores with neighbor contention.
* No frequency-scaling control from user-land.
* Inconsistent host generations (runners are periodically refreshed).
* Turbo / thermal throttling invisible to the process.

Empirical reports from other C++ audio / DSP projects on the same runner SKU
consistently show ≥20% run-to-run variance on tight microbenchmarks
(single-digit-microsecond iterations). A 10% regression gate against a single
prior run would fire on noise more often than on real regressions, train
developers to ignore it, and then miss the real ones when they land.

## Decision

Land Google Benchmark and the benchmark suite **without** a CI regression gate.

The CI workflow (`.github/workflows/benchmarks.yml`) will, on every pull
request and push to `main`:

1. Build `MultiScoperBenchmarks` in `Release` with `-O2`.
2. Run it with `--benchmark_format=json --benchmark_out=bench.json
   --benchmark_repetitions=3 --benchmark_min_time=1s`.
3. Upload `bench.json` as an artifact named `bench-<sha>`.
4. Exit 0 unless the binary actually crashes. No comparison, no threshold.

Locally, `cmake -DMULTISCOPER_BUILD_BENCHMARKS=ON …` produces the same binary under
`build/bench/benchmarks/MultiScoperBenchmarks`, usable for any developer doing
targeted performance work.

## Benchmark surfaces covered

| Type / Method | Benchmark name(s) | Sweep | Cells |
|-|-|-|-|
| `SharedCaptureBuffer::write(const float* const*, int, int, CaptureFrameMetadata, bool)` | `BM_SharedCaptureBuffer_Write` | numSamples ∈ {64, 256, 1024, 4096}, 2 channels | 4 |
| `SharedCaptureBuffer::read(juce::AudioBuffer<float>&, int)` | `BM_SharedCaptureBuffer_Read` | numSamples ∈ {64, 256, 1024, 4096} | 4 |
| `DecimatingCaptureBuffer::write(const float* const*, int, int, CaptureFrameMetadata)` | `BM_DecimatingCaptureBuffer_Write` | numSamples × decimationRatio cross product; ratios {1, 2, 4, 8} mapped via `(QualityPreset, sourceRate)` | 16 |
| `DecimatingCaptureBuffer::readBlocking(juce::AudioBuffer<float>&, int)` | `BM_DecimatingCaptureBuffer_ReadBlocking` | numSamples × decimationRatio same cross product as Write | 16 |
| `DecimatingCaptureBuffer::readSnapshot(juce::AudioBuffer<float>&, int)` | `BM_DecimatingCaptureBuffer_ReadSnapshot` | same cross product; records `tornReads` counter | 16 |
| `SignalProcessor::process(span, span, ProcessingMode, ProcessedSignal&)` | `BM_SignalProcessor_{FullStereo,Mono,Mid,Side,Left,Right}` | numSamples ∈ {64, 256, 1024, 4096} | 24 |
| `TimingEngine::updateHostInfo` + `processBlock` | `BM_TimingEngine_Update` | block size ∈ {64, 128, 256, 512, 1024} at 48 kHz, trigger enabled | 5 |
| `SignalProcessor::{calculatePeak,calculateRMS,calculateCorrelation}` | `BM_Calculate{Peak,RMS,Correlation}` | numSamples ∈ {256, 1024, 2048, 4096} | 12 |
| `AdaptiveDecimator::{processWithEnvelope,process}` | `BM_EnvelopeDecimator`, `BM_PeakDecimator` | (inputSamples ∈ {2048, 4096, 8192, 16384}) × (displayWidth ∈ {800, 1600}) | 16 |

Total: **113 parameterized benchmark cells** (8 shared write/read + 48
decimating write/read + 24 signal-processor + 5 timing + 28 analysis).
The Bonferroni correction in `scripts/bench_compare.py` uses the count of
shared cells between baseline and current runs, so the per-test alpha
scales automatically as the suite grows. At α = 0.05 across 113 cells,
α_per_test ≈ 4.42 × 10⁻⁴.

Each benchmark reports `SetItemsProcessed` (input samples) and
`SetBytesProcessed` so results can be read directly as items/s and bytes/s.

## Local invocation

```bash
cmake -B build/bench -G Ninja \
  -DMULTISCOPER_BUILD_BENCHMARKS=ON \
  -DMULTISCOPER_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/bench --target MultiScoperBenchmarks

./build/bench/benchmarks/MultiScoperBenchmarks \
  --benchmark_filter=SharedCaptureBuffer
```

Developers doing performance work should disable frequency scaling and run on
a quiet machine — see `benchmarks/README.md`.

## Consequences

* **Good**: the surfaces are measurable with a one-command workflow, both
  locally and in CI. Every PR has a `bench.json` artifact that a reviewer can
  download and diff against a previous PR's artifact manually when the diff
  touches suspect code.
* **Good**: no false-positive CI failures from runner noise.
* **Acceptable**: no automatic alerting on regressions. Catching them relies
  on the author or reviewer checking the artifact. This is worse than a gate
  that actually worked, and better than a gate that cried wolf.
* **Deferred**: the real gate. See Future Work.

## Future work

Before reconsidering a CI regression gate, collect ≥50 back-to-back runs of the
full suite on the same runner SKU (either by scheduling the workflow nightly
for a month or by running a one-off matrix job with 50 copies). From that
dataset, compute per-benchmark median + median-absolute-deviation. A gate built
on a Mann–Whitney U test between the PR sample and the last-N-main samples
(N ≈ 30) with a Bonferroni correction for the multiple-comparison problem
across the suite's 113 benchmark cells is the target design.

Self-hosted or dedicated runners would cut variance dramatically and make a
simpler threshold gate viable; this is considered out of scope for the
foreseeable future (hosting cost, security surface).

## Gate infrastructure (staged for activation)

The Mann-Whitney U comparison is implemented in
`scripts/bench_compare.py` and covered by unit tests in
`tests/lint/test_bench_compare.py`:

* One-sided Mann-Whitney U (`alternative='greater'`) per benchmark, with
  scipy when available and a pure-Python fallback when scipy is absent.
  The fallback auto-selects between exact rank-subset enumeration (for
  small samples — `C(n1+n2, min(n1,n2)) ≤ 200_000`) and the normal
  approximation with tie + continuity correction (for larger samples).
  The exact path is required for the CI-typical shape of 3 current samples
  vs 50 baseline samples, where the normal approximation misrepresents
  p-values by up to ~50× and silently turns the gate into a false-negative
  machine.
* Bonferroni correction — the family-wise alpha (default 0.05) is divided
  across the N shared benchmarks before the per-test cutoff is applied.
* Economic-tolerance floor — regressions where the current median is within
  10% of the baseline median are ignored even when statistically significant
  (filters runner-noise "significance").
* New benchmarks and removed benchmarks are logged but never fail the gate.

Activation path:

1. Run the workflow from `main` via **Run workflow → capture_baseline = true**.
   This captures 50 reps, uploads the result as the `bench-baseline`
   artifact, and sets the 90-day retention clock.
2. From the next PR onward, every run of `benchmarks.yml` downloads
   `bench-baseline` (via `dawidd6/action-download-artifact`) and runs
   `scripts/bench_compare.py`. If no baseline exists, the comparison step is
   skipped — the workflow degrades gracefully to the pre-gate behavior.
3. Refresh the baseline after intentional performance changes land on main
   by re-running step 1.

The gate is not "armed" until step 1 runs — deliberately, so the baseline
reflects a known-good main-branch state chosen by a human, not whatever
happened to run in CI last.

## Related ADRs

* ADR-006: Dependency Pinning Policy — Google Benchmark is pinned to the
  tagged release `v1.9.5`.
* ADR-010: libFuzzer Targets — same "optional CMake feature, gated on an
  opt-in flag, with a dedicated workflow" pattern used here.
