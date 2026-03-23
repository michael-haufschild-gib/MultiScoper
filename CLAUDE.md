=== CRITICAL INSTRUCTION BLOCK (CIB-001): MANDATORY TOOLS ===

## REQUIRED TOOLS

| Tool | When to Use |
|------|-------------|
| `mcp__mcp_docker__sequentialthinking` | Coding, research, debugging, any complex reasoning |
| `TodoWrite` | Any task with 2+ steps (track progress) |
| `Task` (subagent) | See CIB-001 delegation decision tree |

## TESTING REQUIREMENTS

- Tests must verify actual functionality, not just that code renders
- Run tests after each fix: `ctest --preset dev`
- Iterate until green - no partial fixes

=== END CIB-001 ===

=== CRITICAL INSTRUCTION BLOCK (CIB-002): SUBAGENT DELEGATION ===

## DELEGATION-FIRST PRINCIPLE

You have specialized subagents. Use them. Doing complex work yourself when a subagent exists is inefficient.

### MANDATORY: Check Delegation Before Every Task

Before starting ANY task, evaluate:

```
DELEGATE IF ANY IS TRUE:
├─ Task matches a subagent specialty? → DELEGATE
├─ Task has 3+ steps? → DELEGATE
├─ Task involves searching/exploring codebase? → DELEGATE (use Explore agent)
├─ Task can run while you do other work? → DELEGATE
├─ Task involves code review? → DELEGATE (use code-reviewer agent)
├─ Task involves debugging? → DELEGATE (use debugger agent)
├─ Task involves writing tests? → DELEGATE (use qa-engineer agent)
├─ Task involves DSP/audio code? → DELEGATE (use dsp-engineer agent)
├─ Task involves UI components? → DELEGATE (use ui-designer agent)
├─ Task involves OpenGL/rendering? → DELEGATE (use render-specialist agent)
├─ Task involves architecture decisions? → DELEGATE (use architecture-guardian agent)
└─ Task involves documentation? → DELEGATE (use technical-writer agent)
```

### HOW TO DELEGATE

1. **Select agent**: Use your `agent-selection` skill (read `.claude/skills/agent-selection/`)
2. **Write a complete prompt** that includes:
   - Full context of what you've already learned
   - Specific files and line numbers relevant to the task
   - References to docs: `docs/architecture.md`, `docs/testing.md`, `docs/prd.md`
   - Clear success criteria
3. **Never send one-liners** - Agents start fresh with no memory of your conversation

### QUALITY GATE: Before NOT Delegating

If you decide to do a task yourself, verify:
- [ ] No subagent specialty matches this task
- [ ] Task is truly simple (< 3 steps)
- [ ] Task cannot run in parallel with other work

If you cannot check all boxes, DELEGATE.

=== END CIB-002 ===

=== CRITICAL INSTRUCTION BLOCK (CIB-003): INVESTIGATION-FIRST DEVELOPMENT ===

## FORBIDDEN

- Changing code based on assumptions
- Changing code without investigating side effects
- Fixing tests without running actual features
- Declaring victory without verification
- Removing code without understanding why it exists

## REQUIRED

- Trace code execution path before changes
- Understand purpose of code you're changing
- Analyze impact on whole codebase
- Verify fixes work

## WORKFLOW

```
1. Analyze request → use sequentialthinking tool
2. Investigate context → delegate to Explore agent if needed
3. Investigate impact → what else does this affect?
4. Plan → create tasks with TodoWrite
5. Implement → one task at a time
6. Test → run tests, fix failures
7. Verify → confirm it works
8. Summarize → one short paragraph in chat
```

=== END CIB-003 ===

## Project Overview

**Project**: Oscil Multi-Track Oscilloscope
**Type**: Professional Audio Plugin (VST3, AU, CLAP, Standalone)
**Language**: C++20
**Framework**: JUCE 8.0.12
**Build System**: CMake 3.25+ (with Ninja & ccache support)

Oscil is a professional audio visualization platform designed for engineers and producers to analyze multi-track audio signals. It offers real-time oscilloscope visualization with cross-DAW compatibility, allowing multiple plugin instances to communicate and display signals on a single aggregator interface.

**Key Features:**
- Real-time visualization of unlimited simultaneous audio sources.
- Per-oscillator signal processing (Stereo, Mono, Mid, Side, L/R).
- Cross-instance source discovery and deduplication.
- Sample-accurate timing synchronization.
- OpenGL-accelerated rendering.

## Directory Structure

```
src/                        # Implementation files (.cpp, .mm)
├── core/                   # Business logic (Oscillator, OscilState, InstanceRegistry, Source)
│   └── dsp/                # Signal processing (SignalProcessor, TimingEngine)
├── platform/macos/         # Platform-specific implementations
├── plugin/                 # Plugin entry points (PluginProcessor, PluginEditor)
├── rendering/              # OpenGL visualization engine
│   ├── effects/            # Post-processing (Bloom, Glitch, Scanline, etc.)
│   ├── materials/          # Material shaders (Glass, Chrome, Crystalline)
│   ├── particles/          # Particle system
│   ├── shaders/            # 2D waveform shaders
│   └── shaders3d/          # 3D visualization shaders
├── tools/                  # Test infrastructure (PluginTestServer)
│   └── test_server/
└── ui/                     # User interface
    ├── components/         # Reusable widgets (OscilButton, OscilSlider, etc.)
    ├── dialogs/            # Modal dialogs (AddOscillator, ColorPicker)
    ├── layout/             # Layout management (Sidebar, Pane, Coordinators)
    │   └── sections/       # Accordion sections
    ├── panels/             # Content panels (OscillatorList, Waveform, Config)
    └── theme/              # Theming (ThemeManager, ColorPicker)

include/                    # Header files (.h), mirrors src/ structure
├── Oscil.h                 # Main convenience header
├── core/                   # Core headers
│   ├── dsp/                # DSP headers (TimingConfig, TimingEngine)
│   └── interfaces/         # Abstract interfaces (IInstanceRegistry, IAudioBuffer)
├── plugin/                 # Plugin headers
├── rendering/              # Rendering headers (same subdirs as src/)
├── tools/                  # Test infrastructure headers
└── ui/                     # UI headers (same subdirs as src/)

tests/                      # Unit tests (GoogleTest)
test_harness/               # E2E test harness application
docs/                       # Documentation
build/                      # Build artifacts (do not edit)
cmake/                      # CMake modules (Sources.cmake)
```

## Build and Run

The project uses CMake Presets for configuration.

**Prerequisites:**
- CMake 3.25+
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
- **Requirement**: All new features and bug fixes must be accompanied by meaningful tests.

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

- **Standard**: C++20
- **Style**: Follow existing JUCE-based patterns (JUCE 8)
- **Shaders**: GLSL 3.30
- **Headers**: Use `#pragma once`
- **Memory**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) where possible
- **JUCE**: Use `juce::` namespace explicitly

## Completion Checklist

Before declaring any task complete:
- [ ] CIB-001: Used required tools (sequentialthinking, TodoWrite)
- [ ] CIB-002: Evaluated delegation for all subtasks
- [ ] CIB-003: Followed investigation-first workflow
- [ ] Tests pass: `ctest --preset dev`
- [ ] No placeholder or TODO code left
