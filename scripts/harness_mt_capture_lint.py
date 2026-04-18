#!/usr/bin/env python3
"""
Lint rule: runOnTrackSync / runOnMessageThreadBlocking capture hygiene.

ADR-016 mandates that every lambda passed to the test-harness message-thread
dispatchers must own its payload — the dispatcher returns WaitFailure on
timeout and the lambda may still execute on the message thread afterwards,
so a lambda that holds references to caller-local variables crashes on
use-after-free. This is the mutex/crash class the ADR was written to kill.

Allowed capture shapes:

    runOnTrackSync(trackId, [](TestTrack& track) { ... });          # no caller state
    runOnTrackSync(trackId, [this](TestTrack& track) { ... });      # `this` only
    runOnTrackSync(trackId, [this, payload](TestTrack& track) {     # shared_ptr payloads
        ...
    });
    runOnTrackSync(trackId, [payload, other](TestTrack& track) { ... });

Forbidden capture shapes (this linter rejects them):

    runOnTrackSync(trackId, [&]        // capture-all-by-reference
    runOnTrackSync(trackId, [&, this]  // capture-all-by-reference with this pinned
    runOnTrackSync(trackId, [this, &x] // per-variable capture-by-reference
    runOnMessageThreadBlocking([&]     // same rules for the other dispatcher

Exit code 1 on any violation. Prints file:line plus the offending capture
clause so reviewers can see the fix immediately (almost always: hoist the
captured variable into a shared_ptr and capture by value).
"""

from __future__ import annotations

import argparse
import bisect
import dataclasses
import re
import sys
from pathlib import Path
from collections.abc import Iterator, Sequence

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lint_utils import strip_comments_and_strings  # noqa: E402

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}
DEFAULT_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".mm"}
DEFAULT_PATHS = ("test_harness/src", "test_harness/include")

# Match `runOnTrackSync(<anything not a comma>, [CAPTURE]` where CAPTURE is
# captured in group 1. We accept nested parens/brackets inside the first arg
# because runOnTrackSync takes `trackId` which is almost always a simple
# identifier, but we also want to tolerate small arithmetic.
_RUN_ON_TRACK_SYNC = re.compile(
    r"""
    \brunOnTrackSync\s*\(      # function name + (
    \s*[^,]+,\s*               # first arg: trackId (no comma inside)
    \[([^\]]*)\]               # lambda capture clause → group 1
    """,
    re.VERBOSE,
)

# runOnMessageThreadBlocking takes the lambda as the first positional arg, so
# the capture clause is the first `[...]` after the opening paren.
_RUN_ON_MT_BLOCKING = re.compile(
    r"""
    \brunOnMessageThreadBlocking\s*\(   # function name + (
    \s*\[([^\]]*)\]                     # lambda capture clause → group 1
    """,
    re.VERBOSE,
)

# Any bare `&` inside a capture clause means capture-by-reference. The clause
# may legitimately contain `&` only as part of a *parameter list* of a
# generalised lambda init-capture like `[x = std::ref(y)]`, but we never use
# that shape in the harness and rejecting it outright is the safe default.
#
# We explicitly check for:
#   - leading & (e.g. `[&]`, `[& , ...]`, `[&this]`)
#   - `&` immediately preceding an identifier (e.g. `[this, &local]`)
#   - `=` (capture-all-by-value: legal but brittle — easy to regress because
#     it promotes any local into the lambda; if you need the payload to
#     outlive the call, use a shared_ptr and capture it explicitly by name).
_FORBIDDEN_IN_CAPTURE = re.compile(r"(?:^|[\s,])(?P<tok>&|=)(?=[\w\s,\]])")


@dataclasses.dataclass(frozen=True)
class Violation:
    path: str
    line: int
    column: int
    site: str           # "runOnTrackSync" | "runOnMessageThreadBlocking"
    capture_clause: str
    offending_token: str


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


def _check_capture_clause(clause: str) -> str | None:
    """Return the offending token ('&' or '=') if the clause is forbidden.

    The clause is the text between `[` and `]` of a lambda capture. We accept
    purely value/name captures (e.g. `this, payload, other`) and reject any
    use of `&` (reference capture) or `=` (capture-all-by-value).
    """
    stripped = clause.strip()
    if not stripped:
        return None
    # Leading `&` or `=` is always forbidden (`[&]`, `[= ]`, `[&, this]`).
    if stripped[0] in ("&", "="):
        return stripped[0]
    # Per-variable reference capture: any `, &identifier` anywhere.
    match = _FORBIDDEN_IN_CAPTURE.search(stripped)
    if match:
        return match.group("tok")
    return None


def _line_col(content: str, offset: int, line_starts: list[int]) -> tuple[int, int]:
    line = bisect.bisect_right(line_starts, offset)
    col = offset - line_starts[line - 1] + 1
    return line, col


def scan_file(path: Path, root: Path) -> list[Violation]:
    try:
        content = path.read_text(encoding="utf-8", errors="ignore")
    except OSError as exc:
        raise RuntimeError(
            f"harness_mt_capture_lint: failed to read {path}: {exc}"
        ) from exc

    sanitized = strip_comments_and_strings(content)

    line_starts = [0]
    line_starts.extend(i + 1 for i, ch in enumerate(sanitized) if ch == "\n")

    relative = (
        str(path.relative_to(root))
        if path.is_relative_to(root)
        else str(path)
    )

    violations: list[Violation] = []

    for site_name, pattern in (
        ("runOnTrackSync", _RUN_ON_TRACK_SYNC),
        ("runOnMessageThreadBlocking", _RUN_ON_MT_BLOCKING),
    ):
        for match in pattern.finditer(sanitized):
            clause = match.group(1)
            offending = _check_capture_clause(clause)
            if offending is None:
                continue
            line, col = _line_col(sanitized, match.start(), line_starts)
            violations.append(
                Violation(
                    path=relative,
                    line=line,
                    column=col,
                    site=site_name,
                    capture_clause=clause.strip(),
                    offending_token=offending,
                )
            )

    return violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Enforce ADR-016: harness MT-dispatch lambdas must own their payload."
        )
    )
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
        help=(
            "Directories to scan relative to the root "
            "(default: test_harness/src test_harness/include)."
        ),
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    files = list(iter_source_files(root, args.paths))
    print("Harness MT-capture lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")
    print(f"  scanned files: {len(files)}")

    if not files:
        print(
            "\nFAILED: no source files matched the configured root/paths.",
            file=sys.stderr,
        )
        return 1

    violations: list[Violation] = []
    for path in files:
        violations.extend(scan_file(path, root))

    if not violations:
        print("\nPASSED: no forbidden MT-dispatch captures found.")
        return 0

    print(f"\nForbidden MT-dispatch lambda captures ({len(violations)}):")
    for v in violations:
        print(
            f"  ERROR: {v.path}:{v.line}:{v.column} — {v.site} lambda "
            f"uses forbidden '{v.offending_token}' in capture [{v.capture_clause}]"
        )
    print(
        "\nADR-016 requires that every lambda passed to the harness MT "
        "dispatchers own its payload via shared_ptr captures. Replace the "
        "offending reference capture with a heap-owned payload captured by "
        "value. See docs/decisions/016-test-harness-mt-dispatch.md."
    )
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
