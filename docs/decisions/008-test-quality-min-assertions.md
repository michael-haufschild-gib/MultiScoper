# ADR-008: Test Quality Lint — Minimum Assertions Stays at One

## Status

Accepted

## Context

`scripts/test_quality_lint.py` enforces several rules on every `TEST` / `TEST_F` body in `tests/`:

1. **Zero assertions** → violation (line 239 of the lint script).
2. **All assertions are existence-only** — nullptr checks, `!x.empty()`, `x.isValid()`, `x.hasValue()`, `x.size() > 0`, and similar — and the test lacks a behavioral indicator → violation (lines 47–68, 249).
3. **Tautological assertions** that can never fail — `EXPECT_EQ(x, x)`, `EXPECT_TRUE(true)`, etc. — always a violation regardless of count (lines 72–86, 227).
4. **Fewer than `--min-assertions` assertions** and no behavioral indicator → violation (line 260).

`BEHAVIORAL_INDICATORS` (lines 89–99) list patterns that mean a test is legitimately exercising behavior even with a single assertion: `EXPECT_DEATH`, `EXPECT_THROW`, `EXPECT_ANY_THROW`, `EXPECT_THAT`, `.processBlock(`, `.process(`, `.paint(`, `.resized(`.

`MULTISCOPER_MIN_ASSERTIONS_PER_TEST` is currently `1` (`CMakeLists.txt:99`).

An earlier audit recommendation proposed raising this to `2` on the grounds that it would catch shallow tests. That recommendation is now withdrawn.

## Decision

`MULTISCOPER_MIN_ASSERTIONS_PER_TEST` remains at `1`. The primary defense against shallow tests is the combination of:

- the existence-only check (rule 2),
- the tautology check (rule 3),
- the behavioral-indicator allowlist, which correctly excludes legitimate single-assertion tests from rule 4.

Raising the minimum to 2 would force every death test, every exception test, every simple paint test, and every simple processBlock test to gain a redundant second assertion — producing lower-quality tests, not higher-quality ones. The minimum-count rule exists only as a backstop for tests that slip past rules 1–3; keeping it at 1 means rules 1–3 do the work.

## Consequences

- Single-assertion tests that exercise real behavior (death tests, throw tests, DSP processing tests, paint/resized tests) remain legal.
- Existence-only assertions (`EXPECT_NE(ptr, nullptr)` alone) remain violations via rule 2, independently of count.
- Tautological assertions (`EXPECT_EQ(x, x)`) remain violations via rule 3.
- The lint script's behavior is load-bearing; modifications to `EXISTENCE_PATTERNS`, `TAUTOLOGY_PATTERNS`, or `BEHAVIORAL_INDICATORS` require review and accompanying tests in `tests/` that exercise the new classification.

## Alternatives rejected

- **Raise minimum to 2.** Rejected — produces lower-quality tests by forcing redundant assertions on legitimately single-assertion behavioral tests.
- **Remove the minimum-count rule entirely.** Rejected — keeps a backstop for tests that somehow slip past rules 1–3.
- **Per-fixture minimum assertions.** Rejected as complexity without evidence it catches more defects than the current rule set.

## References

- Lint implementation: `scripts/test_quality_lint.py`
- CMake wiring: `CMakeLists.txt:99-100`, `CMakeLists.txt:346-356`
- Pre-commit hook: `scripts/pre-commit:110-128`
