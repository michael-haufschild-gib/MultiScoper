---
name: agent-selection
description: Systematic framework for selecting the optimal specialized agent for C++/JUCE VST plugin development. Use when delegating to subagents via the Task tool to ensure the most appropriate specialist is chosen based on the development phase and task type.
---

# Agent Selection (C++/JUCE VST Plugin)

## Overview

Select the optimal specialized agent for C++/JUCE VST plugin development tasks. This skill provides a structured approach to matching tasks with agent expertise across the plugin development lifecycle.

## When to Use This Skill

Invoke this skill before using the Task tool to delegate work to a specialized agent. Specifically use when:

- Starting a new development phase (Requirements, Architecture, Implementation)
- Delegating complex technical tasks (DSP, UI, OpenGL, Threading)
- Performing quality assurance (Code Review, Testing, Profiling)
- Investigating issues (Bugs, Memory Leaks, Performance)
- Writing documentation

## Agent Selection Process

Follow this systematic process to select the appropriate agent:

### Step 1: Identify Development Phase

Determine which phase of the plugin lifecycle the task belongs to:

1.  **Requirements & Analysis**: Defining what to build.
2.  **Architecture & Design**: Planning how to build it.
3.  **Implementation**: Writing the code.
4.  **Quality & Optimization**: Testing and refining.
5.  **Documentation**: Explaining the system.

### Step 2: Apply Selection Logic

Use this decision tree to select the agent:

#### 1. Requirements Phase
- **New plugin concept / PRD creation** → `plugin-requirements-analyst`
- **Analyzing existing JUCE code** → `juce-analyzer`

#### 2. Architecture Phase
- **Overall plugin architecture / Threading model** → `juce-architect`
- **Parameter system design (APVTS)** → `parameter-architect`
- **UI/UX Design & Component Hierarchy** → `ui-designer` (if available) or `ui-engineer` for technical design

#### 3. Implementation Phase
- **Core Plugin (Processor, DSP, State, Host Integration)** → `plugin-engineer`
- **Standard UI (Editor, Components, Layouts, LookAndFeel)** → `ui-engineer`
- **Advanced Visualizations (OpenGL, Shaders, Real-time)** → `opengl-specialist`
- **Complex Animations** → `animation-specialist`

#### 4. Quality & Optimization Phase
- **Code Review (General)** → `code-reviewer`
- **Performance Issues / CPU Usage** → `performance-profiler`
- **Memory Leaks / Lifetime Issues** → `memory-leak-detector`
- **Security / Input Validation** → `security-auditor`
- **Testing Strategy / Unit Tests** → `testing-architect`
- **Debugging Crashes / Logic Errors** → `debugger`

#### 5. Documentation Phase
- **Technical Documentation / User Manuals** → `technical-writer`

### Step 3: Verify Selection

Before delegating, verify the selection makes sense:

1.  **Expertise match**: Does the agent's expertise align with task requirements?
2.  **Scope appropriateness**: Is the task complex enough to warrant delegation?
3.  **Context availability**: Can the task be completed independently by the agent?

### Step 4: Delegate with Clear Context

When using the Task tool:
-   **Provide Context**: Always include references to `docs/prd.md`, `docs/architecture.md`, and relevant `.e/` status files.
-   **Respect Boundaries**: Don't ask `ui-engineer` to write DSP code, or `plugin-engineer` to write UI code.
-   **Parallel Work**: Note that Implementation Phase agents (`plugin-engineer`, `ui-engineer`, `opengl-specialist`) are designed to work in parallel using the `.e/` folder coordination system.

## Quick Reference Patterns

**"Create a PRD for a new delay plugin"**
→ `plugin-requirements-analyst`

**"Design the thread-safe architecture for this synth"**
→ `juce-architect`

**"Implement the delay algorithm and parameter smoothing"**
→ `plugin-engineer`

**"Build the main editor window and knobs"**
→ `ui-engineer`

**"Create a real-time spectrum analyzer using OpenGL"**
→ `opengl-specialist`

**"Fix the memory leak in the voice allocation"**
→ `memory-leak-detector`

**"Optimize the filter for lower CPU usage"**
→ `performance-profiler`

**"Review the new oscillator code"**
→ `code-reviewer`

**"Write unit tests for the envelope generator"**
→ `testing-architect`

## Edge Cases

-   **Hybrid Tasks**: If a task involves both DSP and UI (e.g., "Add a visualizer that reacts to audio"), break it down:
    1.  Task 1 (`plugin-engineer`): Expose audio data safely.
    2.  Task 2 (`ui-engineer` / `opengl-specialist`): Visualize the data.
-   **General C++ Issues**: If it's a generic C++ error not specific to JUCE/Audio, `debugger` or `code-reviewer` are good choices.

