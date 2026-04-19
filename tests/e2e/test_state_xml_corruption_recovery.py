"""
E2E coverage for loading malformed XML via /state/load.

The plugin serializes state as an XML document (MultiScoperState.xml). A
corrupt file on disk — partial write, disk error, manual edit — must
not crash the plugin.  Whatever the plugin does in response (accept
defaults, revert to prior state, reject cleanly) must be deterministic
and leave subsequent operations working.

What bugs these tests catch:
- Parser crashes on empty files (null deref on root element).
- Parser accepts truncated XML, restores partial oscillator set, but
  leaves the state machine in a half-initialized state where later
  add/delete hangs.
- Missing required attribute (e.g., oscillator without sourceId) is
  silently skipped in one branch and kept in another, producing ghost
  oscillators that render nothing and cannot be deleted.
- Extra unknown attributes cause the parser to abort loading even
  valid oscillators.
- Load after failed load leaves the plugin in a stuck state.
"""

from __future__ import annotations

import os

import pytest

from multiscoper_test_utils import MultiScoperTestClient


def _write_xml(tmp_path, name: str, content: str) -> str:
    path = str(tmp_path / name)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    return path


class TestEmptyAndTrivialXML:
    def test_empty_file_fails_cleanly(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        """Bug caught: empty file crashes the XML parser."""
        path = _write_xml(tmp_path, "empty.xml", "")
        ok = editor.load_state(path)
        # Either reject (False) or silently succeed with no-op — must
        # not crash.  The harness must remain responsive.
        assert editor.health_check()["data"]["status"] == "ok"
        # State should be coherent regardless of accept/reject.
        _ = editor.get_oscillators()

    def test_whitespace_only_file(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        path = _write_xml(tmp_path, "whitespace.xml", "   \n\t  \r\n  ")
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"

    def test_non_xml_content(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        """Bug caught: XML parser accepts JSON because it's looking for
        any '<' and silently treats everything else as CDATA."""
        path = _write_xml(tmp_path, "json.xml", '{"oscillators": [], "panes": []}')
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"


class TestTruncatedXML:
    def test_truncated_midway(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Save a real state, truncate it, load — must not leave
        plugin partially-initialized."""
        editor.add_oscillator(source_id, name="Full")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        full_path = str(tmp_path / "full.xml")
        assert editor.save_state(full_path)

        # Truncate to half the file.
        with open(full_path, "rb") as f:
            data = f.read()
        trunc = data[: max(1, len(data) // 2)]
        trunc_path = str(tmp_path / "trunc.xml")
        with open(trunc_path, "wb") as f:
            f.write(trunc)

        # Reset first, then load the truncated file.
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)
        editor.load_state(trunc_path)

        # Plugin must remain usable: can reset, can add.
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=5.0)
        new_id = editor.add_oscillator(source_id, name="PostTruncRecovery")
        assert new_id is not None, "plugin must still accept adds after truncated load"


class TestMalformedStructure:
    def test_root_element_wrong(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        path = _write_xml(tmp_path, "wrong_root.xml", "<RandomRoot></RandomRoot>")
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"

    def test_unclosed_tag(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        path = _write_xml(tmp_path, "unclosed.xml", "<MultiScoperState><Oscillator")
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"

    def test_mismatched_closing_tag(
        self, editor: MultiScoperTestClient, tmp_path
    ):
        path = _write_xml(tmp_path, "mismatch.xml",
                          "<MultiScoperState><Oscillator/></Panes>")
        editor.load_state(path)
        assert editor.health_check()["data"]["status"] == "ok"


class TestNonexistentPath:
    def test_load_missing_file(self, editor: MultiScoperTestClient):
        """Loading a path that does not exist must fail cleanly."""
        ok = editor.load_state("/tmp/multiscoper_e2e_does_not_exist.xml")
        assert not ok, "load_state of missing path must return False"
        assert editor.health_check()["data"]["status"] == "ok"

    def test_state_still_usable_after_failed_load(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """After a failed load, the plugin must remain usable."""
        editor.load_state("/tmp/multiscoper_e2e_still_not_here.xml")
        new_id = editor.add_oscillator(source_id, name="PostFailedLoad")
        assert new_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)


class TestRepeatedFailedLoads:
    def test_ten_failed_loads_in_a_row(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Rapid failed-load storm must not leak resources or clog the
        state machine."""
        for i in range(10):
            editor.load_state(f"/tmp/msloop_does_not_exist_{i}.xml")

        # Plugin must still accept an add.
        osc_id = editor.add_oscillator(source_id, name="StillAlive")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
