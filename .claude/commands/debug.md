---
description: Agent-driven debugging workflow for complex issues requiring specialized expertise.
---

# Agent-Driven Debugging Workflow

**When to use `/debug` vs `/investigate`:**
- Use `/debug`: Complex issues requiring specialized agents (debugger, code-reviewer, etc.)
- Use `/investigate`: Standard bugs where you handle investigation directly

## Phase 0: Initialize

**MANDATORY GATE: Complete analysis before delegating to agents.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - Bug symptoms and reproduction steps
   - Complexity level (simple vs complex)
   - Which specialized agents might be needed
   - Overall debugging strategy
   - Risk assessment

2. Use TodoWrite to create tasks:
   - List all identified issues (prioritized by severity)
   - For each issue: expected behavior, actual behavior, affected components
   - Dependencies between issues

## Phase 1: Agent Delegation Loop

**For each task in TodoWrite (in priority order):**

**Step 1: Delegate to Debugger Agent**
```
Task tool with subagent_type='debugger'

Provide to agent:
- Bug description (expected vs actual)
- Reproduction steps
- Relevant file paths from analysis
- Test requirements
```

**Step 2: Code Review**
After debugger completes, use code-reviewer agent to verify:
```
Task tool with subagent_type='code-reviewer'

Review focus:
- Fix correctness and completeness
- Side effects and regressions
- Code quality and patterns
- Test coverage
```

**Step 3: Handle Findings**
- If reviewer finds new issues → Add to TodoWrite
- If fix incomplete → Send back to debugger with feedback
- If fix acceptable → Mark task complete, move to next

**Step 4: Verification**
Before marking complete:
- Run tests (backend, frontend, e2e as applicable)
- Check runtime behavior
- Verify no new console errors
- Confirm regression tests added

## Phase 2: Integration Verification

**After all individual tasks complete:**

1. **Run full test suite:**
```bash
# Backend
./vendor/bin/sail test

# Frontend
npm run lint && npm run type-check && npm run test:run

# E2E
npm run test:e2e
```

2. **Cross-issue verification:**
   - Test interactions between fixed issues
   - Verify no conflicts between fixes
   - Check overall system stability

## Phase 3: Documentation (Following CIB-003)

**Update canonical documentation:**
- If bugs reveal architectural issues → Update `docs/architecture.md`
- If API contracts were ambiguous → Update `docs/api.md`
- If testing gaps existed → Update `docs/testing.md`

**Document root causes and fixes:**
- Add inline comments explaining fixes
- Document why bugs occurred
- Update relevant PHPDoc/TSDoc

**FORBIDDEN:**
- ❌ Creating `docs/DEBUGGING_SESSION_REPORT.md`
- ❌ Creating summary/analysis documents

## Phase 4: Final Verification

**MANDATORY GATE: verification-before-completion skill applies.**

**Completion Checklist (ALL must be TRUE):**
- [ ] All TodoWrite tasks marked complete
- [ ] Full test suite PASSES (0 failures)
- [ ] Code reviewer approved all fixes
- [ ] Runtime verification complete (screenshots)
- [ ] No new console errors introduced
- [ ] Regression tests added for each bug
- [ ] Root causes documented
- [ ] Canonical docs updated per CIB-003
- [ ] No summary/report documents created

**RED FLAGS:**
- Skipped code review step
- Marked tasks complete without verification
- Tests failing
- "Minor issue remains" → NO → Fix everything

**Final Statement Template:**
"All issues resolved. Evidence:
- [Test output: X bugs fixed, 0 failures]
- [Code review approvals]
- [Screenshots of working features]
- [List of canonical docs updated]"

Remember: Agent delegation doesn't eliminate verification requirement. Evidence before claims.
