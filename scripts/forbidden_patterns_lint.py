#!/usr/bin/env python3
"""
Forbidden-patterns lint for MultiScoper.

Rejects specific API uses that have bitten us before. Each entry pins an
ADR or a past bug so future engineers can evaluate before suppressing.

Rules today:

  1. `juce::LookAndFeel::setDefaultLookAndFeel` (and bare
     `setDefaultLookAndFeel`) is forbidden in src/** and include/**.
     Reason: mutating the JUCE process-global LookAndFeel default from a
     plugin affects other plugins loaded in the same DAW host and cannot be
     safely restored under multi-instance teardown. Use
     `Component::setLookAndFeel(&lookAndFeel_)` on the editor subtree
     instead.  See ADR-014 (failure modes section) and the 2026-Q2 fix to
     MultiScoperPluginEditor.

  2. `setContinuousRepainting(true)` is forbidden everywhere except inside
     `src/ui/controllers/OpenGLLifecycleManager.cpp` (and is only
     permitted there for historical comparison in comments, not in code).
     Reason: MultiScoper relies on signal-gated repainting to keep idle multi-
     instance CPU cost near zero. Re-enabling continuous repainting
     regresses that contract silently. See ADR-014.

Exit code 1 on any violation, prints file:line + offending line and the
rationale so reviewers don't have to guess.
"""

from __future__ import annotations

import argparse
import bisect
import dataclasses
import re
import sys
from pathlib import Path
from collections.abc import Iterator, Sequence
from typing import Optional

# Share the comment/string stripper with scripts/test_quality_lint.py so any
# parser edge-case fix lands in both linters simultaneously. The history here
# is two near-copy state machines that drifted on raw-string handling.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from lint_utils import strip_comments_and_strings  # noqa: E402

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}
DEFAULT_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".mm"}
DEFAULT_PATHS = ("include", "src")


@dataclasses.dataclass(frozen=True)
class ForbiddenRule:
    name: str
    pattern: re.Pattern
    allow_files: frozenset  # paths relative to repo root where this pattern is permitted
    rationale: str


RULES: list[ForbiddenRule] = [
    ForbiddenRule(
        name="setDefaultLookAndFeel",
        # Match either `juce::LookAndFeel::setDefaultLookAndFeel(` or a bare
        # `setDefaultLookAndFeel(` — both route to the process-global.
        pattern=re.compile(r"\bsetDefaultLookAndFeel\s*\("),
        allow_files=frozenset(),
        rationale=(
            "Plugins must not mutate the JUCE process-global default LookAndFeel; "
            "use Component::setLookAndFeel() on your editor subtree instead. "
            "See ADR-014."
        ),
    ),
    ForbiddenRule(
        name="setContinuousRepainting(true)",
        # Matches setContinuousRepainting(true) with optional whitespace.
        # We tolerate literal false — it is the only correct call in MultiScoper.
        pattern=re.compile(r"setContinuousRepainting\s*\(\s*true\s*\)"),
        allow_files=frozenset(),
        rationale=(
            "Signal-gated repainting is a product decision (ADR-014). "
            "Re-enabling continuous repainting regresses idle multi-instance "
            "CPU cost. If you truly need per-frame redraws for a specific "
            "effect, drive it with explicit forceRepaint() calls instead."
        ),
    ),
    ForbiddenRule(
        name="stdio logging (std::cout / std::cerr / printf / fprintf)",
        # Direct writes to process stdout/stderr from plugin code bypass the
        # MULTISCOPER_LOG channel system and leak to the DAW's console, which some
        # hosts route to the user-visible log viewer. Use MULTISCOPER_LOG or DBG()
        # instead so logging stays category-gated and build-type-gated.
        pattern=re.compile(
            r"\b(?:std::c(?:out|err)\s*<<|std::printf\s*\(|printf\s*\(|fprintf\s*\()"
        ),
        allow_files=frozenset(),
        rationale=(
            "Do not write to stdout/stderr directly from plugin code; the DAW "
            "console is user-visible in some hosts. Use MULTISCOPER_LOG(CATEGORY, ...) "
            "for categorised logging or DBG() for debug-only traces."
        ),
    ),
]


@dataclasses.dataclass(frozen=True)
class Violation:
    path: str
    line: int
    text: str
    rule: ForbiddenRule


def iter_source_files(root: Path, scan_paths: Sequence[str]) -> Iterator[Path]:
    for relative in scan_paths:
        start = (root / relative).resolve()
        if not start.exists():
            continue
        for path in start.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in DEFAULT_EXTENSIONS:
                continue
            try:
                rel_parts = path.relative_to(root).parts
            except ValueError:
                rel_parts = path.parts
            if any(part in EXCLUDED_DIR_NAMES for part in rel_parts):
                continue
            yield path


def scan_file(path: Path, root: Path) -> list[Violation]:
    try:
        content = path.read_text(encoding="utf-8", errors="ignore")
    except OSError as exc:
        # Fail loudly instead of silently dropping the file from the scan.
        # Swallowing the error would let a permissions or encoding problem
        # hide a real violation and produce a false CI pass.
        raise RuntimeError(
            f"forbidden_patterns_lint: failed to read {path}: {exc}"
        ) from exc

    # Strip out comments and string literals so rules don't self-trigger on
    # documentation explaining the rule itself.
    sanitized = strip_comments_and_strings(content)
    violations: list[Violation] = []

    # Map character offsets back to 1-based line numbers so matches that span
    # a newline (e.g. `setContinuousRepainting\n(true)`) still report the
    # line of the offending identifier instead of being missed by per-line
    # searches.
    line_starts = [0]
    line_starts.extend(i + 1 for i, ch in enumerate(sanitized) if ch == "\n")

    relative = str(path.relative_to(root)) if path.is_relative_to(root) else str(path)

    for rule in RULES:
        if relative in rule.allow_files:
            continue
        for match in rule.pattern.finditer(sanitized):
            lineno = bisect.bisect_right(line_starts, match.start())
            line_start = line_starts[lineno - 1]
            line_end = sanitized.find("\n", match.start())
            if line_end == -1:
                line_end = len(sanitized)
            line = sanitized[line_start:line_end]
            violations.append(
                Violation(
                    path=relative,
                    line=lineno,
                    text=line.rstrip(),
                    rule=rule,
                )
            )

    return violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Forbid MultiScoper anti-patterns.")
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root (default: repo root based on script location).",
    )
    parser.add_argument(
        "--paths",
        nargs="+",
        default=list(DEFAULT_PATHS),
        help="Directories to scan relative to the root (default: include src).",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    files = list(iter_source_files(root, args.paths))
    print("Forbidden-patterns lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")
    print(f"  scanned files: {len(files)}")
    print(f"  rules: {len(RULES)}")

    if not files:
        # Fail closed: an empty scan almost always means --root/--paths are
        # misconfigured, and silently returning 0 would make the CI gate
        # stop enforcing anything.
        print(
            "\nFAILED: no source files matched the configured root/paths.",
            file=sys.stderr,
        )
        return 1

    violations: list[Violation] = []
    for path in files:
        violations.extend(scan_file(path, root))

    if not violations:
        print("\nPASSED: no forbidden patterns found.")
        return 0

    print(f"\nForbidden patterns found ({len(violations)}):")
    for v in violations:
        print(f"  ERROR: {v.path}:{v.line} — {v.rule.name}")
        print(f"    {v.text.strip()}")
        print(f"    → {v.rule.rationale}")

    print(f"\nFAILED: {len(violations)} forbidden-pattern use(s) detected.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
