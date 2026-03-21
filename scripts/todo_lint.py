#!/usr/bin/env python3
"""
TODO/placeholder lint for C/C++ source files.

Detects deferred-work markers that AI agents leave behind:
  1. TODO, FIXME, HACK, XXX, PLACEHOLDER, STUB comments
  2. Throw-away comments like "implement this", "add later"

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterator, List, Sequence

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}
EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".mm"}
DEFAULT_PATHS = ("include", "src")

# Markers that indicate deferred work.
# TODO/FIXME/HACK/XXX are matched case-insensitively.
# PLACEHOLDER/STUB only match when used as standalone markers (e.g., "// PLACEHOLDER")
# to avoid false positives on descriptive uses ("create placeholder").
MARKER_RE = re.compile(
    r"//\s*\b(?:TODO|FIXME|HACK|XXX)\b"
    r"|/\*\s*\b(?:TODO|FIXME|HACK|XXX)\b"
    r"|//\s*(?:PLACEHOLDER|STUB)\s*(?:[:\-]|$)"
    r"|/\*\s*(?:PLACEHOLDER|STUB)\s*(?:[:\-]|\*/)",
    re.IGNORECASE,
)

# Phrases AI agents use to defer work
DEFER_PHRASES_RE = re.compile(
    r"(?:"
    r"implement\s+(?:this|later|me)"
    r"|add\s+(?:this\s+)?later"
    r"|not\s+(?:yet\s+)?implemented"
    r"|needs?\s+implementation"
    r"|left\s+as\s+(?:an?\s+)?exercise"
    r"|fill\s+in\s+(?:this|here)"
    r"|temporary\s+(?:hack|workaround|fix)"
    r")",
    re.IGNORECASE,
)


def iter_source_files(
    root: Path, scan_paths: Sequence[str]
) -> Iterator[Path]:
    for relative in scan_paths:
        start = (root / relative).resolve()
        if not start.exists():
            continue
        for path in start.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in EXTENSIONS:
                continue
            if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
                continue
            yield path


def scan_file(path: Path) -> List[tuple[int, str, str]]:
    """Return list of (line_number, marker, line_text) for violations."""
    violations: List[tuple[int, str, str]] = []
    content = path.read_text(encoding="utf-8", errors="ignore")

    for line_no, line in enumerate(content.splitlines(), start=1):
        stripped = line.strip()

        # Check for TODO/FIXME/HACK/XXX/PLACEHOLDER/STUB markers
        marker_match = MARKER_RE.search(stripped)
        if marker_match:
            violations.append((line_no, marker_match.group().strip(), stripped))
            continue

        # Check for deferral phrases only in comments
        comment_start = stripped.find("//")
        block_start = stripped.find("/*")
        comment_text = ""
        if comment_start != -1:
            comment_text = stripped[comment_start:]
        elif block_start != -1:
            comment_text = stripped[block_start:]

        if comment_text:
            defer_match = DEFER_PHRASES_RE.search(comment_text)
            if defer_match:
                violations.append((line_no, defer_match.group(), stripped))

    return violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Detect TODO/FIXME/placeholder comments in source files."
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
        default=list(DEFAULT_PATHS),
        help="Directories to scan.",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    print("TODO lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")

    all_violations: List[tuple[str, int, str, str]] = []
    scanned = 0

    for path in iter_source_files(root, args.paths):
        scanned += 1
        file_violations = scan_file(path)
        rel = str(path.relative_to(root))
        for line_no, marker, text in file_violations:
            all_violations.append((rel, line_no, marker, text))

    print(f"  scanned files: {scanned}")

    if all_violations:
        print(f"\nPlaceholder violations ({len(all_violations)}):")
        for rel, line_no, marker, text in sorted(all_violations):
            print(f"  {rel}:{line_no} [{marker}]")
            print(f"    {text}")
        print(f"\nFAILED: {len(all_violations)} placeholder(s) detected.")
        print("\nAll deferred work must be completed before committing.")
        return 1

    print("\nPASSED: no placeholder comments found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
