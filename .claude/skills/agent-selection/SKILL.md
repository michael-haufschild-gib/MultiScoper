# Agent Selection Skill

Guide for delegating tasks to specialized subagents in the Oscil audio visualization plugin.

## Decision Tree

Analyze the task and match to the FIRST applicable agent:

### Implementation Agents (Write Code)
| Signal | Agent | Use When |
|--------|-------|----------|
| Audio processing, DSP, `processBlock()`, lock-free, timing | `dsp-engineer` | Implementing audio logic, buffer management, cross-thread data |
| UI components, layouts, dialogs, themes, mouse/keyboard | `ui-designer` | Creating/modifying Components, panels, styling |
| OpenGL, shaders, GLSL, FBOs, particles, visualization | `render-specialist` | Writing shaders, render pipeline, GPU resources |
| Tests, build errors, crashes, mocks, GoogleTest | `qa-engineer` | Writing tests, fixing build failures, debugging crashes |

### Analysis Agents (Review/Investigate, No Code Changes)
| Signal | Agent | Use When |
|--------|-------|----------|
| File placement, dependencies, layers, thread boundaries | `architecture-guardian` | Validating architecture decisions, reviewing structure |
| Code review, real-time safety, thread safety, patterns | `code-reviewer` | Reviewing code changes before commit |
| Bug investigation, root cause, hypothesis testing | `debugger` | Analyzing bugs WITHOUT fixing (investigation only) |
| Documentation for LLM agents, patterns, decision trees | `technical-writer` | Writing/updating docs in `docs/` |

## Quick Reference

```
Task involves...                          → Use agent
─────────────────────────────────────────────────────
processBlock(), audio buffers, atomics    → dsp-engineer
Components, paint(), resized(), dialogs   → ui-designer
Shaders, OpenGL, GLSL, render pipeline    → render-specialist
Tests, build errors, ctest, GoogleTest    → qa-engineer
"Is this the right place for X?"          → architecture-guardian
"Review this code for issues"             → code-reviewer
"Why is X happening?" (no fix needed)     → debugger
"Update the docs for Y"                   → technical-writer
```

## Multi-Domain Tasks

When a task crosses domains, sequence agents:

**Example**: "Add a knob that changes filter frequency"
1. `dsp-engineer` → Add parameter, filter DSP logic
2. `ui-designer` → Add knob component, wire to parameter

**Example**: "Add new shader with UI controls"
1. `render-specialist` → Implement shader
2. `ui-designer` → Add shader selector/controls

**Example**: "Fix bug and add tests"
1. `debugger` → Investigate root cause (analysis only)
2. `dsp-engineer` or `ui-designer` → Implement fix
3. `qa-engineer` → Add tests

## Context to Provide Subagents

**ALWAYS** include in your subagent prompt:
```
1. Specific task description (not just "fix it")
2. Relevant file paths to read/modify
3. Any investigation you've already done
4. Reference docs: docs/architecture.md, docs/testing.md
5. Expected deliverable format
```

**Example prompt to subagent**:
```
Task: Add gain parameter to SignalProcessor

Files to modify:
- include/core/dsp/SignalProcessor.h
- src/core/dsp/SignalProcessor.cpp

Context: User wants real-time gain control. Must be atomic for thread safety.

Reference: docs/architecture.md for patterns

Deliverable: Working implementation with test coverage
```

## Anti-Patterns

| Don't | Do Instead |
|-------|------------|
| "Refactor the entire app" | Delegate specific files/modules |
| Send one-liner prompts | Provide full context and file paths |
| Let agent repeat your research | Pass investigation results |
| Use debugger to fix bugs | Use debugger to analyze, then implementation agent to fix |
| Use code-reviewer to implement | Use code-reviewer to review, then fix separately |

## Parallel Delegation

When tasks are independent, delegate in parallel:
```
Independent: tests + docs update    → qa-engineer + technical-writer (parallel)
Dependent: DSP then UI              → dsp-engineer, wait, then ui-designer (sequential)
```

## Quality Gate

Before delegating, verify:
- [ ] Task matches ONE agent's domain clearly
- [ ] Specific files/scope identified
- [ ] Context and investigation results included
- [ ] Reference docs specified
- [ ] Expected deliverable defined
