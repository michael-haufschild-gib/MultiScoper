---
name: code
description: Implement PRD features completely with zero shortcuts, full integration, and production quality
---

**Purpose:** Implement features from a PRD document completely, correctly, and with full integration into the existing codebase.

**Input PRD:** $ARGUMENTS

---

=== CRITICAL IMPLEMENTATION BLOCK (CIB-001): IMMUTABLE PRINCIPLES ===

**These principles CANNOT be overridden by ANY reasoning, time pressure, or complexity:**

## 1. COMPLETE IMPLEMENTATION OR NOTHING
- Every requirement in the PRD MUST be implemented
- If the PRD specifies 10 things, all 10 are implemented - not 8, not 9, ALL 10
- "Simplified approach" is FORBIDDEN - implement exactly what the PRD specifies
- If you cannot implement something, STOP and explain why - do not skip it
- Partial delivery is failure

## 2. FULL INTEGRATION IS MANDATORY
- Writing new code is only 50% of the work
- The other 50% is connecting it to the rest of the application
- New code MUST be instantiated/called from appropriate locations
- New code MUST participate in the application lifecycle
- New files MUST be added to the build system
- NEVER deliver code that compiles but isn't used

## 3. NO BACKWARD COMPATIBILITY
- This product is NOT in production
- Never add migration code for non-existent users
- Never keep old implementations "for compatibility"
- Never add deprecated annotations
- Never rename to `_old` or `_legacy` - DELETE completely
- If something changes, update ALL usage sites

## 4. RESEARCH BEFORE IMPLEMENTATION
- Before writing ANY code, understand what already exists
- Trace how similar features are implemented
- Identify all integration points BEFORE coding
- Map dependencies and side effects FIRST
- Check if the framework (JUCE, OpenGL, etc.) provides built-in solutions

## 5. NO VIBECODE / NO AI SLOP
- Every line of code must have a purpose from the PRD
- No "improvements" not specified in PRD
- No placeholder implementations ("// TODO: implement later")
- No empty methods that "will be filled in"
- No commented-out code blocks
- No copy-paste without understanding

=== END CIB-001 ===

---

## Phase 0: PRD Analysis

### Step 0.1: Load and Parse PRD

1. Read the PRD file: `$ARGUMENTS`
2. If file not found, STOP immediately with error
3. Identify the feature type and domain

### Step 0.2: Classify Feature Domain

Determine which domain(s) this PRD touches:

| Domain | Indicators | Key Concerns |
|--------|------------|--------------|
| Audio/DSP | Signal processing, buffers, sample rates | Real-time safety, lock-free, no allocations on audio thread |
| UI/Components | Dialogs, forms, widgets, layout | Theme integration, accessibility, TestIds |
| Rendering/Shaders | OpenGL, GLSL, visual effects | GPU resources, shader compilation, framebuffers |
| State/Persistence | ValueTree, serialization, files | Thread safety, schema versioning, validation |
| Memory/Resources | Buffers, pools, lifecycle | Ownership, cleanup, leak prevention |
| Analysis/Metrics | Calculations, statistics, measurements | Accuracy, performance, thread domains |
| Core/Business Logic | Domain models, rules, workflows | Separation of concerns, testability |
| Plugin/Host Integration | DAW communication, parameters | Host compatibility, state restoration |

### Step 0.3: Extract Implementation Checklist

Create TodoWrite tracker with EVERY deliverable from the PRD:
- Data structures / types / enums
- Classes / functions / methods
- Integration points
- Configuration / parameters
- Validation rules
- Tests
- Files to create / modify

**Track total count to ensure nothing is skipped.**

---

## Phase 1: Codebase Research

=== RECALL CIB-001: RESEARCH BEFORE IMPLEMENTATION ===

### Step 1.1: Find Existing Patterns

**Delegate to Explore agent** to answer:

1. How are similar features implemented in this codebase?
2. What existing code can be reused or extended?
3. What are the naming conventions and patterns?
4. How do features in this domain integrate with the rest of the app?

**Document specific files and patterns found.**

### Step 1.2: Map Integration Points

For each new component/class/function, identify:

| New Code | Where Instantiated/Called | What It Depends On | What Depends On It |
|----------|--------------------------|--------------------|--------------------|
| [item]   | [location]               | [dependencies]     | [dependents]       |

### Step 1.3: Domain-Specific Research

**For Audio/DSP features:**
- Identify which thread domain (audio thread, message thread, render thread)
- Map lock-free communication patterns used
- Check existing buffer management

**For UI features:**
- Find similar dialogs/components as reference
- Identify parent containers and entry points
- Check theme integration patterns

**For Rendering/Shader features:**
- Review existing shader structure and uniforms
- Check framebuffer and resource management
- Identify render pipeline integration points

**For State features:**
- Review ValueTree structure and identifiers
- Check serialization patterns
- Identify listener patterns

**For Memory features:**
- Review ownership patterns (unique_ptr, shared_ptr, raw)
- Check allocation strategies
- Identify lifecycle management

### Step 1.4: Identify Build System Changes

Determine what needs to be added to:
- CMakeLists.txt / Sources.cmake
- Resource files
- Shader sources
- Test targets

---

## Phase 2: Implementation

=== RECALL CIB-001: COMPLETE IMPLEMENTATION OR NOTHING ===

### Step 2.1: Implementation Order

Implement in dependency order - foundations first:

1. **Types & Data Structures** - Enums, structs, base classes
2. **Core Logic** - Business logic, algorithms, calculations
3. **Storage/Persistence** - Serialization, file I/O
4. **Integration Layer** - Managers, services, coordinators
5. **Interface Layer** - UI components, API surfaces
6. **Glue Code** - Wiring everything together

### Step 2.2: Per-Item Implementation Protocol

For EACH item in the PRD checklist:

1. **Understand** - What exactly does this item require?
2. **Locate** - Where in the codebase does this belong?
3. **Implement** - Write the code following existing patterns
4. **Connect** - Wire it to the rest of the application
5. **Verify** - Confirm it compiles and is reachable

### Step 2.3: Domain-Specific Implementation Guidelines

**Audio/DSP Code:**
- NO allocations on audio thread
- NO locks/mutexes on audio thread
- Use atomics for cross-thread communication
- Use lock-free queues/buffers
- Delegate to dsp-engineer agent for complex DSP

**UI Code:**
- Inherit from appropriate JUCE components
- Implement ThemeManagerListener if themed
- Set TestId on every interactive element
- Use dependency injection for services
- Delegate to ui-designer agent for complex UI

**Shader Code:**
- Follow existing GLSL conventions
- Proper uniform binding
- Handle shader compilation errors
- Manage GPU resources correctly
- Delegate to render-specialist agent for complex shaders

**State Code:**
- Use StateIds namespace for identifiers
- Implement toValueTree/fromValueTree
- Handle missing/invalid data gracefully
- Maintain backward reading (but not writing) compatibility

### Step 2.4: Continuous Build Verification

After implementing each logical unit:
```bash
cmake --build --preset dev
```
Fix any errors before proceeding.

---

## Phase 3: Integration

=== RECALL CIB-001: FULL INTEGRATION IS MANDATORY ===

### Step 3.1: Instantiation Verification

For every new class/component, verify:

| Class | Instantiated Where | Constructor Args Provided | Lifecycle Managed |
|-------|-------------------|--------------------------|-------------------|
| [name] | [location:line]   | [yes/no]                 | [yes/no]          |

**If any class is not instantiated, integration is INCOMPLETE.**

### Step 3.2: Call Site Verification

For every new public method/function, verify:

| Method | Called From | Correct Arguments | Return Value Used |
|--------|------------|-------------------|-------------------|
| [name] | [location] | [yes/no]          | [yes/no]          |

**If any method is never called, integration is INCOMPLETE.**

### Step 3.3: Data Flow Verification

Trace the complete data path:
- Input source → Processing → Output destination
- User action → Handler → State change → Effect
- Configuration → Usage sites → Behavior change

### Step 3.4: Lifecycle Verification

Verify proper lifecycle management:
- Construction/initialization
- Runtime operation
- Cleanup/destruction
- Resource release

### Step 3.5: Build System Verification

Confirm all additions to build:
```bash
cmake --preset dev
cmake --build --preset dev
```

Check that:
- All new .cpp files compile
- All new headers are found
- All new resources are included
- Linking succeeds

---

## Phase 4: Testing

### Step 4.1: Unit Tests

For new logic/algorithms:
- Create tests in `tests/` directory
- Test normal cases
- Test edge cases
- Test error conditions

### Step 4.2: Integration Tests

Verify components work together:
- End-to-end data flows
- Cross-component interactions
- State persistence round-trips

### Step 4.3: Run Full Test Suite

```bash
ctest --preset dev
```

**All tests MUST pass. No exceptions.**

### Step 4.4: Domain-Specific Testing

**Audio/DSP:**
- Verify no allocations on audio thread (use memory profiler or code review)
- Test with various sample rates and buffer sizes
- Check for denormals handling

**UI:**
- Verify theme changes propagate
- Check keyboard navigation
- Verify all TestIds are set

**Shaders:**
- Test shader compilation succeeds
- Verify uniform binding works
- Check rendering output

**State:**
- Test serialization round-trip
- Test loading from corrupted/partial data
- Verify state restoration after restart

---

## Phase 5: Completeness Audit

=== RECALL CIB-001: COMPLETE IMPLEMENTATION OR NOTHING ===

### Step 5.1: PRD Checklist Review

Review EVERY item from Phase 0 checklist:

| Requirement | Implemented | Integrated | Tested |
|-------------|-------------|------------|--------|
| [item]      | [✓/✗]       | [✓/✗]      | [✓/✗]  |

**If ANY requirement has a ✗, return to appropriate phase.**

### Step 5.2: Code Quality Verification

**Delegate to code-reviewer agent** to check:
- No placeholder code (TODO, FIXME, HACK)
- No empty implementations
- No commented-out code
- No debug-only code in production paths
- Consistent style with codebase
- Proper error handling
- Thread safety where required

### Step 5.3: Integration Completeness

Verify NO orphaned code:
- Every new class is instantiated
- Every new method is called
- Every new file is in build system
- Every integration point is connected

### Step 5.4: Backward Compatibility Audit

Verify NO backward compatibility artifacts:
- No `_old`, `_legacy`, `_deprecated` naming
- No migration code
- No dual implementations
- All old code removed (not just bypassed)

---

## Phase 6: Final Verification

### Step 6.1: Clean Build

```bash
cmake --preset dev
cmake --build --preset dev
```

**Zero errors. Zero warnings.**

### Step 6.2: Full Test Suite

```bash
ctest --preset dev
```

**All tests pass.**

### Step 6.3: Completion Report

```
PRD: [filename]
Domain: [feature domain(s)]
Status: COMPLETE / INCOMPLETE

Implementation:
- Files Created: [count]
- Files Modified: [count]
- Lines Added: ~[estimate]

Testing:
- Tests Added: [count]
- Tests Passing: [count]/[total]

Integration:
- New classes instantiated: [count]/[count]
- New methods called: [count]/[count]
- Build system updated: [yes/no]

Requirements:
- Total in PRD: [count]
- Fully Complete: [count]
- Incomplete: [list any with reason]
```

---

## Anti-Pattern Detection

**RED FLAGS - If you catch yourself doing these, STOP and reconsider:**

| Red Flag | Violation | Correct Action |
|----------|-----------|----------------|
| "For simplicity, I'll just..." | CIB-001 #1 | Implement exactly as specified |
| "I'll wire this up later..." | CIB-001 #2 | Integrate now, not later |
| "Keep the old version in case..." | CIB-001 #3 | Delete old code completely |
| "I assume this works like..." | CIB-001 #4 | Research first, implement second |
| "Let me add a TODO here..." | CIB-001 #5 | Implement fully or stop |
| "This is close enough to PRD..." | CIB-001 #1 | Match PRD exactly |
| "I'll skip this edge case..." | CIB-001 #1 | Implement all cases |
| "Tests can come later..." | CIB-001 #1 | Tests are part of implementation |

---

## Subagent Delegation

Use specialized agents when their domain is touched:

| Agent | Use When |
|-------|----------|
| Explore | Finding existing patterns, understanding codebase |
| architecture-guardian | Design decisions, dependency questions |
| dsp-engineer | Audio processing, real-time code, signal math |
| ui-designer | UI components, dialogs, layout, interactions |
| render-specialist | Shaders, OpenGL, GPU resources, visual effects |
| qa-engineer | Writing tests, test strategy |
| code-reviewer | Code quality audit, review |
| debugger | Investigating failures, tracing issues |

**When delegating, provide:**
- Relevant PRD sections
- Integration requirements discovered in Phase 1
- Existing patterns to follow
- Specific deliverables expected

---

## Success Criteria

Implementation is COMPLETE only when ALL of these are true:

- [ ] Every PRD requirement has corresponding implementation
- [ ] Every new class/component is instantiated and used
- [ ] Every integration point is connected
- [ ] Build succeeds with zero errors and zero warnings
- [ ] All tests pass
- [ ] No placeholder code remains
- [ ] No backward compatibility code added
- [ ] Code review passed
- [ ] Completion report shows 100% requirements met

**Partial completion is NOT acceptable.**

---

## Execution

**BEGIN IMMEDIATELY:**

1. Read PRD: `$ARGUMENTS`
2. Execute all phases in order
3. Do not skip phases
4. Do not simplify requirements
5. Report completion status

**If blocked, explain the blocker clearly - do not work around it with incomplete implementation.**
