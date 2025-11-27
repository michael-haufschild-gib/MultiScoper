---
description: Autonomous investigate-fix-test workflow that ensures features work 100% correctly
---

# Test and Fix Until Perfect

This command implements an investigation-first workflow: understand what's actually happening, fix critical errors, then write comprehensive tests to verify everything works perfectly.

**When to use:**
- Testing a specific feature or page
- Verifying a feature works end-to-end after changes
- Ensuring data accuracy and complete functionality
- Validating complex user workflows

**Core Principle: INVESTIGATE FIRST, TEST AFTER**
Never write tests before understanding what the page actually does. Browser console output reveals the truth.

**Target:** $ARGUMENTS

---

## Phase 0: Initialize

**MANDATORY: Create todo list and analysis plan.**

Use TodoWrite to create tasks for all phases:
1. Phase 1 — Initial Investigation (Browser First)
2. Phase 2 — Fix Critical Errors (500s, data loading, etc.)
3. Phase 3 — Feature Analysis (Understand Working Feature)
4. Phase 4 — Test Strategy Design (Coverage Plan)
5. Phase 5 — Write Comprehensive Tests (For Working Feature)
6. Phase 6 — Test-Fix Loop (Until 100% Pass)
7. Phase 7 — Final Verification
8. Phase 8 — Documentation

---

## Phase 1: Initial Investigation (Browser First)

**MANDATORY GATE: Understand actual state before any analysis or testing.**

**Goal:** Discover what's ACTUALLY happening, not what SHOULD happen.

### Step 1: Create Investigation Test

Write a SINGLE simple test to investigate the page:

```typescript
// tests/e2e/investigate-[feature]-testnfix.spec.ts
import { test } from '@playwright/test';

async function loginAsAdmin(page) {
  // Copy from .claude/commands/meta/e2e-template.ts
  await page.goto('http://localhost');
  await page.waitForTimeout(1000);

  const cookieAccept = page.locator('[data-testid="cookie-consent-accept"]');
  if (await cookieAccept.isVisible({ timeout: 2000 }).catch(() => false)) {
    await cookieAccept.click();
  }

  await page.locator('[data-testid="header-login-button"]').click();
  await page.waitForTimeout(500);
  await page.locator('[data-testid="login-email-input"] input').fill('test@test.com');
  await page.locator('[data-testid="login-password-input"] input').fill('test1234');
  await page.locator('[data-testid="login-submit-button"]').click();
  await page.waitForTimeout(2000);
}

test('INVESTIGATE: [Feature Name] - Capture actual state', async ({ page }) => {
  const consoleMessages: string[] = [];
  const consoleErrors: string[] = [];
  const networkErrors: string[] = [];

  // Capture ALL console output
  page.on('console', msg => {
    const text = msg.text();
    consoleMessages.push(`[${msg.type()}] ${text}`);
    if (msg.type() === 'error') {
      consoleErrors.push(text);
    }
  });

  // Capture network failures
  page.on('response', response => {
    if (response.status() >= 400) {
      networkErrors.push(`${response.status()} ${response.url()}`);
    }
  });

  // Capture page errors
  page.on('pageerror', err => {
    consoleErrors.push(`PAGE ERROR: ${err.message}`);
  });

  // Login and navigate
  await loginAsAdmin(page);
  await page.goto('$ARGUMENTS');
  await page.waitForTimeout(3000);

  // Log EVERYTHING
  console.log('\n=== INVESTIGATION REPORT ===');
  console.log('\n1. URL:', page.url());
  console.log('\n2. NETWORK ERRORS:', networkErrors.length);
  networkErrors.forEach(err => console.log('   -', err));

  console.log('\n3. CONSOLE ERRORS:', consoleErrors.length);
  consoleErrors.forEach(err => console.log('   -', err));

  console.log('\n4. ALL CONSOLE OUTPUT (last 50 lines):');
  consoleMessages.slice(-50).forEach(msg => console.log('   ', msg));

  // Capture page structure
  const pageTitle = await page.title();
  console.log('\n5. PAGE TITLE:', pageTitle);

  const bodyText = await page.locator('body').textContent();
  console.log('\n6. PAGE HAS CONTENT:', bodyText ? bodyText.length > 100 : false);

  // Check for common containers
  const hasCard = await page.locator('.card').count();
  const hasTable = await page.locator('table').count();
  const hasForm = await page.locator('form').count();
  console.log('\n7. PAGE STRUCTURE:');
  console.log('   - .card elements:', hasCard);
  console.log('   - table elements:', hasTable);
  console.log('   - form elements:', hasForm);

  // Screenshot for visual reference
  await page.screenshot({
    path: 'screenshots/investigate-[feature]-initial.png',
    fullPage: true
  });
  console.log('\n8. Screenshot saved to: screenshots/investigate-[feature]-initial.png');

  console.log('\n=== END INVESTIGATION ===\n');
});
```

### Step 2: Run Investigation Test

```bash
npx playwright test tests/e2e/investigate-[feature]-testnfix.spec.ts
```

### Step 3: Analyze Investigation Output

**CRITICAL: Read the console output carefully.**

Look for:
1. **500/404 errors** → Backend broken, fix IMMEDIATELY (go to Phase 2)
2. **Network errors** → API endpoints failing, fix IMMEDIATELY
3. **Console errors** → JS/Vue errors, investigate cause
4. **No data loading** → Check API calls, data flow
5. **Missing elements** → Feature not rendering, check components

**Decision Point:**
- ❌ **If 500 errors or data not loading** → GO TO PHASE 2 (Fix Critical Errors)
- ✅ **If page loads and data appears** → GO TO PHASE 3 (Feature Analysis)

**Deliverable:**
- Investigation test output (console logs)
- Screenshot of actual page state
- List of critical errors to fix (if any)

---

## Phase 2: Fix Critical Errors (If Needed)

**MANDATORY: Fix blocking errors BEFORE writing comprehensive tests.**

### Step 1: Identify Critical Issues

From Phase 1 investigation, critical issues are:
- 500/503 Server Errors → Backend code broken
- 404 API Not Found → Route/controller missing
- Network timeouts → Service down or slow
- Empty data when should have data → API returning wrong data
- Console errors preventing rendering → Frontend bugs

### Step 2: Fix Each Critical Issue

**For Backend 500 Errors:**

1. Check Laravel logs:
```bash
./vendor/bin/sail logs | grep ERROR
```

2. Find the failing endpoint:
   - Look at network tab in investigation output
   - Check `routes/api.php` for route
   - Check controller in `app/Domain/*/Http/Controllers/Api/`

3. Fix the bug:
   - Common issues: missing relationships, wrong validation, missing permissions
   - Run backend tests: `./vendor/bin/sail test`
   - Verify fix manually

**For 404 API Errors:**

1. Check if route exists in `routes/api.php`
2. Check if controller method exists
3. Check if middleware is blocking (auth, permissions)
4. Add missing route/controller if needed

**For Data Loading Issues:**

1. Check API response in network tab
2. Verify data exists in database
3. Check API resource transformation
4. Check frontend data fetching (composable, API call)

### Step 3: Re-run Investigation Test

After each fix:
```bash
npx playwright test tests/e2e/investigate-[feature]-testnfix.spec.ts
```

Verify:
- ✅ No more 500 errors
- ✅ Data loads successfully
- ✅ Page renders correctly
- ✅ Console errors reduced/eliminated

### Step 4: Update Todo List

Mark critical errors as fixed in TodoWrite.

**Loop until:** Page loads successfully with data, no blocking errors.

**Deliverable:**
- All critical errors fixed
- Investigation test shows clean output
- Page is in working state

---

## Phase 3: Feature Analysis (Understand Working Feature)

**MANDATORY GATE: Now that page works, understand what it does.**

**Goal:** Document the ACTUAL functionality, not assumptions.

### Step 1: Read Architecture Documentation

Read `docs/architecture.md` and `docs/api.md` to understand:
- Domain structure
- API endpoints
- Component patterns
- Data flow

### Step 2: Document Actual Functionality

Based on investigation test output and manual exploration:

**What users can DO:**
- List all buttons, forms, actions available
- Document workflows (edit, delete, create, etc.)
- Note any special features (bulk operations, filters, sorting)

**What data is DISPLAYED:**
- List all fields shown in list view
- List all fields shown in detail/edit view
- Note relationships (user, content, etc.)
- Document data transformations

**What the API provides:**
- List API endpoints used (from network tab)
- Document request/response format
- Note query parameters (filters, sorting, pagination)

### Step 3: Identify Test Scenarios

Based on actual functionality:
- Happy paths (normal usage)
- Error paths (validation, permissions)
- Edge cases (empty data, maximum values)
- Workflows (multi-step processes)

**Deliverable:**
- Complete feature map in TodoWrite
- List of API endpoints used
- List of UI components involved
- Test scenarios to cover

---

## Phase 4: Test Strategy Design (Coverage Plan)

**MANDATORY GATE: Design comprehensive test coverage.**

Based on Phase 3 analysis, create test plan:

### Required Test Categories

**1. Smoke Test (CRITICAL):**
```
- Page loads without errors
- Data loads from API
- Basic UI renders
- No console errors
```

**2. Data Accuracy Tests (CRITICAL):**
```
For each displayed field:
- Verify correct value from database
- Verify correct formatting
- Verify relationships load correctly
```

**3. CRUD Operation Tests (CRITICAL):**
```
For each operation:
- Create: New record appears in list
- Read: Correct data displayed
- Update: Changes persist and display
- Delete: Record removed from list
```

**4. Workflow Tests:**
```
For multi-step processes:
- Navigation between views
- State persistence
- Success/error messages
```

**5. Error Handling:**
```
- Validation errors
- Permission errors
- Network errors
```

### Test Naming Convention

Use descriptive names that explain what is verified:
- ✅ "Comment list displays correct text and status from database"
- ✅ "Updating comment text persists changes and updates UI"
- ✅ "Deleting comment removes record from list and database"
- ❌ "Comment list works"
- ❌ "Edit test"
- ❌ "Delete button"

**Deliverable:**
- Test plan in TodoWrite
- 8-12 meaningful test cases
- Expected outcomes for each test

---

## Phase 5: Write Comprehensive Tests (For Working Feature)

**MANDATORY GATE: Tests verify WORKING functionality.**

### Template Structure

**Use `.claude/commands/meta/e2e-template.ts` as base.**

### Test File Structure

```typescript
// tests/e2e/feature-[feature-name]-testnfix.spec.ts
import { test, expect } from '@playwright/test';

async function loginAsAdmin(page) {
  // Copy from template
}

test.describe('[Feature Name] E2E Tests', () => {
  test.beforeEach(async ({ page }) => {
    // Capture console for all tests
    page.on('console', msg => {
      if (msg.type() === 'error' &&
          !msg.text().includes('ERR_NAME_NOT_RESOLVED')) {
        console.log('BROWSER ERROR:', msg.text());
      }
    });
  });

  test('Smoke: Page loads and displays data', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('$ARGUMENTS');
    await page.waitForTimeout(2000);

    // Verify page loaded
    await expect(page).toHaveURL('$ARGUMENTS');

    // Verify data container exists
    const dataTable = page.locator('table');
    await expect(dataTable).toBeVisible();

    // Log actual data count
    const rowCount = await page.locator('table tbody tr').count();
    console.log('Data rows found:', rowCount);
  });

  test('Data Accuracy: [Feature] displays correct [field] from database', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('$ARGUMENTS');
    await page.waitForTimeout(2000);

    const rows = page.locator('table tbody tr');
    const count = await rows.count();

    if (count > 0) {
      // Verify first item has correct data
      const firstRow = rows.first();
      const fieldValue = await firstRow.locator('[data-field="..."]').textContent();

      // Verify against known value or pattern
      expect(fieldValue).toBeTruthy();
      console.log('First item field value:', fieldValue);
    } else {
      console.log('No data available - test skipped');
    }
  });

  test('CRUD: Creating [item] adds record to list', async ({ page }) => {
    // Similar pattern - test actual CRUD operations
  });

  // Add remaining tests from Phase 4 plan
});
```

### Writing Guidelines

**For EACH test:**
1. Start with login and navigation
2. Wait for data to load
3. Verify SPECIFIC values (not just presence)
4. Log key data points to console
5. Use conditional logic for empty states
6. Capture meaningful output, not just assertions

**Console Output > Screenshots:**
- Use console.log to report test progress
- Log data counts, values, states
- Only use screenshots when visual verification needed

**Handle Empty Data:**
```typescript
if (dataCount > 0) {
  // Test with data
} else {
  console.log('No data available - test skipped');
}
```

**Deliverable:**
- Complete test file with 8-12 meaningful tests
- Tests that verify ACTUAL data accuracy
- Tests that verify COMPLETE workflows
- Console logging for debugging

---

## Phase 6: Test-Fix Loop (Until 100% Pass)

**MANDATORY: Fix code, never fix tests.**

### Step 1: Run Tests

```bash
npx playwright test tests/e2e/feature-[feature-name]-testnfix.spec.ts
```

### Step 2: Analyze Failures

For EACH failing test:

**Read the error carefully:**
- What was expected?
- What was received?
- What selector failed?
- What console errors appeared?

**Investigate root cause:**
- Is the selector wrong? (Check actual DOM)
- Is the data wrong? (Check API response)
- Is the timing wrong? (Need to wait longer?)
- Is functionality missing? (Need to implement)

### Step 3: Fix Issues

**Priority order:**
1. Fix backend bugs (API returning wrong data)
2. Fix frontend bugs (component not rendering)
3. Adjust test selectors (only if DOM structure is different)
4. Add missing functionality (if feature incomplete)

**Never:**
- Change test expectations to match buggy behavior
- Skip tests
- Accept "mostly working"

### Step 4: Re-run Tests

After each fix:
```bash
npx playwright test tests/e2e/feature-[feature-name]-testnfix.spec.ts
```

### Loop Exit Condition

**Continue until:**
- ✅ ALL tests pass (0 failures)
- ✅ All data verifications are CORRECT
- ✅ No console errors (except external resource failures)
- ✅ No regressions in other features

**Deliverable:**
- All tests passing
- Console output showing 0 failures
- All bugs fixed with code changes documented

---

## Phase 7: Final Verification

**MANDATORY GATE: Comprehensive verification before claiming complete.**

### Step 1: Run Full Test Suite

```bash
# E2E tests
npm run test:e2e

# Frontend tests
npm run test:run

# Backend tests
./vendor/bin/sail test

# Linting
npm run lint

# Type check
npm run type-check

# Build
npm run build
```

**ALL must pass with 0 failures.**

### Step 2: Manual Verification

Open browser and manually test:
- All critical workflows
- Edge cases
- Error scenarios
- Related features (regression check)

### Completion Checklist

**ALL must be TRUE:**
- [ ] All tests pass (0 failures)
- [ ] Tests verify CORRECT data (not just presence)
- [ ] Tests verify COMPLETE workflows
- [ ] No console errors (except external resources)
- [ ] No network errors (500s, 404s)
- [ ] Manual testing confirms feature works
- [ ] No regressions in related features
- [ ] All verification commands pass

**Deliverable:**
- Test output showing 0 failures
- Confirmation all verification passed
- Evidence of manual testing

---

## Phase 8: Documentation

**MANDATORY: Update canonical documentation only.**

### Required Updates

**1. Update `docs/testing.md`:**
```markdown
## [Feature Name] E2E Tests

**Location:** `tests/e2e/feature-[feature-name]-testnfix.spec.ts`

**Coverage:**
- [List functionality tested]
- [List workflows verified]
- [List data accuracy checks]

**Critical Tests:**
- [Test name]: [What it verifies]
- [Test name]: [What it verifies]
```

**2. If bugs were fixed, update `docs/api.md` or relevant docs:**
- Document any API contract clarifications
- Document edge case handling
- Document error responses

**3. Add inline code comments:**
```php
// Fixed: [Brief description]
// Root cause: [Why it failed]
// Test coverage: tests/e2e/feature-X-testnfix.spec.ts
```

### FORBIDDEN

- ❌ Do NOT create `docs/TESTNFIX_REPORT_*.md`
- ❌ Do NOT create any dated summary files
- ❌ Do NOT create throwaway documentation

**Deliverable:**
- Updated canonical docs only
- Inline comments for significant fixes

---

## Final Report

**Only after ALL phases complete:**

```
Feature fully tested and working. Evidence:

**Investigation:**
- Initial state: [What was found in Phase 1]
- Critical errors fixed: [List from Phase 2, if any]

**Tests:**
- Total: [X] tests
- Passing: [X] tests
- Failing: 0 tests

**Coverage:**
- [List functionality tested]
- [List data accuracy verified]
- [List workflows validated]

**Fixes Applied:**
- [file:line] - [Brief description]

**Verification:**
- All test suites pass
- Manual testing confirms feature works
- No regressions detected
```

---

## Key Principles

1. **INVESTIGATE FIRST**: Understand actual state before writing tests
2. **FIX CRITICAL ERRORS EARLY**: Don't test broken functionality
3. **TEST AFTER FIXING**: Write tests for working features
4. **BROWSER CONSOLE IS TRUTH**: Read error messages carefully
5. **NO GUESSING**: Every decision based on evidence
6. **FIX CODE, NOT TESTS**: Tests define correct behavior
7. **100% OR NOTHING**: No partial completion

**Success = Feature works perfectly, proven by comprehensive tests.**
