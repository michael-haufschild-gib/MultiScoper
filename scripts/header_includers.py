#!/usr/bin/env python3
"""header_includers.py — fast header->TU reverse index.

Given a compile_commands.json and one or more staged header paths (relative to
the repo root), print the set of .cpp files under src/ that transitively
include any of those headers (directly, or via another header under include/).

Usage:
    python3 scripts/header_includers.py \
        --compile-db build/dev/compile_commands.json \
        --repo-root . \
        --headers include/foo/bar.h include/baz.h

Exit codes:
    0 — success; stdout contains sorted, de-duplicated repo-relative .cpp paths.
    1 — bad arguments / usage error.
    2 — compile-db missing (caller should fall back).

Zero third-party dependencies. Designed to scan the full Oscil tree (~300 TUs)
in well under 2 seconds by:
  * parsing each .cpp / .h file at most once (cache keyed by realpath),
  * resolving #includes against the per-TU -I list and the repo include/ root,
  * only recursing into headers that live inside the repo include/ tree.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import sys
from typing import Dict, FrozenSet, Iterable, List, Optional, Set, Tuple

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]')


def parse_args(argv: List[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        prog="header_includers.py",
        description=(
            "Find every .cpp file under src/ that transitively includes "
            "any of the given headers, using compile_commands.json as the "
            "authoritative include-graph source."
        ),
    )
    p.add_argument("--compile-db", required=True,
                   help="Path to compile_commands.json")
    p.add_argument("--repo-root", required=True,
                   help="Repo root (used to produce relative output paths)")
    p.add_argument("--headers", nargs="+", required=True,
                   help="Repo-relative header paths to search for (e.g. "
                        "include/core/Foo.h)")
    return p.parse_args(argv)


def extract_include_dirs(entry: dict) -> List[str]:
    """Return absolute -I include directories from a compile_commands entry."""
    if "arguments" in entry:
        args = list(entry["arguments"])
    else:
        args = shlex.split(entry.get("command", ""))
    directory = entry.get("directory", "")
    out: List[str] = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "-I" and i + 1 < len(args):
            out.append(_abs(args[i + 1], directory))
            i += 2
            continue
        if a.startswith("-I"):
            out.append(_abs(a[2:], directory))
        elif a == "-isystem" and i + 1 < len(args):
            out.append(_abs(args[i + 1], directory))
            i += 2
            continue
        elif a.startswith("-isystem"):
            out.append(_abs(a[len("-isystem"):], directory))
        i += 1
    return out


def _abs(path: str, base: str) -> str:
    if os.path.isabs(path):
        return os.path.normpath(path)
    return os.path.normpath(os.path.join(base, path))


def read_includes(path: str) -> List[str]:
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError:
        return []
    out: List[str] = []
    for line in text.splitlines():
        m = INCLUDE_RE.match(line)
        if m:
            out.append(m.group(1))
    return out


def resolve_include(
    spec: str,
    includer_dir: str,
    search_dirs: Tuple[str, ...],
    include_root: str,
) -> Optional[str]:
    """Resolve an #include spec to an absolute path inside include_root or None."""
    # Try quoted-include semantics first: relative to includer dir.
    candidates = [os.path.join(includer_dir, spec)]
    candidates.extend(os.path.join(d, spec) for d in search_dirs)
    for c in candidates:
        c_norm = os.path.normpath(c)
        if os.path.isfile(c_norm) and _under(c_norm, include_root):
            return c_norm
    return None


def _under(path: str, root: str) -> bool:
    try:
        rel = os.path.relpath(path, root)
    except ValueError:
        return False
    return not rel.startswith("..") and not os.path.isabs(rel)


def collect_transitive_headers(
    start_file: str,
    search_dirs: Tuple[str, ...],
    include_root: str,
    header_cache: Dict[str, FrozenSet[str]],
) -> FrozenSet[str]:
    """Return the set of absolute header paths (under include_root) transitively
    reachable from start_file via #include directives. Cached per-file."""
    if start_file in header_cache:
        return header_cache[start_file]

    # Iterative DFS with cycle guard; populate cache for every node touched.
    # Use memoization only for headers (files under include_root); TUs are not
    # reused, but caching them is harmless.
    stack: List[str] = [start_file]
    visiting: Set[str] = set()
    # Per-node accumulated result.
    result: Dict[str, Set[str]] = {}
    order: List[str] = []

    # Two-pass approach: compute direct includes for each node, then fold.
    direct: Dict[str, List[str]] = {}
    seen: Set[str] = set()
    dfs_stack: List[str] = [start_file]
    while dfs_stack:
        node = dfs_stack.pop()
        if node in seen:
            continue
        seen.add(node)
        order.append(node)
        includer_dir = os.path.dirname(node)
        resolved: List[str] = []
        for spec in read_includes(node):
            r = resolve_include(spec, includer_dir, search_dirs, include_root)
            if r is not None:
                resolved.append(r)
                if r not in seen:
                    dfs_stack.append(r)
        direct[node] = resolved

    # Fold: each node's full set = its direct includes ∪ those of each direct.
    # Process nodes in reverse discovery order and compute fixed point with
    # cycle-safe accumulation.
    full: Dict[str, Set[str]] = {n: set() for n in order}
    # Initialize with direct includes.
    for n in order:
        full[n].update(direct[n])
    # Iterate to fixed point (cycles resolve in at most O(depth) passes; the
    # include graph is shallow, so a simple loop suffices).
    changed = True
    while changed:
        changed = False
        for n in order:
            before = len(full[n])
            for child in direct[n]:
                if child in full:
                    full[n].update(full[child])
            if len(full[n]) != before:
                changed = True

    for n, s in full.items():
        header_cache.setdefault(n, frozenset(s))

    return header_cache[start_file]


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    compile_db = os.path.abspath(args.compile_db)
    repo_root = os.path.abspath(args.repo_root)
    if not os.path.isfile(compile_db):
        print(f"header_includers: compile-db not found: {compile_db}",
              file=sys.stderr)
        return 2

    try:
        with open(compile_db, "r", encoding="utf-8") as f:
            db = json.load(f)
    except (OSError, ValueError) as e:
        print(f"header_includers: failed to read {compile_db}: {e}",
              file=sys.stderr)
        return 2

    include_root = os.path.join(repo_root, "include")
    src_root = os.path.join(repo_root, "src")

    # Canonicalize staged headers to absolute paths under include_root.
    staged_abs: Set[str] = set()
    for h in args.headers:
        p = h if os.path.isabs(h) else os.path.join(repo_root, h)
        staged_abs.add(os.path.normpath(p))

    header_cache: Dict[str, FrozenSet[str]] = {}
    hits: Set[str] = set()

    for entry in db:
        tu = entry.get("file", "")
        if not tu:
            continue
        tu_abs = os.path.normpath(
            tu if os.path.isabs(tu) else os.path.join(entry.get("directory", ""), tu))
        # Only consider TUs under the repo's src/ tree.
        if not _under(tu_abs, src_root):
            continue
        if not tu_abs.endswith(".cpp"):
            continue
        search_dirs = tuple(extract_include_dirs(entry))
        reachable = collect_transitive_headers(
            tu_abs, search_dirs, include_root, header_cache)
        if reachable & staged_abs:
            hits.add(os.path.relpath(tu_abs, repo_root))

    for path in sorted(hits):
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
