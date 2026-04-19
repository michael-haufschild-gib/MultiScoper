#!/usr/bin/env python3
"""
Test quality lint for GoogleTest C++ files.

Detects shallow / "vibecoded" test patterns that verify existence
instead of behavior:

  1. Tests with zero assertions (empty tests)
  2. Tests where ALL assertions are existence-only
     (nullptr checks, .empty(), hasValue, isDefined, isValid with
      no subsequent behavioral assertions)
  3. Tests with only a single assertion (configurable)

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator, List, Optional, Sequence, Set, Tuple

# Reuse the comment/string stripper with scripts/forbidden_patterns_lint.py.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from lint_utils import strip_comments_and_strings as _strip_comments_and_strings  # noqa: E402

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}

# Matches TEST(Suite, Name) or TEST_F(Fixture, Name) opening
TEST_MACRO_RE = re.compile(
    r"^\s*TEST(?:_F|_P)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)"
)

# All GTest assertion macros (EXPECT_* and ASSERT_*)
ASSERTION_RE = re.compile(
    r"\b(?:EXPECT_|ASSERT_)"
    r"(?:EQ|NE|LT|LE|GT|GE|TRUE|FALSE|STREQ|STRNE|STRCASEEQ|STRCASENE"
    r"|FLOAT_EQ|DOUBLE_EQ|NEAR|THROW|NO_THROW|ANY_THROW|DEATH"
    r"|DEATH_IF_SUPPORTED|NO_FATAL_FAILURE|PRED_FORMAT[12]|THAT)\s*\("
)

# Existence-only assertion patterns — these verify something EXISTS
# but don't verify BEHAVIOR.
EXISTENCE_PATTERNS: List[re.Pattern] = [
    # EXPECT_NE(x, nullptr) / ASSERT_NE(x, nullptr)
    re.compile(r"\b(?:EXPECT|ASSERT)_NE\s*\([^,]+,\s*nullptr\s*\)"),
    # EXPECT_NE(nullptr, x)
    re.compile(r"\b(?:EXPECT|ASSERT)_NE\s*\(\s*nullptr\s*,"),
    # EXPECT_TRUE(x != nullptr) / EXPECT_TRUE(x)
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*\w+\s*!=\s*nullptr\s*\)"),
    # MULTILINE so `$` anchors at end-of-line when the assertion is wrapped
    # across lines and matched via .match(sanitized, pos).
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*\w+\s*\)$", re.MULTILINE),
    # EXPECT_FALSE(x == nullptr)
    re.compile(r"\b(?:EXPECT|ASSERT)_FALSE\s*\(\s*\w+\s*==\s*nullptr\s*\)"),
    # EXPECT_TRUE(!x.empty()) / EXPECT_FALSE(x.empty())
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*!\s*\w+\.empty\s*\(\s*\)\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_FALSE\s*\(\s*\w+\.empty\s*\(\s*\)\s*\)"),
    # EXPECT_TRUE(x.isValid()) / EXPECT_TRUE(x.has_value())
    re.compile(
        r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*\w+\."
        r"(?:isValid|hasValue|has_value|isDefined|isInitialized|isActive|isEnabled|isLoaded)\s*\(\s*\)\s*\)"
    ),
    # EXPECT_GT(x.size(), 0) — checks non-emptiness, not behavior
    re.compile(r"\b(?:EXPECT|ASSERT)_GT\s*\(\s*\w+\.(?:size|count|length)\s*\(\s*\)\s*,\s*0\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_GE\s*\(\s*\w+\.(?:size|count|length)\s*\(\s*\)\s*,\s*1\s*\)"),
    # EXPECT_THAT(x, NotNull()) / Not(IsNull()) / IsEmpty() / Not(IsEmpty())
    # — matchers that check structural properties, not behavior.
    re.compile(
        r"\b(?:EXPECT|ASSERT)_THAT\s*\([^,]+,\s*(?:::)?(?:testing::)?"
        r"(?:NotNull\s*\(\s*\)|Not\s*\(\s*(?:::)?(?:testing::)?IsNull\s*\(\s*\)\s*\))\s*\)"
    ),
    re.compile(
        r"\b(?:EXPECT|ASSERT)_THAT\s*\([^,]+,\s*(?:::)?(?:testing::)?"
        r"(?:IsEmpty\s*\(\s*\)|Not\s*\(\s*(?:::)?(?:testing::)?IsEmpty\s*\(\s*\)\s*\))\s*\)"
    ),
]

# Structural (existence-only) matchers inside EXPECT_THAT / ASSERT_THAT.
# These are matched against the *second argument* of the THAT call — resolved
# via balanced-paren walking so matchers whose first argument contains commas
# (e.g. ``EXPECT_THAT(makeVec(1, 2), IsEmpty())``) still classify correctly.
EXPECT_THAT_EXISTENCE_MATCHER_RE = re.compile(
    r"^\s*(?:::)?(?:testing::)?"
    r"(?:NotNull\s*\(\s*\)"
    r"|Not\s*\(\s*(?:::)?(?:testing::)?IsNull\s*\(\s*\)\s*\)"
    r"|IsEmpty\s*\(\s*\)"
    r"|Not\s*\(\s*(?:::)?(?:testing::)?IsEmpty\s*\(\s*\)\s*\))\s*$"
)

EXPECT_THAT_CALL_RE = re.compile(r"\b(?:EXPECT|ASSERT)_THAT\s*\(")


def _split_expect_that_args(sanitized: str, call_start: int) -> Optional[Tuple[str, list[int], int]]:
    """Given a buffer and the index of ``EXPECT_THAT``/``ASSERT_THAT``, walk
    balanced parens and return ``(body, depth1_commas, close_idx)`` where
    ``body`` is the full call text, ``depth1_commas`` are the absolute offsets
    of every comma seen at depth 1 (arg separators at the outer call level),
    and ``close_idx`` is the offset of the matching close paren.

    Multiple candidate split points are reported (not just the first depth-1
    comma) because only parentheses are tracked here — template ``<A, B>``
    and braced initializer ``{a, b}`` commas will also appear in the list.
    Callers pick the correct split by trying candidates right-to-left (the
    matcher is always the last top-level argument).
    """
    open_paren = sanitized.find("(", call_start)
    if open_paren == -1:
        return None
    depth = 0
    commas: list[int] = []
    i = open_paren
    while i < len(sanitized):
        ch = sanitized[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return sanitized[open_paren + 1 : i], commas, i
        elif ch == "," and depth == 1:
            commas.append(i)
        i += 1
    return None


def _expect_that_is_existence(sanitized: str, call_start: int) -> bool:
    """True iff the EXPECT_THAT/ASSERT_THAT call at ``call_start`` uses one of
    the structural-only matchers (NotNull / Not(IsNull) / IsEmpty / Not(IsEmpty)).

    Candidate split points (all depth-1 commas reported by
    ``_split_expect_that_args``) are tried right-to-left: the matcher is the
    final top-level argument, so the rightmost split that yields a string
    matching the existence-matcher regex is correct. This keeps detection
    robust against commas inside template parameter lists, braced
    initializers, or nested calls in the first argument (e.g.
    ``EXPECT_THAT(std::pair<int, int>{1, 2}, IsEmpty())``).
    """
    parts = _split_expect_that_args(sanitized, call_start)
    if parts is None:
        return False
    _, commas, close_idx = parts
    if not commas:
        return False
    for comma_idx in reversed(commas):
        matcher_arg = sanitized[comma_idx + 1 : close_idx]
        if EXPECT_THAT_EXISTENCE_MATCHER_RE.match(matcher_arg):
            return True
    return False

# Tautological assertion patterns — assertions that can NEVER fail.
# These are worse than shallow tests: they give false confidence.
TAUTOLOGY_PATTERNS: List[re.Pattern] = [
    # EXPECT_EQ(x, x) — comparing a variable to itself
    re.compile(r"\b(?:EXPECT|ASSERT)_EQ\s*\(\s*(\w+(?:\.\w+)*)\s*,\s*\1\s*\)"),
    # EXPECT_TRUE(true) / EXPECT_FALSE(false)
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*true\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_FALSE\s*\(\s*false\s*\)"),
    # EXPECT_EQ(true, true) / EXPECT_EQ(false, false)
    re.compile(r"\b(?:EXPECT|ASSERT)_EQ\s*\(\s*true\s*,\s*true\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_EQ\s*\(\s*false\s*,\s*false\s*\)"),
    # EXPECT_NE(x, x) — always fails, likely a copy-paste error
    re.compile(r"\b(?:EXPECT|ASSERT)_NE\s*\(\s*(\w+(?:\.\w+)*)\s*,\s*\1\s*\)"),
    # EXPECT_GE(x, x) / EXPECT_LE(x, x) — always true
    re.compile(r"\b(?:EXPECT|ASSERT)_GE\s*\(\s*(\w+(?:\.\w+)*)\s*,\s*\1\s*\)"),
    re.compile(r"\b(?:EXPECT|ASSERT)_LE\s*\(\s*(\w+(?:\.\w+)*)\s*,\s*\1\s*\)"),
]

# Assertion-class behavioral indicators. These assertions *are* the behavior
# check (e.g., DEATH/THROW verify an exception path). A test built around
# them is meaningful even with a single assertion and even if *other* on-line
# assertions look existence-only.
#
# EXPECT_THAT / ASSERT_THAT are intentionally NOT listed here: matchers like
# NotNull() and IsEmpty() only check structural properties, so blanket-listing
# THAT would let shallow matcher-only tests bypass the existence-only rule.
ASSERTION_BEHAVIORAL_INDICATORS = [
    re.compile(r"\b(?:EXPECT|ASSERT)_(?:DEATH|DEATH_IF_SUPPORTED|THROW|ANY_THROW|NO_THROW)\b"),
]

# Method-call behavioral indicators. These show the code under test is being
# *exercised*, but they are not assertions. A test that calls `.process()` and
# then only checks `EXPECT_NE(x, nullptr)` is still a shallow test — it runs
# the code but does not verify the behavior. These indicators lift the
# minimum-assertion floor but do NOT bypass the existence-only rule.
METHOD_CALL_INDICATORS = [
    re.compile(r"\.processBlock\s*\("),
    re.compile(r"\.process\s*\("),
    re.compile(r"\.paint\s*\("),
    re.compile(r"\.resized\s*\("),
    re.compile(r"\.mouseDown\s*\("),
    re.compile(r"\.mouseUp\s*\("),
    re.compile(r"\.mouseDrag\s*\("),
    re.compile(r"\.keyPressed\s*\("),
]


@dataclass(frozen=True)
class Violation:
    file: str
    line: int
    suite: str
    name: str
    reason: str


@dataclass
class TestBody:
    suite: str
    name: str
    start_line: int
    lines: List[str] = field(default_factory=list)
    raw_text: str = ""
    _sanitized_text: Optional[str] = None

    @property
    def sanitized_text(self) -> str:
        """raw_text with // and /* */ comments and "..."/'...' string literals
        blanked out. Used by indicator matchers so a commented-out
        ``EXPECT_THROW`` or a string literal containing ``.mouseDown(`` cannot
        trigger a false positive.
        """
        if self._sanitized_text is None:
            self._sanitized_text = _strip_comments_and_strings(self.raw_text)
        return self._sanitized_text


# ---------------------------------------------------------------------------
# Extraction
# ---------------------------------------------------------------------------


def extract_test_bodies(path: Path) -> List[TestBody]:
    """Extract individual TEST/TEST_F bodies from a file."""
    content = path.read_text(encoding="utf-8", errors="ignore")
    source_lines = content.splitlines()
    tests: List[TestBody] = []

    i = 0
    while i < len(source_lines):
        match = TEST_MACRO_RE.match(source_lines[i])
        if not match:
            i += 1
            continue

        suite, name = match.group(1), match.group(2)
        start_line = i + 1  # 1-indexed

        # Find the opening brace
        brace_depth = 0
        found_open = False
        body_lines: List[str] = []
        j = i

        while j < len(source_lines):
            line = source_lines[j]
            for ch in line:
                if ch == "{":
                    brace_depth += 1
                    found_open = True
                elif ch == "}":
                    brace_depth -= 1

            if found_open:
                body_lines.append(line)

            if found_open and brace_depth == 0:
                break
            j += 1

        if body_lines:
            test = TestBody(
                suite=suite,
                name=name,
                start_line=start_line,
                lines=body_lines,
                raw_text="\n".join(body_lines),
            )
            tests.append(test)

        i = j + 1

    return tests


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------


def count_assertions(body: TestBody) -> int:
    return len(ASSERTION_RE.findall(body.sanitized_text))


def is_existence_only(body: TestBody) -> bool:
    """Return True if ALL assertions in the body are existence-only patterns.

    Anchors each pattern at the assertion's start offset in the full
    sanitized buffer (``pattern.match(sanitized, pos)``) so assertions
    wrapped across lines — e.g. ``EXPECT_TRUE(\\n    value.has_value())`` —
    are classified consistently with their single-line equivalents.
    Previously the scanner narrowed to a single line and missed wrapped
    shallow assertions.
    """
    sanitized = body.sanitized_text
    assertions = list(ASSERTION_RE.finditer(sanitized))
    if not assertions:
        return False  # No assertions → separate violation

    for assertion_match in assertions:
        pos = assertion_match.start()
        # EXPECT_THAT / ASSERT_THAT need balanced-paren argument splitting so
        # matcher-arg classification is robust to commas in the first argument.
        if EXPECT_THAT_CALL_RE.match(sanitized, pos):
            if _expect_that_is_existence(sanitized, pos):
                continue
            return False
        is_existence = any(p.match(sanitized, pos) for p in EXISTENCE_PATTERNS)
        if not is_existence:
            return False  # At least one non-existence assertion

    return True


def has_assertion_behavioral_indicator(body: TestBody) -> bool:
    """True when the test contains an assertion that *is* a behavior check
    (e.g., DEATH/THROW). Such tests may legitimately have few assertions
    or assertions that look existence-only to the pattern matcher.

    Searches sanitized text so commented-out indicators (``// TODO:
    EXPECT_THROW(...)``) don't grant false behavioral bypasses.
    """
    return any(p.search(body.sanitized_text) for p in ASSERTION_BEHAVIORAL_INDICATORS)


def has_method_call_indicator(body: TestBody) -> bool:
    """True when the test exercises code via a method call (process/paint/etc.).
    Exercising code is not the same as asserting on its effect; method-call
    indicators lift the assertion-count floor but do NOT excuse existence-only
    assertion patterns.

    Searches sanitized text so a string literal containing ``.mouseDown(``
    does not count as a real call site.
    """
    return any(p.search(body.sanitized_text) for p in METHOD_CALL_INDICATORS)


def has_behavioral_indicator(body: TestBody) -> bool:
    """Back-compat shim: true when a test is non-trivial enough to bypass
    the minimum assertion-count floor. Existence-only detection uses the
    stricter assertion-behavioral indicator.
    """
    return has_assertion_behavioral_indicator(body) or has_method_call_indicator(body)


def find_tautologies(body: TestBody) -> List[str]:
    """Return list of tautological assertion descriptions."""
    found: List[str] = []
    for pattern in TAUTOLOGY_PATTERNS:
        for match in pattern.finditer(body.sanitized_text):
            found.append(match.group().strip())
    return found


def analyze_test(body: TestBody, min_assertions: int) -> Optional[Violation]:
    """Analyze a single test body for quality violations."""
    assertion_count = count_assertions(body)

    # Tautological assertions — always a violation, regardless of count
    tautologies = find_tautologies(body)
    if tautologies:
        examples = "; ".join(tautologies[:3])
        return Violation(
            file="",
            line=body.start_line,
            suite=body.suite,
            name=body.name,
            reason=f"tautological assertion(s) that can never fail: {examples}",
        )

    # Zero assertions — always a violation
    if assertion_count == 0:
        return Violation(
            file="",
            line=body.start_line,
            suite=body.suite,
            name=body.name,
            reason="test has zero assertions",
        )

    # All assertions are existence-only. Only *assertion-class* behavioral
    # indicators (DEATH/THROW) excuse this — exercising a method without
    # asserting on its effect is still a shallow test.
    if is_existence_only(body) and not has_assertion_behavioral_indicator(body):
        return Violation(
            file="",
            line=body.start_line,
            suite=body.suite,
            name=body.name,
            reason=f"all {assertion_count} assertion(s) are existence-only "
            f"(nullptr/empty/isValid checks) — verify behavior, not existence",
        )

    # Below minimum assertion count (skip behavioral tests like death tests)
    if assertion_count < min_assertions and not has_behavioral_indicator(body):
        return Violation(
            file="",
            line=body.start_line,
            suite=body.suite,
            name=body.name,
            reason=f"only {assertion_count} assertion(s) "
            f"(minimum {min_assertions}) — shallow test",
        )

    return None


# ---------------------------------------------------------------------------
# File iteration
# ---------------------------------------------------------------------------


def iter_test_files(
    root: Path, scan_paths: Sequence[str]
) -> Iterator[Path]:
    for relative in scan_paths:
        start = (root / relative).resolve()
        if not start.exists():
            continue
        for path in start.rglob("*.cpp"):
            if not path.is_file():
                continue
            if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
                continue
            if path.suffix == ".inc":
                continue
            # Only scan files that look like test files
            if path.name.startswith("test_"):
                yield path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Detect shallow / vibecoded GoogleTest patterns."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root path.",
    )
    parser.add_argument(
        "--paths",
        nargs="+",
        default=["tests"],
        help="Directories to scan for test files.",
    )
    parser.add_argument(
        "--min-assertions",
        type=int,
        default=1,
        help="Minimum assertions per test (default: 1).",
    )
    parser.add_argument(
        "--files",
        nargs="*",
        default=None,
        help="Specific files to check (overrides --paths).",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    violations: List[Violation] = []
    scanned_tests = 0
    scanned_files = 0

    if args.files:
        files = [Path(f) for f in args.files if Path(f).name.startswith("test_")]
    else:
        files = list(iter_test_files(root, args.paths))

    for file_path in files:
        test_bodies = extract_test_bodies(file_path)
        if not test_bodies:
            continue
        scanned_files += 1

        relative = str(file_path.relative_to(root)) if file_path.is_relative_to(root) else str(file_path)

        for body in test_bodies:
            scanned_tests += 1
            violation = analyze_test(body, args.min_assertions)
            if violation:
                violations.append(Violation(
                    file=relative,
                    line=violation.line,
                    suite=violation.suite,
                    name=violation.name,
                    reason=violation.reason,
                ))

    print("Test quality lint configuration:")
    print(f"  root: {root}")
    print(f"  min assertions per test: {args.min_assertions}")
    print(f"  scanned files: {scanned_files}")
    print(f"  scanned tests: {scanned_tests}")

    if violations:
        print(f"\nTest quality violations ({len(violations)}):")
        for v in violations:
            print(f"  ERROR: {v.file}:{v.line} {v.suite}.{v.name}")
            print(f"    → {v.reason}")
        print(f"\nFAILED: {len(violations)} shallow test(s) detected.")
        print("\nTo fix: add behavioral assertions that verify method output,")
        print("state changes, or side effects — not just existence checks.")
        return 1

    print("\nPASSED: no shallow tests detected.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
