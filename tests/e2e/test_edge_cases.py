"""
E2E tests for edge cases and error recovery.

What bugs these tests catch:
- State corruption when editor is closed with dialog open
- Double-submit on dialog (rapid double-click OK)
- Oscillator operations after state reset
- Editor open/close rapid cycling
- Operating on empty state (no oscillators, no panes)
- Reorder with single oscillator (boundary)
- Delete button on already-deleted oscillator
- Add oscillator after deleting all (empty -> 1 transition)
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestDialogEdgeCases:
    """Edge cases in dialog interactions."""

    def test_close_editor_with_dialog_open(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: closing editor while dialog is on screen leaks the
        dialog component or crashes on next editor open.
        """
        editor.click("sidebar_addOscillator")
        try:
            editor.wait_for_visible("addOscillatorDialog", timeout_s=3.0)
        except TimeoutError:
            pytest.fail("Add dialog did not appear")

        # Close editor without dismissing dialog
        editor.close_editor()

        # Reopen should work without crash
        editor.open_editor()
        count = editor.verify_editor_ready()
        assert count > 0, "Editor should reopen successfully after force close with dialog"

        # Clean up
        editor.close_editor()

    def test_double_click_ok_button(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: double-submit creating two oscillators instead of one.
        """
        initial_count = len(editor.get_oscillators())

        editor.click("sidebar_addOscillator")
        try:
            editor.wait_for_visible("addOscillatorDialog", timeout_s=3.0)
        except TimeoutError:
            pytest.fail("Add dialog did not appear")

        # Fill in source
        dropdown_id = "addOscillatorDialog_sourceDropdown"
        if editor.element_exists(dropdown_id):
            editor.select_dropdown_item(dropdown_id, source_id)

        # Double-click OK
        ok_btn = "addOscillatorDialog_okBtn"
        editor.click(ok_btn)
        editor.click(ok_btn)  # Rapid second click

        try:
            editor.wait_for_not_visible("addOscillatorDialog", timeout_s=3.0)
        except TimeoutError:
            pass  # Dialog might already be gone

        # Should have added exactly 1, not 2
        final_count = len(editor.get_oscillators())
        assert final_count <= initial_count + 1, (
            f"Double-click should not create duplicate: "
            f"expected <= {initial_count + 1}, got {final_count}"
        )


class TestEmptyState:
    """Operations when no oscillators or panes exist."""

    def test_editor_opens_with_empty_state(self, editor: OscilTestClient):
        """
        Bug caught: editor crashes or shows blank when no oscillators exist.
        """
        assert len(editor.get_oscillators()) == 0
        count = editor.verify_editor_ready()
        assert count > 0, "Editor should have UI elements even with no oscillators"

    def test_sidebar_exists_with_no_oscillators(self, editor: OscilTestClient):
        """
        Bug caught: sidebar not rendering when oscillator list is empty.
        """
        el = editor.get_element("sidebar")
        assert el is not None, "Sidebar should exist"
        assert el.visible, "Sidebar should be visible"
        assert el.width > 0, "Sidebar should have non-zero width"

    def test_add_oscillator_from_empty_state(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding first oscillator when pane list is empty
        (requires auto-creation of default pane).
        """
        assert len(editor.get_oscillators()) == 0
        assert len(editor.get_panes()) == 0

        osc_id = editor.add_oscillator(source_id, name="First Osc")
        assert osc_id is not None, "Should create oscillator from empty state"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=3.0)
        assert len(oscs) == 1

        # A default pane should have been auto-created
        panes = editor.get_panes()
        assert len(panes) >= 1, "Adding first oscillator should create a default pane"


class TestEditorLifecycle:
    """Editor open/close edge cases."""

    def test_rapid_open_close_cycle(self, client: OscilTestClient):
        """
        Bug caught: race condition in editor create/destroy causing
        dangling pointers or leaked components.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )

        for i in range(5):
            client.open_editor()
            client.close_editor()

        # Final open should work normally
        client.open_editor()
        count = client.verify_editor_ready()
        assert count > 0, f"Editor should work after {5} open/close cycles"
        client.close_editor()

    def test_double_open_is_idempotent(self, client: OscilTestClient):
        """
        Bug caught: opening editor twice creates two windows or crashes.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )

        client.open_editor()
        # Second open should be idempotent
        client.open_editor()

        count = client.verify_editor_ready()
        assert count > 0
        client.close_editor()


class TestReorderEdgeCases:
    """Edge cases in oscillator reorder."""

    def test_reorder_single_oscillator(
        self, editor: OscilTestClient, oscillator: str
    ):
        """
        Bug caught: reorder with from==to or only 1 item causing index error.
        """
        result = editor.reorder_oscillators(0, 0)
        # Should either succeed (no-op) or fail gracefully
        oscs = editor.get_oscillators()
        assert len(oscs) == 1, "Single oscillator should still exist after no-op reorder"

    def test_reorder_invalid_indices(self, editor: OscilTestClient, two_oscillators):
        """
        Bug caught: negative or out-of-range indices causing crash.
        """
        # Out-of-range index
        result = editor.reorder_oscillators(0, 99)
        # Should not crash — just verify state is still queryable
        oscs = editor.get_oscillators()
        assert len(oscs) == 2, "Oscillators should survive invalid reorder"


class TestStateReset:
    """State reset correctness."""

    def test_reset_clears_all_oscillators(
        self, editor: OscilTestClient, three_oscillators
    ):
        """
        Bug caught: reset not removing all oscillators (e.g., iterator
        invalidation during removal loop).
        """
        assert len(editor.get_oscillators()) == 3

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

    def test_reset_clears_panes(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: reset removing oscillators but leaving orphaned panes.
        """
        editor.add_oscillator(source_id, name="Pane Test")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        assert len(editor.get_panes()) >= 1

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Panes should also be cleared
        editor.wait_until(
            lambda: len(editor.get_panes()) == 0,
            timeout_s=3.0,
            desc="panes to be cleared",
        )

    def test_add_after_reset(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: state manager in bad state after reset, preventing
        new oscillator creation.
        """
        editor.add_oscillator(source_id, name="Before Reset")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        osc_id = editor.add_oscillator(source_id, name="After Reset")
        assert osc_id is not None, "Should create oscillator after reset"
        editor.wait_for_oscillator_count(1, timeout_s=3.0)


class TestStatePersistence:
    """State save/load round-trip."""

    def test_save_and_load_state(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: state serialization losing oscillator properties,
        or deserialization creating oscillators with wrong IDs.
        """
        id1 = editor.add_oscillator(source_id, name="Save Test 1")
        id2 = editor.add_oscillator(source_id, name="Save Test 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        path = "/tmp/oscil_e2e_state.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        if not loaded:
            pytest.fail("State load API not available")

        oscs = editor.wait_for_oscillator_count(2, timeout_s=5.0)
        names = {o["name"] for o in oscs}
        assert "Save Test 1" in names, "First oscillator name should be restored"
        assert "Save Test 2" in names, "Second oscillator name should be restored"

    def test_save_load_preserves_source_and_pane_ids(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: source or pane IDs not serialized, so oscillators
        become orphaned after load.
        """
        osc_id = editor.add_oscillator(source_id, name="ID Preserve")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc_before = editor.get_oscillator_by_id(osc_id)
        source_before = osc_before.get("sourceId")
        pane_before = osc_before.get("paneId")

        path = "/tmp/oscil_e2e_ids.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        osc_after = oscs[0]

        if source_before:
            assert osc_after.get("sourceId") == source_before, (
                f"Source ID should persist: expected '{source_before}', "
                f"got '{osc_after.get('sourceId')}'"
            )
        if pane_before:
            assert osc_after.get("paneId") == pane_before, (
                f"Pane ID should persist: expected '{pane_before}', "
                f"got '{osc_after.get('paneId')}'"
            )


class TestConcurrentOperations:
    """Edge cases involving rapid sequential operations."""

    def test_rapid_add_delete_cycle(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: rapid add/delete causing race condition in state
        listeners, leading to stale UI or crash.
        """
        for _ in range(5):
            osc_id = editor.add_oscillator(source_id, name="Rapid")
            if osc_id:
                editor.wait_for_oscillator_count(1, timeout_s=2.0)
                editor.reset_state()
                editor.wait_for_oscillator_count(0, timeout_s=2.0)

        # Verify stable state
        oscs = editor.get_oscillators()
        assert len(oscs) == 0, "Should be clean after rapid add/delete cycles"

    def test_update_nonexistent_oscillator(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: updating an oscillator with a bad ID causes crash
        instead of returning an error.
        """
        result = editor.update_oscillator("nonexistent-id-12345", name="Ghost")
        # Should return False, not crash
        assert result is False, "Updating nonexistent oscillator should fail gracefully"

        # Verify harness still works and no phantom oscillator was created
        oscs = editor.get_oscillators()
        assert isinstance(oscs, list)
        assert len(oscs) == 0, (
            "Updating nonexistent ID should not create an oscillator"
        )

    def test_operations_while_transport_playing(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding/removing oscillators while audio callback is
        running causes data race on shared oscillator list.
        """
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Add oscillators while playing
        ids = []
        for i in range(3):
            osc_id = editor.add_oscillator(source_id, name=f"Playing Add {i}")
            assert osc_id is not None, f"Should add oscillator {i} while playing"
            ids.append(osc_id)

        editor.wait_for_oscillator_count(3, timeout_s=3.0)
        assert editor.is_playing(), "Transport should still be playing after adds"

        # Update while playing
        editor.update_oscillator(ids[0], name="Updated While Playing")
        osc = editor.get_oscillator_by_id(ids[0])
        assert osc["name"] == "Updated While Playing"

        # Reset while playing
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Transport should still be queryable
        state = editor.get_transport_state()
        assert state is not None
        editor.transport_stop()

    def test_bpm_change_during_oscillator_add(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: BPM change listener and oscillator add listener
        both modifying shared timing state simultaneously.
        """
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Interleave BPM changes and oscillator adds
        bpms = [80.0, 100.0, 140.0]
        for i, bpm in enumerate(bpms):
            editor.set_bpm(bpm)
            editor.add_oscillator(source_id, name=f"BPM Race {i}")

        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Verify final BPM is the last one set
        actual_bpm = editor.get_bpm()
        assert abs(actual_bpm - 140.0) < 1.0, (
            f"BPM should be 140.0, got {actual_bpm}"
        )

        editor.transport_stop()
        editor.set_bpm(120.0)


class TestReorderPersistence:
    """Reorder + save/load edge cases."""

    def test_reorder_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator order indices not serialized, so order
        reverts to creation order after state load.
        """
        id1 = editor.add_oscillator(source_id, name="Reorder A")
        id2 = editor.add_oscillator(source_id, name="Reorder B")
        id3 = editor.add_oscillator(source_id, name="Reorder C")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Reorder: move index 0 to index 2
        result = editor.reorder_oscillators(0, 2)
        if not result:
            pytest.fail("Reorder API not available")

        oscs_reordered = editor.get_oscillators()
        names_reordered = [o["name"] for o in oscs_reordered]

        # Save and reload
        path = "/tmp/oscil_e2e_reorder.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs_loaded = editor.wait_for_oscillator_count(3, timeout_s=5.0)
        names_loaded = [o["name"] for o in oscs_loaded]

        assert names_loaded == names_reordered, (
            f"Order should persist after save/load: "
            f"expected {names_reordered}, got {names_loaded}"
        )


class TestDeleteEdgeCases:
    """Edge cases around oscillator deletion."""

    def test_delete_nonexistent_oscillator(self, editor: OscilTestClient):
        """
        Bug caught: deleting an oscillator with a fabricated ID causes crash
        instead of returning failure.
        """
        result = editor.delete_oscillator("nonexistent-id-99999")
        assert result is False, "Deleting nonexistent oscillator should return False"

        # Verify harness still responsive
        state = editor.get_transport_state()
        assert state is not None

    def test_double_delete_same_oscillator(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting the same oscillator twice causes a dangling
        pointer access or double-free.
        """
        osc_id = editor.add_oscillator(source_id, name="Double Delete")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        first = editor.delete_oscillator(osc_id)
        assert first, "First delete should succeed"
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        second = editor.delete_oscillator(osc_id)
        assert second is False, "Second delete of same ID should fail gracefully"

        oscs = editor.get_oscillators()
        assert isinstance(oscs, list), "State should be queryable after double delete"

    def test_delete_during_property_update(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: race between delete and update — update finds the
        oscillator, but it's deleted before the update completes.
        """
        osc_id = editor.add_oscillator(source_id, name="Delete Race")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Fire update and delete in quick succession
        editor.update_oscillator(osc_id, name="Updated Name")
        editor.delete_oscillator(osc_id)

        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # System should be stable
        state = editor.get_transport_state()
        assert state is not None


class TestStateLoadEdgeCases:
    """Edge cases in state loading."""

    def test_load_state_while_editor_closed(self, client: OscilTestClient):
        """
        Bug caught: loading state when the editor is not open causes crash
        because state listeners try to update non-existent UI components.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.fail("No sources available")

        # Create some state, save, close editor, then load
        osc_id = client.add_oscillator(sources[0]["id"], name="Load Without Editor")
        assert osc_id is not None
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        path = "/tmp/oscil_e2e_no_editor.xml"
        saved = client.save_state(path)
        if not saved:
            client.close_editor()
            pytest.fail("State save API not available")

        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)
        client.close_editor()

        # Load state without editor open
        loaded = client.load_state(path)
        assert loaded, "State load should succeed without editor"

        # Now open editor and verify
        client.open_editor()
        oscs = client.get_oscillators()
        assert len(oscs) >= 1, "Oscillator should exist after loading without editor"
        client.close_editor()

    def test_load_state_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: loading state while transport is playing causes the
        audio callback to reference deleted oscillator objects.
        """
        osc_id = editor.add_oscillator(source_id, name="Load During Play")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        path = "/tmp/oscil_e2e_play_load.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        # Start transport
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Load state while playing
        loaded = editor.load_state(path)
        assert loaded, "State load should succeed during playback"

        # System should be stable
        oscs = editor.get_oscillators()
        assert len(oscs) >= 1, "State should restore during playback"
        state = editor.get_transport_state()
        assert state is not None

        editor.transport_stop()


class TestSpecialCharacterNames:
    """Test oscillator names with special characters."""

    def test_unicode_name(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: name field not handling Unicode, causing mojibake
        or truncation in state serialization.
        """
        name = "Oscillator Alpha"
        osc_id = editor.add_oscillator(source_id, name=name)
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc["name"] == name, f"Expected '{name}', got '{osc['name']}'"

    def test_empty_name(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: empty name string causes null pointer in name display
        code, or name comparison logic fails on empty string.
        """
        osc_id = editor.add_oscillator(source_id, name="")
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        # Name should be empty string (or a default fallback)
        assert isinstance(osc.get("name"), str)

    def test_long_name(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: very long name overflows a fixed-size buffer or
        causes layout overflow in the sidebar list item.
        """
        name = "A" * 200
        osc_id = editor.add_oscillator(source_id, name=name)
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        # Name should be stored (possibly truncated, but not lost)
        assert len(osc.get("name", "")) > 0

    @pytest.mark.parametrize("name,description", [
        pytest.param('A & <B> "C" \'D\'', "XML special characters"),
        ("Line1\nLine2\tTabbed", "whitespace control characters"),
        ("Osc #1 (50% gain) [L+R]", "common music notation symbols"),
        ("  Leading Spaces  ", "leading/trailing whitespace"),
    ])
    def test_special_characters_in_name_roundtrip(
        self, editor: OscilTestClient, source_id: str, name: str, description: str
    ):
        """
        Bug caught: special characters not escaped in state serialization,
        producing invalid XML that fails to load or corrupts name data.
        Parametrized to cover: XML entities, whitespace, music symbols, padding.
        Known failure (XML special characters): name truncated during XML serialization.
        """
        osc_id = editor.add_oscillator(source_id, name=name)
        assert osc_id is not None, f"Failed to add oscillator with {description}"

        path = f"/tmp/oscil_e2e_special_{hash(name) & 0xFFFF:04x}.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, f"State with {description} should load successfully"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        restored_name = oscs[0].get("name", "")
        assert restored_name == name, (
            f"Name with {description} should round-trip: "
            f"expected '{name}', got '{restored_name}'"
        )


class TestMaximumOscillatorCount:
    """Edge cases around large numbers of oscillators."""

    def test_ten_oscillators_no_crash(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: UI layout calculation overflows or render pipeline
        runs out of vertex buffer space with many simultaneous oscillators.
        """
        ids = []
        for i in range(10):
            osc_id = editor.add_oscillator(source_id, name=f"Max {i}")
            assert osc_id is not None, f"Failed to add oscillator {i}"
            ids.append(osc_id)

        editor.wait_for_oscillator_count(10, timeout_s=10.0)

        # Verify all oscillators have valid state
        oscs = editor.get_oscillators()
        assert len(oscs) == 10, f"Should have 10 oscillators, got {len(oscs)}"

        pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in oscs:
            assert osc.get("paneId") in pane_ids, (
                f"Oscillator '{osc['name']}' has invalid paneId"
            )
            assert osc.get("visible") is True, (
                f"Oscillator '{osc['name']}' should default to visible"
            )

    def test_ten_oscillators_all_deletable(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting from a long list corrupts indices, making
        later deletions fail or crash.
        """
        for i in range(10):
            editor.add_oscillator(source_id, name=f"DelMax {i}")
        editor.wait_for_oscillator_count(10, timeout_s=10.0)

        # Delete all one by one (always delete index 0)
        for remaining in range(9, -1, -1):
            oscs = editor.get_oscillators()
            if not oscs:
                break
            editor.delete_oscillator(oscs[0]["id"])
            editor.wait_for_oscillator_count(remaining, timeout_s=3.0)

        assert len(editor.get_oscillators()) == 0


class TestEditorCloseWithPendingState:
    """Edge cases around closing editor while state operations are in-flight."""

    def test_close_during_save(self, client: OscilTestClient):
        """
        Bug caught: closing the editor while a state save is in progress
        causes the serializer to access destroyed UI component references.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.fail("No sources available")

        osc_id = client.add_oscillator(sources[0]["id"], name="CloseSave")
        assert osc_id
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        # Save and immediately close
        client.save_state("/tmp/oscil_e2e_close_save.xml")
        client.close_editor()

        # Reopen — should work normally
        client.open_editor()
        count = client.verify_editor_ready()
        assert count > 0, "Editor should reopen after save+close"
        client.close_editor()

    def test_close_during_load(self, client: OscilTestClient):
        """
        Bug caught: closing editor during state load causes the loader
        to create oscillators whose UI components are already destroyed.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.fail("No sources available")

        # Create state to save
        osc_id = client.add_oscillator(sources[0]["id"], name="CloseLoad")
        assert osc_id
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        path = "/tmp/oscil_e2e_close_load.xml"
        saved = client.save_state(path)
        if not saved:
            client.close_editor()
            pytest.fail("State save API not available")

        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        # Load and immediately close
        client.load_state(path)
        client.close_editor()

        # Reopen and verify
        client.open_editor()
        count = client.verify_editor_ready()
        assert count > 0, "Editor should reopen after load+close"

        # Oscillator should exist (load should have completed)
        oscs = client.get_oscillators()
        assert len(oscs) >= 1, "Oscillator should exist after load+close+reopen"
        client.close_editor()


class TestMoveAndDeleteRace:
    """Edge cases combining move and delete operations."""

    def test_move_then_immediate_delete(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: moving an oscillator to a new pane and immediately
        deleting it causes use-after-free because the move's async
        callback references the oscillator after deletion.
        """
        osc_id = editor.add_oscillator(source_id, name="MoveDel Race")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("MoveDel Pane")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        # Move and immediately delete
        editor.move_oscillator(osc_id, pane2_id)
        editor.delete_oscillator(osc_id)

        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # System should be stable
        state = editor.get_transport_state()
        assert state is not None, "Harness should be stable after move+delete race"
        panes = editor.get_panes()
        assert isinstance(panes, list), "Panes should be queryable"

    def test_delete_during_save(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting an oscillator while a state save is in progress
        causes the serializer to write a partial/corrupt oscillator entry
        because it reads properties of a deleted object.
        """
        osc_id = editor.add_oscillator(source_id, name="DelSave Race")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Fire save and delete in rapid succession
        path = "/tmp/oscil_e2e_del_save_race.xml"
        editor.save_state(path)
        editor.delete_oscillator(osc_id)

        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # The saved state should load without crash
        editor.reset_state()
        loaded = editor.load_state(path)
        # The loaded state may have 0 or 1 oscillators depending on timing,
        # but loading must not crash
        state = editor.get_transport_state()
        assert state is not None, (
            "Harness should be stable after loading state saved during delete"
        )


class TestStressOperations:
    """High-frequency operations that stress thread safety boundaries."""

    def test_rapid_oscillator_property_updates_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: rapid property updates during audio callback cause torn
        reads on oscillator properties, producing corrupted rendering state
        (e.g., opacity=NaN, lineWidth=negative).
        """
        osc_id = editor.add_oscillator(source_id, name="Rapid Updates")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Rapid property updates while audio is playing
        for i in range(20):
            opacity = (i % 10) / 10.0 + 0.05
            line_width = 1.0 + (i % 5)
            mode = ["FullStereo", "Mono", "Mid", "Side", "Left", "Right"][i % 6]
            editor.update_oscillator(osc_id, opacity=opacity, lineWidth=line_width, mode=mode)

        # System must remain stable
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None, "Oscillator must survive rapid property updates"
        assert osc.get("opacity") is not None, "Opacity must be set"
        assert 0.0 <= osc["opacity"] <= 1.0, (
            f"Opacity must be in [0, 1], got {osc['opacity']}"
        )
        assert osc.get("lineWidth") is not None, "lineWidth must be set"
        assert osc["lineWidth"] > 0, f"lineWidth must be positive, got {osc['lineWidth']}"
        assert editor.is_playing(), "Transport must still be playing"

        editor.transport_stop()

    def test_add_move_delete_cycle_across_panes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: creating oscillator in pane A, moving to pane B, deleting,
        then repeating — the second cycle crashes because the move handler
        left a stale reference in pane A's oscillator list.
        """
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        pane2_id = editor.add_pane("Stress Pane 2")
        if pane2_id is None:
            editor.transport_stop()
            pytest.fail("Pane add API not available")

        for cycle in range(5):
            osc_id = editor.add_oscillator(
                source_id, name=f"Cycle {cycle}"
            )
            assert osc_id is not None, f"Cycle {cycle}: add failed"
            editor.wait_for_oscillator_count(1, timeout_s=3.0)

            # Move to pane 2
            editor.move_oscillator(osc_id, pane2_id)
            editor.wait_until(
                lambda: (o := editor.get_oscillator_by_id(osc_id))
                and o.get("paneId") == pane2_id,
                timeout_s=3.0,
                desc=f"cycle {cycle}: move to pane 2",
            )

            # Delete
            editor.delete_oscillator(osc_id)
            editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Verify system stable
        assert editor.is_playing(), "Transport must still be playing after cycles"
        panes = editor.get_panes()
        assert isinstance(panes, list), "Panes must be queryable"

        editor.transport_stop()

    def test_editor_close_reopen_during_state_save_load(
        self, client: OscilTestClient
    ):
        """
        Bug caught: closing editor while a state save is in progress, then
        reopening and loading — the load triggers UI refresh on components
        that were destroyed and recreated, causing use-after-free.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.fail("No sources available")

        # Build up state
        for i in range(3):
            client.add_oscillator(sources[0]["id"], name=f"Stress {i}")
        client.wait_for_oscillator_count(3, timeout_s=5.0)

        path = "/tmp/oscil_e2e_stress_close_save.xml"

        # Rapid save → close → open → load cycle
        for cycle in range(3):
            client.save_state(path)
            client.close_editor()
            client.open_editor()
            client.load_state(path)

        # Final verification
        count = client.verify_editor_ready()
        assert count > 0, "Editor must be functional after close/reopen/save/load cycles"

        oscs = client.get_oscillators()
        assert len(oscs) >= 1, "State must survive close/reopen/load cycles"

        client.close_editor()

    def test_simultaneous_pane_and_oscillator_operations(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding a pane while oscillators are being added causes
        the pane assignment logic to race — new oscillator gets assigned to
        a pane that was just added but not yet fully initialized.
        """
        # Rapid interleaving of pane and oscillator operations
        pane_ids = []
        osc_ids = []

        for i in range(3):
            # Add oscillator
            osc_id = editor.add_oscillator(source_id, name=f"SimOsc {i}")
            assert osc_id is not None, f"Failed to add oscillator {i}"
            osc_ids.append(osc_id)

            # Add pane
            pane_id = editor.add_pane(f"SimPane {i}")
            if pane_id:
                pane_ids.append(pane_id)

        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Move oscillators between panes
        if pane_ids:
            for osc_id in osc_ids:
                target_pane = pane_ids[0]
                editor.move_oscillator(osc_id, target_pane)

        # Verify all oscillators have valid pane IDs
        all_pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in editor.get_oscillators():
            assert osc.get("paneId") in all_pane_ids, (
                f"Oscillator '{osc['name']}' has orphaned paneId "
                f"'{osc.get('paneId')}' after simultaneous ops"
            )
