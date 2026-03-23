"""
E2E tests for pane management operations.

What bugs these tests catch:
- Adding a second pane fails or crashes
- Moving oscillator to a different pane loses audio binding
- Multiple panes with oscillators not all rendering
- Pane count inconsistent with oscillator pane assignments
- Pane properties not serialized correctly in state save/load
- Orphaned oscillators after pane operations
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestPaneCreation:
    """Pane add and auto-creation behavior."""

    def test_first_oscillator_auto_creates_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding an oscillator to empty state does not
        trigger pane auto-creation, leaving oscillator orphaned.
        """
        assert len(editor.get_panes()) == 0

        osc_id = editor.add_oscillator(source_id, name="AutoPane Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) >= 1, "First oscillator should auto-create a pane"

        osc = editor.get_oscillator_by_id(osc_id)
        pane_ids = {p["id"] for p in panes}
        assert osc["paneId"] in pane_ids, (
            f"Oscillator paneId '{osc['paneId']}' not in pane list"
        )

    def test_add_second_pane_via_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane add API not implemented or crashes when a
        pane already exists.
        """
        osc_id = editor.add_oscillator(source_id, name="TwoPane Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        assert len(editor.get_panes()) >= 1

        # Add a second pane
        pane_id = editor.add_pane("Second Pane")
        if pane_id is None:
            pytest.fail("Pane add API not available")

        panes = editor.get_panes()
        assert len(panes) >= 2, (
            f"Should have at least 2 panes after add, got {len(panes)}"
        )

    def test_pane_has_valid_id_and_name(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane serialization returning empty ID or null name.
        """
        osc_id = editor.add_oscillator(source_id, name="PaneProps")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        for pane in panes:
            assert "id" in pane and pane["id"], (
                f"Pane should have non-empty id, got: {pane}"
            )
            assert "name" in pane, f"Pane should have name field, got: {pane}"


class TestPaneOscillatorAssignment:
    """Oscillator-to-pane binding integrity."""

    def test_all_oscillators_assigned_to_existing_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator paneId references a pane that doesn't exist
        (orphaned oscillator invisible in the UI).
        """
        for i in range(3):
            editor.add_oscillator(source_id, name=f"PaneAssign {i}")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in editor.get_oscillators():
            assert osc.get("paneId") in pane_ids, (
                f"Oscillator '{osc['name']}' has paneId '{osc.get('paneId')}' "
                f"not in pane list {pane_ids}"
            )

    def test_multiple_oscillators_default_to_same_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: each oscillator getting its own pane instead of sharing
        the default pane (N oscillators = N panes instead of 1).
        """
        for i in range(3):
            editor.add_oscillator(source_id, name=f"SamePane {i}")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        panes = editor.get_panes()
        assert len(panes) == 1, (
            f"All oscillators should share 1 default pane, got {len(panes)}"
        )

        pane_id = panes[0]["id"]
        for osc in editor.get_oscillators():
            assert osc["paneId"] == pane_id, (
                f"Oscillator '{osc['name']}' should be in pane '{pane_id}', "
                f"got '{osc['paneId']}'"
            )


class TestPaneRemoval:
    """Pane remove operations."""

    def test_remove_empty_pane(
        self, editor: OscilTestClient, two_panes
    ):
        """
        Bug caught: removing an empty pane crashes because the render
        pipeline tries to iterate a destroyed pane's oscillator list.
        """
        pane1_id, pane2_id = two_panes
        panes_before = editor.get_panes()
        count_before = len(panes_before)

        removed = editor.remove_pane(pane2_id)
        if not removed:
            pytest.fail("Pane remove API not available")

        editor.wait_until(
            lambda: len(editor.get_panes()) < count_before,
            timeout_s=3.0,
            desc="pane removal to complete",
        )
        remaining_ids = {p["id"] for p in editor.get_panes()}
        assert pane2_id not in remaining_ids, (
            f"Removed pane '{pane2_id}' should be gone"
        )

    def test_remove_pane_with_oscillator_orphans_check(
        self, editor: OscilTestClient, source_id: str, two_panes
    ):
        """
        Bug caught: removing a pane that contains oscillators leaves them
        orphaned (paneId points to nonexistent pane), making them invisible.
        The expected behavior is: oscillators move to another pane.
        """
        pane1_id, pane2_id = two_panes

        # Add oscillator to the second pane
        osc_id = editor.add_oscillator(
            source_id, name="Pane2 Osc", pane_id=pane2_id
        )
        if osc_id is None:
            # Try without pane_id then move
            osc_id = editor.add_oscillator(source_id, name="Pane2 Osc")
            if osc_id:
                editor.move_oscillator(osc_id, pane2_id)
        if osc_id is None:
            pytest.fail("Cannot add oscillator to second pane")

        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Remove the second pane
        removed = editor.remove_pane(pane2_id)
        if not removed:
            pytest.fail("Pane remove API not available")

        # Verify no orphaned oscillators
        remaining_pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in editor.get_oscillators():
            assert osc.get("paneId") in remaining_pane_ids, (
                f"Oscillator '{osc['name']}' has orphaned paneId "
                f"'{osc.get('paneId')}' after pane removal"
            )


class TestOscillatorPaneMove:
    """Moving oscillators between panes."""

    def test_move_oscillator_to_different_pane(
        self, editor: OscilTestClient, source_id: str, two_panes
    ):
        """
        Bug caught: oscillator move API not updating paneId, or the
        oscillator disappearing from both panes after move.
        """
        pane1_id, pane2_id = two_panes

        # The setup oscillator is in pane1
        oscs = editor.get_oscillators()
        assert len(oscs) >= 1
        osc_id = oscs[0]["id"]
        assert oscs[0]["paneId"] == pane1_id

        # Move to pane2
        moved = editor.move_oscillator(osc_id, pane2_id)
        if not moved:
            pytest.fail("Oscillator move API not available")

        # Verify paneId updated
        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(osc_id))
            and osc.get("paneId") == pane2_id,
            timeout_s=3.0,
            desc="oscillator paneId to update",
        )
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc["paneId"] == pane2_id, (
            f"Oscillator should be in pane2 after move, got '{osc['paneId']}'"
        )

    def test_move_preserves_audio_binding(
        self, editor: OscilTestClient, source_id: str, two_panes
    ):
        """
        Bug caught: moving oscillator to a different pane disconnects
        the audio source binding, causing silent waveform in the new pane.
        """
        pane1_id, pane2_id = two_panes

        oscs = editor.get_oscillators()
        osc_id = oscs[0]["id"]
        source_before = oscs[0].get("sourceId")

        moved = editor.move_oscillator(osc_id, pane2_id)
        if not moved:
            pytest.fail("Oscillator move API not available")

        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(osc_id))
            and osc.get("paneId") == pane2_id,
            timeout_s=3.0,
            desc="oscillator paneId to update after move",
        )

        osc_after = editor.get_oscillator_by_id(osc_id)
        assert osc_after.get("sourceId") == source_before, (
            f"Source binding should survive move: "
            f"expected '{source_before}', got '{osc_after.get('sourceId')}'"
        )

    def test_move_to_nonexistent_pane_fails_gracefully(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: moving oscillator to a nonexistent pane causes crash
        instead of returning an error.
        """
        osc_id = editor.add_oscillator(source_id, name="Bad Move")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.move_oscillator(osc_id, "nonexistent-pane-id-99999")
        # Should either fail or be rejected — not crash
        # Verify oscillator is unchanged
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None, "Oscillator should survive invalid move"
        pane_ids = {p["id"] for p in editor.get_panes()}
        assert osc["paneId"] in pane_ids, (
            "Oscillator should remain in a valid pane after invalid move"
        )


class TestPaneStatePersistence:
    """Pane state survives save/load."""

    def test_pane_count_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane list not serialized in state XML, so all panes
        are lost on load and oscillators become orphaned.
        """
        osc_id = editor.add_oscillator(source_id, name="PanePersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes_before = editor.get_panes()
        assert len(panes_before) >= 1

        path = "/tmp/oscil_e2e_pane_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        editor.wait_for_oscillator_count(1, timeout_s=5.0)
        panes_after = editor.get_panes()
        assert len(panes_after) >= len(panes_before), (
            f"Pane count should survive save/load: "
            f"before={len(panes_before)}, after={len(panes_after)}"
        )

    def test_oscillator_pane_binding_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane IDs regenerated on load instead of restored,
        breaking oscillator-to-pane bindings.
        """
        osc_id = editor.add_oscillator(source_id, name="BindingPersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc_before = editor.get_oscillator_by_id(osc_id)
        pane_before = osc_before.get("paneId", "")

        path = "/tmp/oscil_e2e_binding_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        pane_after = oscs[0].get("paneId", "")

        if pane_before:
            assert pane_after == pane_before, (
                f"Pane binding should persist: before='{pane_before}', after='{pane_after}'"
            )

        # Verify the pane actually exists
        pane_ids = {p["id"] for p in editor.get_panes()}
        assert pane_after in pane_ids, (
            f"Restored paneId '{pane_after}' should exist in pane list"
        )

    def test_pane_names_survive_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane name field not serialized in state XML, so panes
        get default names (e.g., "Pane 1") after load even when user gave
        them meaningful names via pane add API.
        """
        osc_id = editor.add_oscillator(source_id, name="PaneNamePersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Add second pane with a distinctive name
        pane2_id = editor.add_pane("My Custom Pane Name")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        panes_before = editor.get_panes()
        names_before = [p.get("name", "") for p in panes_before]
        assert len(panes_before) >= 2, (
            f"Should have 2+ panes, got {len(panes_before)}"
        )

        path = "/tmp/oscil_e2e_pane_names.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        editor.wait_for_oscillator_count(1, timeout_s=5.0)
        panes_after = editor.get_panes()
        names_after = [p.get("name", "") for p in panes_after]

        assert len(panes_after) >= 2, (
            f"Should have 2+ panes after load, got {len(panes_after)}"
        )

        # Verify the custom pane name was preserved
        assert "My Custom Pane Name" in names_after, (
            f"Custom pane name should survive save/load: "
            f"before={names_before}, after={names_after}"
        )

    def test_multiple_pane_names_survive_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: only the first pane's name is serialized, subsequent
        pane names are lost (common with naive serialization that uses
        a single name field instead of a per-pane list).
        """
        osc_id = editor.add_oscillator(source_id, name="MultiPaneName")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane_names = ["Analysis View", "Reference Track", "Debug Output"]
        for name in pane_names:
            pid = editor.add_pane(name)
            if pid is None:
                pytest.fail("Pane add API not available")

        panes_before = editor.get_panes()
        assert len(panes_before) >= 4  # auto-created + 3 manual

        path = "/tmp/oscil_e2e_multi_pane_names.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        editor.wait_for_oscillator_count(1, timeout_s=5.0)
        panes_after = editor.get_panes()
        names_after = set(p.get("name", "") for p in panes_after)

        for expected_name in pane_names:
            assert expected_name in names_after, (
                f"Pane name '{expected_name}' should survive save/load. "
                f"Got names: {names_after}"
            )
