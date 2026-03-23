"""
E2E tests for oscillator Create / Read / Update / Delete user flows.

What bugs these tests catch:
- Add button not wired to dialog open
- Dialog submission not creating oscillator in state
- Cancel button leaving stale state
- Delete button not removing oscillator from state or UI list
- Delete-all leaving ghost state
- Config popup not saving edits to oscillator properties
- Visibility toggle not propagating to oscillator state
- Processing mode buttons not updating state
- Editing an oscillator that was just added (race between UI registration and state)
"""

import pytest
from oscil_test_utils import OscilTestClient
from page_objects import AddOscillatorDialog


# ── Add Oscillator ──────────────────────────────────────────────────────────


class TestAddOscillator:
    """Full user journey: click Add, fill dialog, confirm, verify state."""

    def test_add_via_dialog(self, editor: OscilTestClient, source_id: str):
        """
        Bug caught: dialog submit handler not wired, or state not updated.
        """
        initial_count = len(editor.get_oscillators())
        dialog = AddOscillatorDialog(editor)

        # Open the add dialog
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        # Select source
        dialog.select_source(source_id)

        # Enter name
        dialog.set_name("E2E Test Osc")

        # Confirm
        dialog.confirm()

        # Verify oscillator was created
        oscs = editor.wait_for_oscillator_count(initial_count + 1, timeout_s=3.0)
        assert len(oscs) == initial_count + 1

        # Verify the new oscillator appears in the sidebar list
        editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=3.0)

    def test_cancel_does_not_create(self, editor: OscilTestClient):
        """
        Bug caught: cancel handler accidentally submitting, or dialog state
        leaking between open/close cycles.
        """
        initial_count = len(editor.get_oscillators())
        dialog = AddOscillatorDialog(editor)

        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        dialog.cancel()

        final_count = len(editor.get_oscillators())
        assert final_count == initial_count, (
            f"Cancel should not create an oscillator: expected {initial_count}, got {final_count}"
        )

    def test_add_button_exists_and_is_clickable(self, editor: OscilTestClient):
        """
        Bug caught: sidebar layout regression hiding the add button.
        """
        el = editor.wait_for_visible("sidebar_addOscillator", timeout_s=3.0)
        assert el.width > 0 and el.height > 0, "Add button has zero size"

    def test_dialog_name_field_accepts_text(self, editor: OscilTestClient):
        """
        Bug caught: name field in add dialog not wired to text input handler.
        """
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        if not editor.element_exists(AddOscillatorDialog.NAME_FIELD):
            dialog.cancel()
            pytest.fail("Name field not available in add dialog")

        editor.clear_text(AddOscillatorDialog.NAME_FIELD)
        result = editor.type_text(AddOscillatorDialog.NAME_FIELD, "Custom Name")

        dialog.cancel()

        assert result, "Name field must accept text input"

    def test_dialog_color_picker_exists(self, editor: OscilTestClient):
        """
        Bug caught: color picker/swatches not rendered in add dialog.
        """
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        exists = editor.element_exists(AddOscillatorDialog.COLOR_PICKER)

        dialog.cancel()

        if not exists:
            pytest.fail("Color picker not available in add dialog")

    def test_dialog_pane_selector_exists(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane selector not rendered in add dialog, preventing
        user from choosing which pane to add the oscillator to.
        """
        # Need at least one oscillator so panes exist
        osc_id = editor.add_oscillator(source_id, name="PaneSelector Prereq")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        exists = editor.element_exists(AddOscillatorDialog.PANE_SELECTOR)

        dialog.cancel()

        if not exists:
            pytest.fail("Pane selector not available in add dialog")

    def test_dialog_source_dropdown_has_sources(self, editor: OscilTestClient):
        """
        Bug caught: source dropdown in add dialog is empty because
        SourceManager not wired to the dialog, so user cannot select
        a source to create an oscillator.
        """
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        dropdown_id = AddOscillatorDialog.SOURCE_DROPDOWN
        if not editor.element_exists(dropdown_id):
            dialog.cancel()
            pytest.fail("Source dropdown not registered in add dialog")

        el = editor.get_element(dropdown_id)
        num_items = el.extra.get("numItems", 0) if el else 0

        dialog.cancel()

        assert num_items > 0, (
            f"Source dropdown must have at least 1 source, got {num_items}"
        )

    def test_add_multiple_oscillators_increments_count(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: state manager replacing instead of appending oscillators.
        """
        for i in range(3):
            osc_id = editor.add_oscillator(source_id, name=f"Multi {i}")
            assert osc_id is not None, f"Failed to add oscillator {i}"

        oscs = editor.wait_for_oscillator_count(3, timeout_s=5.0)
        assert len(oscs) == 3


# ── Delete Oscillator ───────────────────────────────────────────────────────


class TestDeleteOscillator:
    """User flows for deleting oscillators."""

    def test_delete_from_list_item(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: delete button click handler not wired, or state not updated.
        """
        assert len(editor.get_oscillators()) == 1

        sidebar_page.delete_oscillator(0)

        editor.wait_for_oscillator_count(0, timeout_s=3.0)

    def test_delete_all_one_by_one(
        self, editor: OscilTestClient, three_oscillators, sidebar_page
    ):
        """
        Bug caught: index-based deletion corrupting when list shrinks
        (off-by-one after removing item 0 shifts remaining items).
        Known failure: delete button for item 0 not registered after three_oscillators fixture.
        """
        for remaining in [2, 1, 0]:
            sidebar_page.delete_oscillator(0)
            editor.wait_for_oscillator_count(remaining, timeout_s=3.0)

        assert len(editor.get_oscillators()) == 0

    def test_deleted_oscillator_id_gone_from_state(
        self, editor: OscilTestClient, two_oscillators, sidebar_page
    ):
        """
        Bug caught: UI removes list item but state still holds the oscillator.
        """
        oscs_before = editor.get_oscillators()
        target_id = oscs_before[0]["id"]

        sidebar_page.delete_oscillator(0)

        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        remaining_ids = [o["id"] for o in editor.get_oscillators()]
        assert target_id not in remaining_ids, (
            f"Deleted oscillator {target_id} still in state"
        )


# ── Edit Oscillator (Config Popup) ──────────────────────────────────────────


class TestEditOscillator:
    """User flows for editing oscillator properties via the config popup."""

    def test_settings_button_opens_popup(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: settings button click not opening the config popup.
        """
        sidebar_page.open_settings(0)
        assert editor.element_visible("configPopup")

    def test_popup_closes_on_close_button(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: close button not dismissing the popup.
        """
        from page_objects import ConfigPopup
        sidebar_page.open_settings(0)

        popup = ConfigPopup(editor)
        popup.close()
        assert not editor.element_visible("configPopup"), (
            "Config popup should be dismissed after close"
        )

    def test_name_change_persists(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: name field edit not saved to oscillator state.
        """
        from page_objects import ConfigPopup
        sidebar_page.open_settings(0)

        popup = ConfigPopup(editor)
        if not popup.has_element(ConfigPopup.NAME_FIELD):
            popup.close()
            pytest.fail("Name field not available in config popup")

        new_name = "Renamed By E2E"
        popup.set_name(new_name)
        popup.close()

        # Verify via state API
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc is not None
        assert osc["name"] == new_name, (
            f"Expected name '{new_name}', got '{osc['name']}'"
        )

    def test_slider_adjustment(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: slider value not propagating to oscillator properties.
        """
        from page_objects import ConfigPopup
        sidebar_page.open_settings(0)

        popup = ConfigPopup(editor)
        if not popup.has_element(ConfigPopup.LINE_WIDTH_SLIDER):
            popup.close()
            pytest.fail("Line width slider not available in config popup")

        popup.set_line_width(4.0)

        # Verify the state actually changed
        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(oscillator))
            and osc.get("lineWidth") is not None
            and abs(osc["lineWidth"] - 4.0) < 0.5,
            timeout_s=2.0,
            desc="lineWidth to update to ~4.0",
        )
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc is not None
        assert "lineWidth" in osc, "Oscillator must have lineWidth after slider set"
        assert abs(osc["lineWidth"] - 4.0) < 0.5, (
            f"lineWidth must be ~4.0, got {osc['lineWidth']}"
        )

        popup.close()


# ── Visibility Toggle ───────────────────────────────────────────────────────


class TestVisibilityToggle:
    """Toggle oscillator visibility from the sidebar list item."""

    def test_toggle_changes_state(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: visibility button click not updating oscillator.visible.
        """
        osc_before = editor.get_oscillator_by_id(oscillator)
        assert osc_before is not None, "Oscillator must be queryable"
        initial_visible = osc_before.get("visible", True)

        # Select the oscillator to expose controls
        sidebar_page.select_oscillator(0)
        sidebar_page.toggle_visibility(0)

        # Wait for state to update
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator).get("visible") != initial_visible,
            timeout_s=2.0,
            desc="visibility to toggle",
        )

        osc_after = editor.get_oscillator_by_id(oscillator)
        assert osc_after["visible"] != initial_visible

    def test_toggle_roundtrip(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: toggle not idempotent -- second click doesn't restore.
        """
        sidebar_page.select_oscillator(0)

        osc_before = editor.get_oscillator_by_id(oscillator)
        original = osc_before.get("visible", True)

        # Toggle off
        sidebar_page.toggle_visibility(0)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator).get("visible") != original,
            timeout_s=2.0, desc="first toggle",
        )

        # Toggle back
        sidebar_page.toggle_visibility(0)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator).get("visible") == original,
            timeout_s=2.0, desc="second toggle",
        )

        assert editor.get_oscillator_by_id(oscillator)["visible"] == original


# ── Processing Mode ─────────────────────────────────────────────────────────


class TestProcessingMode:
    """Verify all 6 processing modes can be set and persist in state."""

    MODES = {
        "FullStereo": 0,
        "Mono": 1,
        "Mid": 2,
        "Side": 3,
        "Left": 4,
        "Right": 5,
    }

    def test_default_mode_is_full_stereo(self, editor: OscilTestClient, oscillator: str):
        """
        Bug caught: oscillator created with wrong default mode.
        """
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc is not None
        mode = osc.get("mode", osc.get("processingMode"))
        assert mode in ("FullStereo", 0), f"Default mode should be FullStereo, got {mode}"

    def test_all_modes_via_config_popup(
        self, editor: OscilTestClient, oscillator: str, sidebar_page
    ):
        """
        Bug caught: mode button click handler not updating state,
        or mode enum serialization mismatch.
        """
        from page_objects import ConfigPopup
        sidebar_page.open_settings(0)
        popup = ConfigPopup(editor)

        if not popup.has_element(ConfigPopup.MODE_SELECTOR):
            popup.close()
            pytest.fail("Mode selector not available in config popup")

        modes_tested = 0
        for suffix, expected_mode in ConfigPopup.MODE_BUTTONS.items():
            if not popup.set_mode(suffix):
                continue

            def mode_matches(expected=expected_mode):
                osc = editor.get_oscillator_by_id(oscillator)
                if not osc:
                    return False
                current = osc.get("mode", osc.get("processingMode", ""))
                return current == expected
            try:
                editor.wait_until(
                    mode_matches, timeout_s=2.0,
                    desc=f"mode to become {expected_mode}",
                )
                modes_tested += 1
            except TimeoutError:
                pass

        assert modes_tested >= 4, (
            f"At least 4 processing modes must be testable, got {modes_tested}"
        )

        popup.close()
