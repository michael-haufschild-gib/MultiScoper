"""Unit tests for scripts/header_includers.py.

Creates a synthetic repo tree in a temp dir with a hand-crafted
compile_commands.json and a small include graph, then invokes the script as a
subprocess and asserts the set of discovered includer TUs.
"""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

REPO_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SCRIPT = os.path.join(REPO_ROOT, "scripts", "header_includers.py")


def write(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(textwrap.dedent(content))


def run_script(compile_db: str, repo_root: str, headers):
    cmd = [sys.executable, SCRIPT,
           "--compile-db", compile_db,
           "--repo-root", repo_root,
           "--headers", *headers]
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


class HeaderIncludersTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.mkdtemp(prefix="hi_test_")
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.repo = self.tmp
        self.inc = os.path.join(self.repo, "include")
        self.src = os.path.join(self.repo, "src")
        self.build = os.path.join(self.repo, "build")
        os.makedirs(self.inc)
        os.makedirs(self.src)
        os.makedirs(self.build)

        # Header graph:
        #   include/alpha.h        (staged)
        #   include/beta.h         -> #include "alpha.h"
        #   include/gamma.h        (independent)
        write(os.path.join(self.inc, "alpha.h"), '#pragma once\n')
        write(os.path.join(self.inc, "beta.h"),
              '#pragma once\n#include "alpha.h"\n')
        write(os.path.join(self.inc, "gamma.h"), '#pragma once\n')

        # Source files:
        #   src/direct.cpp      -> #include "alpha.h"        (direct)
        #   src/transitive.cpp  -> #include "beta.h"         (transitive)
        #   src/unrelated.cpp   -> #include "gamma.h"        (unrelated)
        write(os.path.join(self.src, "direct.cpp"),
              '#include "alpha.h"\nint direct() { return 1; }\n')
        write(os.path.join(self.src, "transitive.cpp"),
              '#include "beta.h"\nint transitive() { return 2; }\n')
        write(os.path.join(self.src, "unrelated.cpp"),
              '#include "gamma.h"\nint unrelated() { return 3; }\n')

        self.compile_db = os.path.join(self.build, "compile_commands.json")
        entries = []
        for cpp in ("direct.cpp", "transitive.cpp", "unrelated.cpp"):
            entries.append({
                "directory": self.build,
                "command": f"/usr/bin/c++ -I{self.inc} -c {os.path.join(self.src, cpp)}",
                "file": os.path.join(self.src, cpp),
            })
        with open(self.compile_db, "w", encoding="utf-8") as f:
            json.dump(entries, f)

    def _run(self, headers):
        return run_script(self.compile_db, self.repo, headers)

    def test_direct_include_is_found(self):
        r = self._run(["include/alpha.h"])
        self.assertEqual(r.returncode, 0, msg=r.stderr)
        out = set(r.stdout.splitlines())
        self.assertIn("src/direct.cpp", out)

    def test_transitive_include_is_found(self):
        r = self._run(["include/alpha.h"])
        self.assertEqual(r.returncode, 0, msg=r.stderr)
        out = set(r.stdout.splitlines())
        self.assertIn("src/transitive.cpp", out)

    def test_unrelated_tu_is_not_found(self):
        r = self._run(["include/alpha.h"])
        self.assertEqual(r.returncode, 0, msg=r.stderr)
        out = set(r.stdout.splitlines())
        self.assertNotIn("src/unrelated.cpp", out)

    def test_multiple_headers_deduped_and_sorted(self):
        # Staging alpha.h and gamma.h: direct+transitive+unrelated all match.
        r = self._run(["include/alpha.h", "include/gamma.h"])
        self.assertEqual(r.returncode, 0, msg=r.stderr)
        lines = r.stdout.splitlines()
        self.assertEqual(lines, sorted(set(lines)))
        self.assertEqual(
            set(lines),
            {"src/direct.cpp", "src/transitive.cpp", "src/unrelated.cpp"})

    def test_missing_compile_db_exits_2(self):
        r = run_script(os.path.join(self.tmp, "nope.json"), self.repo,
                       ["include/alpha.h"])
        self.assertEqual(r.returncode, 2)

    def test_help_prints_usage(self):
        r = subprocess.run(
            [sys.executable, SCRIPT, "--help"],
            capture_output=True, text=True, check=False)
        self.assertEqual(r.returncode, 0)
        self.assertIn("--compile-db", r.stdout)
        self.assertIn("--headers", r.stdout)


if __name__ == "__main__":
    unittest.main()
