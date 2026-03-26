# Development Guide

**Purpose**: Building, running, and developing the Oscil plugin.
**Build System**: CMake 3.25+ with CMakePresets.json
**Compiler Requirements**: C++20 compatible (Clang 14+, GCC 11+, MSVC 2019+)

## Quick Start (Recommended)

Using CMake presets for fastest builds with ccache + Ninja:

```bash
# 1. Install build tools (one-time)
brew install ninja ccache    # macOS
# apt install ninja-build ccache  # Linux

# 2. Configure and build
cmake --preset dev
cmake --build --preset dev

# 3. Run tests
ctest --preset dev
```

## Available Presets

| Preset | Description | Use Case |
|--------|-------------|----------|
| `dev` | Debug + Ninja + ccache + Inspector | Daily development |
| `dev-release` | RelWithDebInfo + Ninja + ccache | Profiling with debug info |
| `release` | Release + Ninja + ccache | Local release builds |
| `ci` | Release + tests (no inspector) | CI builds |
| `ci-macos` | CI + universal binary | macOS CI |
| `ci-linux` | CI for Linux | Linux CI |
| `ci-windows` | CI for Windows | Windows CI |
| `rtsan` | Debug + RealtimeSanitizer | Audio thread safety testing |

```bash
# List all presets
cmake --list-presets

# Configure with preset
cmake --preset dev

# Build with preset
cmake --build --preset dev

# Test with preset
ctest --preset dev
```

## Legacy Commands (Without Presets)

If you prefer manual configuration:

```bash
# Configure (Debug with Ninja + ccache)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOSCIL_BUILD_TESTS=ON

# Build
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `OSCIL_BUILD_TESTS` | ON | Build unit tests |
| `OSCIL_BUILD_TEST_HARNESS` | OFF | Build E2E test harness |
| `OSCIL_ENABLE_OPENGL` | ON | Enable OpenGL for GPU compositing |
| `OSCIL_ENABLE_INSPECTOR` | OFF | Enable Melatonin Inspector (dev only) |
| `OSCIL_ENABLE_RTSAN` | OFF | Enable RealtimeSanitizer (Clang 20+) |
| `CMAKE_BUILD_TYPE` | (none) | Debug, Release, RelWithDebInfo |

## Build Acceleration

### ccache (Compiler Cache)

Caches compilation results for 5-10x faster rebuilds. CMakeLists.txt auto-detects ccache.

```bash
# Install
brew install ccache    # macOS
apt install ccache     # Linux

# Verify detection
cmake --preset dev 2>&1 | grep ccache
# Should show: "Found ccache: /opt/homebrew/bin/ccache"

# Check cache stats
ccache -s
```

### Ninja (Build System)

Faster than Make, better parallelization.

```bash
# Install
brew install ninja    # macOS
apt install ninja-build  # Linux

# Ninja is auto-selected by presets
cmake --preset dev  # Uses Ninja automatically
```

### Combined Speed Benefits

| Build | Make (cold) | Ninja + ccache (warm) | Speedup |
|-------|-------------|----------------------|---------|
| Full rebuild | ~3 min | ~30 sec | 6x |
| Single file | ~30 sec | ~3 sec | 10x |

## Project Structure

```
oscil4/
├── CMakeLists.txt          # Main build configuration
├── CMakePresets.json       # Build presets (Ninja + ccache)
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

| Output | Location |
|--------|----------|
| VST3 Plugin | `build/<preset>/Oscil_artefacts/Release/VST3/oscil4.vst3` |
| AU Plugin | `build/<preset>/Oscil_artefacts/Release/AU/oscil4.component` |
| Standalone | `build/<preset>/Oscil_artefacts/Release/Standalone/oscil4.app` |
| Test Binary | `build/<preset>/OscilTests` |

Plugins are also copied to system locations when `COPY_PLUGIN_AFTER_BUILD` is enabled:
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`

## Development Workflow

### Making Changes

```bash
# 1. Configure once
cmake --preset dev

# 2. Edit source files
# ...

# 3. Rebuild (fast with ccache)
cmake --build --preset dev

# 4. Run tests
ctest --preset dev

# 5. Test in DAW if needed
```

### Adding New Files

1. Create header in `include/[domain]/ClassName.h`
2. Create implementation in `src/[domain]/ClassName.cpp`
3. Add to `CMakeLists.txt`:

```cmake
# In target_sources(Oscil PRIVATE ...)
src/[domain]/ClassName.cpp
```

4. Rebuild: `cmake --build --preset dev`

### Adding New Tests

1. Create test file: `tests/test_class_name.cpp`
2. Add to `CMakeLists.txt` under `add_executable(OscilTests ...)`
3. If testing new code, also add the source file to OscilTests
4. Rebuild and run: `cmake --build --preset dev && ctest --preset dev`

## Dependencies

All dependencies are fetched automatically via CMake FetchContent:

| Dependency | Version | Purpose |
|------------|---------|---------|
| JUCE | 8.0.12 | Audio plugin framework |
| GoogleTest | 1.17.0 | Unit testing (if tests enabled) |
| cpp-httplib | 0.38.0 | HTTP server for E2E test harness |
| nlohmann/json | 3.11.3 | JSON parsing for test harness API |
| Melatonin Inspector | main | UI debugging (if inspector enabled) |

First build will download dependencies (requires internet).

## IDE Setup

### CLion
1. Open folder as CMake project
2. Select preset from CMake Presets dropdown
3. Build/Run targets available in toolbar

### VS Code
1. Install CMake Tools extension
2. Open folder
3. Select preset from status bar or command palette
4. Configure and build via command palette

### Xcode
```bash
cmake -G Xcode -B build-xcode
open build-xcode/Oscil.xcodeproj
```

## Debugging

### Standalone App
Run the Standalone target directly or attach debugger.

### Plugin in DAW
1. Build Debug configuration: `cmake --preset dev`
2. Launch DAW
3. Attach debugger to DAW process
4. Load plugin

### Test Debugging
```bash
# Run specific test with verbose output
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.*" --gtest_break_on_failure
```

### UI Debugging with Melatonin Inspector
Enable with `OSCIL_ENABLE_INSPECTOR=ON` (included in `dev` preset):
- Press `Cmd+I` (macOS) or `Ctrl+I` (Windows/Linux) to toggle
- Inspect component hierarchy, bounds, and properties

### E2E Test Harness
```bash
# Start test harness (HTTP server on port 8765)
./build/dev/OscilTestHarness_artefacts/OscilTestHarness.app/Contents/MacOS/OscilTestHarness

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

### Issue: ccache not found
**Solution**: Install ccache and ensure it's in PATH:
```bash
brew install ccache  # macOS
which ccache  # Verify: should show path
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
cmake --build --preset dev --verbose

# Clean and rebuild
rm -rf build && cmake --preset dev && cmake --build --preset dev

# Check CMake cache
cmake -L build/dev  # List cached variables

# Regenerate with fresh cache
rm -rf build/dev && cmake --preset dev

# Clear ccache
ccache -C
```

## Performance Profiling

### macOS Instruments
1. Build Release: `cmake --preset release && cmake --build --preset release`
2. Open Instruments
3. Attach to Standalone or DAW
4. Use Time Profiler or Allocations

### Frame rate check
Enable internal timing in `StatusBarComponent` to monitor render performance.

## Release Build

```bash
# Full release build
cmake --preset release
cmake --build --preset release

# Outputs ready for distribution in build/release/Oscil_artefacts/
```
