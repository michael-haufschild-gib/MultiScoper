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
    re.compile(r"\b(?:EXPECT|ASSERT)_TRUE\s*\(\s*\w+\s*\)$"),
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
]

# Patterns that indicate a test IS behavioral even with few assertions
BEHAVIORAL_INDICATORS = [
    # Death tests are single-assertion but meaningful
    re.compile(r"\b(?:EXPECT|ASSERT)_(?:DEATH|DEATH_IF_SUPPORTED|THROW|ANY_THROW)\b"),
    # EXPECT_THAT with matchers
    re.compile(r"\b(?:EXPECT|ASSERT)_THAT\b"),
    # Calling methods that clearly exercise behavior
    re.compile(r"\.processBlock\s*\("),
    re.compile(r"\.process\s*\("),
    re.compile(r"\.paint\s*\("),
    re.compile(r"\.resized\s*\("),
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
    return len(ASSERTION_RE.findall(body.raw_text))


def is_existence_only(body: TestBody) -> bool:
    """Return True if ALL assertions in the body are existence-only patterns."""
    assertions = list(ASSERTION_RE.finditer(body.raw_text))
    if not assertions:
        return False  # No assertions → separate violation

    for assertion_match in assertions:
        # Get the line containing this assertion
        pos = assertion_match.start()
        line_start = body.raw_text.rfind("\n", 0, pos) + 1
        line_end = body.raw_text.find("\n", pos)
        if line_end == -1:
            line_end = len(body.raw_text)
        assertion_line = body.raw_text[line_start:line_end].strip()

        is_existence = any(p.search(assertion_line) for p in EXISTENCE_PATTERNS)
        if not is_existence:
            return False  # At least one non-existence assertion

    return True


def has_behavioral_indicator(body: TestBody) -> bool:
    return any(p.search(body.raw_text) for p in BEHAVIORAL_INDICATORS)


def analyze_test(body: TestBody, min_assertions: int) -> Optional[Violation]:
    """Analyze a single test body for quality violations."""
    assertion_count = count_assertions(body)

    # Zero assertions — always a violation
    if assertion_count == 0:
        return Violation(
            file="",
            line=body.start_line,
            suite=body.suite,
            name=body.name,
            reason="test has zero assertions",
        )

    # All assertions are existence-only (but skip if behavioral indicators present)
    if is_existence_only(body) and not has_behavioral_indicator(body):
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
