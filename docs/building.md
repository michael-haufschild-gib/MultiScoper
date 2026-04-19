# Building Guide

**Purpose**: Instructions for building, running, and developing the MultiScoper plugin locally.
**Tech Stack**: CMake 3.31+, C++20, JUCE 8.0.12, GoogleTest 1.17.0, Ninja

## Quick Reference (Using CMake Presets)

```bash
# Development build (Debug + tests)
cmake --preset dev
cmake --build --preset dev

# Run tests
ctest --preset dev

# Release build
cmake --preset release
cmake --build --preset release

# Test harness for E2E testing
cmake --preset dev -DMULTISCOPER_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target MultiScoperTestHarness
```

**Preferred**: Always use CMake presets for consistent builds.

## Prerequisites

| Requirement | Version | Install (macOS) |
|-------------|---------|-----------------|
| CMake | 3.31+ | `brew install cmake` |
| Ninja | Any | `brew install ninja` |
| Clang/GCC | C++20 support | Xcode Command Line Tools |
| ccache (recommended) | Any | `brew install ccache` |

**macOS**: Install Xcode Command Line Tools: `xcode-select --install`

## CMake Presets (Recommended)

The project uses CMake presets for consistent builds across all developers.

| Preset | Description | Use Case |
|--------|-------------|----------|
| `dev` | Debug + tests | Daily development |
| `dev-release` | RelWithDebInfo + tests | Profiling |
| `release` | Optimized, no tests | Distribution |
| `ci` | Release + tests | CI/CD pipelines |
| `rtsan` | Debug + RealtimeSanitizer | Audio thread safety |
| `coverage` | Debug + coverage | Code coverage reports |

### Using Presets

```bash
# Configure (creates build/dev/ directory)
cmake --preset dev

# Build (auto-detects parallelism)
cmake --build --preset dev

# Run tests
ctest --preset dev

# Build specific target
cmake --build --preset dev --target MultiScoperTests
cmake --build --preset dev --target MultiScoper_VST3
```

### Plugin Output Locations (dev preset)

```text
build/dev/MultiScoper_artefacts/Debug/
├── VST3/MultiScoper.vst3
├── AU/MultiScoper.component
└── Standalone/MultiScoper.app
```

## CMake Options (Advanced)

Only use these when overriding presets:

| Option | Default | Description |
|--------|---------|-------------|
| `MULTISCOPER_BUILD_TESTS` | ON | Build GoogleTest unit tests |
| `MULTISCOPER_ENABLE_OPENGL` | ON | Enable OpenGL GPU rendering |
| `MULTISCOPER_ENABLE_RTSAN` | OFF | Enable RealtimeSanitizer (Clang 20+) |
| `MULTISCOPER_BUILD_TEST_HARNESS` | OFF | Build E2E test harness app |
| `MULTISCOPER_ENABLE_COVERAGE` | OFF | Enable code coverage |

## Build Without Presets (Legacy)

If CMake presets aren't available:

```bash
# Debug build
cmake -B build -DMULTISCOPER_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build -j

# Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build -j
```

## Running Tests

### Using CTest Presets (Recommended)

```bash
# Run all tests with preset
ctest --preset dev

# Verbose output
ctest --preset dev -V
```

### Direct Test Execution

```bash
# Run all tests
./build/dev/MultiScoperTests

# Run tests matching pattern
./build/dev/MultiScoperTests --gtest_filter="SignalProcessorTest.*"
./build/dev/MultiScoperTests --gtest_filter="ThemeManagerTest.*"
./build/dev/MultiScoperTests --gtest_filter="*Shader*"

# Run specific test
./build/dev/MultiScoperTests --gtest_filter="SignalProcessorTest.MonoSumming"

# Exclude tests
./build/dev/MultiScoperTests --gtest_filter="-*Slow*"

# XML output for CI
./build/dev/MultiScoperTests --gtest_output=xml:test-results/results.xml
```

## Plugin Installation

### macOS

After building, plugins are automatically copied to system plugin folders:

- **VST3**: `~/Library/Audio/Plug-Ins/VST3/MultiScoper.vst3`
- **AU**: `~/Library/Audio/Plug-Ins/Components/MultiScoper.component`

To manually install:

```bash
# VST3
cp -r build/MultiScoper_artefacts/Release/VST3/MultiScoper.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/MultiScoper_artefacts/Release/AU/MultiScoper.component ~/Library/Audio/Plug-Ins/Components/
```

### Rescan Plugins in DAW

After installing, rescan plugins in your DAW:
- **Logic Pro**: Preferences → Plug-in Manager → Reset & Rescan
- **Ableton**: Options → Preferences → Plug-Ins → Rescan
- **Reaper**: Options → Preferences → VST → Re-scan

## Running Standalone App

```bash
# macOS (using dev preset)
open build/dev/MultiScoper_artefacts/Debug/Standalone/MultiScoper.app

# Or directly
./build/dev/MultiScoper_artefacts/Debug/Standalone/MultiScoper.app/Contents/MacOS/MultiScoper
```

## E2E Test Harness

The test harness is a standalone app that simulates a DAW for automated testing.

### Build Test Harness

```bash
# Configure with test harness enabled
cmake --preset dev -DMULTISCOPER_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target MultiScoperTestHarness
```

### Run Test Harness

```bash
# Start test harness (HTTP server on port 8765)
"./build/dev/test_harness/MultiScoperTestHarness_artefacts/Debug/MultiScoper Test Harness.app/Contents/MacOS/MultiScoper Test Harness"

# Test that it's running
curl http://localhost:8765/health
```

See `docs/testing.md` for the full HTTP API reference.

## Development Workflow

### Incremental Builds

ccache significantly speeds up rebuilds (auto-configured by presets):

```bash
# Install ccache
brew install ccache ninja

# Just rebuild - ccache is already configured
cmake --build --preset dev
```

### Clean Build

```bash
# Remove preset build directory
rm -rf build/dev

# Or remove all builds
rm -rf build

# Reconfigure
cmake --preset dev
cmake --build --preset dev
```

### Rebuild After Header Changes

Ninja handles dependencies automatically:

```bash
# Just rebuild - Ninja detects header changes
cmake --build --preset dev
```

## Debugging

### Debug Build with Symbols

The `dev` preset includes debug symbols by default:

```bash
cmake --preset dev
cmake --build --preset dev
```

### Xcode Project Generation

```bash
cmake -B build-xcode -G Xcode -DMULTISCOPER_BUILD_TESTS=ON
open build-xcode/MultiScoper.xcodeproj
```

### LLDB Debugging

```bash
# Debug standalone app
lldb ./build/dev/MultiScoper_artefacts/Debug/Standalone/MultiScoper.app/Contents/MacOS/MultiScoper

# Debug tests
lldb ./build/dev/MultiScoperTests
```

## Troubleshooting

### CMake Configuration Fails

**Issue**: CMake can't find compiler
```bash
# Ensure Xcode CLI tools installed
xcode-select --install

# Check compiler
clang++ --version
```

**Issue**: FetchContent download fails
```bash
# Clear CMake cache and retry
rm -rf build
cmake -B build
```

### Build Fails

**Issue**: Missing JUCE modules
```bash
# JUCE is fetched automatically via FetchContent
# If issues, clear build and reconfigure
rm -rf build
cmake -B build -DMULTISCOPER_BUILD_TESTS=ON
```

**Issue**: Linker errors with OpenGL

```bash
# Ensure OpenGL is enabled (default)
cmake -B build -DMULTISCOPER_ENABLE_OPENGL=ON
```

### Tests Fail

**Issue**: Tests can't find resources
```bash
# Run from project root
cd /path/to/MultiScoper
./build/MultiScoperTests
```

**Issue**: OpenGL tests fail in headless environment

```bash
# Some shader tests require OpenGL context
# Run on machine with display or skip with:
./build/MultiScoperTests --gtest_filter="-*GL*:-*Shader*"
```

### Plugin Not Loading in DAW

**Issue**: AU validation fails
```bash
# Validate AU
auval -v aufx Osc1 Osci
```

**Issue**: Plugin crashes on load
```bash
# Run standalone to debug
./build/MultiScoper_artefacts/Debug/Standalone/MultiScoper.app/Contents/MacOS/MultiScoper
```

## CI/CD Commands

```bash
# Full CI build using preset
cmake --preset ci
cmake --build --preset ci
ctest --preset ci

# macOS universal binary CI build
cmake --preset ci-macos
cmake --build --preset ci-macos
ctest --preset ci-macos
```

## Common Mistakes

❌ **Don't**: Use raw cmake commands with `-D` flags
✅ **Do**: Use CMake presets (`cmake --preset dev`)

❌ **Don't**: Run cmake from wrong directory
✅ **Do**: Always run from project root

❌ **Don't**: Forget to add new .cpp files to CMakeLists.txt
✅ **Do**: Add every source file to `target_sources(MultiScoper PRIVATE ...)`

❌ **Don't**: Use Makefile generator
✅ **Do**: Use Ninja (configured by presets)

❌ **Don't**: Commit build directory
✅ **Do**: Ensure `build/` is in `.gitignore`
