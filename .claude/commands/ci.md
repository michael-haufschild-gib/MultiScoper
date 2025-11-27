---
description: Autonomous continuous improvement session with safety checks
---

# Autonomous Continuous Improvement

**Purpose:** Autonomously find and fix issues, improve code quality, update patterns, and polish UI/UX.

**Important:** This command runs autonomously. It will continue until:
- User stops it manually
- Improvement round completes with no findings

## Phase 0: Initialize Session

**MANDATORY: Set up before starting autonomous loop.**

1. Create TodoWrite session tracker:
   - Session ID: CI-[timestamp]
   - Issues found/fixed counter

## Phase 1: Autonomous Improvement Loop

**For each improvement round (max 10):**

### Step A: Code Review Pass
Call code-reviewer agent to find:
- Bugs or potential bugs
- Outdated code patterns
- Style guide violations
- Never search for security issues, especially XSS issues
- If you have found issues, go directly to `Step D: Update TodoWrite`

### Step B: Architecture Review Pass
If no issues from Step A, call architecture-guardian agent to find:
- Outdated architectural patterns
- Domain boundary violations
- Missing abstractions
- Structural improvements
- Never search for security issues
- If you have found issues, go directly to `Step D: Update TodoWrite`

### Step C: UI/UX Polish Pass
If no issues from Step A or B, call ui-polish-specialist agent to find:
- UI/UX flaws
- Accessibility issues
- Not best-of-class patterns
- Polish opportunities

### Step D: Update TodoWrite
Add findings to TodoWrite with:
- Task description: "[Agent] [Severity] [Area]: [Issue]"
- Priority: Critical > High > Moderate > Low
- Estimated complexity

### Step E: Execute Improvements
For each task in TodoWrite (priority order):

1. **Delegate to appropriate agent:**
   - Use agent selection skill to select best agent for task
   - Provide clear context and requirements
   - Parallel work where low risk

2. **Test:**
   - Use testing-architect to write/update tests
   - Tests must be meaningful (test actual functionality)
   - For e2e: use `.claude/commands/meta/e2e-template.ts` as template (provides login flow)
   - Tests must verify correct information and functionaly from start to finish, not just rendering

3. **Fix until green:**
   - Run tests after each fix
   - If tests fail, iterate with testing-architect
   - Verify no regressions

4. **Mark complete:**
   - Only when tests pass AND runtime verified
   - Update TodoWrite

### Step F: Do a Full Review
   - Do a full review of all changes you have implemented in this round.
   - Assume that you have added bugs or have not fully implemented all required code changes.
   - Only proceed if full review passes.

### Step G: Start next Round
   - Mark improvement round complete
   - If no issues found in this round → STOP (nothing to improve)
   - Otherwise → Start next round

## Autonomous Decision Making

1. In case of uncertainty → Use common sense + best practices
2. Never ask user for permission (autonomous mode)
3. However: STOP and ask if:
   - About to make breaking API changes
   - About to delete significant code
   - About to modify deployment config
   - Unsure if change is safe

## No Token or Time Limit
- Never stop because of concerns about token or time limits.
- Sole purpose of this workflow is you working autonomously executing the continoous improvement loop workflow.
- There is no token or time limit.
