# Oscil libFuzzer Targets

Coverage-guided fuzzers for Oscil's two untrusted-input ingress points.
Design rationale and triage workflow live in
`docs/decisions/010-fuzz-targets.md`.

## Targets

| Binary | Exercises | Seed corpus |
|--------|-----------|-------------|
| `fuzz_oscil_state_fromxml`         | `oscil::OscilState::fromXmlString` (DAW-persisted XML loader, `src/core/OscilState.cpp:73`) | `fuzz/corpus/oscil_state/` |
| `fuzz_plugin_test_server_state`    | `nlohmann::json::parse` path shared by every `src/tools/test_server/*Handler.cpp`          | `fuzz/corpus/test_server_state/` |

## Toolchain Requirements

**Supported:** mainline LLVM Clang ≥ 14 on Linux, or Homebrew-installed
LLVM on macOS.

**NOT supported:** AppleClang (the system `clang++` on macOS). AppleClang
does not ship a working libFuzzer runtime — configuring with
`OSCIL_BUILD_FUZZERS=ON` under AppleClang triggers a CMake `fatal_error`
with this exact message.

On macOS, install mainline LLVM:

```bash
brew install llvm
```

This installs `clang++` at `/opt/homebrew/opt/llvm/bin/clang++` on Apple
Silicon (or `/usr/local/opt/llvm/bin/clang++` on Intel). Do **not** add it
to `PATH` globally — only use it via `-DCMAKE_CXX_COMPILER=...` for the
fuzz build so your regular Oscil builds continue to use AppleClang.

## Build and Run Locally

Configure a dedicated build directory with the fuzzers on and tests /
plugin-copy off:

```bash
cmake -B build/fuzz \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DOSCIL_BUILD_FUZZERS=ON \
  -DOSCIL_BUILD_TESTS=OFF \
  -DOSCIL_COPY_PLUGIN_AFTER_BUILD=OFF
```

(On Linux, point `CMAKE_CXX_COMPILER` at `clang++-20` or whichever LLVM
you installed. Add `-G Ninja` if you want Ninja.)

Build one target at a time:

```bash
cmake --build build/fuzz --target fuzz_oscil_state_fromxml
cmake --build build/fuzz --target fuzz_plugin_test_server_state
```

Run — each binary takes the seed corpus as its first positional argument
and libFuzzer flags after it. `-max_total_time` is in seconds.

```bash
./build/fuzz/fuzz/fuzz_oscil_state_fromxml \
    fuzz/corpus/oscil_state/ \
    -max_total_time=60

./build/fuzz/fuzz/fuzz_plugin_test_server_state \
    fuzz/corpus/test_server_state/ \
    -max_total_time=60
```

Useful libFuzzer flags:

* `-runs=N` — stop after N executions instead of on a timer.
* `-print_final_stats=1` — dump coverage stats at the end.
* `-artifact_prefix=crashes/` — write `fuzz-<hash>.crash` files under
  `crashes/` (default is the current directory).
* `-jobs=N` — run N parallel workers.

## Reproducing a Crash

If the fuzzer prints a stack trace and writes `fuzz-<hash>`, replay the
exact input by passing the file as the only positional argument:

```bash
./build/fuzz/fuzz/fuzz_oscil_state_fromxml fuzz-<hash>
```

libFuzzer will minimize the input and re-print the trace. Add the
minimized artifact to the seed corpus once the bug is fixed so the
regression stays covered.

## CI

The `.github/workflows/fuzz.yml` workflow runs both targets nightly at
02:00 UTC on `ubuntu-24.04` with LLVM 20 for 5 minutes each. Crashes
fail the workflow and upload artifacts under the `fuzz-crashes` name.
Trigger manually with the "Run workflow" button if needed.
