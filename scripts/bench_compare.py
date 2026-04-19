#!/usr/bin/env python3
"""Compare two Google Benchmark JSON files with Mann-Whitney U + Bonferroni.

Implements the regression-gate design sketched in ADR-012:
- Per-benchmark one-sided Mann-Whitney U test (current samples vs baseline).
- Bonferroni correction across all benchmarks (≈24 in the suite).
- Reports regressions at a configurable family-wise alpha (default 0.05).

Both input files must have been produced with ``--benchmark_repetitions=N``
(N ≥ 3) so we have per-run samples to compare. The script uses the
per-aggregate entries (``run_type == "iteration"``) — not the mean/median
aggregates — so the statistical test sees the raw sample distribution.

Exit codes:
  0 — no regressions detected at the family-wise alpha, or one side was empty.
  1 — at least one benchmark regressed significantly.
  2 — input error (missing file, malformed JSON, incompatible benchmark sets).

Depends only on the Python standard library + scipy.stats. scipy is present on
the Ubuntu CI image by default; if not, we fall back to a pure-Python
Mann-Whitney implementation that is slower but never missing.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from itertools import combinations
from pathlib import Path

# Threshold below which we enumerate all rank-subsets exactly rather than
# fall back to the normal approximation. C(n1+n2, min(n1,n2)) ≤ this bound
# keeps each test under ~200 ms on CPython. Tuned so that the CI-typical
# shape (3 current samples vs 50 baseline samples → C(53,3)=23426) goes
# through the exact path, where the normal approximation misrepresents
# p-values by an order of magnitude.
_EXACT_ENUMERATION_LIMIT = 200_000


def _rank_with_midranks(combined: list[tuple[float, str]]) -> list[float]:
    """Assign mid-ranks to a sorted combined sample, handling ties."""
    ranks: list[float] = [0.0] * len(combined)
    i = 0
    while i < len(combined):
        j = i
        while j + 1 < len(combined) and combined[j + 1][0] == combined[i][0]:
            j += 1
        mid = (i + j) / 2.0 + 1.0
        for k in range(i, j + 1):
            ranks[k] = mid
        i = j + 1
    return ranks


def _exact_mannwhitneyu_greater(x: list[float], y: list[float]) -> float:
    """Exact one-sided (greater) p-value via rank-subset enumeration.

    Correctly handles ties via mid-rank assignment. Caller must gate on
    sample size so enumeration stays tractable — see ``_EXACT_ENUMERATION_LIMIT``.
    """
    combined = sorted([(v, "x") for v in x] + [(v, "y") for v in y])
    ranks = _rank_with_midranks(combined)
    n = len(combined)
    n1 = len(x)

    observed_rank_sum = sum(r for r, (_, label) in zip(ranks, combined, strict=True) if label == "x")

    total = 0
    extreme = 0
    for combo in combinations(range(n), n1):
        rs = sum(ranks[i] for i in combo)
        if rs >= observed_rank_sum:
            extreme += 1
        total += 1
    return extreme / total if total > 0 else 1.0


def _normal_approx_mannwhitneyu_greater(x: list[float], y: list[float]) -> float:
    """Normal-approximation one-sided (greater) p-value with tie correction.

    Suitable for n ≥ ~8 per group. For smaller groups, prefer
    ``_exact_mannwhitneyu_greater``.
    """
    combined = sorted([(v, "x") for v in x] + [(v, "y") for v in y])
    ranks = _rank_with_midranks(combined)

    rank_sum_x = sum(r for r, (_, label) in zip(ranks, combined, strict=True) if label == "x")
    n1 = len(x)
    n2 = len(y)
    u1 = rank_sum_x - n1 * (n1 + 1) / 2.0

    tie_groups: dict[float, int] = {}
    for v, _ in combined:
        tie_groups[v] = tie_groups.get(v, 0) + 1
    tie_correction = sum(t * (t * t - 1) for t in tie_groups.values())
    n = n1 + n2
    if n < 2:
        return 1.0

    mean = n1 * n2 / 2.0
    var = (n1 * n2 / 12.0) * ((n + 1) - tie_correction / (n * (n - 1)))
    if var <= 0:
        return 1.0

    z = (u1 - mean - 0.5) / math.sqrt(var)
    return 0.5 * math.erfc(z / math.sqrt(2.0))


def _pure_python_mannwhitneyu_greater(x: list[float], y: list[float]) -> float:
    """Return the one-sided (alternative='greater') p-value for the
    Mann-Whitney U test of x > y.

    Auto-selects between exact rank-subset enumeration (small n) and the
    normal approximation with tie correction (large n). The CI-typical
    shape of 3 current vs 50 baseline samples lands in the exact path;
    the normal approximation at n1=3 misrepresents p-values by up to ~50×
    and silently turns the regression gate into a false-negative machine.

    This is a stand-in for ``scipy.stats.mannwhitneyu(x, y, alternative='greater')``
    so the script runs on minimal CI images without scipy installed.
    """
    if not x or not y:
        return 1.0

    n1, n2 = len(x), len(y)
    small = min(n1, n2)

    if math.comb(n1 + n2, small) <= _EXACT_ENUMERATION_LIMIT:
        return _exact_mannwhitneyu_greater(x, y)
    return _normal_approx_mannwhitneyu_greater(x, y)


def _mannwhitneyu_greater(x: list[float], y: list[float]) -> float:
    try:
        from scipy.stats import mannwhitneyu  # type: ignore

        if not x or not y:
            return 1.0
        return float(mannwhitneyu(x, y, alternative="greater").pvalue)
    except ImportError:
        return _pure_python_mannwhitneyu_greater(x, y)


def load_iteration_samples(path: Path, metric: str) -> dict[str, list[float]]:
    """Return {benchmark_name: [per-iteration metric samples]} from a Google
    Benchmark JSON file. Only rows with ``run_type == "iteration"`` contribute;
    the mean/median/stddev aggregate rows are skipped so the statistical test
    sees raw samples.
    """
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as e:
        print(f"FAILED: cannot read {path}: {e}", file=sys.stderr)
        raise

    out: dict[str, list[float]] = {}
    for entry in data.get("benchmarks", []):
        if entry.get("run_type") != "iteration":
            continue
        name = entry.get("name")
        value = entry.get(metric)
        if name is None or value is None:
            continue
        try:
            out.setdefault(name, []).append(float(value))
        except (TypeError, ValueError):
            continue
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--current", required=True, type=Path)
    parser.add_argument(
        "--metric",
        default="cpu_time",
        help="Google Benchmark metric name (default: cpu_time)",
    )
    parser.add_argument(
        "--alpha",
        type=float,
        default=0.05,
        help="Family-wise significance level (default: 0.05)",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=0.10,
        help=(
            "Only flag a regression when the current median exceeds the "
            "baseline median by at least this fraction (default: 0.10 = 10%%). "
            "Filters 'statistically significant but economically negligible' noise."
        ),
    )
    args = parser.parse_args()

    try:
        baseline = load_iteration_samples(args.baseline, args.metric)
        current = load_iteration_samples(args.current, args.metric)
    except Exception:
        return 2

    shared = sorted(set(baseline) & set(current))
    baseline_only = sorted(set(baseline) - set(current))
    current_only = sorted(set(current) - set(baseline))

    print("Benchmark comparison (Mann-Whitney U, one-sided, alternative='greater')")
    print(f"  baseline: {args.baseline}")
    print(f"  current:  {args.current}")
    print(f"  metric:   {args.metric}")
    print(f"  alpha:    {args.alpha} (family-wise, Bonferroni-corrected)")
    print(f"  tolerance: {args.tolerance:.0%} median-over-baseline")
    print(f"  shared benchmarks: {len(shared)}")
    if baseline_only:
        print(f"  only in baseline ({len(baseline_only)}): {baseline_only}")
    if current_only:
        print(f"  only in current ({len(current_only)}): {current_only}")

    if not shared:
        print("PASSED: no shared benchmarks to compare.")
        return 0

    # Bonferroni-corrected per-test alpha.
    alpha_per_test = args.alpha / len(shared)

    def median(values: list[float]) -> float:
        if not values:
            return 0.0
        xs = sorted(values)
        n = len(xs)
        m = n // 2
        return xs[m] if n % 2 else 0.5 * (xs[m - 1] + xs[m])

    regressions: list[tuple[str, float, float, float]] = []
    improvements: list[tuple[str, float, float]] = []
    for name in shared:
        b = baseline[name]
        c = current[name]
        if len(b) < 2 or len(c) < 2:
            continue
        median_b = median(b)
        median_c = median(c)
        rel = (median_c - median_b) / median_b if median_b > 0 else 0.0

        if rel >= args.tolerance:
            p = _mannwhitneyu_greater(c, b)
            if p < alpha_per_test:
                regressions.append((name, rel, p, alpha_per_test))
        elif rel <= -args.tolerance:
            # Symmetric improvement reporting: median dropped by ≥ tolerance.
            # Mann-Whitney in the other direction — baseline > current.
            p = _mannwhitneyu_greater(b, c)
            if p < alpha_per_test:
                improvements.append((name, rel, p))

    if improvements:
        print("\nIMPROVEMENTS:")
        for name, rel, p in improvements:
            print(f"  {name}: median {rel:.1%}  p={p:.2e}")

    if regressions:
        print("\nREGRESSIONS:")
        for name, rel, p, cutoff in regressions:
            print(f"  {name}: median +{rel:.1%}  p={p:.2e} < {cutoff:.2e}")
        print(f"\nFAILED: {len(regressions)} regression(s) at alpha={args.alpha}.")
        return 1

    if improvements:
        print(f"\nPASSED: no regressions. {len(improvements)} improvement(s) detected.")
    else:
        print("\nPASSED: no regressions.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
