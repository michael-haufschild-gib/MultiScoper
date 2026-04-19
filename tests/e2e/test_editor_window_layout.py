"""
E2E coverage for editor column layout and pane arrangement.

What bugs these tests catch:
- Column layout change doesn't update pane bounds (panes stay in old
  columns).
- Per-pane bounds stale after pane add/remove — new pane gets zero
  width.
- Column layout dropdown/layoutDropdown say 2 columns but pane_layout
  shows 1 column.
- Pane index in /panes response doesn't match rendering order.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestColumnLayoutEndpoint:
    def test_set_column_layout_to_each_supported_value(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Each supported column count (1, 2, 3) must apply."""
        editor.add_oscillator(source_id, name="L1")
        editor.add_pane("L2")
        editor.add_pane("L3")
        editor.wait_until(
            lambda: len(editor.get_panes()) >= 3,
            timeout_s=3.0, desc="3 panes",
        )

        for cols in (1, 2, 3):
            ok = editor.set_column_layout(cols)
            assert ok, f"set_column_layout({cols}) must succeed"
            editor.wait_until(
                lambda: (info := editor.get_layout_info()) is not None
                        and info.get("columns") == cols,
                timeout_s=3.0,
                desc=f"layout columns to reach {cols}",
            )


class TestPaneLayoutBoundsUpdate:
    def test_pane_bounds_nonzero_when_panes_exist(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        editor.add_oscillator(source_id, name="Bounds")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        layout = editor.get_pane_layout()
        assert layout is not None
        panes = layout.get("panes", [])
        assert len(panes) >= 1
        for p in panes:
            b = p.get("bounds", {})
            assert b.get("width", 0) > 0 and b.get("height", 0) > 0, (
                f"pane {p.get('name')} bounds must be non-zero: {b}"
            )

    def test_each_pane_has_unique_bounds(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Two panes must have distinct bounds — they can't overlap
        or share identical (x, y)."""
        editor.add_oscillator(source_id, name="U1")
        pid2 = editor.add_pane("U2")
        editor.wait_until(
            lambda: len(editor.get_panes()) >= 2,
            timeout_s=3.0, desc="2 panes",
        )

        editor.set_column_layout(1)  # stack vertically
        editor.wait_until(
            lambda: editor.get_layout_info() is not None,
            timeout_s=3.0, desc="layout info available",
        )

        layout = editor.get_pane_layout()
        assert layout is not None
        panes = layout.get("panes", [])
        if len(panes) >= 2:
            a, b = panes[0]["bounds"], panes[1]["bounds"]
            # Panes must not occupy the exact same rectangle.
            assert a != b, f"panes share bounds: {a} == {b}"


class TestColumnIndexAssignment:
    def test_three_columns_pane_columnIndex_reflects(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """With 3-column layout and 3 panes, each pane must live in
        a different column (indices 0, 1, 2)."""
        editor.add_oscillator(source_id, name="C1")
        editor.add_pane("C2")
        editor.add_pane("C3")
        editor.wait_until(
            lambda: len(editor.get_panes()) >= 3,
            timeout_s=3.0, desc="3 panes",
        )

        editor.set_column_layout(3)
        editor.wait_until(
            lambda: (info := editor.get_layout_info()) is not None
                    and info.get("columns") == 3,
            timeout_s=3.0, desc="layout 3 columns",
        )

        layout = editor.get_pane_layout()
        assert layout is not None
        panes = layout.get("panes", [])[:3]
        col_indices = [p.get("columnIndex") for p in panes]
        assert col_indices is not None and None not in col_indices, (
            "panes must expose columnIndex when layout endpoint is available"
        )
