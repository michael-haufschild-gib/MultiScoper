"""Unit tests for scripts/bench_compare.py.

Seeds synthetic Google Benchmark JSON files and verifies the Mann-Whitney U
comparison correctly flags real regressions, tolerates noise, and handles
missing/extra benchmarks gracefully.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SCRIPT = os.path.join(REPO_ROOT, "scripts", "bench_compare.py")


def _make_bench_json(path: str, samples_by_name: dict[str, list[float]]) -> None:
    """Write a minimal Google Benchmark v1.9+ JSON compatible with the
    script's ``run_type == 'iteration'`` consumer.
    """
    entries = []
    for name, samples in samples_by_name.items():
        for v in samples:
            entries.append(
                {
                    "name": name,
                    "run_name": name,
                    "run_type": "iteration",
                    "cpu_time": v,
                    "real_time": v,
                    "iterations": 1,
                    "time_unit": "ns",
                }
            )
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"benchmarks": entries}, f)


def _run(*args):
    return subprocess.run(
        [sys.executable, SCRIPT, *args], capture_output=True, text=True, check=False
    )


class BenchCompareTests(unittest.TestCase):
    def test_no_regression_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            samples = [100.0, 102.0, 98.0, 101.0, 99.0, 103.0, 97.0, 100.0, 102.0]
            _make_bench_json(base, {"BM_A": samples, "BM_B": samples})
            _make_bench_json(curr, {"BM_A": samples, "BM_B": samples})
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("PASSED", result.stdout)

    def test_regression_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            base_samples = [100.0] * 20
            curr_samples = [140.0] * 20  # 40% regression — far above 10% tolerance
            _make_bench_json(base, {"BM_A": base_samples})
            _make_bench_json(curr, {"BM_A": curr_samples})
            result = _run("--baseline", base, "--current", curr, "--alpha", "0.05")
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("REGRESSIONS", result.stdout)
            self.assertIn("BM_A", result.stdout)

    def test_noise_below_tolerance_passes(self):
        # Small +5% shift should not trip the 10% economic-tolerance floor
        # even if the statistical test is significant.
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {"BM_A": [100.0] * 20})
            _make_bench_json(curr, {"BM_A": [105.0] * 20})
            result = _run("--baseline", base, "--current", curr, "--tolerance", "0.10")
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_new_benchmark_is_ignored_not_flagged(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {"BM_A": [100.0] * 9})
            _make_bench_json(curr, {"BM_A": [100.0] * 9, "BM_NEW": [50.0] * 9})
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("BM_NEW", result.stdout)

    def test_disjoint_benchmark_sets_fail(self):
        # Both sides populated but with no overlapping benchmark names is
        # almost always a rename or suite-naming mismatch, not a legitimate
        # pass. Exit 2 guards against silently disabling the regression gate.
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {"BM_OLD": [100.0] * 9})
            _make_bench_json(curr, {"BM_DIFFERENT": [100.0] * 9})
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 2, msg=result.stdout + result.stderr)
            self.assertIn("do not overlap", result.stderr)

    def test_both_sides_empty_still_passes(self):
        # If neither input has benchmarks, there's nothing to compare and
        # nothing to rename-detect — keep the benign exit-0 path for that.
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {})
            _make_bench_json(curr, {})
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("no shared benchmarks", result.stdout)

    def test_bonferroni_correction_prevents_false_positive(self):
        # 24 benchmarks — all identical means no regression. Without Bonferroni,
        # an alpha=0.05 run over 24 independent tests has a family-wise
        # false-positive rate ≈1 − 0.95^24 ≈ 71%. With Bonferroni this becomes
        # ≈0.05. Verify we don't flag on identical data regardless.
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            samples = [100.0 + (i % 5) for i in range(20)]  # tiny jitter
            names = {f"BM_{i}": samples for i in range(24)}
            _make_bench_json(base, names)
            _make_bench_json(curr, names)
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_malformed_json_exits_2(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            with open(base, "w", encoding="utf-8") as f:
                f.write("not json")
            _make_bench_json(curr, {"BM_A": [100.0] * 9})
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 2)

    def test_small_n_asymmetric_regression_fires(self):
        # The CI-typical shape: 3 current PR samples vs 50 baseline samples,
        # with a realistic ~12% regression under measurement noise.
        # Pre-fix the normal-approximation fallback returned p ≈ 2e-3 (above
        # the Bonferroni cutoff of ~0.05); the exact path returns p ≈ 4e-5
        # and the gate correctly fires.
        import random

        random.seed(1234)
        base_samples = [random.gauss(100.0, 5.0) for _ in range(50)]
        curr_samples = [random.gauss(112.0, 5.0) for _ in range(3)]

        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {"BM_A": base_samples})
            _make_bench_json(curr, {"BM_A": curr_samples})
            result = _run(
                "--baseline",
                base,
                "--current",
                curr,
                "--alpha",
                "0.05",
                "--tolerance",
                "0.10",
            )
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("BM_A", result.stdout)

    def test_improvement_reported_not_failed(self):
        # A symmetric improvement (current faster than baseline by more than
        # the tolerance) must be logged in the output but must not change the
        # exit code — improvements never fail the gate.
        with tempfile.TemporaryDirectory() as tmp:
            base = os.path.join(tmp, "base.json")
            curr = os.path.join(tmp, "curr.json")
            _make_bench_json(base, {"BM_A": [140.0] * 20})
            _make_bench_json(curr, {"BM_A": [100.0] * 20})  # ~28% faster
            result = _run("--baseline", base, "--current", curr)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("IMPROVEMENTS", result.stdout)
            self.assertIn("BM_A", result.stdout)
            self.assertIn("improvement(s) detected", result.stdout)

    def test_exact_mw_matches_scipy_when_available(self):
        # For the CI-typical 3v50 shape with clear separation, the pure-Python
        # exact enumeration must agree with scipy's auto-method to within
        # floating-point tolerance. This guards against the exact-enumeration
        # implementation drifting from the reference.
        try:
            from scipy.stats import mannwhitneyu  # type: ignore
        except ImportError:
            self.skipTest("scipy not available")

        import importlib.util

        spec = importlib.util.spec_from_file_location("bench_compare", SCRIPT)
        bench_compare = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(bench_compare)

        x = [110.0, 111.0, 114.5]
        y = [100.0 + i * 0.1 for i in range(50)]
        scipy_p = float(mannwhitneyu(x, y, alternative="greater").pvalue)
        pure_p = bench_compare._exact_mannwhitneyu_greater(x, y)
        self.assertAlmostEqual(scipy_p, pure_p, places=10)


if __name__ == "__main__":
    unittest.main()
