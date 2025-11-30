=== CRITICAL INSTRUCTION BLOCK (CIB-001): MANDATORY TOOLS ===

## MANDATORY TOOLS

### For Coding, Research, Analysis, Debugging
```
USE: mcp__mcp_docker__sequentialthinking
WHEN: Coding tasks, research, complex reasoning
WHY: Prevents cognitive overload, ensures systematic approach
```

### For Task Management
```
USE: todo_write
WHEN: Coding tasks, any task with 2+ steps
WHY: Tracks progress, maintains focus
```

### For Task Execution
For each task:

1. **Decide if delegating task an agent is beneficial**
   Delegate to subagent if any of the following applies:
   - Task is complex or multi-step (e.g., write tests, debug a module, generate docs).
   - Task matches a predefined subagent role
   - Task generates large output or context (e.g., scanning 50 files, web research).
   - Task can run independently (no need for constant oversight).
   - Task can be executed in parallel (run multiple subagents at once).

   To delegate a task to an agent do this:
   - Use your agent-selection skill to select the right agent.
   - Call the subagent and provide all the context that they need to do the task at highest quality in the context of the whole codebase.
   - Never send a one-liner prompt to a subagent! Always provide them with all context they need to do the job. Do not let them start from scratch.
   - Always include references to required documentation:
     - `docs/architecture.md`
     - `docs/api.md`
     - `docs/testing.md`
     - include other or more documentation references in the agent prompt if needed for the task
   - **NEVER** just handover the task you have been given to an agent without passing on the work you have already done! Do not waste token on letting agents repeat the research or work you have already done!
   - Be conscious of token usage! Do not duplicate work in the agent that you or another agent have already done!

2. **Testing requirements:**
   - Tests must be meaningful (test actual functionality)
   - Tests must verify correct information, not just rendering

3. **Fix until green:**
   - Run tests after each fix
   - If tests fail, iterate and fix and test again
   - Verify no regressions

=== END CIB-001 ===

=== CRITICAL INSTRUCTION BLOCK (CIB-002): INVESTIGATION-FIRST DEVELOPMENT ===

## FORBIDDEN: Code-First Development

❌ **NEVER** change code based on assumptions
❌ **NEVER** change code without investigating side effects
❌ **NEVER** fix tests without running actual features
❌ **NEVER** declare victory without visual verification
❌ **NEVER** remove code without understanding why it exists

## REQUIRED: Investigation-First Workflow

✅ **ALWAYS** trace code execution path
✅ **ALWAYS** what the purpose of the code is you are about to change
✅ **ALWAYS** analyse impact of a change on whole codebase
✅ **ALWAYS** verify fixes with visual proof

**If you skip investigation, you are creating more bugs and unusable code.STOP AND INVESTIGATE.**

## MANDATORY WORKFLOW
```
1. Analyse and understand user request with sequentialthinking tool
2. Investigate context
3. Investigate impact on other parts of the app
4. Plan implementiation and split into smaller tasks
5. Create tasks with todo_write tool
6. Plan updates to tests and new tests with todo_write tool
7. Implement tasks in todo_write tool in order
8. Implement tests
9. Run tests and fix issues
10. Update documentation
11. Conduct final review
12. Give a short one paragraph summary in chat. NEVER write summary documents. NEVER output long summaries in chat.
```

## ABSOLUTE PROHIBITIONS
- Never skip steps in the workflow to save time or tokens.
- Never simplify or summarize steps in the workflow.
- Never invent things to save time or tokens.

## NO TOKEN OR TIME BUDGET
There is no limit on token or time a task can take. Be thorough. Reason: If you make a mistake now, it will later cost much more time and token to fix it.

=== END CIB-002 ===

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
- **`screenshots`**: Any screenshot

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

## Completiong Checklist
- Followed CIB-001 instruction block fully
- Followed CIB-002 instruction block fully
- No legacy code left
- No placeholder or todo code left
