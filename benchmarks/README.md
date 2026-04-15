# Oscil Benchmarks

Google Benchmark microbenchmarks for Oscil's DSP and capture-buffer hot paths.

## Build and run locally

From the repo root:

```bash
cmake -B build/bench -G Ninja \
  -DOSCIL_BUILD_BENCHMARKS=ON \
  -DOSCIL_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/bench --target OscilBenchmarks

./build/bench/benchmarks/OscilBenchmarks \
  --benchmark_filter=SharedCaptureBuffer
```

Run everything:

```bash
./build/bench/benchmarks/OscilBenchmarks
```

Emit JSON for diffing / tooling:

```bash
./build/bench/benchmarks/OscilBenchmarks \
  --benchmark_format=json \
  --benchmark_out=bench.json \
  --benchmark_repetitions=3 \
  --benchmark_min_time=1s
```

## What is covered

| Surface | Benchmarks |
|-|-|
| `SharedCaptureBuffer::write` / `read` | `BM_SharedCaptureBuffer_Write`, `BM_SharedCaptureBuffer_Read` — numSamples ∈ {64, 256, 1024, 4096} |
| `DecimatingCaptureBuffer::write` | `BM_DecimatingCaptureBuffer_Write` — numSamples × decimationRatio ∈ {1, 2, 4, 8} |
| `SignalProcessor::process` | One benchmark per `ProcessingMode` (FullStereo, Mono, Mid, Side, Left, Right) — numSamples ∈ {64, 256, 1024, 4096} |
| `TimingEngine::updateHostInfo` + `processBlock` | `BM_TimingEngine_Update` — block sizes ∈ {64, 128, 256, 512, 1024} |

## Best practices

These numbers are sensitive to every kind of system noise. For reproducible
local measurements:

- Close everything that runs in the background (browsers, DAWs, containers).
- Pin frequency scaling if possible (Linux: `cpupower frequency-set -g performance`).
- On Apple Silicon, run on AC power — the efficiency cores behave differently
  on battery.
- Use `--benchmark_repetitions=10 --benchmark_report_aggregates_only=true` to
  surface variance as median / stddev.
- Warm caches before comparing: the first run is always slower.

## No regression gate in CI

The CI workflow (`.github/workflows/benchmarks.yml`) runs these on every PR and
push to `main`, and uploads the JSON result as an artifact, but does NOT fail
the build on regressions. GitHub-hosted runner variance on tight microbenchmarks
routinely exceeds 20%, which would produce too many false positives under a
percentage-delta gate. See `docs/decisions/012-benchmarks-no-regression-gate.md`
for the full reasoning and the future-work plan (variance characterization over
≥50 runs, then a Mann–Whitney U comparison gate).
