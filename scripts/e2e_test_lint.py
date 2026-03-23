#!/usr/bin/env python3
"""
E2E test quality lint for Python pytest files.

Forbids patterns that hide failures instead of fixing them:

  1. @pytest.mark.xfail — disables a test without fixing the bug
  2. @pytest.mark.skip  — disables a test without fixing the bug
  3. pytest.xfail()     — runtime xfail hides broken test setup
  4. pytest.skip()      — runtime skip hides broken test setup
  5. time.sleep()       — arbitrary waits cause flaky tests

If a test fails, it fails. No exceptions.

Allowed:
  - @pytest.mark.slow  — categorization, not disabling

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Sequence

FORBIDDEN_DECORATORS = [
    (re.compile(r"^\s*@pytest\.mark\.xfail\b"), "xfail decorator disables the test — fix the bug or delete the test"),
    (re.compile(r"^\s*@pytest\.mark\.skip\b"), "skip decorator disables the test — fix the bug or delete the test"),
    (re.compile(r"^\s*@pytest\.mark\.known_failure\b"), "known_failure is banned — fix the bug or delete the test"),
]

FORBIDDEN_INLINE = [
    (re.compile(r"\bpytest\.xfail\s*\("), "pytest.xfail() hides broken test setup — fix the test or assert the condition"),
    (re.compile(r"\bpytest\.skip\s*\("), "pytest.skip() hides failures — if a test fails, it fails"),
]

# time.sleep is only forbidden in test files, not in test utilities/infrastructure
FORBIDDEN_IN_TEST_FILES_ONLY = [
    (re.compile(r"\btime\.sleep\s*\("), "time.sleep() causes flaky tests — use condition-based waits (wait_until)"),
]

# Inside pytest.param(..., marks=...) — same rules apply
FORBIDDEN_PARAM_MARKS = [
    (re.compile(r"marks\s*=\s*pytest\.mark\.xfail\b"), "xfail in parametrize disables the case — fix the bug or remove the case"),
    (re.compile(r"marks\s*=\s*pytest\.mark\.skip\b"), "skip in parametrize disables the case — fix the bug or remove the case"),
    (re.compile(r"marks\s*=\s*pytest\.mark\.known_failure\b"), "known_failure in parametrize is banned"),
]


@dataclass(frozen=True)
class Violation:
    file: str
    line: int
    text: str
    reason: str


def lint_file(path: Path, root: Path) -> List[Violation]:
    """Scan a Python file for forbidden patterns."""
    violations: List[Violation] = []
    relative = str(path.relative_to(root)) if path.is_relative_to(root) else str(path)
    is_test_file = path.name.startswith("test_")

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeDecodeError):
        return violations

    for i, line in enumerate(lines, start=1):
        for pattern, reason in FORBIDDEN_DECORATORS:
            if pattern.search(line):
                violations.append(Violation(relative, i, line.strip(), reason))

        for pattern, reason in FORBIDDEN_INLINE:
            if pattern.search(line):
                violations.append(Violation(relative, i, line.strip(), reason))

        for pattern, reason in FORBIDDEN_PARAM_MARKS:
            if pattern.search(line):
                violations.append(Violation(relative, i, line.strip(), reason))

        if is_test_file:
            for pattern, reason in FORBIDDEN_IN_TEST_FILES_ONLY:
                if pattern.search(line):
                    violations.append(Violation(relative, i, line.strip(), reason))

    return violations


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description="E2E test quality lint.")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--paths", nargs="+", default=["tests/e2e"])
    parser.add_argument("--files", nargs="*", default=None)
    args = parser.parse_args(argv)

    root = args.root.resolve()
    violations: List[Violation] = []
    scanned = 0

    if args.files:
        files = [Path(f) for f in args.files if f.endswith(".py")]
    else:
        files = []
        for rel in args.paths:
            scan_dir = root / rel
            if scan_dir.is_dir():
                files.extend(scan_dir.glob("*.py"))

    for f in sorted(files):
        scanned += 1
        violations.extend(lint_file(f, root))

    print(f"E2E test lint: scanned {scanned} file(s)")

    if violations:
        print(f"\nViolations ({len(violations)}):")
        for v in violations:
            print(f"  {v.file}:{v.line}: {v.reason}")
            print(f"    {v.text}")
        print(f"\nFAILED: {len(violations)} forbidden pattern(s).")
        print("\nTests must either PASS or FAIL. No xfail, no skip decorators, no sleep.")
        print("If a test exposes a production bug, fix the production code.")
        print("If a test is flaky, fix the test to be deterministic.")
        print("If a feature is not implemented, delete the test until it is.")
        return 1

    print("PASSED: no forbidden patterns.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
