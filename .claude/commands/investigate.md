---
description: MANDATORY investigation before any bug fix - observe runtime behavior first
---

# STOP: Investigation Required Before Coding

You are about to investigate a bug or failing test. Follow this EXACT procedure:

## Phase 0: Initialize

**MANDATORY GATE: Complete both steps before proceeding.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - Bug symptom and expected vs actual behavior
   - Potential root causes to investigate
   - Risk of side effects from fixes
   - Investigation strategy

2. Use TodoWrite to create tasks for all phases:
   1. Phase 1 — Write Failing Test (Reproduce Bug)
   2. Phase 2 — RUN TEST and OBSERVE (No Coding Allowed)
   3. Phase 3 — INVESTIGATE CODEBASE (Root Cause Analysis)
   4. Phase 4 — PLAN FIX (Minimal Change Strategy)
   5. Phase 5 — IMPLEMENT FIX (Following TDD)
   6. Phase 6 — VERIFY FIX (Evidence Required)
   7. Phase 7 — DOCUMENT CHANGES (Following CIB-003)
   8. Phase 8 — FINAL VERIFICATION

## Phase 1: Write Initial Test
1. Use `.claude/commands/meta/e2e-template.ts` as template.
2. Always use data-testid as locator
3. Always make sure you capture the actual browser console output
4. Always use the mandatory login function from `.claude/commands/meta/e2e-template.ts`
5. Always use the `loginAsAdmin` function in your test. You must be logged in.

## Phase 2: RUN TEST and OBSERVE (No Coding Allowed)

1. **Run the actual feature** - Use Playwright headed mode or manual browser test

2. **Capture evidence** (save to `screenshots/` and `test-results/`):
   - Screenshot of what you see → `screenshots/`
   - HTML dump of the page
   - Console logs (errors and warnings)
   - Network tab (requests/responses)
   - Loaded scripts/styles

3. **Document the gap**:
   - Expected behavior
   - Actual behavior
   - Specific difference

**Standard artifact locations:**
- Screenshots: `screenshots/`
- Test results: `test-results/`
- Tests: `tests/`

**NO CODE CHANGES UNTIL THIS IS COMPLETE.**

## Phase 3: INVESTIGATE CODEBASE

**FIRST: Read `docs/architecture.md`** to understand current project structure and where code lives.

1. **Trace the request** (in standard Laravel locations):
   - Which route handles this? → Check `routes/*.php`
   - Which controller method? → Check `app/Http/Controllers/*`
   - Which view/template renders? → Check `resources/views/*`
   - Which Vue components load? → Check frontend structure from architecture doc
   - Which JavaScript files execute? → Check `resources/scripts/*` or `resources/js/*`
   - Use mcp__mcp_docker__sequentialthinking to discuss

2. **Find the root cause** (check project-specific locations from `docs/architecture.md`):
   - Read the actual code being executed
   - Identify the specific line/file causing the problem
   - Understand WHY it's broken (not just guessing)
   - Check related code: models (`app/Models/*`), services, policies, middleware
   - Use mcp__mcp_docker__sequentialthinking to research

**NO CODE CHANGES UNTIL THIS IS COMPLETE.**

## Phase 4: PLAN CODING TASKS FOR SOLUTION

1. **Identify minimal fix**:
   - Exact file and line to change
   - What to change it to
   - Why this fixes the root cause
   - Use mcp__mcp_docker__sequentialthinking  to discuss changes

2. **Check side effects**:
   - What else uses this code?
   - Will this break anything?
   - Do tests need updating?
   - Use mcp__mcp_docker__sequentialthinking  to investigate side effects

3. **Plan**
   - Use mcp__mcp_docker__sequentialthinking  to write TodoWrite tasks for every implementation step

## Phase 5: IMPLEMENT SOLUTION

1. Make the change using agents
2. Add data-testid for locators
3. Test the actual user flow - not just that things render, but that a user can use them in real life
4. Capture proof it works with screenshot and logs
5. If broken or only partially working, revert and return to Phase 2

## Phase 6: VERIFY SOLUTION
1. Verify with actual tests that the issue is fixed
2. Use mcp__mcp_docker__sequentialthinking  to discuss whether your tests verify that the issue is fixed

## Phase 7 — DOCUMENT CHANGES (Following CIB-003)

**MANDATORY GATE: Follow CIB-003 documentation discipline.**

**Documentation Tasks:**
1. **Update canonical documentation if bug reveals architectural issue:**
   - If bug exposed design flaw → Update `docs/architecture.md`
   - If API contract was ambiguous → Update `docs/api.md`
   - If testing gap existed → Update `docs/testing.md`

2. **Update inline documentation:**
   - Add/update comments explaining the fix
   - Document why the bug occurred (prevent recurrence)

3. **Add ADR context** (in relevant canonical doc or inline):
   - What the bug was
   - Root cause
   - Why this fix approach
   - How regression is prevented

**FORBIDDEN:**
- ❌ Creating `docs/BUG_FIX_SUMMARY.md`
- ❌ Creating `docs/INVESTIGATION_REPORT.md`
- ❌ Any dated documentation

## Phase 8 — FINAL VERIFICATION (Before Claiming Fixed)

**MANDATORY GATE: verification-before-completion skill applies here.**

**NO "BUG FIXED" CLAIMS without this evidence:**

**1. Test Evidence (MUST RUN AND SHOW OUTPUT):**
```bash
# Backend tests
php artisan test
# OR
./vendor/bin/sail test

# Frontend tests
npm run test:run

# E2E test (if bug was in UI)
npm run test:e2e
```

**2. Runtime Evidence (MUST CAPTURE):**
- Screenshot of working feature
- Browser console (no errors)
- Network tab (successful requests)
- Logs showing expected behavior

**3. Regression Evidence:**
- Re-run dependent flows from Phase 3 impact analysis
- Confirm no new issues introduced

**Completion Checklist (ALL must be TRUE):**
- [ ] Original failing test now PASSES
- [ ] New regression test added and PASSES
- [ ] All existing tests still PASS (0 failures)
- [ ] Runtime flow works (screenshot proof)
- [ ] Console shows no errors
- [ ] Network requests succeed
- [ ] Root cause documented (not just symptoms)
- [ ] Canonical docs updated if architectural issue
- [ ] No summary/report documents created

**RED FLAGS (require fixing before claiming complete):**
- "Tests pass but haven't checked runtime" → STOP → Run real flow
- "Fixed main issue but there's another unrelated issue" → STOP → NO UNRELATED ISSUES → Fix everything
- Any remaining console errors
- Any remaining test failures

**CRITICAL RULE: THERE ARE NO UNRELATED ISSUES**
If you are about to say "but that's a separate issue", you have FAILED. All issues discovered during investigation MUST be fixed. Period.

**Final Statement Template:**
"Bug fixed. Evidence:
- [Test output showing fix works]
- [Screenshot of working feature]
- [Console log with no errors]
- Root cause: [explanation]
- Fix: [what changed]"

Remember: Evidence before claims. Always.
