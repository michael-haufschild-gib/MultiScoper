#!/usr/bin/env python3
"""Run clang-tidy over the project and report per-file failures.

Enforces the .clang-tidy configuration at the repository root. The compile
database (``compile_commands.json``) must already exist — configure with
``cmake --preset dev`` first, since CMake only emits it at configure time
(``CMAKE_EXPORT_COMPILE_COMMANDS=ON`` in the base preset).

On macOS, homebrew clang-tidy ships with its own libc++ headers but not the
macOS SDK; we pass ``-isysroot $(xcrun --show-sdk-path)`` so system headers
(e.g. ``TargetConditionals.h``) resolve. On Linux we fall back to the
compile-db's paths.

The script mirrors the CI ``Run clang-tidy`` step but is packaged for local
ctest so developers get feedback before pushing.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def discover_cpp_files(root: Path, paths: list[str]) -> list[Path]:
    out: list[Path] = []
    for rel in paths:
        base = root / rel
        if not base.exists():
            continue
        for p in base.rglob("*.cpp"):
            if not p.is_file():
                continue
            if any(part in {"build", "_deps"} for part in p.parts):
                continue
            out.append(p)
    return sorted(out)


def resolve_macos_sysroot() -> str | None:
    # Resolve xcrun to an absolute path so subprocess.run doesn't get a
    # partial executable name (satisfies Ruff S607 and tightens launch
    # hygiene). Missing xcrun is benign — the caller just skips --isysroot.
    xcrun = shutil.which("xcrun")
    if xcrun is None:
        return None
    try:
        result = subprocess.run(
            [xcrun, "--show-sdk-path"],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip() or None
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None


def run_one(exe: str, build_dir: str, extra_args: list[str], file_path: str) -> tuple[str, int, str]:
    cmd = [exe, "-p", build_dir, *[a for ea in extra_args for a in ("--extra-arg", ea)], file_path]
    # check=False is intentional: we inspect proc.returncode directly so
    # clang-tidy's normal non-zero "found issues" exit doesn't raise.
    # S603 is suppressed because ``exe`` is resolved via shutil.which() in
    # main() and the argument list uses shell=False.
    proc = subprocess.run(  # noqa: S603
        cmd,
        check=False,
        capture_output=True,
        text=True,
    )
    return file_path, proc.returncode, proc.stdout + proc.stderr


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, help="CMake build dir containing compile_commands.json")
    parser.add_argument("--paths", nargs="+", default=["src"])
    parser.add_argument(
        "--clang-tidy",
        default=None,
        help="clang-tidy executable. Default: /opt/homebrew/opt/llvm/bin/clang-tidy on macOS, else `clang-tidy`",
    )
    parser.add_argument("--jobs", type=int, default=max(os.cpu_count() or 2, 2))
    args = parser.parse_args()

    if args.jobs <= 0:
        print(
            f"FAILED: --jobs must be a positive integer (got {args.jobs}).",
            file=sys.stderr,
        )
        return 2

    root = args.root.resolve()
    build_dir = str((root / args.build_dir).resolve())

    if not (Path(build_dir) / "compile_commands.json").exists():
        print(
            f"FAILED: compile_commands.json not found in {build_dir}. "
            "Configure with: cmake --preset dev",
            file=sys.stderr,
        )
        return 2

    # Default clang-tidy: prefer homebrew LLVM on macOS (stable versioning).
    exe_default = args.clang_tidy
    if exe_default is None:
        if platform.system() == "Darwin":
            brew = "/opt/homebrew/opt/llvm/bin/clang-tidy"
            exe_default = brew if Path(brew).is_file() else "clang-tidy"
        else:
            exe_default = "clang-tidy"

    exe = shutil.which(exe_default) or (exe_default if Path(exe_default).is_file() else None)
    if exe is None:
        print(
            f"FAILED: clang-tidy not found (tried '{exe_default}'). "
            "Install: brew install llvm (macOS) / apt-get install clang-tidy (Linux).",
            file=sys.stderr,
        )
        return 2

    # macOS: inject -isysroot so system headers (TargetConditionals.h, etc.) resolve.
    extra_args: list[str] = []
    if platform.system() == "Darwin":
        sysroot = resolve_macos_sysroot()
        if sysroot:
            extra_args.extend(["-isysroot", sysroot])

    files = discover_cpp_files(root, args.paths)
    print("Clang-tidy lint configuration:")
    print(f"  root: {root}")
    print(f"  build-dir: {build_dir}")
    print(f"  clang-tidy: {exe}")
    print(f"  extra-args: {extra_args}")
    print(f"  scanned files: {len(files)}")
    print(f"  parallelism: {args.jobs}")

    if not files:
        # Exit code 2 matches the other precondition-error returns above
        # (missing compile_commands.json, --jobs <= 0, clang-tidy not on PATH).
        # Exit code 1 is reserved for "clang-tidy found issues".
        print("FAILED: no .cpp files matched — check --paths.", file=sys.stderr)
        return 2

    failures: list[tuple[str, str]] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futs = [pool.submit(run_one, exe, build_dir, extra_args, str(f)) for f in files]
        for fut in concurrent.futures.as_completed(futs):
            path, rc, output = fut.result()
            rel = os.path.relpath(path, root)
            if rc != 0:
                failures.append((rel, output))
                print(f"  FAIL  {rel}")
            else:
                print(f"  ok    {rel}")

    print()
    if failures:
        print(f"FAILED: clang-tidy reported issues in {len(failures)} file(s):")
        for rel, output in failures:
            print(f"\n── {rel} ──")
            # Keep the output bounded so CI logs don't explode.
            head = "\n".join(output.splitlines()[:60])
            print(head)
            if output.count("\n") > 60:
                print(f"  … ({output.count(chr(10))} lines total, truncated)")
        return 1

    print("PASSED: clang-tidy clean across all scanned files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
