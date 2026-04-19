#!/usr/bin/env python3
"""Enforce clang-format compliance across the project.

Runs ``clang-format --dry-run --Werror`` on every C/C++/ObjC++ source file
under the listed paths. Exits non-zero if any file would be reformatted.

This mirrors the CI "clang-format check" step so a local ``ctest --preset
dev`` catches formatting drift before it reaches CI. Auto-fix support is
intentionally NOT provided here; the pre-commit hook
(``scripts/pre-commit``) is the sanctioned auto-fix path.

Invoke:
    python3 scripts/format_lint.py --root . --paths include src tests test_harness
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

EXTENSIONS = {".cpp", ".cc", ".cxx", ".c", ".h", ".hpp", ".hxx", ".mm"}
# Skip generated/third-party trees even if they slip into the path list.
EXCLUDED_PARTS = {"build", "_deps", ".venv", "node_modules"}


def discover_files(root: Path, paths: list[str]) -> list[Path]:
    out: list[Path] = []
    for rel in paths:
        base = root / rel
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if not p.is_file() or p.suffix not in EXTENSIONS:
                continue
            if any(part in EXCLUDED_PARTS for part in p.parts):
                continue
            out.append(p)
    return sorted(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--paths", nargs="+", required=True)
    parser.add_argument(
        "--clang-format",
        default="clang-format",
        help="clang-format executable (default: clang-format on PATH)",
    )
    args = parser.parse_args()

    root = args.root.resolve()

    exe = shutil.which(args.clang_format)
    if exe is None:
        print(
            f"FAILED: '{args.clang_format}' not found on PATH. "
            "Install it (macOS: brew install clang-format; "
            "Linux: apt-get install clang-format) or pass --clang-format <path>.",
            file=sys.stderr,
        )
        return 2

    files = discover_files(root, args.paths)
    print("Format lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")
    print(f"  clang-format: {exe}")
    print(f"  scanned files: {len(files)}")

    if not files:
        print("PASSED: no files to check.")
        return 0

    violations: list[str] = []
    execution_errors: list[str] = []
    # Batch a few at a time to bound the command line length on every OS.
    # clang-format --dry-run --Werror exits non-zero if any file needs work.
    batch_size = 32
    for start in range(0, len(files), batch_size):
        batch = files[start : start + batch_size]
        cmd = [exe, "--dry-run", "--Werror", *[str(f) for f in batch]]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            batch_had_format_violation = False
            # clang-format prints one diagnostic per file+line on stderr.
            for line in proc.stderr.splitlines():
                # Lines of the form "path:line:col: error: code should be clang-formatted"
                if ": error: code should be clang-formatted" in line:
                    path = line.split(":", 1)[0]
                    violations.append(path)
                    batch_had_format_violation = True
            # Non-zero exit without a parseable format diagnostic = real tool
            # failure (missing file, invalid .clang-format, crash). Do not silently
            # treat that as PASS.
            if not batch_had_format_violation:
                execution_errors.append(
                    "clang-format exited {code} on batch starting at {path}\n"
                    "  stdout: {out}\n"
                    "  stderr: {err}".format(
                        code=proc.returncode,
                        path=batch[0],
                        out=proc.stdout.strip() or "(empty)",
                        err=proc.stderr.strip() or "(empty)",
                    )
                )

    violations = sorted(set(violations))
    failed = False
    if violations:
        print(f"\nFormat violations ({len(violations)} file(s)):")
        for v in violations:
            print(f"  {v}")
        print("\nFAILED. Run:")
        print("  clang-format -i <file>")
        print("or commit through scripts/pre-commit (auto-fixes + re-stages).")
        failed = True

    if execution_errors:
        print(f"\nclang-format execution errors ({len(execution_errors)}):")
        for err in execution_errors:
            print(err)
        failed = True

    if failed:
        return 1

    print("PASSED: all files clang-format-clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
