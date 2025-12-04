# Agent Library

Quick reference for all available subagents in the Oscil project.

## Implementation Agents (Write Code)

| Agent | Expertise | Key Files | Delivers |
|-------|-----------|-----------|----------|
| **dsp-engineer** | Real-time audio, lock-free threading, DSP math, `processBlock()` | `src/core/dsp/*`, `src/plugin/PluginProcessor.cpp`, `include/core/SharedCaptureBuffer.h` | Working DSP code with real-time safety |
| **ui-designer** | JUCE Components, layouts, dialogs, themes, DI patterns | `src/ui/*`, `src/plugin/PluginEditor.cpp`, `include/ui/*` | UI components with theme/test support |
| **render-specialist** | OpenGL 3.3, GLSL 3.30, shaders, FBOs, particles | `src/rendering/*`, `include/rendering/*` | Shaders/effects with proper resource management |
| **qa-engineer** | GoogleTest, CMake, build errors, test coverage | `tests/*`, `CMakeLists.txt`, `cmake/*` | Passing tests with edge case coverage |

## Analysis Agents (Review/Investigate Only)

| Agent | Expertise | Focus Areas | Delivers |
|-------|-----------|-------------|----------|
| **architecture-guardian** | Layer separation, file placement, thread boundaries, dependencies | Project structure, `docs/architecture.md` | Architecture validation report |
| **code-reviewer** | Real-time safety, thread safety, JUCE patterns, C++20 | Modified files, git diff | Review with severity-categorized issues |
| **debugger** | Root cause analysis, hypothesis testing, execution tracing | Bug symptoms, stack traces | RCA report (no code fixes) |
| **technical-writer** | LLM-agent docs, patterns, decision trees | `docs/*` | Token-efficient documentation |

## Thread Domain Mapping

| Domain | Primary Agent | Backup |
|--------|---------------|--------|
| Audio thread code | dsp-engineer | - |
| Message thread (UI) | ui-designer | - |
| OpenGL thread | render-specialist | - |
| Cross-thread issues | dsp-engineer | architecture-guardian (review) |

## File Path → Agent Mapping

```
src/core/dsp/*           → dsp-engineer
src/plugin/Processor*    → dsp-engineer
src/ui/*                 → ui-designer
src/plugin/Editor*       → ui-designer
src/rendering/*          → render-specialist
tests/*                  → qa-engineer
docs/*                   → technical-writer
Any architecture question → architecture-guardian
Any code review          → code-reviewer
Any bug investigation    → debugger
```

## Common Task → Agent Chains

| Task Type | Agent Sequence |
|-----------|----------------|
| New audio feature | dsp-engineer → ui-designer → qa-engineer |
| New visualization | render-specialist → ui-designer |
| Bug fix | debugger → (dsp/ui/render) → qa-engineer |
| Code review + fix | code-reviewer → (implementation agent) |
| New component | architecture-guardian → ui-designer → qa-engineer |
| Refactor | architecture-guardian → (implementation agent) → code-reviewer |
