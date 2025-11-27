---
description: STRICT workflow for coding tasks that prioritizes understanding, reuse, impact analysis, tests-first, and side-effect-free delivery
---

# STOP: Understand, Reuse, Plan — THEN Code

This is the non-negotiable workflow for any coding task (feature, refactor, or change). Follow it step-by-step. Do not skip steps. Do not write duplicate code.

Project context: Laravel 12 backend (PHP 8.4+, Eloquent, Services, Policies), Vue 3 + TypeScript frontend (Pinia, Vite), tests via PHPUnit/Vitest/Playwright, code quality via Pint/ESLint/TS.

## Phase 0: Initialize

**MANDATORY GATE: Before proceeding, complete both steps.**

1. Use mcp__mcp_docker__sequentialthinking to analyze:
   - User request intent and explicit requirements
   - Implicit constraints and assumptions
   - Task scope and potential complexity
   - Risk areas requiring extra attention

2. Use TodoWrite to create tasks for all phases:
   1. Phase 1 — Task Intake & Acceptance Criteria (No Coding)
   2. Phase 2 — Discover & Reuse (Anti-Duplication Gate)
   3. Phase 3 — Impact Analysis (Safety Gate)
   4. Phase 4 — Design & Plan (Tests Included)
   5. Phase 5 — Tests First (or in lockstep)
   6. Phase 6 — Implement Incrementally
   7. Phase 7 — Verify in Runtime (Not Just Tests)
   8. Phase 8 — Regression & Side-Effect Checks
   9. Phase 9 — Documentation (Following CIB-003)
   10. Phase 10 — Final Verification

## Phase 1 — Task Intake & Acceptance Criteria (No Coding)

1. Restate the task in your own words and list explicit and implicit requirements.
2. Capture constraints/assumptions (env, data model, API contracts, performance/security considerations).
3. Define acceptance criteria as verifiable checks (user-visible behavior, API responses, edge cases).
4. Identify stakeholders and where changes appear (admin vs user frontend, API, services, migrations).

Deliverable: a short Task Summary + Acceptance Criteria block in the PR description or ADR.

## Phase 2 — Discover & Reuse (Anti-Duplication Gate)

**MANDATORY GATE: DO NOT proceed to Phase 3 without completing this discovery.**

Goal: Reuse existing capabilities. Never reimplement helpers/services/components that already exist.

**Step 1: Understand Current Project Structure**

**FIRST: Read `docs/architecture.md`** to understand:
- Current domain/module organization
- Where business logic lives
- Where base classes and traits are located
- Architectural patterns in use
- Naming conventions

**Step 2: Search Existing Code**

Search in **standard framework locations**:
- `app/Domain/*` - Models, controllers, resources, requests, policies organized by domain
- `app/Support` — Supporting functionality like BBCode, Validators, Mail functions, helpers
- `app/Http/Controllers/*` — Controllers (check for base controller)
- `app/Http/Middleware/*` — Middleware
- `app/Providers/*` — Service providers
- `routes/*.php` — Route definitions
- `config/*` — Configuration files
- `database/migrations/*` — Database schema
- `database/factories/*` — Model factories
- `database/seeders/*` — Database seeders
- `tests/*` — All tests
- `resources/views/*` — Blade templates
- `resources/admin` — Admin components and styles
- `resources/frontend` — Frontend components and styles
- `resources/shared` — Shared components and styles

Search in **project-specific locations** (from `docs/architecture.md`):
- Business logic layer (services, actions, use cases - check architecture doc for location)
- Domain/module-specific code (check architecture doc for domain structure)
- API resources and transformers (check architecture doc for location)
- Form requests and validation (check architecture doc for location)
- Shared utilities and traits (check architecture doc for location)
- Vue components/composables/stores (check architecture doc for frontend structure)

**Step 3: Identify Base Classes and Traits**

**Check `docs/architecture.md`** for base classes and reusable traits.

Common patterns to look for:
- Base controller classes (extend these for new controllers)
- Base resource/transformer classes (extend these for API responses)
- Base request/validation classes (extend these for form validation)
- Reusable traits for common functionality (pagination, filtering, etc.)
- Shared utilities and helper classes

**Before writing new functionality, search for:**
- Existing trait that provides this behavior
- Existing base class method that does this
- Existing utility function that handles this
- Similar implementation to learn from

**Deliverable: Reuse Proof**
List at least 3 existing symbols/files you will leverage, OR provide explicit justification why new code is necessary.

**RED FLAG:** If creating new helper/service/component without checking for existing implementation → STOP → Go back to Step 1.

## Phase 3 — Impact Analysis (Safety Gate)

Enumerate potential blast radius before changing code:
- Data models/relations, migrations, seeds.
- Public API contracts (routes, request/response shapes).
- Caching and queues (cache tags, invalidation, jobs, workers).
- Authorization (Spatie permissions, policies, guards).
- Performance/SEO/analytics implications.
- Frontend state (Pinia), SSR/Blade templates, router.
- Backward compatibility and rollout (feature flags, env toggles).

Deliverable: Impact Matrix — list affected modules/files and planned safeguards (feature flag, fallback, migration order, rollback).

## Phase 4 — Design & Plan (Tests Included)

Write a minimal design/plan in TodoWrite before coding:
- Contract: inputs/outputs, data shapes, errors, success criteria.
- Edge cases: empty/null, permission denied, timeouts, large payloads.
- Change list: exact files to touch, functions to add/modify, schema changes.
- Test plan: which tests to add/update (unit, feature/integration, e2e) and what they assert.
- Observability: logs/metrics you’ll add; how to validate in UI/API.

Deliverable: Implementation Plan + Test Plan in PR/ADR.

## Phase 5 — Tests First (or in lockstep)

Add or update tests before or alongside implementation:
- Backend: PHPUnit Feature/Unit tests for services/controllers/policies.
- Frontend: Vitest for components/composables/stores.
- E2E: Playwright for critical user flows; use only `data-testid` selectors. Use `.claude/commands/meta/e2e-template.ts` as template.
- Fixtures/factories: seed minimal test data; prefer factories over hard-coded IDs.

Quality gates to run locally:
```bash
# Frontend
npm run lint && npm run type-check && npm run test:run

# Backend
./vendor/bin/pint --test && php artisan test

# E2E (if UI flow affected)
npm run test:e2e
```

## Phase 6 — Implement Incrementally

**Pre-Implementation Checklist**
1. ☐ Task restated with acceptance criteria.
2. ☐ Reuse Proof lists existing symbols to leverage.
3. ☐ Impact Matrix drafted with safeguards.
4. ☐ Implementation Plan + Test Plan written.
5. ☐ Tests scaffolded (or updated) for happy path and edge cases.

**Guidelines:**
- Keep changes minimal, cohesive, and typed. Maintain `declare(strict_types=1)` in PHP.
- Reuse existing services/composables. Respect layering (Controller → Service → Model/Policy).
- Add `data-testid` on new interactive UI elements.
- Maintain docs/docblocks, follow style (Pint/ESLint/Prettier), and keep public APIs stable.
- Guard risky changes behind feature flags where appropriate.

## Phase 7 — Verify in Runtime (Not Just Tests)

Run the real user flow and capture evidence:
- Browser or Playwright headed run of the scenario
- **Screenshots of working feature** → Save to `screenshots/` folder
- Console logs (no errors)
- Network tab (successful requests)
- Confirm cache/queue side-effects (e.g., generated images, emails queued)

**Standard artifact locations:**
- Screenshots: `screenshots/`
- Test results: `test-results/`
- Migrations: `database/migrations/`
- Factories: `database/factories/`
- Seeders: `database/seeders/`

Success requires both green tests AND correct runtime behavior.

## Phase 8 — Regression & Side-Effect Checks

1. Re-run dependent flows identified in Impact Analysis.
2. Validate authorization boundaries and rate limits.
3. Confirm performance hasn’t degraded (basic timings) and SEO/meta remain intact.
4. Verify cache invalidation and queue job health; check logs for new errors.

## Phase 9 — Documentation (Following CIB-003)

**MANDATORY GATE: Follow CIB-003 documentation discipline.**

**BEFORE creating or editing ANY .md file:**

```
STOP - Do not touch any .md file yet

1. Check: "Is this file in the canonical documentation registry?"
   - docs/architecture.md — System architecture, domains, patterns
   - docs/api.md — API documentation, endpoints
   - docs/database.md — Schema, migrations, relationships
   - docs/testing.md — Testing strategy
   - docs/deployment.md — Deployment process

   IF YES → Proceed to update it
   IF NO → Creating new file → ASK user permission first

2. Check: "Does this involve dated/summary/status/analysis content?"
   IF YES → STOP → Update canonical doc instead
```

**Documentation Tasks:**
1. **Update canonical documentation:**
   - If API contracts changed → Update `docs/api.md`
   - If architecture/domains changed → Update `docs/architecture.md`
   - If database schema changed → Update `docs/database.md`
   - If testing approach changed → Update `docs/testing.md`

2. **Update inline documentation:**
   - Add/update PHPDoc for new/modified PHP classes/methods
   - Add/update TSDoc for new/modified TypeScript functions/components
   - Update README snippets if public API changed

3. **Add ADR context** (in relevant canonical doc):
   - What changed
   - Why this approach was chosen
   - Alternatives considered
   - Trade-offs accepted

**FORBIDDEN:**
- ❌ Creating `docs/[FEATURE]_SUMMARY_2025.md`
- ❌ Creating `docs/[MODULE]_ANALYSIS.md`
- ❌ Creating `docs/STATUS_REPORT.md`
- ❌ Any documentation with dates in filename or title

**Verification:**
- [ ] Only canonical docs updated (or user gave explicit permission for new file)
- [ ] No summary/analysis/status documents created
- [ ] Inline docs (PHPDoc/TSDoc) added for new code

## Phase 10 — Final Verification (Before Claiming Complete)

**MANDATORY GATE: verification-before-completion skill applies here.**

**NO COMPLETION CLAIMS without running these commands and seeing PASS:**

**Backend Verification:**
```bash
# Code style
./vendor/bin/pint --test

# Static analysis
./vendor/bin/phpstan analyse --memory-limit=4G

# Tests
php artisan test
# OR for Laravel Sail
./vendor/bin/sail test
```

**Frontend Verification:**
```bash
# Linting
npm run lint

# Type checking
npm run type-check

# Tests
npm run test:run

# Build (if UI changed)
npm run build
```

**E2E Verification (if user flows changed):**
```bash
npm run test:e2e
```

**Completion Checklist (ALL must be TRUE):**
- [ ] All quality gates PASS (see commands above with actual output)
- [ ] PHPUnit/Vitest/Playwright tests PASS (0 failures)
- [ ] Test coverage not degraded
- [ ] Runtime flow validated with screenshots and console logs
- [ ] No code duplication introduced (verified in Phase 2)
- [ ] Base classes/traits used where appropriate
- [ ] Canonical documentation updated per CIB-003
- [ ] No summary/analysis/status documents created
- [ ] Inline PHPDoc/TSDoc added for new code
- [ ] No tests skipped or disabled
- [ ] No errors in browser console (for frontend changes)
- [ ] API responses match expected format (for backend changes)

**RED FLAGS (require fixing before claiming complete):**
- Any test failures or skipped tests
- Linter or type-check errors
- Build failures
- Browser console errors
- Duplicated code that should use traits/services
- New .md files created without user permission

**Final Statement Template:**
"Task complete. Evidence:
- [Command output showing all tests pass]
- [Screenshot of working feature]
- [List of canonical docs updated]"

Remember: Evidence before claims. Always.
