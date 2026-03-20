#!/usr/bin/env python3
"""
Repo-wide C/C++ size lint.

Enforces:
- maximum lines per file
- maximum lines per function
"""

from __future__ import annotations

import argparse
import dataclasses
import re
import sys
from collections import deque
from pathlib import Path
from typing import Deque, Iterable, Iterator, List, Optional, Sequence, Tuple

DEFAULT_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
DEFAULT_PATHS = ("include", "src", "tests", "test_harness")
EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}

CONTROL_KEYWORD_RE = re.compile(r"\b(?:if|for|while|switch|catch)\s*\([^)]*\)\s*$")
FLOW_KEYWORD_RE = re.compile(r"\b(?:else|do|try)\s*$")
LAMBDA_RE = re.compile(
    r"\[[^\]]*\]\s*\([^)]*\)\s*(?:mutable\s*)?"
    r"(?:noexcept(?:\s*\([^)]*\))?\s*)?"
    r"(?:->\s*[^{};]+)?\s*$"
)
FUNCTION_TAIL_RE = re.compile(
    r"\)\s*(?:noexcept(?:\s*\([^)]*\))?\s*)?"
    r"(?:(?:consteval|constexpr|const|override|final|volatile|&|&&)\s*)*"
    r"(?:->\s*[^{};]+)?\s*"
    r"(?:requires\s+[^{};]+)?\s*"
    r"(?::\s*[^{}]*)?$"
)


@dataclasses.dataclass(frozen=True)
class FileViolation:
    path: str
    line_count: int
    limit: int


@dataclasses.dataclass(frozen=True)
class FunctionViolation:
    path: str
    start_line: int
    end_line: int
    line_count: int
    limit: int
    signature_preview: str


@dataclasses.dataclass
class FunctionContext:
    start_line: int
    brace_depth: int
    signature_preview: str


@dataclasses.dataclass
class SanitizeState:
    in_block_comment: bool = False
    in_string: bool = False
    string_quote: Optional[str] = None


def normalize_whitespace(text: str) -> str:
    return " ".join(text.split())


def sanitize_line(line: str, state: SanitizeState) -> str:
    out: List[str] = []
    i = 0
    line_len = len(line)

    while i < line_len:
        ch = line[i]

        if state.in_block_comment:
            if i + 1 < line_len and line[i] == "*" and line[i + 1] == "/":
                state.in_block_comment = False
                out.extend((" ", " "))
                i += 2
            else:
                out.append(" ")
                i += 1
            continue

        if state.in_string:
            if ch == "\\" and i + 1 < line_len:
                out.extend((" ", " "))
                i += 2
                continue
            if state.string_quote == ch:
                state.in_string = False
                state.string_quote = None
                out.append(" ")
                i += 1
                continue
            out.append(" ")
            i += 1
            continue

        if i + 1 < line_len and line[i] == "/" and line[i + 1] == "/":
            out.extend(" " * (line_len - i))
            break

        if i + 1 < line_len and line[i] == "/" and line[i + 1] == "*":
            state.in_block_comment = True
            out.extend((" ", " "))
            i += 2
            continue

        if ch in ("'", '"'):
            state.in_string = True
            state.string_quote = ch
            out.append(" ")
            i += 1
            continue

        out.append(ch)
        i += 1

    return "".join(out)


def iter_code_files(
    root: Path, scan_paths: Sequence[str], extensions: Sequence[str]
) -> Iterator[Path]:
    ext_set = {ext.lower() for ext in extensions}

    for relative in scan_paths:
        start = (root / relative).resolve()
        if not start.exists():
            continue

        for path in start.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in ext_set:
                continue
            if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
                continue
            yield path


def build_context_snippet(
    recent_lines: Deque[Tuple[int, str]], current_line: int, brace_index: int
) -> str:
    parts: List[str] = []
    for line_no, text in recent_lines:
        if line_no < current_line:
            stripped = text.strip()
            if stripped:
                parts.append(stripped)
        elif line_no == current_line:
            prefix = text[: brace_index + 1].strip()
            if prefix:
                parts.append(prefix)

    if len(parts) > 8:
        parts = parts[-8:]
    return normalize_whitespace(" ".join(parts))


def estimate_signature_start(
    recent_lines: Deque[Tuple[int, str]], current_line: int
) -> int:
    start_line = current_line
    relevant = [item for item in recent_lines if item[0] <= current_line]

    for line_no, text in reversed(relevant):
        if line_no == current_line:
            continue
        stripped = text.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            break
        if stripped.endswith(";") or stripped.endswith("{") or stripped.endswith("}"):
            break
        start_line = line_no

    return start_line


def looks_like_function_opening(snippet: str) -> bool:
    snippet = normalize_whitespace(snippet)
    if not snippet.endswith("{"):
        return False

    body = snippet[:-1].strip()
    if "(" not in body or ")" not in body:
        return False

    # Exclude obvious non-function scopes/control blocks.
    if CONTROL_KEYWORD_RE.search(body) or FLOW_KEYWORD_RE.search(body):
        return False
    if re.search(r"\b(?:class|struct|namespace|enum|union)\b\s+[^{()]*$", body):
        return False
    if LAMBDA_RE.search(body):
        return False

    return FUNCTION_TAIL_RE.search(body) is not None


def analyze_file(
    path: Path,
    root: Path,
    max_file_lines: int,
    max_function_lines: int,
) -> Tuple[List[FileViolation], List[FunctionViolation]]:
    content = path.read_text(encoding="utf-8", errors="ignore")
    lines = content.splitlines()
    file_line_count = len(lines)
    relative_path = str(path.relative_to(root))

    file_violations: List[FileViolation] = []
    function_violations: List[FunctionViolation] = []

    if file_line_count > max_file_lines:
        file_violations.append(
            FileViolation(relative_path, file_line_count, max_file_lines)
        )

    sanitize_state = SanitizeState()
    recent_lines: Deque[Tuple[int, str]] = deque(maxlen=24)

    function_stack: List[FunctionContext] = []
    brace_depth = 0

    for line_no, raw_line in enumerate(lines, start=1):
        clean_line = sanitize_line(raw_line, sanitize_state)
        recent_lines.append((line_no, clean_line))

        for idx, ch in enumerate(clean_line):
            if ch == "{":
                snippet = build_context_snippet(recent_lines, line_no, idx)
                if looks_like_function_opening(snippet):
                    start_line = estimate_signature_start(recent_lines, line_no)
                    function_stack.append(
                        FunctionContext(
                            start_line=start_line,
                            brace_depth=brace_depth + 1,
                            signature_preview=snippet,
                        )
                    )
                brace_depth += 1

            elif ch == "}":
                brace_depth = max(0, brace_depth - 1)
                while function_stack and brace_depth < function_stack[-1].brace_depth:
                    context = function_stack.pop()
                    function_line_count = line_no - context.start_line + 1
                    if function_line_count > max_function_lines:
                        function_violations.append(
                            FunctionViolation(
                                path=relative_path,
                                start_line=context.start_line,
                                end_line=line_no,
                                line_count=function_line_count,
                                limit=max_function_lines,
                                signature_preview=context.signature_preview,
                            )
                        )

    return file_violations, function_violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Enforce max file/function line limits across C/C++ source files."
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
        help="Top-level paths to scan (relative to --root).",
    )
    parser.add_argument(
        "--extensions",
        nargs="+",
        default=sorted(DEFAULT_EXTENSIONS),
        help="File extensions to include.",
    )
    parser.add_argument(
        "--max-file-lines",
        type=int,
        default=500,
        help="Maximum allowed lines per file.",
    )
    parser.add_argument(
        "--max-function-lines",
        type=int,
        default=60,
        help="Maximum allowed lines per function.",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    all_file_violations: List[FileViolation] = []
    all_function_violations: List[FunctionViolation] = []
    scanned_files = 0

    for file_path in iter_code_files(root, args.paths, args.extensions):
        scanned_files += 1
        file_violations, function_violations = analyze_file(
            path=file_path,
            root=root,
            max_file_lines=args.max_file_lines,
            max_function_lines=args.max_function_lines,
        )
        all_file_violations.extend(file_violations)
        all_function_violations.extend(function_violations)

    all_file_violations.sort(key=lambda v: (v.path, v.line_count))
    all_function_violations.sort(key=lambda v: (v.path, v.start_line))

    print("Size lint configuration:")
    print(f"  root: {root}")
    print(f"  paths: {', '.join(args.paths)}")
    print(f"  max file lines: {args.max_file_lines}")
    print(f"  max function lines: {args.max_function_lines}")
    print(f"  scanned files: {scanned_files}")

    if all_file_violations:
        print(f"\nFile length violations ({len(all_file_violations)}):")
        for violation in all_file_violations:
            print(
                f"  {violation.path}: {violation.line_count} lines "
                f"(max {violation.limit})"
            )

    if all_function_violations:
        print(f"\nFunction length violations ({len(all_function_violations)}):")
        for violation in all_function_violations:
            print(
                f"  {violation.path}:{violation.start_line}-{violation.end_line} "
                f"{violation.line_count} lines (max {violation.limit})"
            )
            print(f"    signature: {violation.signature_preview}")

    total_violations = len(all_file_violations) + len(all_function_violations)
    if total_violations:
        print(f"\nFAILED: {total_violations} total violation(s).")
        return 1

    print("\nPASSED: no size-limit violations found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
