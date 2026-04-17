#!/usr/bin/env python3
"""Summarize Google Benchmark JSON: median time per benchmark with CV.

Usage: scripts/bench_summary.py path/to/results.json [path/to/second.json]

If two paths are given, prints a side-by-side diff with percent change and
a naive noise flag (change magnitude < CV of baseline).
"""

import json
import sys
from pathlib import Path


def load_aggregates(path):
    data = json.loads(Path(path).read_text())
    medians = {}
    cvs = {}
    for entry in data["benchmarks"]:
        if entry.get("run_type") != "aggregate":
            continue
        name = entry["run_name"]
        agg = entry["aggregate_name"]
        if agg == "median":
            medians[name] = entry["real_time"]
        elif agg == "cv":
            cvs[name] = entry["real_time"] * 100.0  # cv as percent
    return medians, cvs


def print_single(path):
    m, cvs = load_aggregates(path)
    print(f"{'benchmark':55} {'median (ns)':>14} {'cv %':>8}")
    print("-" * 80)
    for name, t in sorted(m.items()):
        cv = cvs.get(name, float("nan"))
        print(f"{name:55} {t:14.2f} {cv:8.2f}")


def print_diff(base_path, new_path):
    bm, bcv = load_aggregates(base_path)
    nm, ncv = load_aggregates(new_path)
    keys = sorted(set(bm) | set(nm))
    print(f"{'benchmark':55} {'base (ns)':>12} {'new (ns)':>12} {'Δ %':>8} {'base cv':>8} {'new cv':>8} note")
    print("-" * 115)
    for k in keys:
        b = bm.get(k)
        n = nm.get(k)
        if b is None:
            print(f"{k:55} {'—':>12} {n:12.2f}")
            continue
        if n is None:
            print(f"{k:55} {b:12.2f} {'—':>12}")
            continue
        delta = (n - b) / b * 100.0
        bcvp = bcv.get(k, 0.0)
        ncvp = ncv.get(k, 0.0)
        noise_floor = max(bcvp, ncvp)
        flag = "NOISE" if abs(delta) < noise_floor else ("WIN " if delta < 0 else "LOSS")
        arrow = "↓" if delta < 0 else ("↑" if delta > 0 else "=")
        print(f"{k:55} {b:12.2f} {n:12.2f} {arrow}{abs(delta):6.2f}% {bcvp:7.2f}% {ncvp:7.2f}% {flag}")


def main():
    if len(sys.argv) == 2:
        print_single(sys.argv[1])
    elif len(sys.argv) == 3:
        print_diff(sys.argv[1], sys.argv[2])
    else:
        print(__doc__, file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
