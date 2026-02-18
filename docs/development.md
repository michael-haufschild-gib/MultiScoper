# Development Guide for LLM Coding Agents

**Purpose**: Instructions for setup, building, and running the Oscil plugin.
**Build System**: CMake 3.21+ with presets | Ninja + ccache

---

## Quick Start

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

---

## Key Commands

| Task | Command |
|------|---------|
| Configure | `cmake --preset dev` |
| Build | `cmake --build --preset dev` |
| Run tests | `ctest --preset dev` |
| Run specific test | `ctest --preset dev -R "Pattern"` |
| Clean rebuild | `rm -rf build/dev && cmake --preset dev && cmake --build --preset dev` |
| Release build | `cmake --preset release && cmake --build --preset release` |
| List presets | `cmake --list-presets` |

---

## Available Presets

| Preset | Use Case |
|--------|----------|
| `dev` | Daily development (Debug + tests + inspector) |
| `dev-release` | Profiling with debug info |
| `release` | Production builds |
| `ci` | CI builds |
| `rtsan` | Audio thread safety testing (Clang 20+) |

---

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `OSCIL_BUILD_TESTS` | ON | Build unit tests |
| `OSCIL_BUILD_TEST_HARNESS` | OFF | Build E2E test harness |
| `OSCIL_ENABLE_OPENGL` | ON | Enable GPU rendering |
| `OSCIL_ENABLE_INSPECTOR` | OFF | Melatonin Inspector (dev only) |
| `OSCIL_ENABLE_RTSAN` | OFF | RealtimeSanitizer |

---

## Build Output Locations

| Output | Path (after `dev` build) |
|--------|--------------------------|
| VST3 | `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3` |
| AU | `build/dev/Oscil_artefacts/Debug/AU/oscil4.component` |
| Standalone | `build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app` |
| Tests | `build/dev/OscilTests` |
| Test Harness | `build/dev/test_harness/OscilTestHarness_artefacts/Debug/...` |

---

## Development Workflow

### Standard Workflow
```bash
# 1. Configure (once)
cmake --preset dev

# 2. Edit source files

# 3. Rebuild (fast with ccache)
cmake --build --preset dev

# 4. Run tests
ctest --preset dev
```

### Adding New Files

1. Create header: `include/[domain]/ClassName.h`
2. Create source: `src/[domain]/ClassName.cpp`
3. Add to `cmake/Sources.cmake`:
```cmake
set(OSCIL_SOURCES
    # ... existing ...
    ${CMAKE_SOURCE_DIR}/src/[domain]/ClassName.cpp
)
```
4. Rebuild: `cmake --build --preset dev`

---

## E2E Testing Workflow

```bash
# 1. Build with test harness
cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
cmake --build --preset dev

# 2. Start test harness
"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &

# 3. Run E2E tests
cd tests/e2e
python -m pytest -v
```

---

## Debugging

### Standalone App
Run directly or attach debugger to `oscil4.app`

### Plugin in DAW
1. Build Debug: `cmake --preset dev`
2. Launch DAW
3. Attach debugger to DAW process
4. Load plugin

### UI Inspector (dev preset only)
Press `Cmd+I` (macOS) or `Ctrl+I` (Windows/Linux) to toggle Melatonin Inspector

### Test Debugging
```bash
./build/dev/OscilTests --gtest_filter="TestClass.*" --gtest_break_on_failure
```

---

## Dependencies

All fetched automatically via CMake FetchContent:

| Dependency | Purpose |
|------------|---------|
| JUCE 8.0.5 | Audio plugin framework |
| GoogleTest | Unit testing |
| cpp-httplib | E2E test harness HTTP server |
| nlohmann/json | JSON for test harness API |
| Melatonin Inspector | UI debugging |

First build downloads dependencies (requires internet).

---

## Troubleshooting

### ccache not found
```bash
brew install ccache && cmake --preset dev
```

### Build fails
```bash
# Clean and rebuild
rm -rf build/dev && cmake --preset dev && cmake --build --preset dev
```

### Plugin not loading in DAW
```bash
# macOS AU validation
auval -v aufx Osc1 Osci
```

### Tests crash on JUCE init
Add required JUCE module to `target_link_libraries(OscilTests ...)` in CMakeLists.txt

### Compiler version issues
C++20 requires: Clang 14+, GCC 11+, MSVC 2019+
```bash
clang++ --version
```

---

## Common Mistakes

**Don't**: Forget to add new files to `cmake/Sources.cmake`
**Do**: Add source files immediately after creation

**Don't**: Build without configuring first
```bash
cmake --build --preset dev  # Fails if not configured
```
**Do**: Configure then build
```bash
cmake --preset dev && cmake --build --preset dev
```

**Don't**: Commit build artifacts
**Do**: Build directory is in `.gitignore`

**Don't**: Mix presets
```bash
cmake --preset dev
cmake --build --preset release  # Wrong! Uses different preset
```
**Do**: Use matching presets
```bash
cmake --preset dev
cmake --build --preset dev
```
