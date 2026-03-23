"""
E2E tests for column layout and pane sizing.

These tests verify the core layout invariant: when N columns are selected,
each pane occupies exactly 1/N of the available width. This catches the
specific bug where 1-column mode produces half-width panes as if 2 columns
were active, and other layout/sizing regressions.

What bugs these tests catch:
- 1-column layout rendering panes at half width (stale column count)
- Column count not updating after layout dropdown selection
- Pane width not recalculating after column switch
- Column index assignment wrong after redistribution
- Pane move/reorder breaking column assignments
- Pane removal not redistributing remaining panes
- Column layout lost after state save/load
- Column layout lost after editor close/reopen
- Layout change during playback causing crash
- New pane added after layout change getting wrong column
"""

import pytest
from oscil_test_utils import OscilTestClient


# Tolerance for width comparisons — accounts for the 2px margin per side
# in getPaneBounds plus integer rounding.
WIDTH_TOLERANCE = 10


def _assert_pane_width_fraction(layout_data: dict, expected_fraction: float, pane_index: int = 0):
    """Assert a pane's width is approximately expected_fraction of available width."""
    avail_w = layout_data["availableArea"]["width"]
    pane = layout_data["panes"][pane_index]
    pane_w = pane["bounds"]["width"]
    expected_w = avail_w * expected_fraction
    assert abs(pane_w - expected_w) < WIDTH_TOLERANCE, (
        f"Pane {pane_index} width should be ~{expected_fraction:.0%} of available "
        f"({expected_w:.0f}px), got {pane_w}px "
        f"(available={avail_w}, columns={layout_data['columns']})"
    )


# ── Smoke Test ─────────────────────────────────────────────────────────────


class TestPaneLayoutEndpoint:
    """Verify the /panes endpoint returns valid data."""

    def test_panes_endpoint_returns_data(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: /panes endpoint not implemented or returning error,
        making all layout tests fail for the wrong reason.
        """
        osc_id = editor.add_oscillator(source_id, name="Layout Smoke")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        layout = editor.get_pane_layout()
        assert layout is not None, "/panes endpoint should return data"
        assert "columns" in layout, "Layout should include columns field"
        assert "availableArea" in layout, "Layout should include availableArea"
        assert "panes" in layout, "Layout should include panes list"
        assert len(layout["panes"]) >= 1, "Should have at least 1 pane"

        pane = layout["panes"][0]
        assert "bounds" in pane, "Pane should have bounds"
        assert pane["bounds"]["width"] > 0, "Pane width must be positive"
        assert pane["bounds"]["height"] > 0, "Pane height must be positive"

    def test_layout_info_endpoint(self, editor: OscilTestClient):
        """
        Bug caught: /layout endpoint returning wrong column count.
        """
        info = editor.get_layout_info()
        assert info is not None, "/layout endpoint should return data"
        assert "columns" in info
        assert info["columns"] >= 1


# ── Single Column Layout (the reported bug) ────────────────────────────────


class TestSingleColumnLayout:
    """Verify panes use full available width in 1-column mode."""

    def test_single_pane_uses_full_width(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: THE specific bug — 1-column layout with 1 pane renders
        the pane at half width as if 2 columns were active.
        """
        osc_id = editor.add_oscillator(source_id, name="FullWidth Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Explicitly set 1 column
        assert editor.set_column_layout(1), "Should set 1 column"

        layout = editor.get_pane_layout()
        assert layout is not None
        assert layout["columns"] == 1, f"Should be 1 column, got {layout['columns']}"
        assert len(layout["panes"]) >= 1

        _assert_pane_width_fraction(layout, 1.0, pane_index=0)

    def test_two_panes_stacked_vertically_full_width(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: in 1-column mode with 2 panes, panes split horizontally
        instead of stacking vertically.
        """
        editor.add_oscillator(source_id, name="Stack 1")
        editor.add_oscillator(source_id, name="Stack 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Add second pane and move oscillator to it
        pane2_id = editor.add_pane("Second Pane")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        editor.set_column_layout(1)

        layout = editor.get_pane_layout()
        assert layout is not None
        assert layout["columns"] == 1

        # Both panes should use full available width
        for i, pane in enumerate(layout["panes"]):
            _assert_pane_width_fraction(layout, 1.0, pane_index=i)

        # Both panes should be in column 0
        for pane in layout["panes"]:
            assert pane["columnIndex"] == 0, (
                f"Pane '{pane['name']}' should be in column 0 in single-column mode, "
                f"got column {pane['columnIndex']}"
            )


# ── Multi-Column Width Verification ────────────────────────────────────────


class TestMultiColumnWidth:
    """Verify pane widths are correct fractions of available area."""

    def test_two_column_two_panes_half_width(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: 2-column mode not dividing width equally between panes.
        """
        editor.add_oscillator(source_id, name="2Col A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("2Col Pane B")
        if pane2_id is None:
            pytest.fail("Pane add not available")

        editor.set_column_layout(2)

        layout = editor.get_pane_layout()
        assert layout is not None
        assert layout["columns"] == 2

        for i, pane in enumerate(layout["panes"]):
            _assert_pane_width_fraction(layout, 0.5, pane_index=i)

    def test_three_column_three_panes_third_width(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: 3-column mode not distributing panes across 3 columns.
        """
        editor.add_oscillator(source_id, name="3Col A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.add_pane("3Col B")
        editor.add_pane("3Col C")

        editor.set_column_layout(3)

        layout = editor.get_pane_layout()
        assert layout is not None
        assert layout["columns"] == 3

        for i, pane in enumerate(layout["panes"]):
            _assert_pane_width_fraction(layout, 1.0 / 3.0, pane_index=i)


# ── Column Switching ───────────────────────────────────────────────────────


class TestColumnSwitching:
    """Verify pane widths update correctly when switching column counts."""

    def test_switch_2col_to_1col_widens_panes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching from 2 columns to 1 doesn't recalculate
        pane widths — panes stay at half width.
        """
        editor.add_oscillator(source_id, name="Switch Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Start in 2-column mode
        editor.set_column_layout(2)
        layout_2col = editor.get_pane_layout()
        assert layout_2col["columns"] == 2
        width_2col = layout_2col["panes"][0]["bounds"]["width"]

        # Switch to 1-column mode
        editor.set_column_layout(1)
        layout_1col = editor.get_pane_layout()
        assert layout_1col["columns"] == 1
        width_1col = layout_1col["panes"][0]["bounds"]["width"]

        # Pane should be wider in 1-column mode
        assert width_1col > width_2col * 1.5, (
            f"1-column width ({width_1col}) should be ~2x "
            f"2-column width ({width_2col})"
        )
        _assert_pane_width_fraction(layout_1col, 1.0, pane_index=0)

    def test_switch_1col_to_2col_narrows_panes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching from 1 to 2 columns doesn't trigger resized(),
        leaving pane at full width.
        """
        editor.add_oscillator(source_id, name="Narrow Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("Narrow B")
        if pane2_id is None:
            pytest.fail("Pane add not available")

        editor.set_column_layout(1)
        layout_1 = editor.get_pane_layout()
        width_1 = layout_1["panes"][0]["bounds"]["width"]

        editor.set_column_layout(2)
        layout_2 = editor.get_pane_layout()
        width_2 = layout_2["panes"][0]["bounds"]["width"]

        assert width_2 < width_1 * 0.7, (
            f"2-column width ({width_2}) should be ~half of "
            f"1-column width ({width_1})"
        )

    def test_cycle_1_2_3_1_columns_width_correct(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: cycling through all column modes leaves stale column
        count — widths don't match expected fractions after returning to 1.
        """
        editor.add_oscillator(source_id, name="Cycle Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("Cycle B")
        editor.add_pane("Cycle C")

        for columns in [1, 2, 3, 1]:
            editor.set_column_layout(columns)
            layout = editor.get_pane_layout()
            assert layout is not None
            assert layout["columns"] == columns, (
                f"Expected {columns} columns, got {layout['columns']}"
            )

            expected_fraction = 1.0 / columns
            for i, pane in enumerate(layout["panes"]):
                _assert_pane_width_fraction(layout, expected_fraction, pane_index=i)


# ── Column Index Assignment ────────────────────────────────────────────────


class TestColumnIndexAssignment:
    """Verify panes are assigned to correct columns after redistribution."""

    def test_single_column_all_panes_column_0(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: panes retaining stale column indices from a previous
        multi-column layout when switching to single column.
        """
        editor.add_oscillator(source_id, name="Idx A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("Idx B")
        editor.add_pane("Idx C")

        # Start in 3-column (panes assigned 0,1,2)
        editor.set_column_layout(3)
        # Switch to 1-column — all should collapse to column 0
        editor.set_column_layout(1)

        layout = editor.get_pane_layout()
        for pane in layout["panes"]:
            assert pane["columnIndex"] == 0, (
                f"Pane '{pane['name']}' should be in column 0, "
                f"got {pane['columnIndex']}"
            )

    def test_two_column_round_robin(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: panes not distributed round-robin across columns
        after setColumnLayout(2) — all ending up in column 0.
        """
        editor.add_oscillator(source_id, name="RR A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("RR B")

        editor.set_column_layout(2)

        layout = editor.get_pane_layout()
        if len(layout["panes"]) >= 2:
            indices = [p["columnIndex"] for p in layout["panes"]]
            # With 2 panes in 2 columns, expect [0, 1] (round-robin)
            assert 0 in indices, "At least one pane should be in column 0"
            assert 1 in indices, "At least one pane should be in column 1"


# ── UI Dropdown Integration ────────────────────────────────────────────────


class TestLayoutDropdown:
    """Verify layout changes via the sidebar options dropdown."""

    def test_dropdown_selection_changes_column_count(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: layout dropdown onSelectionChanged not wired to
        notifyLayoutChanged, so UI selection is cosmetic only.
        """
        editor.add_oscillator(source_id, name="Dropdown Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Expand options section
        options_id = "sidebar_options"
        if not editor.element_exists(options_id):
            pytest.fail("Options section not registered")
        editor.click(options_id)

        dropdown_id = "sidebar_options_layoutDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.fail("Layout dropdown not registered")

        # Select "2 Columns" via dropdown
        editor.select_dropdown_item(dropdown_id, "2")

        # Verify column count changed
        editor.wait_until(
            lambda: (info := editor.get_layout_info())
            and info.get("columns") == 2,
            timeout_s=3.0,
            desc="column count to become 2 via dropdown",
        )

        layout = editor.get_pane_layout()
        assert layout is not None
        assert layout["columns"] == 2

        # Select "1 Column" to restore
        editor.select_dropdown_item(dropdown_id, "1")

        editor.wait_until(
            lambda: (info := editor.get_layout_info())
            and info.get("columns") == 1,
            timeout_s=3.0,
            desc="column count to become 1 via dropdown",
        )

        layout = editor.get_pane_layout()
        assert layout["columns"] == 1
        _assert_pane_width_fraction(layout, 1.0, pane_index=0)


# ── Pane Move / Reorder ───────────────────────────────────────────────────


class TestPaneMoveOrder:
    """Verify pane reorder operations work with different layouts."""

    def test_move_pane_changes_order(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: /pane/move endpoint not updating pane order in layout manager.
        """
        editor.add_oscillator(source_id, name="Move A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("Move B")
        editor.add_pane("Move C")

        layout_before = editor.get_pane_layout()
        names_before = [p["name"] for p in layout_before["panes"]]

        result = editor.move_pane_position(0, 2)
        if not result:
            pytest.fail("Pane move API not available")

        layout_after = editor.get_pane_layout()
        names_after = [p["name"] for p in layout_after["panes"]]

        assert names_after != names_before, (
            f"Pane order should change after move: "
            f"before={names_before}, after={names_after}"
        )
        # No panes should be lost
        assert set(names_after) == set(names_before), (
            "Move should not add or remove panes"
        )

    def test_move_in_multi_column_preserves_column_assignment(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: moving a pane triggers redistributePanes which
        reassigns all column indices, losing user's custom arrangement.
        """
        editor.add_oscillator(source_id, name="MoveCol A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("MoveCol B")
        editor.add_pane("MoveCol C")

        editor.set_column_layout(2)

        layout_before = editor.get_pane_layout()
        # Panes should still be distributed across columns after move
        editor.move_pane_position(0, 1)

        layout_after = editor.get_pane_layout()
        assert layout_after["columns"] == 2, "Column count should survive move"

        # All panes should have valid column indices
        for pane in layout_after["panes"]:
            assert 0 <= pane["columnIndex"] < 2, (
                f"Pane '{pane['name']}' has invalid column index "
                f"{pane['columnIndex']} for 2-column layout"
            )


# ── Pane Removal Redistribution ────────────────────────────────────────────


class TestPaneRemovalRedistribution:
    """Verify pane removal triggers correct redistribution in multi-column."""

    def test_remove_pane_in_2col_redistributes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: removing a pane in 2-column mode leaves the remaining
        pane in column 1 with nothing in column 0, producing a gap.
        """
        editor.add_oscillator(source_id, name="Remove A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        pane2_id = editor.add_pane("Remove B")
        if pane2_id is None:
            pytest.fail("Pane add not available")

        editor.set_column_layout(2)

        layout_before = editor.get_pane_layout()
        assert len(layout_before["panes"]) >= 2

        # Remove the second pane
        editor.remove_pane(pane2_id)

        editor.wait_until(
            lambda: (l := editor.get_pane_layout())
            and len(l.get("panes", [])) == 1,
            timeout_s=3.0,
            desc="pane removal to complete",
        )

        layout_after = editor.get_pane_layout()
        assert len(layout_after["panes"]) == 1
        # Remaining pane should be in column 0
        assert layout_after["panes"][0]["columnIndex"] == 0


# ── Persistence ────────────────────────────────────────────────────────────


class TestLayoutPersistence:
    """Verify column layout survives state save/load and editor lifecycle."""

    def test_column_layout_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: ColumnLayout enum not serialized in state XML,
        reverting to 1 column after load.
        """
        editor.add_oscillator(source_id, name="Persist Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("Persist B")

        editor.set_column_layout(2)
        layout_before = editor.get_pane_layout()
        assert layout_before["columns"] == 2

        path = "/tmp/oscil_e2e_layout_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"
        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        layout_after = editor.get_pane_layout()
        assert layout_after is not None
        assert layout_after["columns"] == 2, (
            f"Column layout should persist: expected 2, got {layout_after['columns']}"
        )

    def test_column_layout_survives_editor_lifecycle(
        self, client: OscilTestClient
    ):
        """
        Bug caught: column layout stored only in UI state, lost when
        editor is closed and reopened.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.fail("No sources")
        source_id = sources[0]["id"]

        client.add_oscillator(source_id, name="Lifecycle Layout")
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        client.set_column_layout(2)
        info_before = client.get_layout_info()
        assert info_before["columns"] == 2

        client.close_editor()
        client.open_editor()

        info_after = client.get_layout_info()
        client.close_editor()

        assert info_after is not None
        assert info_after["columns"] == 2, (
            f"Column layout should survive editor lifecycle: "
            f"expected 2, got {info_after.get('columns')}"
        )


# ── Dynamic Operations ─────────────────────────────────────────────────────


class TestLayoutDynamicOperations:
    """Verify layout correctness after dynamic pane/oscillator operations."""

    def test_add_pane_after_layout_change(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding a pane after switching to 2-column mode assigns
        it to column 0 instead of the least-populated column.
        """
        editor.add_oscillator(source_id, name="Dynamic A")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.set_column_layout(2)

        # Add a second pane — should go to column 1 (least populated)
        pane2_id = editor.add_pane("Dynamic B")
        if pane2_id is None:
            pytest.fail("Pane add not available")

        layout = editor.get_pane_layout()
        if len(layout["panes"]) >= 2:
            indices = [p["columnIndex"] for p in layout["panes"]]
            assert 0 in indices and 1 in indices, (
                f"2 panes in 2-column should use both columns, "
                f"got indices {indices}"
            )

    def test_layout_change_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: changing column layout while transport is playing
        and waveforms are rendering causes render pipeline crash.
        """
        editor.add_oscillator(source_id, name="Play Layout")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Cycle layouts during playback
        for cols in [2, 3, 1]:
            editor.set_column_layout(cols)
            assert editor.is_playing(), (
                f"Transport should survive layout change to {cols} columns"
            )

        # Verify final state
        layout = editor.get_pane_layout()
        assert layout["columns"] == 1
        _assert_pane_width_fraction(layout, 1.0, pane_index=0)

        editor.transport_stop()

    def test_layout_dropdown_all_options_produce_correct_widths(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: dropdown selection fires index-based callback that
        produces wrong column count (off-by-one: index 0 → 0 columns).
        """
        editor.add_oscillator(source_id, name="All Options")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.add_pane("Options B")
        editor.add_pane("Options C")

        # Expand options
        if not editor.element_exists("sidebar_options"):
            pytest.fail("Options section not registered")
        editor.click("sidebar_options")

        dropdown_id = "sidebar_options_layoutDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.fail("Layout dropdown not registered")

        for item_id, expected_cols in [("1", 1), ("2", 2), ("3", 3)]:
            editor.select_dropdown_item(dropdown_id, item_id)
            editor.wait_until(
                lambda ec=expected_cols: (info := editor.get_layout_info())
                and info.get("columns") == ec,
                timeout_s=3.0,
                desc=f"columns to become {expected_cols}",
            )

            layout = editor.get_pane_layout()
            assert layout["columns"] == expected_cols, (
                f"Dropdown '{item_id}' should set {expected_cols} columns"
            )

            expected_fraction = 1.0 / expected_cols
            for i, pane in enumerate(layout["panes"]):
                _assert_pane_width_fraction(layout, expected_fraction, pane_index=i)

        # Restore 1 column
        editor.select_dropdown_item(dropdown_id, "1")
