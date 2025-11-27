---
description: Laravel 12 + Vue 3 style guide enforcement audit with actionable fixes.
---

# Laravel 12 + Vue 3 Style Guide Audit

**Purpose:** Enforce project style guide and architectural patterns. Catch violations before they accumulate.

## Phase 0: Initialize Audit

1. Use mcp__mcp_docker__sequentialthinking to plan scope:
   - Which modules/files to review
   - Focus areas (if user specified)
   - Priority rules (security first, then architecture, then style)

2. Use TodoWrite to track violations:
   - Will add findings as discovered
   - Prioritize by severity
   - Track fixes

Now audit this repository for violations of the style rules defined below and return precise, actionable refactoring instructions.

=== OPERATING PRINCIPLES (IMMUTABLE) ===
1. **Scope Discipline:** Focus on first-party source code in `app/`, `bootstrap/`, `config/`, `database/`, `routes/`, `resources/`, `lang/`, `public/` (besides compiled assets), `tests/`, and `packages/`. Ignore generated artifacts (`storage/`, `public/build/`, `vendor/`, `node_modules/`, compiled caches, coverage reports).
2. **Evidence-Backed Findings:** Cite exact file paths and line ranges pulled from real code. Do not infer or speculate without seeing source.
3. **Actionable Guidance:** Every violation must produce concrete remediation steps that align with the governing rule and explain the expected outcome.
4. **Adherence Guard:** Surface missing context or blocking information instead of guessing when a rule cannot be evaluated.

=== STYLE GUIDE CONTRACT ===
Treat each statement below as an enforceable rule. This prompt is self-contained—do not look elsewhere for guidance. When scanning code, map every finding to one (or more) rule IDs:

#### Core Architecture
- **CA1 Layered boundaries:** Controllers stay thin; application/service classes host orchestration; domain logic and policies live outside views/components.
- **CA2 Dependency flow:** Dependencies move inward (controllers → services → domain); forbid reverse coupling (e.g., Blade/Vue importing domain internals directly).
- **CA3 Config separation:** Runtime configuration, feature flags, secrets, and build-time environment variables resolve via `config/*.php`, `.env`, and Vite/TypeScript env declarations—never hard-coded in source.
- **CA4 Observability:** Critical path jobs, HTTP actions, async flows, and frontend feature entry points emit structured logs/metrics and wire telemetry events with privacy-aware payloads.

#### Laravel Backend Conventions
- **LB1 Validation & authorization:** HTTP controllers delegate request validation to Form Request classes and enforce policies/gates before mutating state.
- **LB2 Database integrity:** Eloquent queries use scopes/builders for reuse, guard against N+1 access with eager loading, and wrap multi-write operations in transactions.
- **LB3 Resource shaping:** API responses serialize through `Resource` classes or typed DTOs; avoid leaking raw models or hidden attributes.
- **LB4 Queue & scheduling:** Jobs, listeners, and scheduled tasks remain idempotent, handle retries gracefully, capture failure context in logs, and manage lifecycle concerns (`dispatchAfterCommit`, `withoutOverlapping`, timeout/backoff) explicitly.
- **LB5 Blade hygiene:** Blade views remain presentational, rely on view composers or view models for heavy logic, and escape output by default.
- **LB6 Caching & indexing:** Cache usage declares TTL/invalidation strategy, tags, and warmup; database schema changes add appropriate indexes and cover query patterns noted in code/docs.

#### Vue Frontend Practices
- **VF1 Composition discipline:** `setup()` blocks stay focused; shared logic lives in composables within `resources/scripts` (or equivalent) with clear inputs/outputs.
- **VF2 State management:** Use Pinia/store modules or Vue reactivity utilities for cross-component state; avoid mutating global singletons or ad-hoc event buses.
- **VF3 Template accessibility:** Templates keep semantic structure, ensure form controls have labels, and wire ARIA attributes for dynamic content.
- **VF4 Async UX:** Remote calls expose loading/error states, avoid silent failures, and cancel/cleanup long-running watchers where needed.
- **VF5 Performance safeguards:** Leverage computed properties/memoization, key list rendering correctly, and split large components with dynamic imports where appropriate.
- **VF6 Internationalization:** UI strings route through the i18n layer with locale fallbacks, date/number formatting utilities, and translation keys documented for copy updates.
- **VF7 Telemetry & analytics:** User interactions and error states emit analytics/telemetry events via approved adapters with opt-out and consent handling baked in.

#### API & Data Contracts
- **AD1 Consistent contracts:** HTTP routes, controllers, and Vue clients agree on payload shape; breaking changes versioned or negotiated.
- **AD2 Serialization rules:** Dates, money, and enums serialize consistently (use value objects/casts/resources); avoid pushing presentation formatting to consumers.
- **AD3 Validation feedback:** Surface actionable validation errors to the frontend with machine-readable codes/messages.
- **AD4 Environment parity:** Document env-specific endpoints/feature switches; ensure Vite/TypeScript build configs mirror backend expectations across staging, preview, and production.

#### Security & Resilience
- **SR1 Input hardening:** Validate and sanitize all external input (HTTP, CLI, queue payloads); guard against mass-assignment via `$fillable`/`$guarded`.
- **SR2 Authorization guarantees:** Apply `authorize()`/policies/scopes to every privileged action; avoid exposing IDs without ownership checks.
- **SR3 Error handling:** Capture unexpected exceptions, return safe responses, and never leak stack traces or secrets outside trusted logs.
- **SR4 CSRF/session hygiene:** Ensure state-changing routes enforce CSRF protection and session regeneration on login/logout flows.

#### Testing Expectations
- **T1 Feature coverage:** HTTP routes, controllers, Livewire/Inertia endpoints, and queues carry Feature tests covering success and failure paths.
- **T2 Unit boundaries:** Domain services, value objects, and composables ship with deterministic unit tests.
- **T3 Frontend reliability:** Vue components/composables use Vitest/Jest + `@vue/test-utils`; complex flows have Cypress/Playwright end-to-end checks.
- **T4 Database lifecycle:** Migrations/seeds tested for reversibility; factories kept in sync with schema.

#### Documentation & Knowledge Sharing
- **DO1 ADR discipline:** Significant architectural decisions and third-party integrations documented in `/docs` or ADR logs.
- **DO2 Runbook currency:** README/setup docs reflect current env vars, queues, scheduled commands, and build steps.
- **DO3 API references:** Public endpoints and events documented for frontend consumption and partner teams.

#### Developer Experience
- **DX1 Code style:** PHP adheres to PSR-12 and Laravel Pint; JS/TS/Vue follow ESLint/Prettier config; no manual formatting overrides.
- **DX2 Type safety:** Prefer `declare(strict_types=1)` in PHP entry points, strong return types, typed properties, and TypeScript (or JSDoc) for Vue code.
- **DX3 Build hygiene:** Vite/Webpack configs avoid dead aliases, redundant plugins, or slow dev-server options; keep bundle budgets visible.
- **DX4 Tooling boundaries:** Local dev tooling stays isolated from production builds and is toggled via config/feature flags.
- **DX5 Type tooling gates:** CI runs `npm run lint`, `vue-tsc --noEmit`, and tailored TypeScript/Vue analyzers; failing lint/type-checks block merges.
- **DX6 Vite verification:** `npm run build` (or equivalent) exercises production Vite config, validates asset hashing/code-splitting, and documents SSR/Inertia nuances including required env vars.

#### Code Review Checklist Compliance
- **Q1 Respect domain boundaries and layering.**
- **Q2 Validate, authorize, and handle errors deterministically.**
- **Q3 Address performance characteristics (queries, rendering, bundle size).**
- **Q4 Ensure observability and security mitigations are in place.**
- **Q5 Tests cover happy and unhappy paths across layers.**
- **Q6 Documentation reflects system behavior post-change.**

#### Common Mistakes (Explicit Anti-Patterns)
- **M1 Fat controllers:** Business rules and branching logic parked inside controllers, jobs, or views.
- **M2 N+1/iterative queries:** Looping over collections and triggering database calls without eager loading or batching.
- **M3 Hidden side effects:** Singletons, facades, or global events mutating shared state without tests.
- **M4 Spaghetti reactivity:** Vue watchers/refs with unclear lifecycle, leading to memory leaks or inconsistent UI.
- **M5 Untested concurrency:** Queue jobs or scheduled commands lacking retry/idempotency tests.
- **M6 Unhardened forms:** Missing CSRF, unsanitized HTML, or leaking validation errors without context.

#### Continuous Improvement
- **I1 Retire legacy code paths once feature flags stabilize.**
- **I2 Consolidate config and environment usage when duplication emerges.**
- **I3 Automate repetitive operational tasks (queue health checks, cache priming) and document ownership.**

=== REVIEW WORKFLOW ===
1. **Orientation:** Map the change set to the relevant rules above (routes/service layer, Vue components, config, database migrations, tests, docs).
2. **Targeted Recon:** Use repo search to find high-risk areas—heavy controllers, complex queries, large Vue components, background jobs, build config, migrations, test coverage gaps.
3. **Deep Inspection:** For each candidate file, confirm violations with precise line ranges and explain which rule is broken and why.
4. **Refactor Design:** Outline the minimal fix: structural refactors, query optimization, validation hardening, state management cleanup, test additions, doc updates. Note cascading impacts.
5. **Prioritization:** Assign severity (`blocker`, `high`, `moderate`, `info`) based on production risk, security impact, data integrity, or maintainability.
6. **Verification Plan:** Recommend specific tests/commands/monitoring to validate the fix (e.g., `phpunit`, `pest`, `npm run test`, `npm run lint`, `vue-tsc --noEmit`, `npm run build`, `php artisan queue:listen` in staging).

=== OUTPUT FORMAT ===
Respond in the following fenced block. All arrays must be non-empty when present; omit sections that truly have zero findings.

```codereview
{
  "summary": {
    "scope": "short description of inspected areas",
    "overall_risk": "blocker|high|moderate|low",
    "key_themes": ["brief bullets of recurring issues"]
  },
  "violations": [
    {
      "rule_id": "e.g. LB2",
      "severity": "blocker|high|moderate|info",
      "file": "relative/path.php",
      "lines": "L45-L78",
      "evidence": "Quote or paraphrase of the problematic code/behavior",
      "impact": "Why this matters (performance, correctness, DX)",
      "refactor_plan": {
        "steps": ["ordered list of changes"],
        "aligned_rule": "Rule(s) the fix satisfies",
        "verification": ["tests or checks to run"]
      }
    }
  ],
  "follow_up": {
    "fast_wins": ["1-2 small QoL improvements"],
    "bigger_initiatives": ["Larger refactors or docs updates to schedule"],
    "open_questions": ["Any uncertainties requiring human clarification"]
  }
}
```

=== QUALITY GATE (CHECK BEFORE RETURNING) ===
- ✓ Each violation maps to at least one rule ID listed above.
- ✓ Every `refactor_plan.steps` entry is actionable (mentions concrete code changes, tests, or documentation updates).
- ✓ All cited files and line ranges exist and correspond to real evidence gathered.
- ✓ `overall_risk` reflects the highest severity among findings; set to `low` only if no violations.
- ✓ If no violations found, return an empty `violations` array but still provide `summary` and `follow_up` (with `fast_wins` suggestions).

If any check fails, revise once. If still failing due to missing data, return a refusal with the format `{"error": "styleguide review blocked", "reason": "..."}` inside the `codereview` fence.

## Phase 1: Add Findings to TodoWrite

**After completing the audit:**

1. Add each blocker/high severity violation as a TodoWrite task:
   - Task description: "[Severity] [Rule ID] [File]: [Issue]"
   - Example: "Blocker LB1 app/Http/Controllers/PaymentController.php:78: Missing authorization check before charge"

2. Prioritize tasks:
   - Blockers first (security, data integrity)
   - High severity second (correctness, architecture)
   - Moderate can be batched

## Phase 2: Actionable Summary

**Provide clear next steps:**

1. **Blockers** (must fix before deploy):
   - List specific violations
   - Provide exact refactoring steps
   - Estimated time per fix

2. **High severity** (must fix soon):
   - Grouped by rule category
   - Priority order

3. **Moderate/Info** (technical debt):
   - Can be scheduled for future work
   - Group related violations

4. **Documentation needs:**
   - If violations reveal gaps → Note which canonical docs need updates
   - NO creation of review reports or analysis documents

**Final Output Template:**
"Code review complete. Found:
- X Blocker violations (security/data integrity)
- Y High severity violations (architecture/correctness)
- Z Moderate/Info violations (maintainability)

All blockers/high severity added to TodoWrite. See tasks for refactoring steps.

Overall Risk: [blocker|high|moderate|low]"

