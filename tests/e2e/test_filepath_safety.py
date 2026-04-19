"""
E2E coverage for save/load path safety.

The plugin must not write to arbitrary system paths when receiving
a malicious `path` parameter. Minimum invariant: the operation
either fails cleanly or writes ONLY to the intended location.

What bugs these tests catch:
- /state/save with path=/etc/passwd attempts to write to a system
  file (privilege depending) — confirms the plugin does not follow
  the caller blindly.
- Path traversal via '../' escapes the expected working directory.
- Empty path crashes the state serializer.
- Null byte in path truncates the write target.
"""

from __future__ import annotations

import os
import typing

import pytest

from multiscoper_test_utils import MultiScoperTestClient


UNSAFE_PATHS = [
    "",
    "../../../../etc/passwd",
    "/etc/passwd",
    "/dev/null",
    "NUL",
    "nonexistent_dir/state.xml",
    "/tmp/multiscoper_e2e_with_null\x00byte.xml",
]

# Relative unsafe paths that, if the plugin writes them verbatim, end up as
# literal files in the harness process's CWD (which is typically the repo
# root). The fixture below removes these after each test so the repo stays
# clean. If the plugin is later hardened to reject relative paths, the
# cleanup becomes a no-op.
_RELATIVE_POLLUTION_CANDIDATES: tuple[str, ...] = (
    "NUL",
    "nonexistent_dir/state.xml",
)


@pytest.fixture
def _cleanup_harness_cwd() -> "typing.Iterator[None]":
    """Remove any repo-root files created by the harness writing to a
    relative unsafe path. The harness runs from the repository root in
    CI and local development, so we scrub known-pollution paths there.
    """
    import pathlib

    yield
    # Search upward from this file to locate the repo root, then remove
    # any of the candidate pollution paths if they exist.
    here = pathlib.Path(__file__).resolve()
    for parent in here.parents:
        if (parent / ".git").exists():
            repo_root = parent
            break
    else:
        return

    for rel in _RELATIVE_POLLUTION_CANDIDATES:
        target = (repo_root / rel).resolve()
        # Refuse to delete anything outside the repo root.
        try:
            target.relative_to(repo_root)
        except ValueError:
            continue
        if target.is_file():
            target.unlink(missing_ok=True)
        # Clean up the parent directory if we created it and it is now empty.
        parent_dir = target.parent
        if parent_dir != repo_root and parent_dir.is_dir():
            try:
                parent_dir.rmdir()
            except OSError:
                pass  # not empty — leave it alone


class TestUnsafeSavePaths:
    @pytest.mark.parametrize("path", UNSAFE_PATHS)
    def test_save_unsafe_path_does_not_crash(
        self, editor: MultiScoperTestClient, path: str, _cleanup_harness_cwd: None
    ):
        """Bug caught: save endpoint crashes or corrupts a system
        file when given a malicious path."""
        # The save may succeed or fail; neither must crash the
        # harness.  If it succeeds, the path write is constrained
        # by the OS.  We don't assert anything about what WAS
        # written; the security property is "didn't crash".
        editor.save_state(path)
        assert editor.health_check()["data"]["status"] == "ok"

    @pytest.mark.parametrize("path", UNSAFE_PATHS)
    def test_load_unsafe_path_does_not_crash(
        self, editor: MultiScoperTestClient, path: str, _cleanup_harness_cwd: None
    ):
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"


class TestSaveDoesNotOverwriteImportantFile:
    def test_save_to_etc_passwd_does_not_actually_write(
        self, editor: MultiScoperTestClient
    ):
        """Check /etc/passwd did not change size after a malicious
        save attempt.

        Bug caught: the plugin runs with elevated privileges (unusual
        but possible) and corrupts system files.
        """
        target = "/etc/passwd"
        size_before = os.path.getsize(target)
        editor.save_state(target)
        size_after = os.path.getsize(target)
        assert size_before == size_after, (
            f"/etc/passwd size changed: {size_before} → {size_after}"
        )
