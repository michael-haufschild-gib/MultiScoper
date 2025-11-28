# Copilot Coding Agent Instructions

This file provides context for GitHub Copilot coding agent when working on this repository.

## Project Overview

**Oscil** is a multi-track oscilloscope VST3/AU audio plugin built with:
- **Framework**: JUCE 8.0.5
- **Language**: C++20
- **Build System**: CMake 3.21+ with CMakePresets.json
- **Testing**: GoogleTest 1.17.0
- **CI/CD**: GitHub Actions (macOS, Linux, Windows)

All code lives in the `oscil` namespace.

## Build Commands

```bash
# Configure (use presets)
cmake --preset dev          # Development build
cmake --preset ci-macos     # CI macOS build
cmake --preset ci-linux     # CI Linux build
cmake --preset ci-windows   # CI Windows build

# Build
cmake --build --preset <preset-name>

# Test
ctest --preset <preset-name>

# Run specific test (use ctest for cross-platform compatibility)
ctest --preset <preset-name> -R "TestName"
```

## Project Structure

```
oscil4/
├── CMakeLists.txt          # Main build configuration
├── CMakePresets.json       # Build presets
├── include/                # Header files (.h)
│   ├── core/               # Core logic: processors, state, data models
│   ├── dsp/                # Digital signal processing algorithms
│   └── ui/                 # UI components (juce::Component subclasses)
│       ├── components/     # Reusable UI component library
│       ├── sections/       # Sidebar collapsible sections
│       └── coordinators/   # State-UI coordination logic
├── src/                    # Implementation files (.cpp)
│   ├── core/               # Core implementations
│   ├── dsp/                # DSP implementations
│   └── ui/                 # UI implementations
├── tests/                  # GoogleTest unit tests
├── test_harness/           # E2E test harness (optional)
└── docs/                   # Documentation
```

## Key Documentation Files

For detailed guidance, refer to:
- `docs/architecture.md` - Code organization, patterns, and class creation
- `docs/development.md` - Build setup, IDE configuration, debugging
- `docs/testing.md` - Test writing patterns and E2E test harness

## Common CI Failure Patterns

### 1. CMake Configuration Errors
- Check `CMakePresets.json` for missing/incorrect presets
- Verify platform conditions in presets
- Ensure `testPresets` match `configurePresets`

### 2. Dependency Issues
- Dependencies are fetched via `FetchContent` in CMakeLists.txt
- Current versions: JUCE 8.0.5, GoogleTest 1.17.0, cpp-httplib 0.27.0, nlohmann/json 3.12.0

### 3. Platform-Specific Failures
- **macOS**: Universal binary (arm64;x86_64), requires Xcode tools
- **Linux**: Requires JUCE dependencies (libasound2-dev, libfreetype6-dev, etc.)
- **Windows**: Uses Ninja generator, requires Visual Studio tools

### 4. Test Failures
- Tests are in `tests/` directory
- Run specific test: `./build/<preset>/OscilTests --gtest_filter="TestName.*"`
- Check for thread safety issues (audio code uses lock-free patterns)

### 5. pluginval Failures
- Plugin validation runs after build
- Check VST3 plugin at `build/<preset>/Oscil_artefacts/Release/VST3/oscil4.vst3`

## Code Style Guidelines

- Use C++20 features appropriately
- Follow JUCE coding conventions
- Keep DSP code lock-free and real-time safe
- Use `jassert` for debug assertions
- Prefer RAII for resource management
- All code must be in the `oscil` namespace
- Classes: `PascalCase` (e.g., `WaveformComponent`)
- Member variables: `camelCase_` with trailing underscore
- Methods: `camelCase` (e.g., `getCaptureBuffer()`)
- Headers in `include/`, implementations in `src/`

## Thread Safety Rules

**Audio Thread** (in `processBlock()`):
- Use ONLY lock-free operations
- NO allocations after `prepareToPlay()`
- NO mutex locks - use atomics and ring buffers

**UI Thread**:
- All JUCE Component methods are UI-thread only
- Use message queue for cross-thread communication

## Git Workflow

- Create feature branches from `main` or `develop`
- Write clear, descriptive commit messages
- Keep commits focused on single changes
- Run tests before pushing

## Boundaries - What NOT to Modify

Do NOT modify these without explicit instruction:
- `.github/workflows/` - CI/CD configuration
- Third-party dependencies or their versions in `CMakeLists.txt`
- JUCE module configurations
- Build presets in `CMakePresets.json` (unless fixing CI)

## When Fixing Issues

1. **Minimal changes**: Fix only what's broken
2. **Test locally**: Ensure tests pass with `ctest --preset dev` (macOS) or `ctest --preset ci-linux` (Linux)
3. **Cross-platform**: Consider all three platforms
4. **No regressions**: Don't break existing functionality
5. **Update docs**: If changing build/dependency versions, update `docs/development.md`

## Files to Check First

| Issue Type | Check These Files |
|------------|-------------------|
| Build failure | `CMakeLists.txt`, `CMakePresets.json` |
| CI failure | `.github/workflows/build_and_test.yml` |
| Test failure | `tests/test_*.cpp` |
| Dependency issue | `CMakeLists.txt` (FetchContent section) |
| Architecture questions | `docs/architecture.md` |
| Build/debug questions | `docs/development.md` |
| Testing questions | `docs/testing.md` |

## Adding New Code

When adding new source files:
1. Create header in `include/[domain]/ClassName.h`
2. Create implementation in `src/[domain]/ClassName.cpp`
3. Add `.cpp` file to `CMakeLists.txt` under `target_sources(Oscil PRIVATE ...)`
4. Rebuild: `cmake --build --preset dev`

When adding tests:
1. Create test file: `tests/test_class_name.cpp`
2. Add the test file to `CMakeLists.txt` under `add_executable(OscilTests ...)`
3. If testing new source code, also add that source file (e.g., `src/core/MyClass.cpp`) to OscilTests
4. Run: `ctest --preset dev`
