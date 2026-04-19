"""Unit tests for scripts/forbidden_patterns_lint.py.

Seeds a temporary repo tree with files that either contain the forbidden
patterns (expected to fail the lint) or legitimate uses (expected to pass)
and asserts the script's exit code and offender report.

The goal is not to test the real repo — the lint already runs over the real
repo as a ctest target — but to guarantee the lint SCRIPT catches what it
claims to catch and does not self-trigger on its own documentation.
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import textwrap
import unittest

REPO_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
SCRIPT = os.path.join(REPO_ROOT, "scripts", "forbidden_patterns_lint.py")


def write(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(textwrap.dedent(content))


def run_script(root: str, paths=("src",)):
    cmd = [sys.executable, SCRIPT, "--root", root, "--paths", *paths]
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


class ForbiddenPatternsLintTests(unittest.TestCase):
    def test_catches_setDefaultLookAndFeel(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "evil.cpp"), """
                #include <juce_gui_basics/juce_gui_basics.h>
                namespace multiscoper {
                void installDefault() {
                    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
                }
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("setDefaultLookAndFeel", result.stdout)
            self.assertIn("src/evil.cpp", result.stdout)

    def test_catches_continuous_repainting_true(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "evil.cpp"), """
                void configureContext(juce::OpenGLContext& ctx) {
                    ctx.setContinuousRepainting(true);
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("setContinuousRepainting(true)", result.stdout)

    def test_allows_continuous_repainting_false(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "ok.cpp"), """
                void configureContext(juce::OpenGLContext& ctx) {
                    ctx.setContinuousRepainting(false);
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_ignores_forbidden_pattern_inside_a_comment(self):
        # The script's own rationale comments reference the pattern; that must
        # not self-trigger. Simulate by embedding the forbidden token only
        # inside a comment.
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "doc.cpp"), """
                // NOTE: setDefaultLookAndFeel(nullptr) is forbidden here — see ADR-014.
                /* Also never call setContinuousRepainting(true) from plugin code. */
                void legitFunction() {}
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_ignores_forbidden_pattern_inside_a_string_literal(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "str.cpp"), """
                const char* describe() {
                    return "Never call setDefaultLookAndFeel from plugin code";
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_fails_closed_on_empty_tree(self):
        # A zero-file scan almost always means --root / --paths were
        # misconfigured; silently exiting 0 would let CI stop enforcing
        # the forbidden-patterns gate without anyone noticing.
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("no source files matched", result.stderr + result.stdout)

    def test_catches_wrapped_forbidden_call(self):
        # Regression: a forbidden call split across lines — e.g.
        # `setContinuousRepainting\n(true)` — must still be reported at the
        # identifier's line, not slip past per-line scanning.
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "src", "evil.cpp"), """
                void configureContext(juce::OpenGLContext& ctx) {
                    ctx.setContinuousRepainting
                        (true);
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("setContinuousRepainting(true)", result.stdout)

    def test_catches_stdio_logging(self):
        for snippet in [
            'std::printf("%s", "x");',
            'std::cout << "hello" << std::endl;',
            'std::cerr << "oops";',
            'printf("%d", 1);',
            'fprintf(stderr, "%d", 1);',
        ]:
            with self.subTest(call=snippet):
                with tempfile.TemporaryDirectory() as tmp:
                    write(os.path.join(tmp, "src", "evil.cpp"), f"""
                        void bad() {{ {snippet} }}
                    """)
                    result = run_script(tmp)
                    self.assertEqual(
                        result.returncode,
                        1,
                        msg=f"{snippet} did not trigger lint.\n{result.stdout}\n{result.stderr}",
                    )
                    self.assertIn("stdio logging", result.stdout)

    def test_real_repo_passes(self):
        # Sanity: the actual repository must not contain forbidden patterns;
        # the ctest target covers this but we double-check from Python so a
        # developer running just this unit-test file gets fast feedback.
        result = run_script(REPO_ROOT)
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
