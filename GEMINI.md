=== CRITICAL GLOBAL INSTRUCTION BLOCK (CGIB-001): MANDATORY TOOLS ===

## MANDATORY TOOLS

### For Coding, Research, Analysis, Debugging
```
USE: mcp__mcp_docker__sequentialthinking
WHEN: Coding tasks, research, complex reasoning
WHY: Prevents cognitive overload, ensures systematic approach
```

### For Task Management
```
USE: write_todo
WHEN: Coding tasks, any task with 2+ steps
WHY: Tracks progress, maintains focus
```

### For Codebase Investigation
```
USE: codebase_investigator
WHEN: Starting any new task, refactoring, or bug fixing
WHY: Builds a comprehensive understanding of the code, structure, and dependencies before changes
```

=== END CRITICAL GLOBAL INSTRUCTION BLOCK (CGIB-001)===

# Oscil Multi-Track Oscilloscope Plugin

**Project**: Oscil Multi-Track Oscilloscope
**Type**: Professional Audio Plugin (VST3, AU, CLAP, Standalone)
**Language**: C++20
**Framework**: JUCE 8.0.5
**Build System**: CMake 3.21+ (with Ninja & ccache support)

## Project Overview

Oscil is a professional audio visualization platform designed for engineers and producers to analyze multi-track audio signals. It offers real-time oscilloscope visualization with cross-DAW compatibility, allowing multiple plugin instances to communicate and display signals on a single aggregator interface.

**Key Features:**
- Real-time visualization of unlimited simultaneous audio sources.
- Per-oscillator signal processing (Stereo, Mono, Mid, Side, L/R).
- Cross-instance source discovery and deduplication.
- Sample-accurate timing synchronization.
- OpenGL-accelerated rendering.

## Directory Structure

- **`src/`**: Implementation files (`.cpp`).
    - **`core/`**: Core logic (PluginProcessor, Oscillator, InstanceRegistry).
    - **`dsp/`**: Signal processing and timing engine.
    - **`ui/`**: User interface components (PluginEditor, WaveformComponent).
    - **`rendering/`**: Advanced rendering engine for GPU rendering with shaders, particle effects and postprocessing effects
- **`include/`**: Header files (`.h`), mirroring the `src` structure.
- **`tests/`**: Unit tests using GoogleTest.
- **`test_harness/`**: E2E test harness application.
- **`docs/`**: Documentation (PRD, Architecture, Development Guide).
- **`cmake-build-test/`** & **`build/`**: Build artifacts (do not edit).
- **`CMakeLists.txt`**: Main build configuration.
- **`CLAUDE.md`**: Original agent context and instructions.

## Development Workflow

**IMPORTANT**: Follow the **Investigation-First Workflow**. Never change code based on assumptions.

1.  **Analyze**: Understand the request using `mcp__mcp_docker__sequentialthinking`.
2.  **Investigate**: Use `codebase_investigator` to map relevant files and dependencies.
3.  **Plan**: Break down the task using `write_todo`.
4.  **Implement**: Write code and corresponding tests.
5.  **Verify**: Run tests to ensure correctness (`ctest`).

## Build and Run

The project uses CMake Presets for configuration.

**Prerequisites:**
- CMake 3.21+
- Ninja & ccache (Recommended)
- C++20 compatible compiler (Clang 14+, GCC 11+, MSVC 2019+)

**Standard Development Commands:**

| Action | Command | Description |
| :--- | :--- | :--- |
| **Configure** | `cmake --preset dev` | Configures for Debug build with Ninja & ccache. |
| **Build** | `cmake --build --preset dev` | Compiles the project. |
| **Run Tests** | `ctest --preset dev` | Runs all unit tests. |
| **Build Release** | `cmake --preset release && cmake --build --preset release` | Builds optimized release binaries. |

**Artifact Locations (after `dev` build):**
- **VST3**: `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3`
- **AU**: `build/dev/Oscil_artefacts/Debug/AU/oscil4.component`
- **Standalone**: `build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app`
- **Test Binary**: `build/dev/OscilTests`

## Testing

- **Unit Tests**: Located in `tests/`. Built automatically with `OSCIL_BUILD_TESTS=ON` (default in `dev` preset).
- **E2E Tests**: Located in `test_harness/`.
- **Requirement**: All new features and bug fixes must be accompanied by meaningful tests. Run tests after every change to ensure no regressions.

### E2E Testing

The project includes a standalone `OscilTestHarness` that hosts the plugin and exposes an HTTP API for automation.

1.  **Build Harness**:
    ```bash
    cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON
    cmake --build --preset dev --target OscilTestHarness
    ```

2.  **Start Harness**:
    ```bash
    "./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &
    ```

3.  **Run Python Tests**:
    Interact with `http://localhost:8765`. Ensure you **open the editor** first.
    ```python
    import requests, time
    URL = "http://localhost:8765"

    # 1. Open Editor (Critical!)
    requests.post(f"{URL}/track/0/showEditor")
    time.sleep(1) # Wait for UI

    # 2. Inspect Element
    info = requests.get(f"{URL}/ui/element/my_test_id").json()

    # 3. Interact
    requests.post(f"{URL}/ui/click", json={"elementId": "my_button_id"})
    ```

## Coding Conventions

- **Standard**: C++20.
- **Style**: Follow existing JUCE-based patterns. Juce 8
- **Shaders**: GSLS 3.30
- **Headers**: Use `#pragma once`.
- **Memory**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) where possible; avoid raw pointers for ownership.
- **JUCE**: Use `juce::` namespace explicitly or strictly follow file-level `using`.
