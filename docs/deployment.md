# Local Development Guide for LLM Coding Agents

**Purpose**: Teach agents HOW to build, run, and debug the Oscil audio plugin locally.

**Tech Stack**: CMake 3.25+, Ninja, ccache, C++20, JUCE 8.0.12

---

## Prerequisites

### Required
- CMake 3.25+
- C++20 compiler (Clang 14+, GCC 11+, MSVC 2019+)
- macOS: Xcode Command Line Tools

### Recommended
- Ninja (faster builds)
- ccache (build caching)

### macOS Setup
```bash
# Install Xcode CLI tools
xcode-select --install

# Install build tools via Homebrew
brew install cmake ninja ccache
```

---

## Quick Start

```bash
# 1. Configure
cmake --preset dev

# 2. Build
cmake --build --preset dev

# 3. Test
ctest --preset dev
```

---

## Build Commands

### Development Build (Debug)
```bash
cmake --preset dev              # Configure
cmake --build --preset dev      # Build
```

### Release Build
```bash
cmake --preset release
cmake --build --preset release
```

### Clean Build
```bash
rm -rf build/dev
cmake --preset dev
cmake --build --preset dev
```

### Build Specific Target
```bash
# Plugin only (VST3, AU, Standalone)
cmake --build --preset dev --target Oscil_VST3
cmake --build --preset dev --target Oscil_AU
cmake --build --preset dev --target Oscil_Standalone

# Tests only
cmake --build --preset dev --target OscilTests

# Test Harness (E2E testing)
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
```

---

## Output Artifacts

After `dev` build:

| Artifact | Location |
|----------|----------|
| VST3 Plugin | `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3` |
| AU Plugin | `build/dev/Oscil_artefacts/Debug/AU/oscil4.component` |
| Standalone | `build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app` |
| Test Binary | `build/dev/OscilTests` |
| Test Harness | `build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app` |

### Install Locations (auto-copied)
- VST3: `~/Library/Audio/Plug-Ins/VST3/oscil4.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/oscil4.component`

---

## Running the Plugin

### Standalone
```bash
open build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app
```

### In DAW
After build, the plugin is auto-installed. Rescan plugins in your DAW:
- **Logic Pro**: Preferences → Plug-in Manager → Reset & Rescan
- **Ableton Live**: Preferences → Plug-Ins → Rescan
- **Reaper**: Options → Preferences → Plug-ins → Re-scan

---

## Running Tests

### All Tests
```bash
ctest --preset dev
```

### Specific Tests
```bash
# By name pattern
ctest --preset dev -R "Oscillator"

# With output on failure
ctest --preset dev --output-on-failure

# Verbose
ctest --preset dev -V
```

### Direct GoogleTest Execution
```bash
# Run all
./build/dev/OscilTests

# Filter tests
./build/dev/OscilTests --gtest_filter="OscillatorTest.*"

# List tests
./build/dev/OscilTests --gtest_list_tests
```

---

## Test Harness (E2E Testing)

### Build
```bash
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev --target OscilTestHarness
```

### Run
```bash
# Start harness (background)
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &

# Wait for startup
sleep 2

# Verify running
curl http://localhost:8765/health
```

### Open Editor (Required for UI tests)
```bash
curl -X POST http://localhost:8765/track/0/showEditor
```

---

## CMake Presets

| Preset | Description |
|--------|-------------|
| `dev` | Debug build with Ninja + ccache |
| `release` | Optimized release build |
| `xcode` | Xcode project generation |

### Custom Configuration
```bash
# Enable test harness
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON

# Disable OpenGL (software rendering)
cmake --preset dev -DOSCIL_ENABLE_OPENGL=OFF

# Disable tests
cmake --preset dev -DOSCIL_BUILD_TESTS=OFF
```

---

## Adding New Source Files

### Step 1: Create Files
```bash
# Header
touch include/core/MyFeature.h

# Implementation
touch src/core/MyFeature.cpp
```

### Step 2: Update Sources.cmake
```cmake
# In cmake/Sources.cmake, add to OSCIL_SOURCES:
${CMAKE_SOURCE_DIR}/src/core/MyFeature.cpp
```

### Step 3: Rebuild
```bash
cmake --build --preset dev
```

**Note**: Headers don't need explicit listing (discovered via includes).

---

## Debugging

### macOS (lldb)
```bash
# Debug standalone
lldb build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app

# In lldb
(lldb) run
(lldb) bt  # Backtrace on crash
```

### VS Code
1. Install C/C++ extension
2. Use generated `compile_commands.json` in `build/dev/`
3. Configure `.vscode/launch.json`:
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Standalone",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app/Contents/MacOS/oscil4",
      "cwd": "${workspaceFolder}"
    }
  ]
}
```

### Debug Tests
```bash
lldb ./build/dev/OscilTests
(lldb) run --gtest_filter="OscillatorTest.IdGeneration"
```

---

## Troubleshooting

### Build Fails: "file not found"
**Cause**: New file not in Sources.cmake
**Fix**: Add file path to `cmake/Sources.cmake`

### Build Fails: ccache errors
**Fix**: Clear ccache
```bash
ccache -C
cmake --build --preset dev
```

### Plugin Not Appearing in DAW
**Fix**: Check install location and rescan
```bash
ls ~/Library/Audio/Plug-Ins/VST3/oscil4.vst3
ls ~/Library/Audio/Plug-Ins/Components/oscil4.component
```

### Tests Fail Immediately
**Cause**: JUCE not initialized
**Fix**: Ensure `JuceTestEnvironment` is in test main

### OpenGL Errors
**Fix**: Run on machine with GPU, or disable OpenGL
```bash
cmake --preset dev -DOSCIL_ENABLE_OPENGL=OFF
```

---

## Quick Reference

| Task | Command |
|------|---------|
| Configure | `cmake --preset dev` |
| Build | `cmake --build --preset dev` |
| Test | `ctest --preset dev` |
| Clean | `rm -rf build/dev && cmake --preset dev` |
| Run Standalone | `open build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app` |
| Build Test Harness | `cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON && cmake --build --preset dev --target OscilTestHarness` |

---

## Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `CCACHE_DIR` | ccache directory | `~/.ccache` |
| `CMAKE_BUILD_PARALLEL_LEVEL` | Parallel jobs | `8` |

```bash
# Speed up builds
export CMAKE_BUILD_PARALLEL_LEVEL=8
cmake --build --preset dev
```
