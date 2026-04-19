"""Unit tests for scripts/format_lint.py.

Seeds a temporary tree with clang-format-conformant and non-conformant files
and asserts the script's exit code and violation report. Skips the whole
suite when clang-format is not available on PATH.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SCRIPT = os.path.join(REPO_ROOT, "scripts", "format_lint.py")
CLANG_FORMAT_CFG = os.path.join(REPO_ROOT, ".clang-format")


def _have_clang_format() -> bool:
    return shutil.which("clang-format") is not None


def write(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def run_script(root: str, paths=("src",)):
    cmd = [sys.executable, SCRIPT, "--root", root, "--paths", *paths]
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


@unittest.skipUnless(_have_clang_format(), "clang-format not available")
class FormatLintTests(unittest.TestCase):
    def _seed_tree(self, tmp: str) -> None:
        # Copy the project's .clang-format so the temp tree uses identical rules.
        shutil.copy(CLANG_FORMAT_CFG, os.path.join(tmp, ".clang-format"))

    def test_conformant_file_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._seed_tree(tmp)
            # clang-format the file first so we know it's conformant under the
            # project's rules, then run the lint.
            src = os.path.join(tmp, "src", "ok.cpp")
            write(src, "int main()\n{\n    return 0;\n}\n")
            subprocess.run(["clang-format", "-i", src], check=True)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("PASSED", result.stdout)

    def test_non_conformant_file_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._seed_tree(tmp)
            # Deliberately odd indentation (1 space instead of 4) and K&R
            # brace-on-same-line — both violate the Allman+4-space config.
            write(
                os.path.join(tmp, "src", "bad.cpp"),
                "int main() {\n return 0;\n}\n",
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("bad.cpp", result.stdout)
            self.assertIn("FAILED", result.stdout)

    def test_empty_tree_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._seed_tree(tmp)
            os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("PASSED", result.stdout)

    def test_build_and_deps_paths_are_excluded(self):
        with tempfile.TemporaryDirectory() as tmp:
            self._seed_tree(tmp)
            # Seed a non-conformant file inside a build dir — the script must
            # not flag it because build/_deps are generated/third-party.
            write(
                os.path.join(tmp, "src", "build", "gen.cpp"),
                "int main() {\n  return 0;}\n",
            )
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_missing_clang_format_reports_error(self):
        # Force a bogus binary name so we hit the "not on PATH" code path.
        with tempfile.TemporaryDirectory() as tmp:
            self._seed_tree(tmp)
            write(os.path.join(tmp, "src", "ok.cpp"), "int main() { return 0; }\n")
            cmd = [
                sys.executable,
                SCRIPT,
                "--root",
                tmp,
                "--paths",
                "src",
                "--clang-format",
                "clang-format-definitely-not-installed-xyzzy",
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 2)
            self.assertIn("not found on PATH", result.stderr)

    def test_real_repo_passes(self):
        result = run_script(REPO_ROOT, paths=("include", "src", "tests", "test_harness"))
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
