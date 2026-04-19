"""
E2E coverage for pane reordering — both the /pane/move API and
drag-on-header gesture.

What bugs these tests catch:
- pane/move API returns success=true but the pane list order is
  unchanged (handler no-ops silently).
- Moving a pane index out-of-range corrupts the list order.
- Reorder not preserved across save/load.
"""

from __future__ import annotations

from multiscoper_test_utils import MultiScoperTestClient


class TestPaneMoveAPI:
    def test_move_first_to_last(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Three panes; move the first to the last slot.  Verify
        the resulting order."""
        editor.add_oscillator(source_id, name="P0")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        p2 = editor.add_pane("P1")
        p3 = editor.add_pane("P2")
        assert p2 is not None and p3 is not None, (
            "add_pane must succeed for this test"
        )

        before = [p["id"] for p in editor.get_panes()]
        assert len(before) == 3

        # Move index 0 → 2.
        ok = editor.move_pane_position(from_index=0, to_index=2)
        assert ok, "move_pane_position must return success"

        editor.wait_until(
            lambda: [p["id"] for p in editor.get_panes()] != before,
            timeout_s=3.0,
            desc="pane order to change after move",
        )
        after = [p["id"] for p in editor.get_panes()]
        assert after[2] == before[0], (
            f"moved pane must end up at index 2: {before} -> {after}"
        )

    def test_move_out_of_range_does_not_corrupt(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        editor.add_oscillator(source_id, name="P0")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("P1")
        editor.wait_until(
            lambda: len(editor.get_panes()) == 2,
            timeout_s=3.0, desc="second pane to be added",
        )

        before = [p["id"] for p in editor.get_panes()]

        # Try to move into an impossible slot.
        editor.move_pane_position(from_index=0, to_index=99)
        editor.wait_until(
            lambda: len(editor.get_panes()) == len(before),
            timeout_s=2.0, desc="pane count unchanged after bad move",
        )
        after = [p["id"] for p in editor.get_panes()]
        # Panes may still be the same order (no-op) or clamped-reordered,
        # but no pane must be lost.
        assert set(after) == set(before), (
            f"panes lost after bad move: {set(before) - set(after)}"
        )


class TestPaneOrderStatePersistence:
    def test_pane_order_survives_save_load(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        editor.add_oscillator(source_id, name="R0")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("R1")
        editor.add_pane("R2")
        editor.wait_until(
            lambda: len(editor.get_panes()) == 3,
            timeout_s=3.0, desc="3 panes to exist",
        )

        # Capture the pre-move first name so we can wait for it to appear
        # at index 2 — the actual post-move contract.
        pre_first = editor.get_panes()[0]["name"]

        # Move pane 0 to 2.
        editor.move_pane_position(from_index=0, to_index=2)
        # Previously this wait was `lambda: ... or True`, an always-True
        # predicate that made the wait a no-op (the conditions were
        # inverted; without the `or True` the wait would time out). The
        # real post-condition is "the previously-first pane is now at
        # index 2".
        editor.wait_until(
            lambda: len(editor.get_panes()) == 3
            and editor.get_panes()[2]["name"] == pre_first,
            timeout_s=3.0,
            desc=f"pane '{pre_first}' to settle at index 2 after move",
        )
        expected_names = [p["name"] for p in editor.get_panes()]
        assert expected_names[2] == pre_first, (
            f"moved pane must be at index 2; got order {expected_names}"
        )

        path = str(tmp_path / "pane_order.xml")
        assert editor.save_state(path)
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)
        assert editor.load_state(path)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        restored_names = [p["name"] for p in editor.get_panes()]
        assert restored_names == expected_names, (
            f"pane order must round-trip: expected={expected_names}, "
            f"restored={restored_names}"
        )
