"""
E2E coverage for pane removal with bound oscillators.

When a pane is removed, any oscillators assigned to it must be handled
coherently — either reassigned to another pane, marked invisible, or
cleanly orphaned with a valid state the UI can recover from.

What bugs these tests catch:
- Oscillators left with a dangling paneId pointing at a gone pane
  (state appears intact but UI can't render them and Delete requires
  re-opening the select-pane dialog).
- Remove_pane returns success but the pane is still in state (orphan
  pane from the UI side).
- Cascade delete loses oscillators (wrong product decision silently
  applied).
- Removing the last pane leaves orphan oscillators with no way to
  re-display them.
"""

from __future__ import annotations

from multiscoper_test_utils import MultiScoperTestClient


class TestRemovePaneWithBoundOscillators:
    """When a pane is removed, bound oscillators must be handled
    coherently."""

    def test_remove_non_last_pane_reassigns_or_orphans(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Create two panes, one osc each, remove the first pane.
        The surviving osc in pane 2 must still be renderable; the
        osc in pane 1 must be reassigned OR orphaned — never leave
        state inconsistent.
        """
        id_a = editor.add_oscillator(source_id, name="A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("Second Pane")
        assert pane2_id is not None, "add_pane must succeed for this test"

        id_b = editor.add_oscillator(source_id, name="B", pane_id=pane2_id)
        assert id_b is not None
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) == 2
        pane_first = next(p for p in panes if p["id"] != pane2_id)

        editor.remove_pane(pane_first["id"])
        editor.wait_until(
            lambda: len(editor.get_panes()) == 1,
            timeout_s=3.0,
            desc="pane count to drop to 1",
        )

        # Oscillators must both still be valid objects in state.
        oscs = editor.get_oscillators()
        osc_ids = {o["id"] for o in oscs}
        assert id_a in osc_ids and id_b in osc_ids, (
            f"both oscillators must survive pane removal; got {osc_ids}"
        )

        # The osc in pane 2 must still point at pane 2.
        remaining_pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in oscs:
            # Every oscillator's paneId must now be valid (points to
            # a pane that exists) OR empty (orphaned but coherent).
            pid = osc.get("paneId", "")
            assert pid == "" or pid in remaining_pane_ids, (
                f"osc {osc['id']} has dangling paneId={pid!r}"
            )


class TestRemoveLastPaneWithOscillators:
    """Removing the only pane with bound oscillators tests the edge
    case of "no valid pane remains"."""

    def test_remove_last_pane_is_refused_cleanly(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """The server MUST refuse to remove the last pane — this is the
        guard that prevents the "null pane context" path in the UI
        refresh from ever running in production. The test previously
        claimed to catch that crash but didn't assert the refusal, so
        it passed vacuously when the server already declined the call
        (and would have equally passed if the server silently erased the
        pane). Verify the refusal + state invariants explicitly.
        """
        editor.add_oscillator(source_id, name="A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) == 1

        # The API must return False (server-side "Cannot remove the last pane").
        refused = not editor.remove_pane(panes[0]["id"])
        assert refused, (
            "Server must refuse to remove the last pane — removing it would "
            "leave oscillators with no valid pane for the UI refresh"
        )

        # Post-condition: pane still there, oscillator still there and bound to it.
        panes_after = editor.get_panes()
        assert len(panes_after) == 1, (
            f"Refused removal must not mutate state; pane count={len(panes_after)}"
        )
        assert panes_after[0]["id"] == panes[0]["id"], (
            "Same pane must still exist after refused removal"
        )

        oscs = editor.get_oscillators()
        assert len(oscs) == 1, "Oscillator must survive the refused removal"
        assert oscs[0].get("paneId") == panes[0]["id"], (
            f"Oscillator's paneId should still point at the surviving pane, "
            f"got {oscs[0].get('paneId')!r}"
        )

        # Harness must stay responsive.
        assert editor.health_check()["data"]["status"] == "ok"


class TestPaneCountAfterMixedAddRemove:
    """Add/remove a sequence of panes and verify count invariants."""

    def test_add_remove_pane_cycle(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Verify that N adds followed by N removes returns to the
        original pane count.  Bug caught: add_pane leaks a pane after
        a remove_pane (orphan state entry)."""
        osc_id = editor.add_oscillator(source_id, name="Seed")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        baseline_count = len(editor.get_panes())

        added_panes = []
        for i in range(5):
            pid = editor.add_pane(f"Cycle{i}")
            if pid:
                added_panes.append(pid)
        assert len(added_panes) == 5

        editor.wait_until(
            lambda: len(editor.get_panes()) == baseline_count + 5,
            timeout_s=5.0,
            desc=f"pane count to reach {baseline_count + 5}",
        )

        for pid in added_panes:
            editor.remove_pane(pid)

        editor.wait_until(
            lambda: len(editor.get_panes()) == baseline_count,
            timeout_s=5.0,
            desc=f"pane count to return to {baseline_count}",
        )
