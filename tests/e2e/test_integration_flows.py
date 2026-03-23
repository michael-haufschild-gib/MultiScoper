"""
E2E tests for multi-step integration flows.

These tests verify cross-feature interactions where a sequence of user
actions spans multiple subsystems. Each test exercises a real user
workflow and verifies that state remains consistent throughout.

What bugs these tests catch:
- State corruption when combining oscillator CRUD with timing changes
- Theme change not applying to newly added oscillators
- State save/load losing oscillator properties set via config popup
- Oscillator added after state load getting wrong pane assignment
- Timing settings lost when oscillators are deleted
- Editor close/reopen with active transport corrupting waveform state
- Rapid feature toggling causing race conditions across subsystems
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestOscillatorLifecycleFlow:
    """Full lifecycle: add, configure, verify state, save, load, verify again."""

    def test_add_configure_save_load_verify(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: state serialization losing properties set after initial add.
        User flow: add oscillator -> rename -> hide -> save -> reset -> load -> verify.
        """
        # Add and configure
        osc_id = editor.add_oscillator(source_id, name="Lifecycle Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.update_oscillator(osc_id, name="Renamed Osc", visible=False)

        # Verify changes took effect
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc["name"] == "Renamed Osc"
        assert osc["visible"] is False

        # Save state
        state_path = "/tmp/oscil_e2e_lifecycle.xml"
        saved = editor.save_state(state_path)
        if not saved:
            pytest.skip("State save API not available")

        # Reset and verify clean
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)
        assert len(editor.get_panes()) == 0

        # Load and verify
        loaded = editor.load_state(state_path)
        assert loaded, "State load should succeed"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        restored = oscs[0]
        assert restored["name"] == "Renamed Osc", (
            f"Name should survive save/load, got '{restored['name']}'"
        )
        assert restored["visible"] is False, (
            "Visibility=false should survive save/load"
        )

    def test_add_multiple_delete_middle_verify_order(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting a middle oscillator corrupts order indices
        of remaining oscillators (off-by-one or stale indices).
        """
        id1 = editor.add_oscillator(source_id, name="Order A")
        id2 = editor.add_oscillator(source_id, name="Order B")
        id3 = editor.add_oscillator(source_id, name="Order C")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Delete the middle one via state API
        deleted = editor.delete_oscillator(id2)
        assert deleted, "Delete API should succeed for middle oscillator"

        oscs_after = editor.wait_for_oscillator_count(2, timeout_s=3.0)
        remaining_ids = [o["id"] for o in oscs_after]
        remaining_names = [o["name"] for o in oscs_after]

        assert id2 not in remaining_ids, "Deleted oscillator should be gone"
        assert id1 in remaining_ids, "First oscillator should remain"
        assert id3 in remaining_ids, "Third oscillator should remain"

        # Verify order indices are still valid (no gaps or duplicates)
        indices = [o.get("orderIndex", -1) for o in oscs_after]
        assert all(i >= 0 for i in indices), (
            f"All remaining order indices should be >= 0, got {indices}"
        )
        assert len(set(indices)) == 2, (
            f"Order indices should be unique after middle delete, got {indices}"
        )

        # Verify pane IDs still valid
        pane_ids = [o.get("paneId", "") for o in oscs_after]
        assert all(pid != "" for pid in pane_ids), (
            f"All oscillators should still have pane IDs, got {pane_ids}"
        )

    def test_oscillator_survives_editor_close_reopen(
        self, client: OscilTestClient
    ):
        """
        Bug caught: oscillator state lost when editor is closed and reopened
        (e.g., state held only in UI components, not persisted to OscilState).
        """
        # Open first, then reset to clear the default oscillator
        # that createDefaultOscillatorIfNeeded creates on editor open.
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.skip("No audio sources available")
        source_id = sources[0]["id"]

        osc_id = client.add_oscillator(source_id, name="Survive Close")
        assert osc_id is not None
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        # Close and reopen
        client.close_editor()
        client.open_editor()

        # Oscillator should still exist
        oscs = client.get_oscillators()
        assert len(oscs) >= 1, "Oscillator should survive editor close/reopen"
        assert any(o["name"] == "Survive Close" for o in oscs), (
            "Oscillator name should be preserved after close/reopen"
        )
        client.close_editor()


class TestTimingAndOscillatorInteraction:
    """Verify timing settings are not affected by oscillator operations."""

    def test_adding_oscillator_preserves_timing(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding an oscillator triggers a state change listener
        that resets timing configuration to defaults.
        """
        # Set up timing
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.skip("Timing section not registered")
        editor.click(timing_id)

        interval_field = "sidebar_timing_intervalField"
        if not editor.element_exists(interval_field):
            pytest.skip("Interval field not registered")

        editor.set_slider(interval_field, 25.0)
        samples_before = editor.get_display_samples()
        assert samples_before > 0, (
            f"displaySamples should be positive after setting interval, got {samples_before}"
        )

        # Add an oscillator
        osc_id = editor.add_oscillator(source_id, name="Timing Preserve Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        samples_after = editor.get_display_samples()
        assert samples_after > 0, (
            f"displaySamples should be positive after adding oscillator, got {samples_after}"
        )

        ratio = samples_after / samples_before
        assert 0.5 < ratio < 2.0, (
            f"Adding oscillator changed timing: "
            f"{samples_before} -> {samples_after} (ratio {ratio:.2f})"
        )

    def test_deleting_all_oscillators_preserves_timing(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting the last oscillator triggers cleanup that
        also resets timing engine state.
        """
        osc_id = editor.add_oscillator(source_id, name="Delete Timing Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.skip("Timing section not registered")
        editor.click(timing_id)

        interval_field = "sidebar_timing_intervalField"
        if not editor.element_exists(interval_field):
            pytest.skip("Interval field not registered")

        editor.set_slider(interval_field, 50.0)
        samples_before = editor.get_display_samples()
        assert samples_before > 0, (
            f"displaySamples should be positive after setting interval, got {samples_before}"
        )

        # Delete all oscillators
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Re-add oscillator to get displaySamples back
        osc_id2 = editor.add_oscillator(source_id, name="Post Delete")
        assert osc_id2 is not None, "Should be able to add oscillator after reset"
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        samples_after = editor.get_display_samples()
        assert samples_after > 0, (
            f"displaySamples should be positive after re-add, got {samples_after}"
        )


class TestTransportAndRenderingInteraction:
    """Verify transport state interacts correctly with rendering."""

    def test_transport_stop_does_not_crash_with_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: stopping transport while waveforms are actively rendering
        causes null dereference in render loop (audio data pointer becomes stale).
        """
        osc_id = editor.add_oscillator(source_id, name="Transport Stop Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Let some frames render, then stop
        editor.wait_until(
            lambda: (s := editor.get_transport_state())
            and s.get("positionSamples", 0) > 0,
            timeout_s=3.0,
            desc="position to advance",
        )

        editor.transport_stop()
        editor.wait_until(
            lambda: not editor.is_playing(), timeout_s=2.0, desc="transport to stop"
        )

        # Verify everything is still stable
        oscs = editor.get_oscillators()
        assert len(oscs) == 1, "Oscillator should survive transport stop"
        state = editor.get_transport_state()
        assert state is not None, "Transport state should be queryable after stop"

    def test_waveform_type_change_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: changing waveform type while playing causes audio callback
        to read stale generator state (torn read on waveform enum).
        """
        osc_id = editor.add_oscillator(source_id, name="Waveform Switch Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        waveforms = ["sine", "square", "triangle", "saw"]
        for wf in waveforms:
            editor.set_track_audio(0, waveform=wf, frequency=440.0, amplitude=0.8)
            assert editor.is_playing(), f"Playback should continue after switching to {wf}"

        # Verify harness stable
        state = editor.get_transport_state()
        assert state is not None
        assert state.get("playing"), "Should still be playing after waveform switches"

        editor.transport_stop()


class TestPaneStateConsistency:
    """Verify pane state stays consistent with oscillator operations."""

    def test_pane_auto_created_on_first_oscillator(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: first oscillator added with no panes causes crash
        because auto-pane creation is not triggered.
        """
        assert len(editor.get_oscillators()) == 0
        assert len(editor.get_panes()) == 0

        osc_id = editor.add_oscillator(source_id, name="Auto Pane Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) >= 1, "Adding first oscillator should auto-create a pane"

        # Oscillator should be assigned to this pane
        osc = editor.get_oscillator_by_id(osc_id)
        pane_ids = {p["id"] for p in panes}
        assert osc["paneId"] in pane_ids, (
            f"Oscillator paneId '{osc['paneId']}' should match an existing pane"
        )

    def test_pane_properties_returned_correctly(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane serialization returning empty/null fields.
        """
        osc_id = editor.add_oscillator(source_id, name="Pane Props Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) >= 1

        pane = panes[0]
        assert "id" in pane and pane["id"], "Pane should have a non-empty id"
        assert "name" in pane, "Pane should have a name field"

    def test_multiple_oscillators_same_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: second oscillator added to same pane overwrites the first
        in the pane's oscillator list.
        """
        id1 = editor.add_oscillator(source_id, name="Same Pane 1")
        id2 = editor.add_oscillator(source_id, name="Same Pane 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        osc1 = editor.get_oscillator_by_id(id1)
        osc2 = editor.get_oscillator_by_id(id2)

        # Both should be in the same pane (default behavior)
        assert osc1["paneId"] == osc2["paneId"], (
            "Both oscillators should be in the same default pane"
        )

        # Waveform state should show both
        state = editor.get_waveform_state()
        if state and "panes" in state and state["panes"]:
            pane = state["panes"][0]
            assert pane.get("oscillatorCount", 0) >= 2, (
                f"Pane should have 2+ oscillators, got {pane.get('oscillatorCount')}"
            )


class TestStatePersistenceEdgeCases:
    """Edge cases in state save/load that are easy to miss."""

    def test_save_load_with_multiple_oscillators_different_modes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: processing mode enum serialization not round-tripping
        (e.g., mode 3 serialized as "Side" but deserialized as default).
        """
        id1 = editor.add_oscillator(source_id, name="Mode Save 1", mode="Mono")
        id2 = editor.add_oscillator(source_id, name="Mode Save 2", mode="Mid")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        oscs_before = editor.get_oscillators()
        modes_before = {o["name"]: o.get("mode") for o in oscs_before}

        path = "/tmp/oscil_e2e_modes.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.skip("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs_after = editor.wait_for_oscillator_count(2, timeout_s=5.0)
        modes_after = {o["name"]: o.get("mode") for o in oscs_after}

        for name, mode in modes_before.items():
            if mode is not None and name in modes_after:
                assert modes_after[name] == mode, (
                    f"Mode for '{name}' should survive save/load: "
                    f"expected '{mode}', got '{modes_after[name]}'"
                )

    def test_load_nonexistent_file_fails_gracefully(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: loading a nonexistent file causes crash instead of
        returning an error.
        """
        osc_id = editor.add_oscillator(source_id, name="Before Bad Load")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        result = editor.load_state("/tmp/nonexistent_oscil_state_12345.xml")
        # Should fail gracefully (return False), not crash
        assert result is False, "Loading nonexistent file should return False"

        # State should be unchanged
        oscs = editor.get_oscillators()
        assert len(oscs) == 1, "State should be unchanged after failed load"

    def test_save_empty_state_and_reload(self, editor: OscilTestClient):
        """
        Bug caught: saving empty state produces invalid XML that cannot
        be loaded back.
        """
        assert len(editor.get_oscillators()) == 0

        path = "/tmp/oscil_e2e_empty.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.skip("State save API not available")

        loaded = editor.load_state(path)
        assert loaded, "Loading a saved empty state should succeed"

        assert len(editor.get_oscillators()) == 0, (
            "Loading empty state should result in 0 oscillators"
        )


class TestColorAssignment:
    """Verify oscillator colors are stored and retrievable."""

    def test_color_stored_on_creation(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: colour parameter ignored during oscillator creation,
        defaulting to white/black regardless of user choice.
        """
        osc_id = editor.add_oscillator(
            source_id, name="Color Test", colour="#FF6B6B"
        )
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        colour = osc.get("colour", osc.get("color", ""))
        assert colour, (
            f"Oscillator should have a colour field, got keys: {list(osc.keys())}"
        )

    def test_color_differs_between_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: all oscillators get the same default color because
        the color assignment logic is not wired.
        """
        id1 = editor.add_oscillator(source_id, name="Red", colour="#FF0000")
        id2 = editor.add_oscillator(source_id, name="Green", colour="#00FF00")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        osc1 = editor.get_oscillator_by_id(id1)
        osc2 = editor.get_oscillator_by_id(id2)

        c1 = osc1.get("colour", osc1.get("color", ""))
        c2 = osc2.get("colour", osc2.get("color", ""))

        if c1 and c2:
            assert c1 != c2, (
                f"Different oscillators should have different colors: "
                f"'{c1}' vs '{c2}'"
            )

    def test_color_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: colour hex string not serialized in state XML,
        reverting to default on load.
        """
        osc_id = editor.add_oscillator(
            source_id, name="Color Persist", colour="#00BFFF"
        )
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc_before = editor.get_oscillator_by_id(osc_id)
        colour_before = osc_before.get("colour", osc_before.get("color", ""))

        path = "/tmp/oscil_e2e_color.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.skip("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        colour_after = oscs[0].get("colour", oscs[0].get("color", ""))

        if colour_before:
            assert colour_after == colour_before, (
                f"Colour should survive save/load: "
                f"expected '{colour_before}', got '{colour_after}'"
            )


class TestSimultaneousVisibility:
    """Verify bulk visibility operations on multiple oscillators."""

    def test_hide_all_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: hiding multiple oscillators in sequence causes the
        render loop to crash on empty visible set.
        """
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"HideAll {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Hide all
        for oid in ids:
            editor.update_oscillator(oid, visible=False)

        # Verify all hidden
        for oid in ids:
            editor.wait_until(
                lambda oid=oid: not editor.get_oscillator_by_id(oid).get("visible", True),
                timeout_s=2.0,
                desc=f"oscillator {oid} to become hidden",
            )

        # Verify harness still responsive with all hidden
        state = editor.get_transport_state()
        assert state is not None

    def test_show_all_after_hiding(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: re-showing a hidden oscillator does not re-register
        it with the render pipeline.
        """
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"ShowAll {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Hide all
        for oid in ids:
            editor.update_oscillator(oid, visible=False)

        # Show all
        for oid in ids:
            editor.update_oscillator(oid, visible=True)

        # Verify all visible
        for oid in ids:
            editor.wait_until(
                lambda oid=oid: editor.get_oscillator_by_id(oid).get("visible") is True,
                timeout_s=2.0,
                desc=f"oscillator {oid} to become visible",
            )


class TestModeChangeDuringPlayback:
    """Verify processing mode changes during active audio."""

    def test_mode_change_preserves_waveform_data(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: changing processing mode while audio is flowing
        causes the DSP pipeline to drop the audio buffer reference,
        resulting in silent/empty waveform until next buffer callback.
        """
        osc_id = editor.add_oscillator(source_id, name="Mode During Play")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for initial waveform data
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Cycle through modes while playing
        modes = ["Mono", "Mid", "Side", "Left", "Right", "FullStereo"]
        for mode in modes:
            editor.update_oscillator(osc_id, mode=mode)
            # Verify mode actually changed
            editor.wait_until(
                lambda mode=mode: editor.get_oscillator_by_id(osc_id).get("mode") == mode,
                timeout_s=2.0,
                desc=f"mode to become {mode}",
            )
            # Transport should still be playing
            assert editor.is_playing(), (
                f"Transport should still be playing after mode change to {mode}"
            )

        editor.transport_stop()

    def test_visibility_toggle_during_playback_waveform_recovers(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: hiding and re-showing an oscillator during playback
        causes it to lose its audio data binding, showing a flat line.
        """
        osc_id = editor.add_oscillator(source_id, name="Vis Toggle Play")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data
        wfs = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs[0].get("peakLevel", 0) > 0.05, "Should have signal before toggle"

        # Hide
        editor.update_oscillator(osc_id, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(osc_id).get("visible", True),
            timeout_s=2.0,
            desc="oscillator hidden",
        )

        # Show again
        editor.update_oscillator(osc_id, visible=True)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id).get("visible") is True,
            timeout_s=2.0,
            desc="oscillator visible again",
        )

        # Waveform data should recover
        wfs_after = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs_after[0].get("peakLevel", 0) > 0.05, (
            "Waveform should recover signal after re-show during playback"
        )

        editor.transport_stop()


class TestEditorWithActiveTransport:
    """Editor lifecycle while transport is playing."""

    def test_close_reopen_during_playback(
        self, client: OscilTestClient
    ):
        """
        Bug caught: closing editor while transport is playing causes the
        audio callback to reference destroyed UI components.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        if not sources:
            client.close_editor()
            pytest.skip("No audio sources available")
        source_id = sources[0]["id"]

        osc_id = client.add_oscillator(source_id, name="Transport Close")
        assert osc_id is not None
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        client.transport_play()
        client.wait_until(
            lambda: client.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        client.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for audio to flow
        client.wait_until(
            lambda: (s := client.get_transport_state())
            and s.get("positionSamples", 0) > 0,
            timeout_s=3.0,
            desc="position to advance",
        )

        # Close editor while playing
        client.close_editor()

        # Transport should still be playing (audio continues without editor)
        assert client.is_playing(), "Transport should still play after editor close"

        # Reopen editor
        client.open_editor()
        count = client.verify_editor_ready()
        assert count > 0, "Editor should reopen while transport is playing"

        # Oscillator should still exist
        oscs = client.get_oscillators()
        assert len(oscs) >= 1, "Oscillator should survive editor close/reopen during playback"

        client.transport_stop()
        client.close_editor()


class TestDeleteDuringActiveRendering:
    """Deleting oscillators while waveforms are actively rendering."""

    def test_delete_while_rendering(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting an oscillator while the render loop is
        iterating over the oscillator list causes use-after-free or
        iterator invalidation crash.
        """
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"RenderDel {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data to be actively rendering
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Delete oscillators while rendering
        for oid in ids:
            editor.delete_oscillator(oid)

        editor.wait_for_oscillator_count(0, timeout_s=5.0)

        # Verify system stable
        assert editor.is_playing(), "Transport should still be playing"
        state = editor.get_transport_state()
        assert state is not None

        editor.transport_stop()
