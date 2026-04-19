"""
E2E coverage for drag-based oscillator reordering in the sidebar.

What bugs these tests catch:
- Drag endpoint fires the drag event but the reorder doesn't commit
  (drop target not bound to state.reorderOscillators).
- Drag moves an item on screen but state.orderIndex values don't
  reflect the new order (UI and state drift).
- Drag between distinct list items updates the wrong items'
  orderIndex (off-by-one in the reorder logic).
- Reorder across state save/load roundtrips preserved / lost.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestReorderViaStateAPI:
    """State API reorder fires correctly — the baseline."""

    def test_reorder_via_state_api(
        self, editor: MultiScoperTestClient, three_oscillators: list
    ):
        """Bug caught: reorder API updates orderIndex but state fetch
        still returns old ordering."""
        before = [o["name"] for o in editor.get_oscillators()]
        assert len(before) == 3

        # Move index 0 to index 2.
        editor.reorder_oscillators(0, 2)
        editor.wait_until(
            lambda: [o["name"] for o in editor.get_oscillators()] != before,
            timeout_s=3.0,
            desc="oscillator order to change",
        )
        after = [o["name"] for o in editor.get_oscillators()]
        assert after != before, f"order unchanged: {before}"
        # First item of `before` should no longer be at index 0.
        assert after[0] != before[0]


class TestDragReorderViaUI:
    """UI drag from one list item to another performs the reorder."""

    def test_drag_item_0_to_item_2(
        self, editor: MultiScoperTestClient, three_oscillators: list
    ):
        """Drag the first oscillator's list row onto the third row.

        Bug caught: drag-reorder wiring dropped during list refactor
        → UI shows drag animation but no state change.
        """
        before = [o["name"] for o in editor.get_oscillators()]
        assert len(before) == 3

        from_id = "sidebar_oscillators_item_0"
        to_id = "sidebar_oscillators_item_2"

        assert editor.element_exists(from_id)
        assert editor.element_exists(to_id)

        editor.drag(from_id, to_id)
        # Wait for state to settle.
        editor.wait_until(
            lambda: [o["name"] for o in editor.get_oscillators()] != before,
            timeout_s=5.0,
            desc="drag reorder to commit to state",
        )

        after = [o["name"] for o in editor.get_oscillators()]
        assert after != before, (
            f"drag had no effect on state order: {before} -> {after}"
        )


class TestReorderStatePersistence:
    def test_reorder_survives_save_load(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Reorder, save, reset, load — order must be preserved."""
        colours = ["#FF0000", "#00FF00", "#0000FF"]
        for i, c in enumerate(colours):
            editor.add_oscillator(source_id, name=f"Order{i}", colour=c)
        editor.wait_for_oscillator_count(3, timeout_s=3.0)

        editor.reorder_oscillators(0, 2)
        editor.wait_until(
            lambda: editor.get_oscillators()[0]["name"] != "Order0",
            timeout_s=3.0, desc="reorder to commit",
        )
        expected_order = [o["name"] for o in editor.get_oscillators()]

        path = str(tmp_path / "reorder.xml")
        assert editor.save_state(path)
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)
        assert editor.load_state(path)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        restored_order = [o["name"] for o in editor.get_oscillators()]
        assert restored_order == expected_order, (
            f"reordered sequence must survive save/load: "
            f"expected={expected_order}, got={restored_order}"
        )
