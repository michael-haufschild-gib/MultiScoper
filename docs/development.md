# Development Guide for LLM Coding Agents

**Purpose**: Instructions for building, running, and developing the Oscil plugin.
**Build System**: CMake 3.20+
**Compiler Requirements**: C++20 compatible (Clang 14+, GCC 11+, MSVC 2019+)

## Quick Start

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure
cmake -DOSCIL_BUILD_TESTS=ON ..

# 3. Build
cmake --build . --config Release

# 4. Run tests
./OscilTests
```

## Key Commands

| Task | Command |
|------|---------|
| Configure (Debug) | `cmake -DCMAKE_BUILD_TYPE=Debug -DOSCIL_BUILD_TESTS=ON ..` |
| Configure (Release) | `cmake -DCMAKE_BUILD_TYPE=Release ..` |
| Build plugin | `cmake --build . --target Oscil` |
| Build tests | `cmake --build . --target OscilTests` |
| Build E2E harness | `cmake --build . --target OscilTestHarness` |
| Build all | `cmake --build .` |
| Run tests | `./OscilTests` |
| Run tests (CTest) | `ctest --output-on-failure` |
| Clean build | `rm -rf build && mkdir build` |

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `OSCIL_BUILD_TESTS` | ON | Build unit tests |
| `OSCIL_BUILD_TEST_HARNESS` | OFF | Build E2E test harness |
| `OSCIL_ENABLE_OPENGL` | ON | Enable OpenGL for GPU compositing |
| `CMAKE_BUILD_TYPE` | (none) | Debug, Release, RelWithDebInfo |

```bash
# Example: Release build with OpenGL
cmake -DCMAKE_BUILD_TYPE=Release -DOSCIL_ENABLE_OPENGL=ON ..

# Example: Debug build with E2E test harness
cmake -DCMAKE_BUILD_TYPE=Debug -DOSCIL_BUILD_TESTS=ON -DOSCIL_BUILD_TEST_HARNESS=ON ..
```

## Project Structure

```
oscil4/
├── CMakeLists.txt          # Main build configuration
├── include/                # Header files
│   ├── core/               # Core logic headers
│   ├── dsp/                # DSP headers
│   ├── ui/                 # UI headers
│   │   ├── sections/       # Sidebar section headers
│   │   └── coordinators/   # Coordinator headers
│   └── Oscil.h             # Main header
├── src/                    # Implementation files
│   ├── core/               # Core implementations
│   ├── dsp/                # DSP implementations
│   └── ui/                 # UI implementations
│       ├── sections/       # Sidebar section implementations
│       └── coordinators/   # Coordinator implementations
├── tests/                  # Unit tests (GoogleTest)
├── test_harness/           # E2E test harness
│   ├── include/            # Test harness headers
│   └── src/                # Test harness implementations
├── docs/                   # Documentation
├── screenshots/            # Screenshots (for visual testing)
├── build/                  # Build output (generated)
└── resources/              # Plugin resources
```

## Build Output Locations

After building, find outputs at:

| Output | Location (macOS) |
|--------|------------------|
| VST3 Plugin | `build/Oscil_artefacts/VST3/Oscil.vst3` |
| AU Plugin | `build/Oscil_artefacts/AU/Oscil.component` |
| Standalone | `build/Oscil_artefacts/Standalone/Oscil.app` |
| Test Binary | `build/OscilTests` |
| E2E Test Harness | `build/OscilTestHarness_artefacts/OscilTestHarness.app` |

Plugins are also copied to system locations when `COPY_PLUGIN_AFTER_BUILD` is enabled:
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`

## Development Workflow

### Making Changes

1. Edit source files in `include/` and `src/`
2. Rebuild: `cmake --build build`
3. Run tests: `./build/OscilTests`
4. Test in DAW if needed

### Adding New Files

1. Create header in `include/[domain]/ClassName.h`
2. Create implementation in `src/[domain]/ClassName.cpp`
3. Add to `CMakeLists.txt`:

```cmake
# In target_sources(Oscil PRIVATE ...)
src/[domain]/ClassName.cpp
```

4. Rebuild: `cmake --build build`

### Adding New Tests

1. Create test file: `tests/test_class_name.cpp`
2. Add to `CMakeLists.txt` under `add_executable(OscilTests ...)`
3. If testing new code, also add the source file to OscilTests
4. Rebuild and run: `cmake --build build && ./build/OscilTests`

## Dependencies

All dependencies are fetched automatically via CMake FetchContent:

| Dependency | Version | Purpose |
|------------|---------|---------|
| JUCE | 8.0.4 | Audio plugin framework |
| GoogleTest | 1.14.0 | Unit testing (if tests enabled) |
| cpp-httplib | 0.14.3 | HTTP server for E2E test harness |
| nlohmann/json | 3.11.3 | JSON parsing for test harness API |

First build will download dependencies (requires internet).

## IDE Setup

### CLion
1. Open folder as CMake project
2. Select build configuration (Debug/Release)
3. Build/Run targets available in toolbar

### VS Code
1. Install CMake Tools extension
2. Open folder
3. Select kit (compiler)
4. Configure and build via command palette

### Xcode
```bash
cmake -G Xcode ..
open Oscil.xcodeproj
```

## Debugging

### Standalone App
Run the Standalone target directly or attach debugger.

### Plugin in DAW
1. Build Debug configuration
2. Launch DAW
3. Attach debugger to DAW process
4. Load plugin

### Test Debugging
```bash
# Run specific test with verbose output
./build/OscilTests --gtest_filter="SignalProcessorTest.*" --gtest_break_on_failure
```

### E2E Test Harness
```bash
# Start test harness (HTTP server on port 8765)
./build/OscilTestHarness_artefacts/OscilTestHarness.app/Contents/MacOS/OscilTestHarness

# Health check
curl http://localhost:8765/health

# Run E2E tests (separate terminal)
python tests/e2e/test_waveform.py
```

## Common Issues

### Issue: JUCE fetch fails
**Solution**: Check internet connection. If behind proxy, configure CMake:
```bash
cmake -DCMAKE_HTTPS_PROXY="http://proxy:port" ..
```

### Issue: Plugin not loading in DAW
**Solution**: Check AU/VST3 validation:
```bash
# macOS AU validation
auval -a  # List all AUs
auval -v aufx Osc1 Osci  # Validate specific AU
```

### Issue: Build fails with C++20 errors
**Solution**: Ensure compiler supports C++20:
```bash
# Check compiler version
clang++ --version  # Need 14+
g++ --version      # Need 11+
```

### Issue: Tests crash on JUCE initialization
**Solution**: Tests link only required JUCE modules. If test needs more JUCE functionality, add the module to `target_link_libraries(OscilTests ...)` in CMakeLists.txt.

## Troubleshooting

```bash
# Verbose build output
cmake --build . --verbose

# Clean and rebuild
rm -rf build && mkdir build && cd build && cmake .. && cmake --build .

# Check CMake cache
cmake -L .  # List cached variables

# Regenerate with fresh cache
rm CMakeCache.txt && cmake ..
```

## Performance Profiling

### macOS Instruments
1. Build Release
2. Open Instruments
3. Attach to Standalone or DAW
4. Use Time Profiler or Allocations

### Frame rate check
Enable internal timing in `StatusBarComponent` to monitor render performance.

## Release Build

```bash
# Full release build
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DOSCIL_BUILD_TESTS=OFF ..
cmake --build . --config Release

# Outputs ready for distribution in build-release/Oscil_artefacts/
```
