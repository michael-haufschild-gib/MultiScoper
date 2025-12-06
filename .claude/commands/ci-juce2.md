---
name: ci-juce2
description: Autonomous continuous improvement session with safety checks for Juce 8 plugins 2
---
# Autonomous Continuous Improvement (Exploratory Edition)

**Purpose:** Autonomously explore the codebase to discover and fix bugs, issues, and improvement opportunities of any kind.

**Context:**
- **Project:** Oscil Multi-Track Oscilloscope (professional audio plugin)
- **Framework:** JUCE 8.0.5
- **Language:** C++20
- **Rendering:** OpenGL 3.3 with shaders and effects
- **Architecture:** Hybrid (Strict Core separation, Dependency Injection in UI, Service Contexts)
- **Constraints:** Hard real-time requirements for audio thread

=== CRITICAL CI INSTRUCTION BLOCK (CI-001): IMMUTABLE PRINCIPLES ===

These principles CANNOT be overridden by any subsequent reasoning:

## 1. Mandatory Subagent Delegation
- For every significant review or implementation task, delegate to specialized subagents
- Use your agent-selection skill to choose the right agent for each task
- Never handle all workload yourself when specialists exist

## 2. Fix Everything You Find
- Any bug, issue, or problem you discover MUST be fixed
- Never skip issues because they "already existed" or are "unrelated to current task"
- Bugs, build errors, and broken features are never acceptable

## 3. No Task Is Too Large
- Never skip improvements because they require "larger refactoring"
- There is no time or token limit - quality is the only goal
- Plan thoroughly, implement carefully, test completely

## 4. No Backward Compatibility
- This project is not in production yet
- Never add backward-compatibility features
- Never keep legacy code - remove it completely

## 5. Investigation Before Action
- NEVER change code based on assumptions
- ALWAYS trace code execution paths before modifications
- ALWAYS understand why code exists before removing it
- ALWAYS analyze impact on the whole codebase
- ALWAYS check if there is a "JUCE 8" way for a solution

=== END CI-001 ===

## Phase 0: Initialize Session

**MANDATORY: Set up tracking before starting.**

Create a TodoWrite session tracker with:
- Session ID: CI-[timestamp]
- Running count of issues found/fixed

## Phase 1: Autonomous Exploration Loop

**For each improvement round (max 10):**

### Step A: Explore and Discover

Your goal is to **find issues** - be curious, thorough, and creative in your exploration. Look everywhere, question everything.

**Exploration Mindset:**
- Act like a detective investigating a codebase
- Follow hunches and investigate anomalies
- Check if features actually work as intended
- Look for what's missing, not just what's broken

**Exploration Techniques (use any combination):**
- Read through files looking for problems
- Trace execution paths end-to-end
- Check if UI components behave correctly
- Verify that features do what they claim
- Look for code that seems wrong, incomplete, or fragile
- Test edge cases and boundary conditions mentally
- Check consistency across similar components
- Look for patterns that should exist but don't

**Everything is in scope:**
- Bugs (crashes, incorrect behavior, edge cases)
- UI issues (layout, responsiveness, visual glitches)
- Feature problems (broken, incomplete, poorly designed)
- Performance issues (slow paths, memory leaks, inefficiency, unnecessary complex calculations)
- Architecture violations (wrong patterns, bad dependencies)
- Code quality (dead code, duplication, unclear logic)
- Real-time safety (audio thread violations)
- Thread safety (race conditions, deadlocks)
- Resource management (leaks, missing cleanup)
- Modernization opportunities (outdated patterns)
- Missing functionality (gaps in implementation)
- Test coverage (untested critical paths)

**What makes a good discovery:**
- Something that's actually broken or could break
- Something that violates project principles
- Something that makes the codebase harder to maintain
- Something that could cause user-facing problems
- Something that wastes resources or performance

**Key areas to explore (not exhaustive):**
- `src/core/` - Business logic and state management
- `src/core/dsp/` - Signal processing (TimingEngine, SignalProcessor)
- `src/plugin/` - Plugin entry points (PluginProcessor, PluginEditor)
- `src/ui/` - All user interface components
- `src/rendering/` - OpenGL visualization pipeline
- `include/` - Headers and interfaces
- `tests/` - Test coverage and quality

### Step B: Document Findings

Add all discoveries to TodoWrite with:
- Clear description of the issue
- Location (file:line when possible)
- Severity assessment (Critical > High > Moderate > Low)
- **Include ALL findings regardless of severity**

### Step C: Fix Issues

For each issue (in priority order):

**1. Investigate:**
- Understand the root cause completely
- Trace the impact throughout the codebase
- Plan the fix and its side effects
- ALWAYS check if there is a "JUCE 8 way" for the solution - something done differently in the context of JUCE 8 or JUCE 8 already providing a solution

**2. Implement:**
- Make focused, minimal changes
- Follow existing code patterns
- Delegate to specialist subagents when appropriate

**3. Test:**
- Write/update unit tests in `tests/`
- **MANDATORY:** Run `ctest --preset dev` after changes
- Create reproduction tests for crashes/leaks when possible

**4. Verify:**
- Confirm the fix works
- Check for regressions
- Self-review: Did you introduce new problems?

**5. Iterate:**
- If tests fail, fix immediately
- Only mark complete when `ctest` passes
- Update TodoWrite status

### Step D: Full Build Verification

After fixing issues:
- Run `cmake --build --preset dev`
- Ensure clean build with no warnings
- Review your changes for unintended consequences

### Step E: Continue Exploration

- Mark improvement round complete
- If issues remain in TodoWrite, continue fixing
- If TodoWrite is empty, start fresh exploration (Step A)
- **MANDATORY:** Autonomously start the next round
- **NEVER STOP:** Continue working without asking the user whether to continue - user will stop the chat when necessary

## Quality Principles

**Real-Time Safety:**
- Audio thread must never block, allocate, or lock
- If a change risks audio glitches, redesign it
- Be mindful that memory access errors, memory leaks, race conditions and thread violations are common issues in this type of project. Always be on the lookout for them.

**Stability Over Style:**
- Fix crashes and race conditions before cleanup
- Working code is more important than pretty code

**Thorough Over Fast:**
- One properly fixed issue > ten partially fixed issues
- Understand before you change

**Cross-Platform and Cross-DAW:**
- Solutions must work on MacOS, Windows and Linux
- Solutions must work with all major DAWs: Ableton Live, Bitwig, Reaper, Cubase, Logic

**Separation of Concern**
- Follow separation of concern principles
- Use dependency injection instead of singletons

## What Success Looks Like

A successful CI session:
- Discovers issues the previous session missed
- Fixes all discovered issues completely
- Leaves no broken tests or build errors
- Improves overall codebase quality
- Makes the product more stable and reliable
