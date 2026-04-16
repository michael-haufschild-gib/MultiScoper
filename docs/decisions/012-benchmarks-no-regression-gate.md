# ADR-012: Benchmarks Land as Developer Tool and CI Artifact — No Regression Gate

## Status

Accepted

## Context

Oscil's audio-thread and UI-thread hot paths (`SharedCaptureBuffer::write` and
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

1. Build `OscilBenchmarks` in `Release` with `-O2`.
2. Run it with `--benchmark_format=json --benchmark_out=bench.json
   --benchmark_repetitions=3 --benchmark_min_time=1s`.
3. Upload `bench.json` as an artifact named `bench-<sha>`.
4. Exit 0 unless the binary actually crashes. No comparison, no threshold.

Locally, `cmake -DOSCIL_BUILD_BENCHMARKS=ON …` produces the same binary under
`build/bench/benchmarks/OscilBenchmarks`, usable for any developer doing
targeted performance work.

## Benchmark surfaces covered

| Type / Method | Benchmark name(s) | Sweep |
|-|-|-|
| `SharedCaptureBuffer::write(const float* const*, int, int, CaptureFrameMetadata, bool)` | `BM_SharedCaptureBuffer_Write` | numSamples ∈ {64, 256, 1024, 4096}, 2 channels |
| `SharedCaptureBuffer::read(juce::AudioBuffer<float>&, int)` | `BM_SharedCaptureBuffer_Read` | numSamples ∈ {64, 256, 1024, 4096} |
| `DecimatingCaptureBuffer::write(const float* const*, int, int, CaptureFrameMetadata)` | `BM_DecimatingCaptureBuffer_Write` | numSamples × decimationRatio cross product; ratios {1, 2, 4, 8} mapped via `(QualityPreset, sourceRate)` |
| `SignalProcessor::process(span, span, ProcessingMode, ProcessedSignal&)` | `BM_SignalProcessor_{FullStereo,Mono,Mid,Side,Left,Right}` | numSamples ∈ {64, 256, 1024, 4096} |
| `TimingEngine::updateHostInfo` + `processBlock` | `BM_TimingEngine_Update` | block size ∈ {64, 128, 256, 512, 1024} at 48 kHz, trigger enabled |

Each benchmark reports `SetItemsProcessed` (input samples) and
`SetBytesProcessed` so results can be read directly as items/s and bytes/s.

## Local invocation

```bash
cmake -B build/bench -G Ninja \
  -DOSCIL_BUILD_BENCHMARKS=ON \
  -DOSCIL_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/bench --target OscilBenchmarks

./build/bench/benchmarks/OscilBenchmarks \
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
across the ~24 benchmarks in the suite is the target design.

Self-hosted or dedicated runners would cut variance dramatically and make a
simpler threshold gate viable; this is considered out of scope for the
foreseeable future (hosting cost, security surface).

## Related ADRs

* ADR-006: Dependency Pinning Policy — Google Benchmark is pinned to the
  tagged release `v1.9.5`.
* ADR-010: libFuzzer Targets — same "optional CMake feature, gated on an
  opt-in flag, with a dedicated workflow" pattern used here.
