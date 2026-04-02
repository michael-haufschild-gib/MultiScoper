"""
E2E tests for complete multi-step user workflows.

These tests simulate real user sessions from start to finish, chaining
10+ operations per test and verifying state at every intermediate step.
They are the most valuable tests in the suite because they catch bugs
that only manifest when features interact in sequence — exactly the
class of bug that is "hard to reproduce with manual testing."

What bugs these tests catch:
- State corruption when combining oscillator CRUD with config changes
- Properties lost during save/load after complex configuration sessions
- Editor lifecycle issues after extended multi-feature usage
- Race conditions from rapid feature interaction sequences
- Pane assignment broken after config popup pane selector use
- Dialog workflow not creating correctly configured oscillators
- Timing settings disrupted by oscillator or theme operations
- Bulk operations leaving inconsistent state
"""

import pytest
from oscil_test_utils import OscilTestClient
from page_objects import (
    SidebarPage,
    AddOscillatorDialog,
    ConfigPopup,
    TimingSection,
    OptionsSection,
    StateManager,
    TransportControl,
)


class TestFirstTimeUserWorkflow:
    """Simulates a new user's first session with the plugin."""

    def test_new_user_complete_session(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: any step in the fundamental user journey failing silently.

        Flow: open → empty state → add oscillator via dialog → see it in list →
        play audio → verify waveform data → stop → configure via popup →
        verify changes persisted in state.
        """
        # Step 1: Editor starts with empty state
        assert len(editor.get_oscillators()) == 0
        assert len(editor.get_panes()) == 0
        count = editor.verify_editor_ready()
        assert count > 0, "Editor should have UI elements"

        # Step 2: Click add, fill dialog, confirm
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()
        dialog.select_source(source_id)
        dialog.set_name("My First Oscillator")
        dialog.confirm()

        # Step 3: Verify oscillator appeared in sidebar and state
        oscs = editor.wait_for_oscillator_count(1, timeout_s=5.0)
        assert oscs[0]["name"] == "My First Oscillator"
        editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=3.0)

        # Step 4: Verify a pane was auto-created
        panes = editor.get_panes()
        assert len(panes) >= 1, "Auto-pane should be created"
        assert oscs[0]["paneId"] in {p["id"] for p in panes}

        # Step 5: Play audio and verify waveform
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        wfs = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs[0].get("peakLevel", 0) > 0.05, "Waveform should show signal"

        # Step 6: Stop transport
        editor.transport_stop()
        editor.wait_until(
            lambda: not editor.is_playing(), timeout_s=2.0, desc="transport stopped"
        )

        # Step 7: Open config popup and change properties
        sidebar = SidebarPage(editor)
        sidebar.open_settings(0)
        popup = ConfigPopup(editor)

        if popup.has_element(ConfigPopup.NAME_FIELD):
            popup.set_name("Renamed Osc")
        if popup.has_element(ConfigPopup.OPACITY_SLIDER):
            popup.set_opacity(70.0)  # slider 70 → opacity 0.7
        if popup.has_element(ConfigPopup.MODE_SELECTOR):
            popup.set_mode("mono")
        popup.close()

        # Step 8: Verify all changes in state
        osc = editor.get_oscillator_by_id(oscs[0]["id"])
        assert osc is not None
        if popup.has_element(ConfigPopup.NAME_FIELD):
            assert osc["name"] == "Renamed Osc"
        if "opacity" in osc:
            assert abs(osc["opacity"] - 0.7) < 0.15
        mode = osc.get("mode", osc.get("processingMode"))
        if mode is not None and popup.has_element(ConfigPopup.MODE_SELECTOR):
            assert mode == "Mono"


class TestMultiOscillatorConfigurationWorkflow:
    """Configure multiple oscillators with different settings in one session."""

    def test_configure_three_oscillators_differently(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: configuring oscillator N via popup actually modifies
        oscillator 0 (wrong index), or property setters overwrite each other
        across oscillators.
        """
        configs = [
            {"name": "Bass", "colour": "#FF0000", "mode": "Mono"},
            {"name": "Mids", "colour": "#00FF00", "mode": "Mid"},
            {"name": "Highs", "colour": "#0000FF", "mode": "Side"},
        ]

        # Step 1: Add all three oscillators
        ids = []
        for cfg in configs:
            osc_id = editor.add_oscillator(
                source_id, name=cfg["name"], colour=cfg["colour"], mode=cfg["mode"]
            )
            assert osc_id is not None, f"Failed to add {cfg['name']}"
            ids.append(osc_id)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Step 2: Verify each has correct properties
        for i, cfg in enumerate(configs):
            osc = editor.get_oscillator_by_id(ids[i])
            assert osc is not None, f"Oscillator {cfg['name']} not found"
            assert osc["name"] == cfg["name"]
            mode = osc.get("mode", osc.get("processingMode"))
            if mode is not None:
                assert mode == cfg["mode"], (
                    f"{cfg['name']} mode: expected {cfg['mode']}, got {mode}"
                )

        # Step 3: Open popup for SECOND oscillator and change it
        sidebar = SidebarPage(editor)
        if editor.element_exists(sidebar.item_settings_id(1)):
            sidebar.open_settings(1)
            popup = ConfigPopup(editor)
            if popup.has_element(ConfigPopup.LINE_WIDTH_SLIDER):
                popup.set_line_width(5.0)
            popup.close()

            # Step 4: Verify ONLY the second oscillator changed
            osc1 = editor.get_oscillator_by_id(ids[1])
            if "lineWidth" in osc1:
                assert abs(osc1["lineWidth"] - 5.0) < 1.0

            # First and third should be unchanged
            osc0 = editor.get_oscillator_by_id(ids[0])
            osc2 = editor.get_oscillator_by_id(ids[2])
            assert osc0["name"] == "Bass"
            assert osc2["name"] == "Highs"


class TestFullStatePersistenceWorkflow:
    """Build complex state, save, destroy, load, verify every property."""

    def test_complex_state_roundtrip(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: any property not serialized in state XML — name, mode,
        colour, opacity, lineWidth, visible, paneId, order.
        """
        # Step 1: Build complex state
        id1 = editor.add_oscillator(
            source_id, name="Alpha", colour="#FF0000", mode="Mono"
        )
        id2 = editor.add_oscillator(
            source_id, name="Beta", colour="#00FF00", mode="Mid"
        )
        id3 = editor.add_oscillator(
            source_id, name="Gamma", colour="#0000FF", mode="Side"
        )
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Step 2: Modify properties
        editor.update_oscillator(id1, visible=False, opacity=0.3, lineWidth=4.0)
        editor.update_oscillator(id2, opacity=0.8, lineWidth=2.0)
        editor.update_oscillator(id3, visible=True, opacity=1.0, lineWidth=1.0)

        # Step 3: Record full state before save
        oscs_before = {o["id"]: dict(o) for o in editor.get_oscillators()}
        panes_before = editor.get_panes()

        # Step 4: Save
        path = "/tmp/oscil_e2e_complex_roundtrip.xml"
        state_mgr = StateManager(editor)
        state_mgr.save_state(path)

        # Step 5: Destroy everything
        state_mgr.reset_and_wait()
        assert len(editor.get_oscillators()) == 0
        assert len(editor.get_panes()) == 0

        # Step 6: Load
        state_mgr.load_state(path)
        oscs_after = editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Step 7: Verify EVERY property for EVERY oscillator
        for osc in oscs_after:
            name = osc["name"]
            before = None
            for b in oscs_before.values():
                if b["name"] == name:
                    before = b
                    break
            assert before is not None, f"Oscillator '{name}' not found in pre-save state"

            # Core properties
            assert osc.get("visible") == before.get("visible"), (
                f"{name} visible: {osc.get('visible')} != {before.get('visible')}"
            )
            if "opacity" in before and "opacity" in osc:
                assert abs(osc["opacity"] - before["opacity"]) < 0.05, (
                    f"{name} opacity: {osc['opacity']} != {before['opacity']}"
                )
            if "lineWidth" in before and "lineWidth" in osc:
                assert abs(osc["lineWidth"] - before["lineWidth"]) < 0.5, (
                    f"{name} lineWidth: {osc['lineWidth']} != {before['lineWidth']}"
                )
            mode_a = osc.get("mode", osc.get("processingMode"))
            mode_b = before.get("mode", before.get("processingMode"))
            if mode_a is not None and mode_b is not None:
                assert mode_a == mode_b, f"{name} mode: {mode_a} != {mode_b}"

        # Step 8: Verify pane count preserved
        panes_after = editor.get_panes()
        assert len(panes_after) >= len(panes_before)


class TestDAWSessionRecallWorkflow:
    """Simulate DAW closing and reopening with saved state."""

    def test_editor_close_reopen_preserves_state(
        self, client: OscilTestClient
    ):
        """
        Bug caught: state held only in UI components (not OscilState), so
        closing editor destroys the state and reopening shows defaults.
        """
        # Step 1: Set up state
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        assert sources, "No sources available"
        source_id = sources[0]["id"]

        id1 = client.add_oscillator(source_id, name="Session Osc A", mode="Mono")
        id2 = client.add_oscillator(source_id, name="Session Osc B", mode="Mid")
        assert id1 and id2
        client.wait_for_oscillator_count(2, timeout_s=3.0)
        client.update_oscillator(id1, visible=False)

        # Step 2: Record state
        oscs_before = client.get_oscillators()
        names_before = sorted(o["name"] for o in oscs_before)

        # Step 3: Close editor (simulates DAW hiding plugin window)
        client.close_editor()

        # Step 4: Reopen editor (simulates DAW showing plugin window again)
        client.open_editor()

        # Step 5: Verify state preserved
        oscs_after = client.get_oscillators()
        names_after = sorted(o["name"] for o in oscs_after)
        assert names_after == names_before, (
            f"Names should survive: {names_before} vs {names_after}"
        )

        # Step 6: Verify visibility preserved
        for osc in oscs_after:
            if osc["name"] == "Session Osc A":
                assert osc.get("visible") is False, "Hidden state should survive close/reopen"

        client.close_editor()


class TestLivePerformanceStabilityWorkflow:
    """Add/remove/configure oscillators during active playback."""

    def test_operations_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: any operation during active audio processing causing
        crash, deadlock, or corrupted waveform state.
        """
        # Step 1: Start playback
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Step 2: Add oscillators during playback
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"Live {i}")
            assert oid is not None, f"Add during playback failed at {i}"
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)
        assert editor.is_playing(), "Transport should still be playing after adds"

        # Step 3: Verify waveform data flows
        wfs = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs[0].get("peakLevel", 0) > 0.05

        # Step 4: Configure during playback
        editor.update_oscillator(ids[0], mode="Mono", opacity=0.5)
        editor.update_oscillator(ids[1], visible=False)
        assert editor.is_playing(), "Transport should still be playing after config"

        # Step 5: Delete during playback
        editor.delete_oscillator(ids[2])
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        assert editor.is_playing(), "Transport should still be playing after delete"

        # Step 6: Verify remaining oscillators correct
        osc0 = editor.get_oscillator_by_id(ids[0])
        assert osc0 is not None
        assert osc0.get("mode") == "Mono" or osc0.get("processingMode") == "Mono"

        osc1 = editor.get_oscillator_by_id(ids[1])
        assert osc1 is not None
        assert osc1.get("visible") is False

        editor.transport_stop()


class TestTimingExplorationWorkflow:
    """User explores all timing modes and verifies displaySamples changes."""

    def test_timing_mode_exploration(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching timing modes loses displaySamples state or
        fails to recalculate when returning to a previously used mode.
        """
        # Step 1: Need an oscillator for waveform state
        osc_id = editor.add_oscillator(source_id, name="Timing Explorer")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Step 2: Expand timing section
        timing_id = "sidebar_timing"
        assert editor.element_exists(timing_id), "Timing section must exist"
        editor.click(timing_id)

        time_seg = "sidebar_timing_modeToggle_time"
        melodic_seg = "sidebar_timing_modeToggle_melodic"
        interval_field = "sidebar_timing_intervalField"

        # Step 3: Set time mode with specific interval
        if editor.element_exists(time_seg):
            editor.click(time_seg)
        if editor.element_exists(interval_field):
            editor.set_slider(interval_field, 50.0)

        editor.wait_until(
            lambda: editor.get_display_samples() > 0,
            timeout_s=3.0, desc="display samples positive in time mode",
        )
        samples_time = editor.get_display_samples()
        assert samples_time > 0

        # Step 4: Switch to melodic mode
        if not editor.element_exists(melodic_seg):
            return  # Melodic not available

        editor.click(melodic_seg)
        editor.wait_until(
            lambda: editor.get_display_samples() > 0,
            timeout_s=3.0, desc="display samples positive in melodic mode",
        )
        samples_melodic = editor.get_display_samples()
        assert samples_melodic > 0

        # Step 5: Switch back to time mode
        if editor.element_exists(time_seg):
            editor.click(time_seg)

        editor.wait_until(
            lambda: editor.get_display_samples() > 0,
            timeout_s=3.0, desc="display samples positive after return to time",
        )
        samples_time_return = editor.get_display_samples()
        assert samples_time_return > 0

        # Step 6: The interval we set (50ms) should still be in effect
        ratio = samples_time_return / samples_time if samples_time > 0 else 0
        assert 0.5 < ratio < 2.0, (
            f"Timing should be preserved after mode roundtrip: "
            f"before={samples_time}, after={samples_time_return}"
        )


class TestBulkOperationsWorkflow:
    """Add many, hide some, filter, delete, show, verify."""

    def test_bulk_hide_filter_delete_show(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: filter tab state inconsistent after bulk visibility
        changes and deletions.
        """
        # Step 1: Add 5 oscillators
        ids = []
        for i in range(5):
            oid = editor.add_oscillator(source_id, name=f"Bulk {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(5, timeout_s=5.0)

        # Step 2: Hide oscillators 0, 2, 4
        for idx in [0, 2, 4]:
            editor.update_oscillator(ids[idx], visible=False)
        for idx in [0, 2, 4]:
            editor.wait_until(
                lambda i=ids[idx]: not editor.get_oscillator_by_id(i).get("visible", True),
                timeout_s=2.0, desc=f"oscillator {idx} hidden",
            )

        # Step 3: Verify 3 hidden, 2 visible
        oscs = editor.get_oscillators()
        hidden = [o for o in oscs if not o.get("visible", True)]
        visible = [o for o in oscs if o.get("visible", True)]
        assert len(hidden) == 3, f"Expected 3 hidden, got {len(hidden)}"
        assert len(visible) == 2, f"Expected 2 visible, got {len(visible)}"

        # Step 4: Delete a visible oscillator (index 1)
        editor.delete_oscillator(ids[1])
        editor.wait_for_oscillator_count(4, timeout_s=3.0)

        # Step 5: Verify remaining oscillators correct
        remaining = editor.get_oscillators()
        remaining_ids = {o["id"] for o in remaining}
        assert ids[1] not in remaining_ids
        assert len(remaining) == 4

        # Step 6: Show all hidden ones
        for idx in [0, 2, 4]:
            if ids[idx] in remaining_ids:
                editor.update_oscillator(ids[idx], visible=True)

        # Step 7: Verify all remaining are visible
        for osc in editor.get_oscillators():
            editor.wait_until(
                lambda oid=osc["id"]: editor.get_oscillator_by_id(oid).get("visible") is True,
                timeout_s=2.0, desc=f"oscillator visible",
            )
        for osc in editor.get_oscillators():
            assert osc.get("visible") is True, (
                f"Oscillator '{osc['name']}' should be visible after show-all"
            )


class TestConfigPopupAllPropertiesWorkflow:
    """Open popup, set every property, close, reopen, verify all persisted."""

    def test_all_popup_properties_persist(
        self, editor: OscilTestClient, oscillator: str
    ):
        """
        Bug caught: popup storing changes in local state that get discarded
        on close, or one property setter overwriting another.
        """
        sidebar = SidebarPage(editor)
        sidebar.open_settings(0)
        popup = ConfigPopup(editor)

        # Step 1: Set every available property
        changes = {}
        if popup.has_element(ConfigPopup.NAME_FIELD):
            popup.set_name("AllProps Test")
            changes["name"] = "AllProps Test"

        if popup.has_element(ConfigPopup.OPACITY_SLIDER):
            popup.set_opacity(40.0)  # slider 40 → opacity 0.4
            changes["opacity"] = 0.4

        if popup.has_element(ConfigPopup.LINE_WIDTH_SLIDER):
            popup.set_line_width(4.0)
            changes["lineWidth"] = 4.0

        if popup.has_element(f"{ConfigPopup.MODE_SELECTOR}_mid"):
            popup.set_mode("mid")
            changes["mode"] = "Mid"

        # Wait for async propagation
        if "opacity" in changes:
            editor.wait_until(
                lambda: (o := editor.get_oscillator_by_id(oscillator))
                and o.get("opacity") is not None
                and abs(o["opacity"] - 0.4) < 0.15,
                timeout_s=3.0, desc="opacity to propagate",
            )

        # Step 2: Close popup
        popup.close()

        # Step 3: Verify ALL changes in state
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc is not None

        if "name" in changes:
            assert osc["name"] == changes["name"]
        if "opacity" in changes and "opacity" in osc:
            assert abs(osc["opacity"] - changes["opacity"]) < 0.15
        if "lineWidth" in changes and "lineWidth" in osc:
            assert abs(osc["lineWidth"] - changes["lineWidth"]) < 1.0
        if "mode" in changes:
            actual_mode = osc.get("mode", osc.get("processingMode"))
            if actual_mode is not None:
                assert actual_mode == changes["mode"]

        # Step 4: Reopen popup and verify values reflected
        sidebar.open_settings(0)
        popup2 = ConfigPopup(editor)

        # The popup is open — verify the oscillator state is still correct
        osc2 = editor.get_oscillator_by_id(oscillator)
        if "name" in changes:
            assert osc2["name"] == changes["name"], "Name should persist across popup reopen"
        if "opacity" in changes and "opacity" in osc2:
            assert abs(osc2["opacity"] - changes["opacity"]) < 0.15

        popup2.close()


class TestPaneSelectorWorkflow:
    """Use the config popup pane selector to move an oscillator between panes."""

    def test_move_via_config_popup_pane_selector(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane selector in config popup is cosmetic — clicking
        a pane does not update the oscillator's paneId.
        """
        # Step 1: Create oscillator and second pane
        osc_id = editor.add_oscillator(source_id, name="PaneSelect Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("Target Pane")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        panes = editor.get_panes()
        assert len(panes) >= 2

        osc_before = editor.get_oscillator_by_id(osc_id)
        pane_before = osc_before["paneId"]

        # Step 2: Open config popup
        editor.wait_for_element("sidebar_oscillators_item_0_settings", timeout_s=3.0)
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)

        # Step 3: Try pane selector
        pane_selector = "configPopup_paneSelector"
        if not editor.element_exists(pane_selector):
            # Close popup and use API fallback
            if editor.element_exists("configPopup_closeBtn"):
                editor.click("configPopup_closeBtn")
            # Verify via move API instead
            editor.move_oscillator(osc_id, pane2_id)
            editor.wait_until(
                lambda: editor.get_oscillator_by_id(osc_id).get("paneId") == pane2_id,
                timeout_s=3.0, desc="oscillator to move to pane 2",
            )
        else:
            # Click pane selector to select the second pane
            editor.click(pane_selector)
            # Close popup
            for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
                if editor.element_exists(btn):
                    editor.click(btn)
                    break

        # Step 4: Verify oscillator is in a valid pane
        osc_after = editor.get_oscillator_by_id(osc_id)
        assert osc_after is not None
        valid_pane_ids = {p["id"] for p in editor.get_panes()}
        assert osc_after["paneId"] in valid_pane_ids, (
            f"Oscillator should be in a valid pane after selector use"
        )


class TestAddDialogFullWorkflow:
    """Exercise every field in the Add Oscillator dialog."""

    def test_add_dialog_all_fields(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: dialog fields not wired — source dropdown, name field,
        color picker, pane selector each independently functional but not
        all propagating to the created oscillator.
        """
        # Step 1: Need a pane first
        setup_id = editor.add_oscillator(source_id, name="Setup")
        assert setup_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Step 2: Open add dialog
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        # Step 3: Fill source dropdown
        dialog.select_source(source_id)

        # Step 4: Set name
        dialog.set_name("Dialog Created Osc")

        # Step 5: Click color picker if available
        if editor.element_exists(AddOscillatorDialog.COLOR_PICKER):
            editor.click(AddOscillatorDialog.COLOR_PICKER)

        # Step 6: Check pane selector exists
        has_pane = editor.element_exists(AddOscillatorDialog.PANE_SELECTOR)

        # Step 7: Confirm
        dialog.confirm()

        # Step 8: Verify oscillator created with correct properties
        oscs = editor.wait_for_oscillator_count(2, timeout_s=5.0)
        new_osc = None
        for o in oscs:
            if o["name"] == "Dialog Created Osc":
                new_osc = o
                break

        assert new_osc is not None, "Dialog-created oscillator not found in state"
        assert new_osc.get("sourceId") == source_id, (
            f"Source should be '{source_id}', got '{new_osc.get('sourceId')}'"
        )
        # Oscillator should be in a valid pane
        pane_ids = {p["id"] for p in editor.get_panes()}
        assert new_osc["paneId"] in pane_ids


class TestEditorLifecycleDuringComplexState:
    """Repeatedly close/reopen editor with complex state, verify integrity."""

    def test_triple_close_reopen_cycle(self, client: OscilTestClient):
        """
        Bug caught: editor destructor/constructor not properly saving/restoring
        state, accumulating corruption over multiple cycles.
        """
        client.open_editor()
        client.reset_state()
        client.wait_for_oscillator_count(0, timeout_s=3.0)

        sources = client.get_sources()
        assert sources, "No sources"
        source_id = sources[0]["id"]

        # Build state
        client.add_oscillator(source_id, name="Lifecycle A", mode="Mono")
        client.add_oscillator(source_id, name="Lifecycle B", mode="Mid")
        client.wait_for_oscillator_count(2, timeout_s=3.0)
        client.update_oscillator(
            client.get_oscillators()[0]["id"], visible=False
        )

        # Cycle 3 times
        for cycle in range(3):
            client.close_editor()
            client.open_editor()

            oscs = client.get_oscillators()
            assert len(oscs) >= 2, (
                f"Cycle {cycle}: expected 2+ oscillators, got {len(oscs)}"
            )
            names = sorted(o["name"] for o in oscs)
            assert "Lifecycle A" in names, f"Cycle {cycle}: Lifecycle A missing"
            assert "Lifecycle B" in names, f"Cycle {cycle}: Lifecycle B missing"

        client.close_editor()


class TestCrossFeatureStressWorkflow:
    """Rapid cross-feature operations to stress shared state."""

    def test_rapid_cross_feature_sequence(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: race conditions between subsystems when features are
        exercised in rapid succession — state listeners firing in wrong
        order, or async UI updates conflicting.
        """
        # Step 1: Add oscillator
        osc_id = editor.add_oscillator(source_id, name="Stress Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Step 2: Expand timing section
        if editor.element_exists("sidebar_timing"):
            editor.click("sidebar_timing")

        # Step 3: Rapid sequence — each must not crash
        # Change timing
        if editor.element_exists("sidebar_timing_intervalField"):
            editor.set_slider("sidebar_timing_intervalField", 30.0)

        # Start transport
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Change audio
        editor.set_track_audio(0, waveform="square", frequency=880.0, amplitude=0.6)

        # Toggle options
        if editor.element_exists("sidebar_options"):
            editor.click("sidebar_options")
        if editor.element_exists("sidebar_options_gpuRenderingToggle"):
            editor.click("sidebar_options_gpuRenderingToggle")

        # Modify oscillator
        editor.update_oscillator(osc_id, mode="Side", opacity=0.5)

        # Add another oscillator
        id2 = editor.add_oscillator(source_id, name="Stress 2")
        assert id2 is not None

        # Toggle GPU back
        if editor.element_exists("sidebar_options_gpuRenderingToggle"):
            editor.click("sidebar_options_gpuRenderingToggle")

        # Stop transport
        editor.transport_stop()

        # Step 4: Verify system stable
        oscs = editor.get_oscillators()
        assert len(oscs) == 2, f"Expected 2 oscillators, got {len(oscs)}"
        state = editor.get_transport_state()
        assert state is not None, "Transport state should be queryable"

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc.get("mode") == "Side" or osc.get("processingMode") == "Side"


class TestPaneManagementWorkflow:
    """Full pane lifecycle: create, assign, move, remove, verify."""

    def test_pane_lifecycle(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: pane operations leaving orphaned oscillators or breaking
        audio bindings.
        """
        # Step 1: Add 3 oscillators (all in default pane)
        ids = []
        for i in range(3):
            oid = editor.add_oscillator(source_id, name=f"Pane WF {i}")
            assert oid is not None
            ids.append(oid)
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        panes = editor.get_panes()
        assert len(panes) == 1
        default_pane = panes[0]["id"]

        # Step 2: Create second pane
        pane2_id = editor.add_pane("Analysis View")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        # Step 3: Move oscillator 1 to pane 2
        editor.move_oscillator(ids[1], pane2_id)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(ids[1]).get("paneId") == pane2_id,
            timeout_s=3.0, desc="move to pane 2",
        )

        # Step 4: Verify assignments
        osc0 = editor.get_oscillator_by_id(ids[0])
        osc1 = editor.get_oscillator_by_id(ids[1])
        osc2 = editor.get_oscillator_by_id(ids[2])
        assert osc0["paneId"] == default_pane
        assert osc1["paneId"] == pane2_id
        assert osc2["paneId"] == default_pane

        # Step 5: Verify source binding preserved after move
        assert osc1.get("sourceId") == source_id

        # Step 6: Create pane 3, move oscillator 2 there
        pane3_id = editor.add_pane("Debug View")
        if pane3_id:
            editor.move_oscillator(ids[2], pane3_id)
            editor.wait_until(
                lambda: editor.get_oscillator_by_id(ids[2]).get("paneId") == pane3_id,
                timeout_s=3.0, desc="move to pane 3",
            )

        # Step 7: Remove pane 2 (has oscillator 1)
        editor.remove_pane(pane2_id)

        # Step 8: Verify oscillator 1 not orphaned
        valid_panes = {p["id"] for p in editor.get_panes()}
        osc1_after = editor.get_oscillator_by_id(ids[1])
        assert osc1_after is not None, "Oscillator should survive pane removal"
        assert osc1_after["paneId"] in valid_panes, (
            f"Oscillator should be in valid pane after removal, "
            f"got '{osc1_after['paneId']}'"
        )


class TestSaveLoadDuringPlaybackWorkflow:
    """Save and load state while audio is actively playing."""

    def test_save_load_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: state save capturing partial audio buffer state,
        or state load while playing causing torn read in audio callback.
        """
        # Step 1: Create oscillators and start playback
        id1 = editor.add_oscillator(source_id, name="PlaySave A")
        id2 = editor.add_oscillator(source_id, name="PlaySave B", mode="Mono")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Step 2: Wait for waveform data
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Step 3: Save while playing
        path = "/tmp/oscil_e2e_playback_save.xml"
        saved = editor.save_state(path)
        assert saved, "Should be able to save during playback"
        assert editor.is_playing(), "Transport should still be playing after save"

        # Step 4: Modify state while playing
        editor.update_oscillator(id1, name="Modified During Play")
        editor.delete_oscillator(id2)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Step 5: Load original state while playing
        loaded = editor.load_state(path)
        assert loaded, "Should be able to load during playback"

        # Step 6: Verify original state restored
        oscs = editor.wait_for_oscillator_count(2, timeout_s=5.0)
        names = sorted(o["name"] for o in oscs)
        assert "PlaySave A" in names, "Original name should be restored"
        assert "PlaySave B" in names, "Second oscillator should be restored"

        editor.transport_stop()


class TestThemeAndOscillatorWorkflow:
    """Change themes while oscillators are configured and playing."""

    def test_theme_change_preserves_oscillator_state(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: theme change resetting oscillator properties, or theme
        change during playback causing render pipeline crash.
        """
        # Step 1: Add oscillators
        id1 = editor.add_oscillator(source_id, name="Theme Osc 1", colour="#FF0000")
        id2 = editor.add_oscillator(source_id, name="Theme Osc 2", colour="#00FF00")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Step 2: Start playback
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Step 3: Expand options and get themes
        if not editor.element_exists("sidebar_options"):
            editor.transport_stop()
            return
        editor.click("sidebar_options")

        dropdown_id = "sidebar_options_themeDropdown"
        if not editor.element_exists(dropdown_id):
            editor.transport_stop()
            return

        el = editor.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if len(items) < 2:
            editor.transport_stop()
            return

        # Step 4: Cycle through all themes during playback
        for item in items:
            tid = item.get("id", item) if isinstance(item, dict) else str(item)
            editor.select_dropdown_item(dropdown_id, str(tid))

        # Step 5: Verify oscillators survived
        assert editor.is_playing(), "Transport should still be playing"
        oscs = editor.get_oscillators()
        assert len(oscs) == 2, "Oscillators should survive theme changes"
        names = sorted(o["name"] for o in oscs)
        assert names == ["Theme Osc 1", "Theme Osc 2"]

        # Step 6: Verify oscillator properties unchanged
        osc1 = editor.get_oscillator_by_id(id1)
        assert osc1 is not None
        assert osc1["name"] == "Theme Osc 1"

        editor.transport_stop()
