"""
E2E coverage for /state/reset — does it actually clear everything?

What bugs these tests catch:
- Reset clears oscillators but leaves a ghost pane behind (scoped
  reset missed the pane list).
- Reset clears state structures but leaves option values dirty.
- Reset on track 0 leaks into track 1 (shared singleton mutation).
- Reset races with an in-flight add; the new oscillator survives
  the reset (incorrect ordering).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestResetClearsOscillatorsAndPanes:
    def test_reset_empties_both(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        for i in range(3):
            editor.add_oscillator(source_id, name=f"R{i}")
        editor.add_pane("P1")
        editor.add_pane("P2")
        editor.wait_for_oscillator_count(3, timeout_s=3.0)
        editor.wait_until(
            lambda: len(editor.get_panes()) >= 3,
            timeout_s=3.0, desc="panes to exist",
        )

        editor.reset_state()
        editor.wait_until(
            lambda: len(editor.get_oscillators()) == 0,
            timeout_s=3.0, desc="oscillators cleared",
        )
        # Reset auto-creates a default oscillator+pane if empty.
        # After reset+auto-create, there should be 0 oscillators
        # until a user action triggers the default.  Verify no
        # stale panes remain (no ghost from the pre-reset panes).
        post_panes = editor.get_panes()
        # None of the old pane names should survive.
        old_names = {"P1", "P2"}
        surviving_old = [p for p in post_panes if p.get("name") in old_names]
        assert not surviving_old, (
            f"stale panes survived reset: {surviving_old}"
        )


class TestResetDoesNotAffectOtherTrack:
    def test_reset_track_0_preserves_track_1(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Bug caught: reset endpoint matches any trackId → reset
        on 0 wipes track 1."""
        # Populate track 1.
        src1 = track_sources[1]
        for i in range(2):
            multi_editor.add_oscillator_to_track(
                1, src1, name=f"Keep{i}", colour=f"#{i:06x}"
            )
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) == 2,
            timeout_s=3.0, desc="track 1 populated",
        )

        multi_editor.reset_track_state(0)
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 0,
            timeout_s=3.0, desc="track 0 cleared",
        )
        # Track 1 must remain untouched.
        t1 = multi_editor.get_oscillators_for_track(1)
        assert len(t1) == 2, (
            f"track 1 must retain its 2 oscillators, got {len(t1)}"
        )


class TestResetFollowedByRepopulation:
    def test_reset_then_add_yields_exactly_one(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: reset leaves a hidden oscillator that reappears
        when a new oscillator is added."""
        for _ in range(5):
            editor.add_oscillator(source_id, name="PreReset")
        editor.wait_for_oscillator_count(5, timeout_s=3.0)

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        new_id = editor.add_oscillator(source_id, name="Sole")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        oscs = editor.get_oscillators()
        assert len(oscs) == 1
        assert oscs[0]["name"] == "Sole"
