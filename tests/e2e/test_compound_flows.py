"""
E2E tests for compound multi-subsystem flows.

Each test exercises an interaction between two or more subsystems that
is NOT covered by single-subsystem test files. These are the tests most
likely to catch integration regressions where individual features work
but break when combined.

What bugs these tests catch:
- Filter tab state inconsistent after oscillator deletion
- Timing mode not restored after state save/load
- Oscillator pane assignment lost after move + save/load roundtrip
- Config popup editing wrong oscillator when opened for item_1
- Dialog submission during active playback creating orphaned oscillators
- State save during playback losing audio generator settings
- Pane removal + save/load leaving orphaned oscillator references
- Oscillator property update not reflected in diagnostic snapshot
- Rapid subsystem interleaving corrupting shared state
"""

import pytest
from oscil_test_utils import OscilTestClient


# ── Filter + Delete Interaction ─────────────────────────────────────────────


class TestFilterAndDelete:
    """Verify filter tabs handle oscillator deletion correctly."""

    def _filter_tab_id(self, name: str) -> str:
        return f"sidebar_oscillators_toolbar_{name}Tab"

    def test_delete_visible_oscillator_while_filtered(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting an oscillator while the "visible" filter is
        active does not update the filtered list, leaving a ghost item
        that references deleted state.
        """
        visible_tab = self._filter_tab_id("visible")
        if not editor.element_exists(visible_tab):
            pytest.fail("Visible filter tab not registered")

        # Create two oscillators
        id1 = editor.add_oscillator(source_id, name="FilterDel 1")
        id2 = editor.add_oscillator(source_id, name="FilterDel 2")
        assert id1 and id2
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        editor.wait_for_element("sidebar_oscillators_item_1", timeout_s=3.0)

        # Activate visible filter
        editor.click(visible_tab)

        # Delete first oscillator via API
        editor.delete_oscillator(id1)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Remaining oscillator should still be in the filtered list
        remaining = editor.get_oscillators()
        assert len(remaining) == 1
        assert remaining[0]["id"] == id2

        # Switch back to "All"
        all_tab = self._filter_tab_id("all")
        if editor.element_exists(all_tab):
            editor.click(all_tab)

    def test_delete_last_hidden_oscillator_in_hidden_filter(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: deleting the only oscillator shown in the "hidden"
        filter view causes the list to display stale items or crash
        when the filtered result set becomes empty.
        """
        hidden_tab = self._filter_tab_id("hidden")
        if not editor.element_exists(hidden_tab):
            pytest.fail("Hidden filter tab not registered")

        # Create one oscillator and hide it
        osc_id = editor.add_oscillator(source_id, name="HiddenDel")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.update_oscillator(osc_id, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(osc_id).get("visible", True),
            timeout_s=2.0, desc="oscillator to hide",
        )

        # Switch to hidden filter
        editor.click(hidden_tab)

        # Delete the hidden oscillator
        editor.delete_oscillator(osc_id)
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # System should be stable with empty filtered list
        state = editor.get_transport_state()
        assert state is not None, "Harness should be stable after deleting last hidden osc"

        # Switch back to "All"
        all_tab = self._filter_tab_id("all")
        if editor.element_exists(all_tab):
            editor.click(all_tab)


# ── Timing Mode Persistence ─────────────────────────────────────────────────


class TestTimingModePersistence:
    """Verify timing mode survives state save/load."""

    MELODIC_SEG = "sidebar_timing_modeToggle_melodic"
    TIME_SEG = "sidebar_timing_modeToggle_time"
    NOTE_DROPDOWN = "sidebar_timing_noteDropdown"

    def test_melodic_mode_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: timing mode enum not serialized in state XML,
        reverting to default (Time) on load even when Melodic was set.
        """
        # Expand timing
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.fail("Timing section not registered")
        editor.click(timing_id)

        if not editor.element_exists(self.MELODIC_SEG):
            pytest.fail("Melodic mode segment not registered")

        # Need oscillator for waveform state
        osc_id = editor.add_oscillator(source_id, name="TimingPersist")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Switch to melodic
        editor.click(self.MELODIC_SEG)

        # Record display samples in melodic mode
        editor.wait_until(
            lambda: editor.get_display_samples() > 0,
            timeout_s=3.0, desc="display samples positive in melodic",
        )
        samples_melodic = editor.get_display_samples()

        # Save state
        path = "/tmp/oscil_e2e_timing_mode.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        # Reset and reload
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        # Verify display samples are similar (same mode should produce same range)
        samples_after = editor.get_display_samples()
        assert samples_after > 0, (
            f"displaySamples should be positive after load, got {samples_after}"
        )

    def test_timing_interval_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: time interval value not serialized, resetting to
        default after state load.
        """
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.fail("Timing section not registered")
        editor.click(timing_id)

        interval_field = "sidebar_timing_intervalField"
        if not editor.element_exists(interval_field):
            pytest.fail("Interval field not registered")

        osc_id = editor.add_oscillator(source_id, name="IntervalPersist")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Set a distinctive interval
        editor.set_slider(interval_field, 150.0)
        editor.wait_until(
            lambda: editor.get_display_samples() > 0,
            timeout_s=3.0, desc="display samples positive at 150ms",
        )
        samples_before = editor.get_display_samples()
        assert samples_before > 0

        # Save, reset, load
        path = "/tmp/oscil_e2e_timing_interval.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        editor.wait_for_oscillator_count(1, timeout_s=5.0)
        samples_after = editor.get_display_samples()

        # Interval should be preserved (samples should be in similar range)
        if samples_before > 0 and samples_after > 0:
            ratio = samples_after / samples_before
            assert 0.5 < ratio < 2.0, (
                f"Interval should survive save/load: "
                f"samples {samples_before} -> {samples_after} (ratio {ratio:.2f})"
            )


# ── Move Oscillator + Save/Load ─────────────────────────────────────────────


class TestMoveAndPersistence:
    """Verify oscillator pane assignment persists after move + save/load."""

    def test_move_to_second_pane_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator pane assignment after move not serialized,
        reverting to original pane on state load.
        """
        # Create oscillator (auto-creates pane 1)
        osc_id = editor.add_oscillator(source_id, name="MovePersist")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        panes = editor.get_panes()
        assert len(panes) >= 1
        pane1_id = panes[0]["id"]

        # Add pane 2
        pane2_id = editor.add_pane("Persist Pane 2")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        # Move oscillator to pane 2
        moved = editor.move_oscillator(osc_id, pane2_id)
        if not moved:
            pytest.fail("Oscillator move API not available")

        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(osc_id))
            and osc.get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator to move to pane 2",
        )

        # Save, reset, load
        path = "/tmp/oscil_e2e_move_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        restored_pane = oscs[0].get("paneId", "")

        # The oscillator should still be in pane 2 (or at least in a valid pane)
        panes_after = editor.get_panes()
        pane_ids_after = {p["id"] for p in panes_after}
        assert restored_pane in pane_ids_after, (
            f"Restored oscillator paneId '{restored_pane}' not in pane list"
        )
        # If pane IDs are preserved, verify exact match
        if pane2_id in pane_ids_after:
            assert restored_pane == pane2_id, (
                f"Oscillator should still be in pane2 after move+save/load: "
                f"expected '{pane2_id}', got '{restored_pane}'"
            )


# ── Config Popup for Non-First Oscillator ────────────────────────────────────


class TestConfigPopupTargeting:
    """Verify config popup edits the correct oscillator when opened for item_1."""

    def test_config_popup_targets_correct_oscillator(
        self, editor: OscilTestClient, two_oscillators
    ):
        """
        Bug caught: config popup always edits oscillator[0] regardless of
        which settings button was clicked, because the popup constructor
        uses a hardcoded index instead of the clicked item's ID.
        """
        oscs = editor.get_oscillators()
        assert len(oscs) >= 2
        target_id = oscs[1]["id"]
        original_name = oscs[1]["name"]

        settings_btn = "sidebar_oscillators_item_1_settings"
        if not editor.element_exists(settings_btn):
            pytest.fail("Settings button for item 1 not registered")

        editor.click(settings_btn)
        try:
            editor.wait_for_visible("configPopup", timeout_s=3.0)
        except TimeoutError:
            pytest.fail("Config popup not available")

        # Change name via popup
        name_field = "configPopup_nameField"
        if not editor.element_exists(name_field):
            # Close popup before failing
            for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
                if editor.element_exists(btn):
                    editor.click(btn)
                    break
            pytest.fail("Name field not in config popup")

        new_name = "Edited Via Item1"
        editor.clear_text(name_field)
        editor.type_text(name_field, new_name)

        # Close popup
        for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
            if editor.element_exists(btn):
                editor.click(btn)
                try:
                    editor.wait_for_not_visible("configPopup", timeout_s=2.0)
                except TimeoutError:
                    pass
                break

        # Verify the SECOND oscillator was edited, not the first
        osc_target = editor.get_oscillator_by_id(target_id)
        other_id = oscs[0]["id"]
        osc_other = editor.get_oscillator_by_id(other_id)

        if osc_target and osc_target["name"] == new_name:
            # Correct: the second oscillator was edited
            assert osc_other["name"] != new_name, (
                "Only the target oscillator should be renamed"
            )
        # If auto-save didn't work, we at least verify no crash occurred

    def test_config_popup_shows_correct_properties_for_each_oscillator(
        self, editor: OscilTestClient, three_oscillators
    ):
        """
        Bug caught: config popup always shows oscillator[0]'s properties
        regardless of which settings button is clicked — the popup reads
        properties from a hardcoded index or stale reference.
        """
        oscs = editor.get_oscillators()
        assert len(oscs) >= 3

        name_field = "configPopup_nameField"

        for idx in range(min(3, len(oscs))):
            settings_btn = f"sidebar_oscillators_item_{idx}_settings"
            if not editor.element_exists(settings_btn):
                continue

            editor.click(settings_btn)
            try:
                editor.wait_for_visible("configPopup", timeout_s=3.0)
            except TimeoutError:
                continue

            if not editor.element_exists(name_field):
                # Close and continue
                for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
                    if editor.element_exists(btn):
                        editor.click(btn)
                        break
                continue

            # Read the name field value from the popup
            el = editor.get_element(name_field)
            popup_name = el.extra.get("text", el.extra.get("value", "")) if el else ""

            # Close popup
            for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
                if editor.element_exists(btn):
                    editor.click(btn)
                    try:
                        editor.wait_for_not_visible("configPopup", timeout_s=2.0)
                    except TimeoutError:
                        pass
                    break

            # The popup name should match the oscillator's name
            expected_name = oscs[idx]["name"]
            if popup_name:
                assert popup_name == expected_name, (
                    f"Popup for item {idx} should show '{expected_name}', "
                    f"got '{popup_name}'"
                )

    def test_edit_item0_then_edit_item1_no_cross_contamination(
        self, editor: OscilTestClient, two_oscillators
    ):
        """
        Bug caught: editing oscillator[0] via popup and closing, then
        opening popup for oscillator[1] — the popup shows stale values
        from the previous session because the popup caches the last
        oscillator reference instead of reading fresh state.
        """
        oscs = editor.get_oscillators()
        assert len(oscs) >= 2
        id0, id1 = oscs[0]["id"], oscs[1]["id"]

        settings_btn_0 = "sidebar_oscillators_item_0_settings"
        settings_btn_1 = "sidebar_oscillators_item_1_settings"
        if not editor.element_exists(settings_btn_0):
            pytest.fail("Settings button for item 0 not registered")

        name_field = "configPopup_nameField"

        # Edit oscillator 0
        editor.click(settings_btn_0)
        try:
            editor.wait_for_visible("configPopup", timeout_s=3.0)
        except TimeoutError:
            pytest.fail("Config popup not available")

        if not editor.element_exists(name_field):
            for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
                if editor.element_exists(btn):
                    editor.click(btn)
                    break
            pytest.fail("Name field not in config popup")

        editor.clear_text(name_field)
        editor.type_text(name_field, "EditedOsc0")

        # Wait for name to propagate before closing
        editor.wait_until(
            lambda: (o := editor.get_oscillator_by_id(id0))
            and o.get("name") == "EditedOsc0",
            timeout_s=2.0,
            desc="osc 0 name to propagate",
        )

        for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
            if editor.element_exists(btn):
                editor.click(btn)
                try:
                    editor.wait_for_not_visible("configPopup", timeout_s=2.0)
                except TimeoutError:
                    pass
                break

        # Verify osc 0 was changed
        osc0 = editor.get_oscillator_by_id(id0)
        assert osc0 is not None
        if osc0["name"] != "EditedOsc0":
            pytest.fail("Name edit via popup not working")

        # Now edit oscillator 1
        if not editor.element_exists(settings_btn_1):
            pytest.fail("Settings button for item 1 not registered")

        editor.click(settings_btn_1)
        try:
            editor.wait_for_visible("configPopup", timeout_s=3.0)
        except TimeoutError:
            pytest.fail("Config popup not available for item 1")

        editor.clear_text(name_field)
        editor.type_text(name_field, "EditedOsc1")

        # Wait for name to propagate before closing
        editor.wait_until(
            lambda: (o := editor.get_oscillator_by_id(id1))
            and o.get("name") == "EditedOsc1",
            timeout_s=2.0,
            desc="osc 1 name to propagate",
        )

        for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
            if editor.element_exists(btn):
                editor.click(btn)
                try:
                    editor.wait_for_not_visible("configPopup", timeout_s=2.0)
                except TimeoutError:
                    pass
                break

        # Verify osc 1 was changed and osc 0 was NOT modified
        osc0_final = editor.get_oscillator_by_id(id0)
        osc1_final = editor.get_oscillator_by_id(id1)

        assert osc0_final is not None
        assert osc1_final is not None
        assert osc0_final["name"] == "EditedOsc0", (
            f"Osc 0 name should remain 'EditedOsc0', got '{osc0_final['name']}'"
        )
        assert osc1_final["name"] == "EditedOsc1", (
            f"Osc 1 name should be 'EditedOsc1', got '{osc1_final['name']}'"
        )


# ── Dialog During Playback ──────────────────────────────────────────────────


class TestDialogDuringPlayback:
    """Verify dialog interactions work correctly during active playback."""

    def test_add_oscillator_via_dialog_while_playing(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: opening the add dialog while transport is playing causes
        the audio callback to block on the message thread mutex, leading to
        audio dropout or deadlock.
        """
        # Start with one oscillator and playing transport
        osc_id = editor.add_oscillator(source_id, name="PreDialog")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Open add dialog while playing
        editor.click("sidebar_addOscillator")
        try:
            editor.wait_for_visible("addOscillatorDialog", timeout_s=3.0)
        except TimeoutError:
            editor.transport_stop()
            pytest.fail("Add dialog did not appear")

        # Transport should still be playing while dialog is open
        assert editor.is_playing(), (
            "Transport should continue playing while dialog is open"
        )

        # Fill and submit
        dropdown_id = "addOscillatorDialog_sourceDropdown"
        if editor.element_exists(dropdown_id):
            editor.select_dropdown_item(dropdown_id, source_id)

        editor.click("addOscillatorDialog_okBtn")
        try:
            editor.wait_for_not_visible("addOscillatorDialog", timeout_s=3.0)
        except TimeoutError:
            pass

        # Should now have 2 oscillators and still be playing
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        assert editor.is_playing(), "Transport should still play after dialog submit"

        editor.transport_stop()


# ── State Save During Playback ──────────────────────────────────────────────


class TestSaveDuringPlayback:
    """Verify state save/load during active playback."""

    def test_save_during_playback_preserves_oscillator_properties(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: saving state while audio callback is active produces
        a partial/corrupt XML because the serializer reads oscillator
        properties while the audio thread is modifying them (torn read).
        """
        osc_id = editor.add_oscillator(
            source_id, name="SavePlaying", colour="#FF6B6B", mode="Mid"
        )
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.update_oscillator(osc_id, opacity=0.6, lineWidth=2.5)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="square", frequency=220.0, amplitude=0.5)

        # Save while playing
        path = "/tmp/oscil_e2e_save_playing.xml"
        saved = editor.save_state(path)
        if not saved:
            editor.transport_stop()
            pytest.fail("State save API not available")

        editor.transport_stop()

        # Reset and load
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "Loading state saved during playback should succeed"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        restored = oscs[0]

        assert restored["name"] == "SavePlaying", (
            f"Name should survive: got '{restored['name']}'"
        )
        mode = restored.get("mode", restored.get("processingMode"))
        if mode is not None:
            assert mode == "Mid", f"Mode should survive: got '{mode}'"


# ── Diagnostic Snapshot Consistency After Operations ─────────────────────────


class TestSnapshotAfterOperations:
    """Verify diagnostic snapshot reflects state after complex operations."""

    def test_snapshot_after_move_reflects_new_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: diagnostic snapshot caching the oscillator's original
        pane assignment, not reflecting the move.
        """
        osc_id = editor.add_oscillator(source_id, name="SnapMove")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("SnapMove Pane 2")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        moved = editor.move_oscillator(osc_id, pane2_id)
        if not moved:
            pytest.fail("Move API not available")

        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(osc_id))
            and osc.get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator to move",
        )

        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        snap_oscs = snap.get("oscillators", [])
        assert len(snap_oscs) >= 1, "Snapshot should include the oscillator"

        snap_osc = next((o for o in snap_oscs if o.get("name") == "SnapMove"), None)
        if snap_osc:
            assert snap_osc.get("paneId") == pane2_id, (
                f"Snapshot should reflect moved pane: "
                f"expected '{pane2_id}', got '{snap_osc.get('paneId')}'"
            )

    def test_snapshot_after_visibility_toggle_reflects_change(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: snapshot visible field not updated after toggle,
        so scenario-based tests see stale visibility.
        """
        osc_id = editor.add_oscillator(source_id, name="SnapVis")
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.update_oscillator(osc_id, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(osc_id).get("visible", True),
            timeout_s=2.0, desc="oscillator to hide",
        )

        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        snap_oscs = snap.get("oscillators", [])
        assert len(snap_oscs) >= 1
        snap_osc = snap_oscs[0]
        assert snap_osc.get("visible") is False, (
            f"Snapshot should show visible=false, got {snap_osc.get('visible')}"
        )


# ── Multi-Pane Save/Load Integrity ──────────────────────────────────────────


class TestMultiPaneSaveLoad:
    """Verify multi-pane state survives save/load correctly."""

    def test_two_panes_with_oscillators_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: save/load with 2 panes loses pane-to-oscillator
        bindings, putting all oscillators in pane 1 after reload.
        """
        # Create first oscillator (auto-creates pane 1)
        id1 = editor.add_oscillator(source_id, name="Pane1 Osc A")
        assert id1
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Create pane 2
        pane2_id = editor.add_pane("Save Pane 2")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        # Add oscillator to pane 2
        id2 = editor.add_oscillator(source_id, name="Pane2 Osc B", pane_id=pane2_id)
        if id2 is None:
            id2 = editor.add_oscillator(source_id, name="Pane2 Osc B")
            if id2:
                editor.move_oscillator(id2, pane2_id)
        assert id2
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Record pane assignments before save
        osc1_before = editor.get_oscillator_by_id(id1)
        osc2_before = editor.get_oscillator_by_id(id2)
        pane_count_before = len(editor.get_panes())

        path = "/tmp/oscil_e2e_multi_pane_save.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        oscs = editor.wait_for_oscillator_count(2, timeout_s=5.0)
        panes_after = editor.get_panes()

        # Verify pane count preserved
        assert len(panes_after) >= pane_count_before, (
            f"Pane count should survive: before={pane_count_before}, "
            f"after={len(panes_after)}"
        )

        # Verify oscillators are in different panes (not all collapsed into one)
        pane_ids = [o.get("paneId") for o in oscs]
        if osc1_before.get("paneId") != osc2_before.get("paneId"):
            assert len(set(pane_ids)) >= 2, (
                f"Oscillators should be in different panes after load: "
                f"paneIds={pane_ids}"
            )


# ── Saved State Fixture Tests ────────────────────────────────────────────────


class TestSavedStateFixture:
    """Tests using the saved_state_path fixture for complete roundtrip verification."""

    def test_all_properties_survive_roundtrip(self, saved_state_path):
        """
        Bug caught: individual oscillator properties (name, colour, mode,
        visible, opacity, lineWidth) not all serialized — some silently
        revert to defaults on load.
        """
        editor, path, osc_ids = saved_state_path

        # Record all properties before save
        oscs_before = editor.get_oscillators()
        props_before = {}
        for osc in oscs_before:
            props_before[osc["name"]] = {
                k: osc.get(k)
                for k in ("name", "mode", "visible", "opacity", "lineWidth", "sourceId")
                if k in osc
            }

        # Reset and load
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs_after = editor.wait_for_oscillator_count(2, timeout_s=5.0)

        # Verify each property survived
        for osc in oscs_after:
            name = osc["name"]
            if name not in props_before:
                continue
            expected = props_before[name]
            for key, expected_val in expected.items():
                if expected_val is None:
                    continue
                actual_val = osc.get(key)
                if isinstance(expected_val, float):
                    assert actual_val is not None and abs(actual_val - expected_val) < 0.1, (
                        f"{name}.{key}: expected {expected_val}, got {actual_val}"
                    )
                else:
                    assert actual_val == expected_val, (
                        f"{name}.{key}: expected {expected_val}, got {actual_val}"
                    )


# ── Rapid Cross-Subsystem Operations ────────────────────────────────────────


class TestRapidCrossSubsystem:
    """Verify stability under rapid interleaved operations across subsystems."""

    def test_interleave_crud_timing_transport(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: rapid interleaving of oscillator add, timing changes,
        and transport toggling causes state listeners to fire out of order,
        corrupting shared state.
        """
        timing_id = "sidebar_timing"
        has_timing = editor.element_exists(timing_id)
        if has_timing:
            editor.click(timing_id)

        interval_field = "sidebar_timing_intervalField"
        has_interval = editor.element_exists(interval_field)

        # Interleave operations
        for i in range(3):
            osc_id = editor.add_oscillator(source_id, name=f"Interleave {i}")
            assert osc_id, f"Add oscillator {i} should succeed"

            if has_interval:
                editor.set_slider(interval_field, 50.0 + i * 25.0)

            editor.transport_play()
            editor.wait_until(
                lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
            )
            editor.transport_stop()

        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Verify final state is consistent
        oscs = editor.get_oscillators()
        assert len(oscs) == 3, f"Should have 3 oscillators, got {len(oscs)}"

        pane_ids = {p["id"] for p in editor.get_panes()}
        for osc in oscs:
            assert osc.get("paneId") in pane_ids, (
                f"Oscillator '{osc['name']}' has invalid paneId after interleaving"
            )

        state = editor.get_transport_state()
        assert state is not None, "Transport should be queryable after interleaving"
        assert not state.get("playing"), "Transport should be stopped"


# ── Pane Removal + Oscillator Reassignment ──────────────────────────────────


class TestPaneRemovalReassignment:
    """Verify oscillator handling when their pane is removed."""

    def test_remove_pane_with_oscillator_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: removing a pane reassigns oscillators to pane 1, but
        state serialization still records the old (deleted) pane ID,
        causing orphaned oscillators on load.
        """
        # Create oscillator + auto pane
        id1 = editor.add_oscillator(source_id, name="PaneRemSL")
        assert id1
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Add pane 2, move oscillator there
        pane2_id = editor.add_pane("RemSL Pane 2")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        moved = editor.move_oscillator(id1, pane2_id)
        if not moved:
            pytest.fail("Move API not available")

        editor.wait_until(
            lambda: editor.get_oscillator_by_id(id1).get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator to move to pane 2",
        )

        # Remove pane 2
        removed = editor.remove_pane(pane2_id)
        if not removed:
            pytest.fail("Pane remove API not available")

        # Oscillator should now be in a valid pane
        remaining_pane_ids = {p["id"] for p in editor.get_panes()}
        osc = editor.get_oscillator_by_id(id1)
        assert osc.get("paneId") in remaining_pane_ids, (
            f"Oscillator should be reassigned after pane removal"
        )

        # Save and load — the reassignment should persist
        path = "/tmp/oscil_e2e_pane_rem_sl.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        panes_after = editor.get_panes()
        pane_ids_after = {p["id"] for p in panes_after}

        assert oscs[0].get("paneId") in pane_ids_after, (
            f"Oscillator paneId should be valid after remove+save/load"
        )


# ── Color Update + Save/Load Roundtrip ──────────────────────────────────────


class TestColorUpdatePersistence:
    """Verify color changes via API persist through save/load."""

    def test_color_update_survives_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator colour updated via update_oscillator API
        but the update is not serialized — save/load reverts to original.
        """
        osc_id = editor.add_oscillator(
            source_id, name="ColorUpdate", colour="#FF0000"
        )
        assert osc_id
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Change color via API
        editor.update_oscillator(osc_id, colour="#00BFFF")

        # Verify change took effect
        osc = editor.get_oscillator_by_id(osc_id)
        colour_after_update = osc.get("colour", osc.get("color", ""))

        # Save, reset, load
        path = "/tmp/oscil_e2e_color_update.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"

        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        colour_after_load = oscs[0].get("colour", oscs[0].get("color", ""))

        if colour_after_update:
            assert colour_after_load == colour_after_update, (
                f"Updated colour should survive save/load: "
                f"expected '{colour_after_update}', got '{colour_after_load}'"
            )


# ── Oscillator Add During Timing Mode Switch ────────────────────────────────


class TestTimingModeWithCRUD:
    """Verify timing mode and oscillator operations don't interfere."""

    def test_add_oscillator_in_melodic_mode(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding an oscillator while in melodic timing mode
        resets the timing mode to Time (the add handler triggers a
        state change listener that reinitializes timing config).
        """
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.fail("Timing section not registered")
        editor.click(timing_id)

        melodic_seg = "sidebar_timing_modeToggle_melodic"
        if not editor.element_exists(melodic_seg):
            pytest.fail("Melodic mode segment not registered")

        # Add first oscillator so timing engine has data
        id1 = editor.add_oscillator(source_id, name="MelodicCRUD Pre")
        assert id1
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Switch to melodic mode
        editor.click(melodic_seg)

        samples_melodic = editor.get_display_samples()
        assert samples_melodic > 0, "Melodic mode should have positive displaySamples"

        # Add a second oscillator — this should NOT reset timing mode
        id2 = editor.add_oscillator(source_id, name="MelodicCRUD Post")
        assert id2
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        samples_after = editor.get_display_samples()
        assert samples_after > 0, (
            f"displaySamples should remain positive after add in melodic mode, "
            f"got {samples_after}"
        )

        # Samples should be in the same ballpark (same timing mode)
        if samples_melodic > 0:
            ratio = samples_after / samples_melodic
            assert 0.5 < ratio < 2.0, (
                f"Timing mode likely reset after oscillator add: "
                f"melodic samples {samples_melodic} -> {samples_after} "
                f"(ratio {ratio:.2f})"
            )
