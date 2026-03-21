#!/usr/bin/env python3
"""
Source registry lint for CMake-based C++ projects.

Cross-references .cpp files on disk against cmake/Sources.cmake entries.
Detects:
  1. Orphan .cpp files in src/ not listed in Sources.cmake
  2. Stale Sources.cmake entries pointing to nonexistent files
  3. Duplicate entries in Sources.cmake

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import List, Sequence, Set, Tuple

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}

# Matches lines like:  ${CMAKE_SOURCE_DIR}/src/core/Source.cpp
CMAKE_SOURCE_RE = re.compile(
    r"\$\{CMAKE_SOURCE_DIR\}/(\S+\.(?:cpp|mm))"
)

# Platform-conditional blocks: if(APPLE) ... endif()
# We track these to mark platform-specific sources
PLATFORM_IF_RE = re.compile(r"^\s*if\s*\(\s*(APPLE|WIN32|UNIX|LINUX)\s*\)")
PLATFORM_ENDIF_RE = re.compile(r"^\s*endif\s*\(")


def parse_sources_cmake(sources_cmake: Path) -> Tuple[List[str], List[str]]:
    """Parse Sources.cmake and return (all_entries, duplicate_entries)."""
    content = sources_cmake.read_text(encoding="utf-8")
    lines = content.splitlines()

    seen: Set[str] = set()
    entries: List[str] = []
    duplicates: List[str] = []

    for line in lines:
        match = CMAKE_SOURCE_RE.search(line)
        if match:
            rel_path = match.group(1)
            if rel_path in seen:
                duplicates.append(rel_path)
            else:
                seen.add(rel_path)
                entries.append(rel_path)

    return entries, duplicates


def find_cpp_files(root: Path, scan_dir: str) -> Set[str]:
    """Find all .cpp and .mm files under a directory."""
    start = (root / scan_dir).resolve()
    if not start.exists():
        return set()

    result: Set[str] = set()
    for path in start.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in (".cpp", ".mm"):
            continue
        if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
            continue
        result.add(str(path.relative_to(root)))

    return result


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Cross-reference .cpp files against Sources.cmake."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root path.",
    )
    parser.add_argument(
        "--sources-cmake",
        type=str,
        default="cmake/Sources.cmake",
        help="Path to Sources.cmake (relative to root).",
    )
    parser.add_argument(
        "--scan-dir",
        type=str,
        default="src",
        help="Directory to scan for .cpp files (relative to root).",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()
    sources_cmake = root / args.sources_cmake

    if not sources_cmake.exists():
        print(f"ERROR: Sources.cmake not found: {sources_cmake}")
        return 1

    print("Source registry lint configuration:")
    print(f"  root: {root}")
    print(f"  sources cmake: {args.sources_cmake}")
    print(f"  scan dir: {args.scan_dir}")

    cmake_entries, duplicates = parse_sources_cmake(sources_cmake)
    disk_files = find_cpp_files(root, args.scan_dir)

    cmake_set = set(cmake_entries)

    # Orphan files: on disk but not in Sources.cmake
    orphans = sorted(disk_files - cmake_set)

    # Stale entries: in Sources.cmake but not on disk
    stale = sorted(
        entry for entry in cmake_entries
        if entry.startswith(args.scan_dir + "/") and not (root / entry).exists()
    )

    print(f"  cmake entries: {len(cmake_entries)}")
    print(f"  disk files: {len(disk_files)}")

    violations = 0

    if orphans:
        print(f"\nOrphan files ({len(orphans)}) — on disk but not in Sources.cmake:")
        for f in orphans:
            print(f"  {f}")
        violations += len(orphans)

    if stale:
        print(f"\nStale entries ({len(stale)}) — in Sources.cmake but not on disk:")
        for f in stale:
            print(f"  {f}")
        violations += len(stale)

    if duplicates:
        print(f"\nDuplicate entries ({len(duplicates)}) — listed multiple times:")
        for f in duplicates:
            print(f"  {f}")
        violations += len(duplicates)

    if violations:
        print(f"\nFAILED: {violations} source registry violation(s).")
        return 1

    print("\nPASSED: all source files are registered.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
