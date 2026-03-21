#!/usr/bin/env python3
"""
Architecture boundary lint for the Oscil project.

Enforces the include dependency DAG to prevent layer violations:

  core/        → may include: core/ only
  core/dsp/    → may include: core/ only
  core/analysis/ → may include: core/ only
  rendering/   → may include: core/, rendering/, ui/ (existing coupling)
  ui/          → may include: core/, rendering/, ui/
  plugin/      → may include: anything (top-level integration)
  tools/       → may include: anything (test infrastructure)

Forbidden edges (these would create architectural debt):
  core/      → ui/, rendering/, plugin/, tools/
  rendering/ → plugin/, tools/
  ui/        → plugin/, tools/

Only project-internal includes (quoted includes like "foo/bar.h") are checked.
System includes (<juce_core/...>) are ignored.

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, Iterator, List, Sequence, Set, Tuple

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}
EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".mm"}
DEFAULT_PATHS = ("include", "src")

# Matches: #include "some/path.h"
QUOTED_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')

# Module = first directory component under src/ or include/
# e.g., "core", "core/dsp", "rendering", "ui", "plugin", "tools"

# Map from module prefix to set of forbidden include prefixes.
# A file in module X must NOT include files whose path starts with any forbidden prefix.
FORBIDDEN_INCLUDES: Dict[str, List[str]] = {
    "core/": ["ui/", "rendering/", "plugin/", "tools/"],
    "rendering/": ["plugin/", "tools/"],
    "ui/": ["plugin/", "tools/"],
}

# Grandfathered violations — existing coupling that predates this lint.
# These are tracked so they can be fixed incrementally without blocking CI.
# Format: "relative/path:line_number" or "relative/path" (any line in file).
# To fix: refactor the dependency, then remove the allowlist entry.
ALLOWLISTED_FILES: Set[str] = {
    "include/rendering/GpuRenderCoordinator.h",
    "include/ui/controllers/OscillatorPanelController.h",
    "include/ui/layout/PluginEditorLayout.h",
    "include/ui/managers/PerformanceMetricsController.h",
    "src/ui/layout/PaneComponent.cpp",
    "src/ui/layout/pane/PaneBody.cpp",
    "src/ui/layout/pane/WaveformStack.cpp",
}


def get_module(file_path: str) -> str:
    """Extract the module prefix from a file path like 'src/core/dsp/Foo.cpp' → 'core/'."""
    parts = Path(file_path).parts
    # Skip 'src/' or 'include/' prefix
    if parts and parts[0] in ("src", "include"):
        parts = parts[1:]
    if parts:
        return parts[0] + "/"
    return ""


def iter_source_files(
    root: Path, scan_paths: Sequence[str]
) -> Iterator[Tuple[Path, str]]:
    """Yield (absolute_path, relative_path) for source files."""
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
            yield path, str(path.relative_to(root))


def check_file(
    path: Path, rel_path: str
) -> List[Tuple[int, str, str, str]]:
    """Return list of (line_no, file_module, included_path, forbidden_prefix) violations."""
    module = get_module(rel_path)
    forbidden = FORBIDDEN_INCLUDES.get(module)
    if not forbidden:
        return []

    violations: List[Tuple[int, str, str, str]] = []
    content = path.read_text(encoding="utf-8", errors="ignore")

    for line_no, line in enumerate(content.splitlines(), start=1):
        match = QUOTED_INCLUDE_RE.match(line)
        if not match:
            continue

        included = match.group(1)
        for prefix in forbidden:
            if included.startswith(prefix):
                violations.append((line_no, module, included, prefix))
                break

    return violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Enforce include dependency boundaries between modules."
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

    print("Architecture lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")
    print("  rules:")
    for module, forbidden in sorted(FORBIDDEN_INCLUDES.items()):
        print(f"    {module} must NOT include: {', '.join(forbidden)}")

    all_violations: List[Tuple[str, int, str, str, str]] = []
    allowlisted_count = 0
    scanned = 0

    for path, rel_path in iter_source_files(root, args.paths):
        scanned += 1
        file_violations = check_file(path, rel_path)
        for line_no, module, included, prefix in file_violations:
            if rel_path in ALLOWLISTED_FILES:
                allowlisted_count += 1
            else:
                all_violations.append((rel_path, line_no, module, included, prefix))

    print(f"  scanned files: {scanned}")
    if allowlisted_count:
        print(f"  allowlisted (grandfathered): {allowlisted_count}")

    if all_violations:
        print(f"\nArchitecture violations ({len(all_violations)}):")
        for rel, line_no, module, included, prefix in sorted(all_violations):
            print(f"  {rel}:{line_no}")
            print(f"    {module} must not include {prefix} → #include \"{included}\"")
        print(f"\nFAILED: {len(all_violations)} boundary violation(s).")
        return 1

    print("\nPASSED: no new architecture boundary violations.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
