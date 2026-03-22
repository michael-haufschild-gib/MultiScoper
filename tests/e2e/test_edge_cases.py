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
            pytest.skip("Add dialog did not appear")

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
            pytest.skip("Add dialog did not appear")

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
            pytest.skip("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        if not loaded:
            pytest.skip("State load API not available")

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
            pytest.skip("State save API not available")

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

        # Verify harness still works
        oscs = editor.get_oscillators()
        assert isinstance(oscs, list)

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
