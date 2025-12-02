# Building Guide for LLM Coding Agents

**Purpose**: Instructions for building, running, and developing the Oscil plugin locally.
**Tech Stack**: CMake 3.21+, C++20, JUCE 8.0.5, GoogleTest 1.17.0, Ninja

## Quick Reference (Using CMake Presets)

```bash
# Development build (Debug + tests + Melatonin Inspector)
cmake --preset dev
cmake --build --preset dev

# Run tests
ctest --preset dev

# Release build
cmake --preset release
cmake --build --preset release

# Test harness for E2E testing
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
```

**Preferred**: Always use CMake presets for consistent builds.

## Prerequisites

| Requirement | Version | Install (macOS) |
|-------------|---------|-----------------|
| CMake | 3.21+ | `brew install cmake` |
| Ninja | Any | `brew install ninja` |
| Clang/GCC | C++20 support | Xcode Command Line Tools |
| ccache (recommended) | Any | `brew install ccache` |

**macOS**: Install Xcode Command Line Tools: `xcode-select --install`

## CMake Presets (Recommended)

The project uses CMake presets for consistent builds across all developers.

| Preset | Description | Use Case |
|--------|-------------|----------|
| `dev` | Debug + tests + Inspector | Daily development |
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
cmake --build --preset dev --target OscilTests
cmake --build --preset dev --target Oscil_VST3
```

### Plugin Output Locations (dev preset)

```
build/dev/Oscil_artefacts/Debug/
├── VST3/oscil4.vst3
├── AU/oscil4.component
└── Standalone/oscil4.app
```

## CMake Options (Advanced)

Only use these when overriding presets:

| Option | Default | Description |
|--------|---------|-------------|
| `OSCIL_BUILD_TESTS` | ON | Build GoogleTest unit tests |
| `OSCIL_ENABLE_OPENGL` | ON | Enable OpenGL GPU rendering |
| `OSCIL_ENABLE_INSPECTOR` | OFF | Enable Melatonin Inspector (dev only) |
| `OSCIL_ENABLE_RTSAN` | OFF | Enable RealtimeSanitizer (Clang 20+) |
| `OSCIL_BUILD_TEST_HARNESS` | OFF | Build E2E test harness app |
| `OSCIL_ENABLE_COVERAGE` | OFF | Enable code coverage |

## Build Without Presets (Legacy)

If CMake presets aren't available:

```bash
# Debug build
cmake -B build -DOSCIL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -G Ninja
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
./build/dev/OscilTests

# Run tests matching pattern
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.*"
./build/dev/OscilTests --gtest_filter="ThemeManagerTest.*"
./build/dev/OscilTests --gtest_filter="*Shader*"

# Run specific test
./build/dev/OscilTests --gtest_filter="SignalProcessorTest.MonoSumming"

# Exclude tests
./build/dev/OscilTests --gtest_filter="-*Slow*"

# XML output for CI
./build/dev/OscilTests --gtest_output=xml:test-results/results.xml
```

## Plugin Installation

### macOS

After building, plugins are automatically copied to system plugin folders:

- **VST3**: `~/Library/Audio/Plug-Ins/VST3/oscil4.vst3`
- **AU**: `~/Library/Audio/Plug-Ins/Components/oscil4.component`

To manually install:

```bash
# VST3
cp -r build/Oscil_artefacts/Release/VST3/oscil4.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/Oscil_artefacts/Release/AU/oscil4.component ~/Library/Audio/Plug-Ins/Components/
```

### Rescan Plugins in DAW

After installing, rescan plugins in your DAW:
- **Logic Pro**: Preferences → Plug-in Manager → Reset & Rescan
- **Ableton**: Options → Preferences → Plug-Ins → Rescan
- **Reaper**: Options → Preferences → VST → Re-scan

## Running Standalone App

```bash
# macOS (using dev preset)
open build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app

# Or directly
./build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4
```

## E2E Test Harness

The test harness is a standalone app that simulates a DAW for automated testing.

### Build Test Harness

```bash
# Configure with test harness enabled
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
```

### Run Test Harness

```bash
# Start test harness (HTTP server on port 8765)
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness"

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
cmake -B build-xcode -G Xcode -DOSCIL_BUILD_TESTS=ON
open build-xcode/Oscil.xcodeproj
```

### LLDB Debugging

```bash
# Debug standalone app
lldb ./build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4

# Debug tests
lldb ./build/dev/OscilTests
```

### Melatonin Inspector (UI Debugging)

The `dev` preset includes Melatonin Inspector for browser-like dev tools:

```bash
# Already enabled in dev preset
cmake --preset dev
cmake --build --preset dev

# Toggle inspector: Cmd+I (macOS) / Ctrl+I (Windows/Linux)
```

**Note**: Inspector creates a separate window. Disabled in CI builds.

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
cmake -B build -DOSCIL_BUILD_TESTS=ON
```

**Issue**: Linker errors with OpenGL
```bash
# Ensure OpenGL is enabled (default)
cmake -B build -DOSCIL_ENABLE_OPENGL=ON
```

### Tests Fail

**Issue**: Tests can't find resources
```bash
# Run from project root
cd /path/to/MultiScoper
./build/OscilTests
```

**Issue**: OpenGL tests fail in headless environment
```bash
# Some shader tests require OpenGL context
# Run on machine with display or skip with:
./build/OscilTests --gtest_filter="-*GL*:-*Shader*"
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
./build/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4
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
✅ **Do**: Add every source file to `target_sources(Oscil PRIVATE ...)`

❌ **Don't**: Use Makefile generator
✅ **Do**: Use Ninja (configured by presets)

❌ **Don't**: Commit build directory
✅ **Do**: Ensure `build/` is in `.gitignore`
