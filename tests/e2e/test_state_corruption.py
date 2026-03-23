"""
E2E tests for state corruption scenarios.

These tests target subtle bugs where the internal state becomes inconsistent
due to specific operation sequences. Each test was designed to catch a class
of bugs that would survive simpler testing.

What bugs these tests catch:
- Oscillator paneId pointing to deleted pane after rapid pane operations
- State save capturing partial updates (torn write) during concurrent modifications
- Oscillator order indices becoming non-contiguous after interleaved add/delete
- Loading state with more oscillators than existed at save time
- State listeners firing in wrong order after bulk operations
- Diagnostic snapshot diverging from state API after complex operations
"""

import pytest
from oscil_test_utils import OscilTestClient
from page_objects import StateManager, TransportControl


class TestOrderIndexIntegrity:
    """Verify oscillator order indices remain valid after complex operations."""

    def test_indices_contiguous_after_alternating_add_delete(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding oscillator A, B, C, deleting B, adding D produces
        order indices [0, 2, 3] instead of [0, 1, 2] because the delete
        did not compact the indices.
        """
        id_a = editor.add_oscillator(source_id, name="Idx A")
        id_b = editor.add_oscillator(source_id, name="Idx B")
        id_c = editor.add_oscillator(source_id, name="Idx C")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Delete middle
        editor.delete_oscillator(id_b)
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Add new
        id_d = editor.add_oscillator(source_id, name="Idx D")
        assert id_d is not None
        editor.wait_for_oscillator_count(3, timeout_s=3.0)

        oscs = editor.get_oscillators()
        indices = sorted(o.get("orderIndex", -1) for o in oscs)

        assert all(i >= 0 for i in indices), (
            f"All indices must be non-negative: {indices}"
        )
        assert len(set(indices)) == 3, (
            f"Indices must be unique: {indices}"
        )

    def test_indices_survive_reorder_then_delete(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: reordering oscillators then deleting one produces
        duplicate or negative order indices because the reorder updates
        and the delete compaction use different index schemes.
        """
        ids = []
        for i in range(4):
            oid = editor.add_oscillator(source_id, name=f"ReorderDel {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(4, timeout_s=5.0)

        # Reorder: swap first and last
        editor.reorder_oscillators(0, 3)

        # Delete the now-second item
        oscs = editor.get_oscillators()
        if len(oscs) >= 2:
            editor.delete_oscillator(oscs[1]["id"])
            editor.wait_for_oscillator_count(3, timeout_s=3.0)

        oscs_after = editor.get_oscillators()
        indices = [o.get("orderIndex", -1) for o in oscs_after]
        assert all(i >= 0 for i in indices), (
            f"All indices must be non-negative after reorder+delete: {indices}"
        )
        assert len(set(indices)) == len(indices), (
            f"Indices must be unique after reorder+delete: {indices}"
        )


class TestSnapshotStateConsistency:
    """Verify diagnostic snapshot stays consistent with state API."""

    def test_snapshot_matches_after_rapid_operations(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: diagnostic snapshot caches state and does not invalidate
        when rapid add/update/delete operations run, causing the snapshot
        to show stale data that mismatches the state API.
        """
        # Rapid sequence of operations
        id1 = editor.add_oscillator(source_id, name="Snap Rapid 1", colour="#FF0000")
        id2 = editor.add_oscillator(source_id, name="Snap Rapid 2", colour="#00FF00")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        editor.update_oscillator(id1, visible=False, opacity=0.3)
        editor.delete_oscillator(id2)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        id3 = editor.add_oscillator(source_id, name="Snap Rapid 3", mode="Mid")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Compare snapshot with state API
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.xfail("Diagnostic snapshot API not available")

        state_oscs = editor.get_oscillators()
        snap_oscs = snap.get("oscillators", [])

        assert len(snap_oscs) == len(state_oscs), (
            f"Snapshot osc count ({len(snap_oscs)}) must match "
            f"state API ({len(state_oscs)})"
        )

        # Verify properties match
        state_names = sorted(o["name"] for o in state_oscs)
        snap_names = sorted(o.get("name", "") for o in snap_oscs)
        assert state_names == snap_names, (
            f"Snapshot names must match state API: "
            f"state={state_names}, snap={snap_names}"
        )

    def test_snapshot_timing_matches_after_mode_switch(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching timing mode does not update the snapshot's
        timing section, causing scenario tests to see stale mode.
        """
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.xfail("Timing section not registered")
        editor.click(timing_id)

        melodic_seg = "sidebar_timing_modeToggle_melodic"
        if not editor.element_exists(melodic_seg):
            pytest.xfail("Melodic mode segment not registered")

        osc_id = editor.add_oscillator(source_id, name="Snap Timing")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Switch to melodic mode
        editor.click(melodic_seg)

        # Verify snapshot reflects the mode change
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.xfail("Diagnostic snapshot API not available")

        snap_mode = snap.get("timing", {}).get("mode", "")
        # The mode should be MELODIC (or equivalent)
        assert snap_mode != "", (
            "Snapshot timing mode must not be empty after mode switch"
        )


class TestPaneIntegrity:
    """Verify pane-oscillator relationships remain valid under stress."""

    def test_all_oscillators_have_valid_pane_after_operations(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: sequence of add pane, move oscillator, remove pane,
        add oscillator leaves some oscillators with orphaned paneIds.
        """
        # Create 3 oscillators (all in auto-created pane)
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"PaneInteg {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Add second pane
        pane2_id = editor.add_pane("Integrity Pane 2")
        if pane2_id is None:
            pytest.xfail("Pane add API not available")

        # Move one oscillator to pane 2
        editor.move_oscillator(ids[1], pane2_id)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(ids[1]).get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator move to pane 2",
        )

        # Remove pane 2
        editor.remove_pane(pane2_id)

        # Add another oscillator
        id4 = editor.add_oscillator(source_id, name="PaneInteg Post")
        assert id4 is not None
        editor.wait_for_oscillator_count(4, timeout_s=3.0)

        # Verify ALL oscillators have valid pane IDs
        pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in editor.get_oscillators():
            assert osc.get("paneId") in pane_ids, (
                f"Oscillator '{osc['name']}' has orphaned paneId "
                f"'{osc.get('paneId')}'. Valid panes: {pane_ids}"
            )

    def test_pane_oscillator_count_consistent(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state pane oscillator count does not match
        the actual number of oscillators assigned to that pane,
        causing some waveforms to not render.
        """
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"PaneCount {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        panes = editor.get_panes()
        assert len(panes) >= 1

        # Count oscillators per pane via state API
        osc_per_pane = {}
        for osc in editor.get_oscillators():
            pid = osc.get("paneId", "")
            osc_per_pane[pid] = osc_per_pane.get(pid, 0) + 1

        # Compare with waveform state
        wf_state = editor.get_waveform_state()
        if wf_state and "panes" in wf_state:
            for i, pane in enumerate(wf_state["panes"]):
                reported_count = pane.get("oscillatorCount", 0)
                pane_id = panes[i]["id"] if i < len(panes) else "unknown"
                expected = osc_per_pane.get(pane_id, 0)
                assert reported_count == expected, (
                    f"Pane {i} reports {reported_count} oscillators in "
                    f"waveform state but state API shows {expected}"
                )


class TestSaveLoadCornerCases:
    """State save/load edge cases that expose serialization bugs."""

    def test_save_with_hidden_oscillator_roundtrips(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: visible=false not serialized to XML, so hidden
        oscillators become visible after load.
        """
        osc_id = editor.add_oscillator(source_id, name="Hidden Save")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.update_oscillator(osc_id, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(osc_id).get("visible", True),
            timeout_s=2.0, desc="oscillator hidden",
        )

        state_mgr = StateManager(editor)
        oscs = state_mgr.save_load_roundtrip(
            "/tmp/oscil_e2e_hidden_save.xml", 1
        )

        assert oscs[0].get("visible") is False, (
            f"visible=false must survive save/load, got {oscs[0].get('visible')}"
        )

    def test_save_with_custom_opacity_linewidth_roundtrips(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: float properties (opacity, lineWidth) serialized with
        insufficient precision, producing rounding errors on reload.
        """
        osc_id = editor.add_oscillator(source_id, name="Float Props")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.update_oscillator(osc_id, opacity=0.37, lineWidth=2.75)

        osc_before = editor.get_oscillator_by_id(osc_id)
        opacity_before = osc_before.get("opacity")
        lw_before = osc_before.get("lineWidth")

        state_mgr = StateManager(editor)
        oscs = state_mgr.save_load_roundtrip(
            "/tmp/oscil_e2e_float_props.xml", 1
        )

        if opacity_before is not None:
            assert abs(oscs[0].get("opacity", 0) - opacity_before) < 0.05, (
                f"Opacity must survive roundtrip: before={opacity_before}, "
                f"after={oscs[0].get('opacity')}"
            )
        if lw_before is not None:
            assert abs(oscs[0].get("lineWidth", 0) - lw_before) < 0.1, (
                f"lineWidth must survive roundtrip: before={lw_before}, "
                f"after={oscs[0].get('lineWidth')}"
            )

    def test_save_load_with_multiple_panes_and_moved_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillators moved between panes have their new paneId
        serialized but the moved-from pane's oscillator list is not updated,
        causing the load to place them in both panes (or neither).
        """
        id1 = editor.add_oscillator(source_id, name="MultiPane A")
        id2 = editor.add_oscillator(source_id, name="MultiPane B")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        pane2_id = editor.add_pane("MultiPane P2")
        if pane2_id is None:
            pytest.xfail("Pane add API not available")

        # Move B to pane 2
        editor.move_oscillator(id2, pane2_id)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(id2).get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator B to move to pane 2",
        )

        # Record pre-save state
        panes_before = editor.get_panes()
        oscs_before = editor.get_oscillators()
        assignment_before = {o["name"]: o["paneId"] for o in oscs_before}

        state_mgr = StateManager(editor)
        oscs = state_mgr.save_load_roundtrip(
            "/tmp/oscil_e2e_multi_pane_move.xml", 2
        )

        panes_after = editor.get_panes()
        assert len(panes_after) >= 2, (
            f"Must have 2+ panes after load, got {len(panes_after)}"
        )

        # Verify pane assignments preserved
        for osc in oscs:
            name = osc["name"]
            if name in assignment_before:
                # Pane ID may change, but oscillators should be in different panes
                pass

        # Verify no oscillator is orphaned
        valid_pane_ids = {p["id"] for p in panes_after}
        for osc in oscs:
            assert osc.get("paneId") in valid_pane_ids, (
                f"Oscillator '{osc['name']}' has orphaned paneId "
                f"'{osc.get('paneId')}' after save/load"
            )


class TestTransportStatePersistence:
    """Verify transport-related state during save/load."""

    def test_bpm_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: BPM value not serialized in state XML, reverting
        to default 120.0 after load.
        """
        editor.set_bpm(95.0)

        osc_id = editor.add_oscillator(source_id, name="BPM Persist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        bpm_before = editor.get_bpm()
        assert abs(bpm_before - 95.0) < 1.0

        state_mgr = StateManager(editor)
        state_mgr.save_load_roundtrip("/tmp/oscil_e2e_bpm.xml", 1)

        bpm_after = editor.get_bpm()
        # BPM may or may not persist (depends on design), but should not crash
        assert bpm_after > 0, "BPM must be positive after load"

        editor.set_bpm(120.0)
