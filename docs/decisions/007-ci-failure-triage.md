# ADR-007: CI Failure Triage — Human Investigation First

## Status

Accepted

## Context

A `copilot-auto-fix.yml` GitHub Actions workflow was previously configured to automatically file a GitHub issue and assign it to `copilot-swe-agent` whenever the main CI workflow failed. The assigned AI agent would attempt to investigate logs, identify root cause, and submit a fix PR.

This pattern conflicts with the engineering discipline applied elsewhere in the project. Oscil enforces:

- strict C++ warnings (`-Werror`) with clang-tidy in the critical path,
- a pre-commit hook with 8 independent lint gates (`scripts/pre-commit`),
- RealtimeSanitizer on audio-path tests (`CMakePresets.json:121-131`),
- pluginval strictness 10 on macOS/Windows (`build_and_test.yml:287-295`),
- a custom test-quality lint that rejects shallow or tautological tests (`scripts/test_quality_lint.py`),
- an architecture-boundary lint (`scripts/architecture_lint.py`), and
- ADR-005 codifying layer enforcement.

In this environment, a CI failure is a signal that the project's quality gates caught something the author missed. It is precisely the moment that demands deliberate human judgment: is this a genuine bug, a flaky test, a toolchain drift, a platform-specific issue? Delegating that judgment to an AI agent produces three failure modes:

1. **Shallow fixes.** The agent modifies the symptom (disable the test, widen the threshold, suppress the warning) rather than the cause.
2. **Silent suppressions.** NOLINT directives, `// FIXME` comments, and skip-marks accumulate because they make CI green fastest.
3. **Loss of institutional memory.** When the human who caused the failure never investigates it, the root cause is never understood, and the same failure recurs elsewhere.

## Decision

CI failures are triaged by a human. AI coding agents are not auto-assigned to CI failures on any production-path branch (`main`, `develop`, feature branches, release branches).

The `copilot-auto-fix.yml` workflow is removed.

### Narrow exceptions

AI auto-fix is acceptable only when all of the following hold:

- the failure is on a Dependabot, Renovate, or equivalent dependency-update branch;
- the fix is mechanical (bump a version string, update a lockfile, regenerate compile commands);
- the fix does not touch `src/`, `include/`, `tests/` (excluding fixture updates), `scripts/`, `.github/workflows/`, or `CMakeLists.txt` beyond the dep declaration;
- a human reviewer approves the resulting PR before merge.

No such exception is implemented at this time. Adding one requires a follow-up ADR.

## Consequences

- Authors investigate their own CI failures. The feedback loop stays tight between "I broke it" and "I understand why."
- The pre-commit hook and the CI lint gate share identical configuration, so most failures surface locally before push.
- When an AI coding agent is used (e.g., Claude Code), it is invoked by a human on the failing commit — not triggered autonomously by the workflow runner.
- The removed workflow file can be restored from git history if a narrow, audited use case arises.

## Alternatives rejected

- **Restrict the auto-fix to `dependabot/*` branches only.** Does not address the root objection that the mechanism exists. Deferred until there is concrete demand.
- **Keep the workflow but require human review before PR merge.** Still routes the first diagnostic pass to an AI, which is exactly what this ADR refuses to do.

## References

- Deleted workflow: `.github/workflows/copilot-auto-fix.yml` (removed in the same change that landed this ADR).
