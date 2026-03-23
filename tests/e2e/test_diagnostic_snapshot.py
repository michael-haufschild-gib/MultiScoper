"""
E2E tests for the diagnostic snapshot endpoint.

The diagnostic snapshot is the single most important test infrastructure
endpoint — it provides a complete state dump that the scenario runner
and AI agents use for assertions. If it returns incomplete or incorrect
data, every scenario-based test is unreliable.

What bugs these tests catch:
- Snapshot endpoint returning empty/malformed data
- Snapshot missing required sections (transport, timing, oscillators, etc.)
- Snapshot oscillator data not matching state API data
- Snapshot timing data not reflecting actual timing engine state
- Snapshot transport data stale or incorrect
- Snapshot not updating after state changes
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestSnapshotStructure:
    """Verify the diagnostic snapshot has all required sections."""

    def test_snapshot_returns_data(self, editor: OscilTestClient):
        """
        Bug caught: /diagnostic/snapshot endpoint not implemented or returning error.
        """
        snap = editor.get_diagnostic_snapshot()
        assert snap is not None, "Diagnostic snapshot should return data"
        assert isinstance(snap, dict), "Snapshot should be a dict"

    def test_snapshot_has_transport_section(self, editor: OscilTestClient):
        """
        Bug caught: transport data missing from snapshot.
        """
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        assert "transport" in snap, (
            f"Snapshot should have transport section, got keys: {list(snap.keys())}"
        )
        transport = snap["transport"]
        assert "playing" in transport, "Transport should have 'playing' field"
        assert "bpm" in transport, "Transport should have 'bpm' field"

    def test_snapshot_has_timing_section(self, editor: OscilTestClient):
        """
        Bug caught: timing data missing from snapshot.
        """
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        assert "timing" in snap, (
            f"Snapshot should have timing section, got keys: {list(snap.keys())}"
        )
        timing = snap["timing"]
        assert "mode" in timing, "Timing should have 'mode' field"
        assert "displaySamples" in timing, "Timing should have 'displaySamples' field"

    def test_snapshot_has_oscillators_section(self, editor: OscilTestClient):
        """
        Bug caught: oscillators not included in snapshot.
        """
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        assert "oscillators" in snap, (
            f"Snapshot should have oscillators section, got keys: {list(snap.keys())}"
        )
        assert isinstance(snap["oscillators"], list)

    def test_snapshot_has_sources_section(self, editor: OscilTestClient):
        """
        Bug caught: sources not included in snapshot.
        """
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        assert "sources" in snap, (
            f"Snapshot should have sources section, got keys: {list(snap.keys())}"
        )


class TestSnapshotConsistency:
    """Verify snapshot data matches individual API responses."""

    def test_oscillator_count_matches_state_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: snapshot oscillator list out of sync with /state/oscillators.
        """
        for i in range(2):
            editor.add_oscillator(source_id, name=f"SnapConsist {i}")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        state_oscs = editor.get_oscillators()
        snap_oscs = snap.get("oscillators", [])

        assert len(snap_oscs) == len(state_oscs), (
            f"Snapshot oscillator count ({len(snap_oscs)}) should match "
            f"state API ({len(state_oscs)})"
        )

    def test_transport_state_matches_api(self, editor: OscilTestClient):
        """
        Bug caught: snapshot transport data cached/stale, not reflecting
        current transport state.
        """
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            editor.transport_stop()
            pytest.fail("Snapshot API not available")

        assert snap["transport"]["playing"] is True, (
            "Snapshot should show transport as playing"
        )

        editor.transport_stop()
        editor.wait_until(
            lambda: not editor.is_playing(), timeout_s=2.0, desc="transport stopped"
        )

        snap2 = editor.get_diagnostic_snapshot()
        assert snap2["transport"]["playing"] is False, (
            "Snapshot should show transport as stopped after stop command"
        )

    def test_snapshot_updates_after_oscillator_add(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: snapshot returning cached data that doesn't reflect
        oscillators added after the initial snapshot.
        """
        snap1 = editor.get_diagnostic_snapshot()
        if snap1 is None:
            pytest.fail("Snapshot API not available")

        count_before = len(snap1.get("oscillators", []))

        editor.add_oscillator(source_id, name="SnapUpdate")
        editor.wait_for_oscillator_count(count_before + 1, timeout_s=3.0)

        snap2 = editor.get_diagnostic_snapshot()
        count_after = len(snap2.get("oscillators", []))

        assert count_after == count_before + 1, (
            f"Snapshot should reflect new oscillator: "
            f"before={count_before}, after={count_after}"
        )

    def test_snapshot_reflects_bpm_change(self, editor: OscilTestClient):
        """
        Bug caught: BPM in snapshot not updated after set_bpm call.
        """
        initial_bpm = editor.get_bpm()
        target_bpm = 85.0 if initial_bpm != 85.0 else 95.0

        editor.set_bpm(target_bpm)

        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            editor.set_bpm(initial_bpm)
            pytest.fail("Snapshot API not available")

        snap_bpm = snap.get("transport", {}).get("bpm", 0)
        assert abs(snap_bpm - target_bpm) < 1.0, (
            f"Snapshot BPM should be {target_bpm}, got {snap_bpm}"
        )

        editor.set_bpm(initial_bpm)


class TestElementRegistry:
    """Verify the element registry reports registered elements correctly."""

    def test_editor_has_core_elements_registered(self, editor: OscilTestClient):
        """
        Bug caught: element registry not populated after editor open,
        making all UI interaction tests silently skip instead of fail.
        """
        element_ids = editor.get_registered_element_ids()
        assert len(element_ids) > 0, (
            "Element registry should have elements when editor is open"
        )

        # These core IDs must always exist when the editor is open.
        # Imported from conftest to maintain a single source of truth.
        from conftest import CORE_ELEMENTS
        for expected_id in CORE_ELEMENTS:
            assert expected_id in element_ids, (
                f"Core element '{expected_id}' should be registered. "
                f"Got {len(element_ids)} elements total."
            )

    def test_oscillator_elements_registered_after_add(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator list items not registering their child
        elements (settings, delete, visibility buttons) in the registry,
        making them invisible to test automation.
        """
        osc_id = editor.add_oscillator(source_id, name="RegistryTest")
        assert osc_id is not None
        editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=5.0)

        element_ids = editor.get_registered_element_ids()

        # The list item and its action buttons should all be registered
        expected_items = [
            "sidebar_oscillators_item_0",
            "sidebar_oscillators_item_0_settings",
            "sidebar_oscillators_item_0_delete",
        ]
        for item_id in expected_items:
            assert item_id in element_ids, (
                f"'{item_id}' should be registered after adding oscillator"
            )

    def test_elements_deregistered_after_delete(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting an oscillator does not remove its elements
        from the registry, causing stale references and potential crashes
        when tests interact with ghost elements.
        """
        osc_id = editor.add_oscillator(source_id, name="DeregTest")
        assert osc_id is not None
        editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=5.0)

        # Verify registered
        assert "sidebar_oscillators_item_0" in editor.get_registered_element_ids()

        # Delete
        editor.delete_oscillator(osc_id)
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Verify deregistered
        element_ids = editor.get_registered_element_ids()
        assert "sidebar_oscillators_item_0" not in element_ids, (
            "Deleted oscillator's list item should be deregistered"
        )
