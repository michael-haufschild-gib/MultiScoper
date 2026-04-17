"""Unit tests for scripts/test_quality_lint.py.

Covers the sanitizer and the existence-only / tautology detectors with
hand-crafted fixtures, so the CI gate keeps rejecting shallow tests
even when the author reflows them across multiple lines.

The goal is not to re-test the real repo (the ctest target does that);
it is to pin the linter's behaviour against specific regressions that
formatting could otherwise sneak past.
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import textwrap
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SCRIPT = os.path.join(REPO_ROOT, "scripts", "test_quality_lint.py")


def write(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(textwrap.dedent(content))


def run_script(root: str, paths=("tests",), min_assertions: int = 1):
    cmd = [
        sys.executable,
        SCRIPT,
        "--root",
        root,
        "--paths",
        *paths,
        "--min-assertions",
        str(min_assertions),
    ]
    # S603: the executable is sys.executable and the script path is repo-fixed;
    # all other args come from hand-rolled fixtures in this file.
    return subprocess.run(  # noqa: S603
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
    )


class TestQualityLintTests(unittest.TestCase):
    def test_catches_single_line_existence_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(
                os.path.join(tmp, "tests", "test_shallow.cpp"),
                """
                #include <gtest/gtest.h>
                TEST(ShallowSuite, OnlyExistence) {
                    auto value = make();
                    EXPECT_TRUE(value.has_value());
                    EXPECT_NE(value, nullptr);
                }
                """,
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("existence-only", result.stdout)

    def test_catches_wrapped_existence_only_assertions(self):
        # Regression: wrapped `EXPECT_TRUE(\n    x.has_value())` and
        # `EXPECT_GT(\n    items.size(), 0)` used to slip past the
        # per-line matcher.
        with tempfile.TemporaryDirectory() as tmp:
            write(
                os.path.join(tmp, "tests", "test_shallow_wrapped.cpp"),
                """
                #include <gtest/gtest.h>
                TEST(ShallowSuite, WrappedOnlyExistence) {
                    auto value = make();
                    EXPECT_TRUE(
                        value.has_value());
                    EXPECT_GT(
                        items.size(), 0);
                }
                """,
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("existence-only", result.stdout)

    def test_allows_behavioral_assertions(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(
                os.path.join(tmp, "tests", "test_ok.cpp"),
                """
                #include <gtest/gtest.h>
                TEST(BehaviorSuite, ChecksValues) {
                    auto result = compute(2, 3);
                    EXPECT_EQ(result, 5);
                    EXPECT_NEAR(other(), 1.0f, 0.001f);
                }
                """,
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_catches_zero_assertions(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(
                os.path.join(tmp, "tests", "test_empty.cpp"),
                """
                #include <gtest/gtest.h>
                TEST(EmptySuite, NoAsserts) {
                    int x = 1;
                    (void) x;
                }
                """,
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("zero assertions", result.stdout)

    def test_catches_tautology(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(
                os.path.join(tmp, "tests", "test_taut.cpp"),
                """
                #include <gtest/gtest.h>
                TEST(TautologySuite, AlwaysTrue) {
                    EXPECT_TRUE(true);
                }
                """,
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("tautological", result.stdout)

    @unittest.skipUnless(
        os.environ.get("OSCIL_LINT_FULL_REPO") == "1",
        "Full-repo scan is owned by the ctest target; "
        "set OSCIL_LINT_FULL_REPO=1 to opt in locally.",
    )
    def test_real_repo_passes(self):
        # Opt-in only: the ctest target is the canonical repo-wide gate.
        # Pinning this assertion here used to make unrelated shallow tests
        # fail this unit file even when the linter itself was correct.
        result = run_script(REPO_ROOT)
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
