"""Unit tests for scripts/harness_mt_capture_lint.py.

Seeds a synthetic test_harness/ tree and asserts the lint catches the
forbidden reference/capture-all shapes and accepts the value/name-only
shapes. The real harness sources are also linted by a ctest target; this
file guards against regressions in the lint logic itself.
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
SCRIPT = os.path.join(REPO_ROOT, "scripts", "harness_mt_capture_lint.py")


def write(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(textwrap.dedent(content))


def run_script(root: str, paths=("test_harness/src",)):
    cmd = [sys.executable, SCRIPT, "--root", root, "--paths", *paths]
    return subprocess.run(cmd, capture_output=True, text=True, check=False)


class HarnessMtCaptureLintTests(unittest.TestCase):
    def test_catches_capture_all_by_reference(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "bad.cpp"), """
                void handler(int trackId) {
                    int local = 0;
                    runOnTrackSync(trackId, [&](TestTrack& t) {
                        (void)t; (void)local;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("runOnTrackSync", result.stdout)
            self.assertIn("[&]", result.stdout)

    def test_catches_per_variable_reference_capture(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "bad.cpp"), """
                void handler(int trackId) {
                    int local = 0;
                    runOnTrackSync(trackId, [this, &local](TestTrack& t) {
                        (void)t; (void)local;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("runOnTrackSync", result.stdout)
            self.assertIn("this, &local", result.stdout)

    def test_catches_capture_all_by_value(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "bad.cpp"), """
                void handler(int trackId) {
                    int local = 0;
                    runOnTrackSync(trackId, [=](TestTrack& t) {
                        (void)t; (void)local;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("'='", result.stdout)

    def test_catches_run_on_message_thread_blocking_reference(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "bad.cpp"), """
                void handler() {
                    int local = 0;
                    runOnMessageThreadBlocking([&local]() {
                        (void)local;
                    }, 3000);
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)
            self.assertIn("runOnMessageThreadBlocking", result.stdout)

    def test_allows_shared_ptr_value_capture(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "good.cpp"), """
                void handler(int trackId) {
                    auto payload = std::make_shared<int>(42);
                    runOnTrackSync(trackId, [this, payload](TestTrack& t) {
                        (void)t; (void)*payload;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertIn("PASSED", result.stdout)

    def test_allows_this_only_capture(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "good.cpp"), """
                void handler(int trackId) {
                    runOnTrackSync(trackId, [this](TestTrack& t) {
                        (void)t;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_allows_no_captures(self):
        with tempfile.TemporaryDirectory() as tmp:
            write(os.path.join(tmp, "test_harness", "src", "good.cpp"), """
                void handler(int trackId) {
                    runOnTrackSync(trackId, [](TestTrack& t) {
                        (void)t;
                    });
                }
            """)
            result = run_script(tmp)
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_empty_scan_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            # No files created → must exit 1 so a misconfigured path cannot
            # silently pass the gate.
            result = run_script(tmp)
            self.assertEqual(result.returncode, 1, msg=result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
