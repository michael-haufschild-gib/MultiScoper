# Copilot Coding Agent Instructions

This file provides context for GitHub Copilot coding agent when working on this repository.

## Project Overview

**Oscil** is a multi-track oscilloscope VST3/AU audio plugin built with:
- **Framework**: JUCE 8.0.5
- **Language**: C++20
- **Build System**: CMake 3.21+ with CMakePresets.json
- **Testing**: GoogleTest 1.17.0
- **CI/CD**: GitHub Actions (macOS, Linux, Windows)

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
```

## Project Structure

```
oscil4/
├── CMakeLists.txt          # Main build configuration
├── CMakePresets.json       # Build presets
├── include/                # Header files
│   ├── core/               # Core logic
│   ├── dsp/                # DSP processing
│   └── ui/                 # UI components
├── src/                    # Implementation files
├── tests/                  # GoogleTest unit tests
└── docs/                   # Documentation
```

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

## When Fixing Issues

1. **Minimal changes**: Fix only what's broken
2. **Test locally**: Ensure `ctest --preset dev` passes
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
