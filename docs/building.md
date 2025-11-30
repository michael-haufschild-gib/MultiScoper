# Building Guide for LLM Coding Agents

**Purpose**: Instructions for building, running, and developing the Oscil plugin locally.
**Tech Stack**: CMake 3.20+, C++20, JUCE 8.0.5, GoogleTest 1.17.0

## Quick Reference

```bash
# Build plugin (VST3, AU, Standalone)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Oscil_All -j$(nproc)

# Build and run tests
cmake -B build -DOSCIL_BUILD_TESTS=ON
cmake --build build --target OscilTests -j$(nproc)
./build/OscilTests

# Build everything (plugin + tests)
cmake -B build -DOSCIL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

## Prerequisites

| Requirement | Version | Install (macOS) |
|-------------|---------|-----------------|
| CMake | 3.20+ | `brew install cmake` |
| Clang/GCC | C++20 support | Xcode Command Line Tools |
| ccache (optional) | Any | `brew install ccache` |

**macOS**: Install Xcode Command Line Tools: `xcode-select --install`

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `OSCIL_BUILD_TESTS` | ON | Build GoogleTest unit tests |
| `OSCIL_ENABLE_OPENGL` | ON | Enable OpenGL GPU rendering |
| `OSCIL_ENABLE_INSPECTOR` | OFF | Enable Melatonin Inspector (dev only) |
| `OSCIL_ENABLE_RTSAN` | OFF | Enable RealtimeSanitizer (Clang 20+) |
| `OSCIL_BUILD_TEST_HARNESS` | OFF | Build E2E test harness app |
| `CMAKE_BUILD_TYPE` | - | Debug, Release, RelWithDebInfo |

## Build Commands

### Debug Build (Development)

```bash
# Configure with tests and debug symbols
cmake -B build -DOSCIL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build everything
cmake --build build -j$(nproc)

# Plugin outputs:
# - build/Oscil_artefacts/Debug/VST3/oscil4.vst3
# - build/Oscil_artefacts/Debug/AU/oscil4.component
# - build/Oscil_artefacts/Debug/Standalone/oscil4.app
```

### Release Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Oscil_All -j$(nproc)

# Plugin outputs:
# - build/Oscil_artefacts/Release/VST3/oscil4.vst3
# - build/Oscil_artefacts/Release/AU/oscil4.component
```

### Build Specific Target

```bash
# Just the plugin
cmake --build build --target Oscil_All

# Just tests
cmake --build build --target OscilTests

# Just VST3
cmake --build build --target Oscil_VST3

# Just AU
cmake --build build --target Oscil_AU

# Just Standalone
cmake --build build --target Oscil_Standalone
```

## Running Tests

### All Tests

```bash
./build/OscilTests
```

### Specific Test Suite

```bash
# Run tests matching pattern
./build/OscilTests --gtest_filter="SignalProcessorTest.*"
./build/OscilTests --gtest_filter="ThemeManagerTest.*"
./build/OscilTests --gtest_filter="*Shader*"

# Run specific test
./build/OscilTests --gtest_filter="SignalProcessorTest.MonoSumming"

# Exclude tests
./build/OscilTests --gtest_filter="-*Slow*"
```

### Test Output Formats

```bash
# XML output for CI
./build/OscilTests --gtest_output=xml:test-results/results.xml

# Verbose output
./build/OscilTests --gtest_print_time=1

# Run via CTest
cd build && ctest --output-on-failure
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
# macOS
open build/Oscil_artefacts/Debug/Standalone/oscil4.app

# Or directly
./build/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4
```

## E2E Test Harness

The test harness is a standalone app that simulates a DAW for automated testing.

### Build Test Harness

```bash
cmake -B build -DOSCIL_BUILD_TEST_HARNESS=ON -DOSCIL_BUILD_TESTS=ON
cmake --build build --target OscilTestHarness
```

### Run Test Harness

```bash
# Start test harness (HTTP server on port 8765)
./build/OscilTestHarness_artefacts/OscilTestHarness.app/Contents/MacOS/OscilTestHarness

# Test that it's running
curl http://localhost:8765/health
```

See `docs/testing.md` for the full HTTP API reference.

## Development Workflow

### Incremental Builds

ccache significantly speeds up rebuilds:

```bash
# Install ccache
brew install ccache

# CMake auto-detects ccache - just rebuild
cmake --build build -j$(nproc)
```

### Clean Build

```bash
# Remove build directory
rm -rf build

# Or clean specific targets
cmake --build build --target clean
```

### Rebuild After Header Changes

Header changes require rebuilding dependent files:

```bash
# Touch the header to force rebuild
touch include/core/PluginProcessor.h
cmake --build build -j$(nproc)
```

## Debugging

### Debug Build with Symbols

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Xcode Project Generation

```bash
cmake -B build-xcode -G Xcode
open build-xcode/Oscil.xcodeproj
```

### LLDB Debugging

```bash
# Debug standalone app
lldb ./build/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4

# Debug tests
lldb ./build/OscilTests
```

### Melatonin Inspector (UI Debugging)

Enable browser-like dev tools for JUCE components:

```bash
cmake -B build -DOSCIL_ENABLE_INSPECTOR=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Note**: Inspector creates a separate window that can timeout in headless environments. Disabled by default for CI.

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
# Full CI build
cmake -B build -DOSCIL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/OscilTests --gtest_output=xml:test-results/results.xml
```

## Common Mistakes

**Don't**: Run cmake from wrong directory
**Do**: Always run from project root or specify paths

**Don't**: Forget to add new .cpp files to CMakeLists.txt
**Do**: Add every source file to `target_sources(Oscil PRIVATE ...)`

**Don't**: Build without tests during development
**Do**: Always use `-DOSCIL_BUILD_TESTS=ON` for development

**Don't**: Commit build directory
**Do**: Ensure `build/` is in `.gitignore`
