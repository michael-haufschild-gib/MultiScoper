"""
E2E coverage for editor detach / reattach without destroying the processor
(Primitive 2.5).

The old harness only modelled a clean teardown: hideEditor destroyed the
editor and showEditor created a fresh one.  Real DAWs (Logic, Cubase,
Studio One) reparent the editor component — the window goes away, the
editor stays alive, then a later reopen reattaches the same editor to a
new host window.

`OscilPluginEditor::parentHierarchyChanged` (PluginEditor.cpp:247-256)
has the detach-OpenGL-on-reparent branch that fires every time the
editor's parent goes null.  This branch was never exercised by the old
harness because it only fired on destruction.  This test file covers it.

Endpoints under test
--------------------
  - POST /track/{id}/detachEditor   : clears the editor from its window
                                      but keeps the editor alive.
  - POST /track/{id}/reattachEditor : puts the surviving editor back as
                                      the window's content.

Assertions
----------
  1. 50 detach/reattach cycles under active playback do not crash.
  2. Registered element IDs remain populated after every cycle — the
     editor is still wired into the TestElementRegistry (if ids went
     empty, either the editor tore itself down, or the registry
     invalidated itself on reparent).
  3. The test server's port binding (PluginEditor.cpp:118-123) does not
     collide — a second bind attempt on the same port would surface as
     a crash or elements-missing.
"""

from __future__ import annotations

import pytest

from oscil_test_utils import OscilTestClient


def _detach(client: OscilTestClient, track_id: int = 0) -> dict:
    resp = client._post_json(f"/track/{track_id}/detachEditor")
    assert resp is not None and resp.get("success"), f"detach failed: {resp}"
    return resp.get("data", {})


def _reattach(client: OscilTestClient, track_id: int = 0) -> dict:
    resp = client._post_json(f"/track/{track_id}/reattachEditor")
    assert resp is not None and resp.get("success"), f"reattach failed: {resp}"
    return resp.get("data", {})


class TestDetachReattachCycle:
    def test_single_detach_then_reattach(self, editor: OscilTestClient):
        """
        Bare-minimum sanity: after open_editor, detach clears the window
        content but leaves the editor alive; reattach restores it.
        """
        d = _detach(editor)
        assert d.get("detached") is True, (
            f"First detach should succeed, got {d}"
        )

        r = _reattach(editor)
        assert r.get("reattached") is True, (
            f"Reattach should succeed after detach, got {r}"
        )

    def test_idempotent_detach_reports_false(self, editor: OscilTestClient):
        """Calling detach twice in a row returns {detached: false} the
        second time — the operation is genuinely a no-op, not a crash."""
        _detach(editor)
        second = _detach(editor)
        assert second.get("detached") is False, second

    def test_fifty_cycles_under_playback_no_crash(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        The real stress test.  50 detach/reattach passes while audio is
        playing, with an element-id check after every reattach to prove
        the editor is still wired into the registry.
        """
        editor.add_oscillator(source_id, name="Editor Lifecycle")
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=5.0, desc="transport playing"
        )

        baseline_ids = set(editor.get_registered_element_ids())
        assert baseline_ids, "Editor must register elements before the cycle"

        for i in range(50):
            _detach(editor)
            _reattach(editor)

            # After reattach the element registry should remain populated.
            # We poll rather than assert directly to accommodate the
            # OpenGL reattach path taking a couple of frames.
            editor.wait_until(
                lambda: bool(editor.get_registered_element_ids()),
                timeout_s=5.0,
                desc=f"elements registered after reattach #{i}",
            )

        # Final sanity: the set of elements should still cover whatever
        # was there before the sweep.  We don't require exact equality —
        # some elements (scroll positions, etc.) legitimately come and
        # go — but the baseline sidebar/timing/options must still exist.
        post_ids = set(editor.get_registered_element_ids())
        core = {"sidebar", "sidebar_addOscillator", "sidebar_timing", "sidebar_options"}
        missing = core - post_ids
        assert not missing, (
            f"Core elements lost after 50 cycles: {missing}. "
            f"Baseline={len(baseline_ids)} Post={len(post_ids)}"
        )


class TestDetachReattachInvariants:
    def test_detach_without_editor_is_graceful(self, client: OscilTestClient):
        """
        Without a prior open_editor, detach returns success with
        detached=false.  The harness must not 500 just because there is
        nothing to detach.
        """
        client.close_editor(0)
        # No editor = nothing to detach, but the handler still returns 200.
        resp = client._post_json("/track/0/detachEditor")
        assert resp is not None and resp.get("success"), resp
        assert resp.get("data", {}).get("detached") is False
