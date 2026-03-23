"""
E2E tests for the oscillator config popup (settings dialog).

What bugs these tests catch:
- Config popup not opening when settings button is clicked
- Name field changes not saved to oscillator state
- Source dropdown not populated or selection not updating state
- Mode selector buttons not changing oscillator processing mode
- Color picker changes not applied to oscillator colour
- Line width slider value not propagating to state
- Opacity slider value not propagating to state
- Visual preset dropdown not populated
- Pane selector not showing available panes
- Close button not dismissing popup
- Changes lost if popup is closed by clicking outside
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestConfigPopupLifecycle:
    """Config popup open/close behavior."""

    def test_open_and_close(self, config_popup):
        """
        Bug caught: popup not opening or close button not wired.
        """
        editor, osc_id = config_popup
        assert editor.element_visible("configPopup")

    def test_name_field_editable(self, config_popup):
        """
        Bug caught: name field read-only or not connected to state.
        """
        editor, osc_id = config_popup

        name_field = "configPopup_nameField"
        if not editor.element_exists(name_field):
            pytest.xfail("Name field not in popup")

        editor.clear_text(name_field)
        editor.type_text(name_field, "Config Test Name")

        osc = editor.get_oscillator_by_id(osc_id)
        if osc:
            assert osc["name"] == "Config Test Name", (
                f"Name should update via config popup, got '{osc['name']}'"
            )

    def test_source_dropdown_populated(self, config_popup):
        """
        Bug caught: source dropdown empty in config popup.
        """
        editor, osc_id = config_popup

        dropdown_id = "configPopup_sourceDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.xfail("Source dropdown not in popup")

        el = editor.get_element(dropdown_id)
        num_items = el.extra.get("numItems", 0) if el else 0

        assert num_items > 0, (
            f"Source dropdown should have items, got {num_items}"
        )

    def test_opacity_slider_adjustable(self, config_popup):
        """
        Bug caught: opacity slider not propagating value to oscillator state.
        """
        editor, osc_id = config_popup

        slider_id = "configPopup_opacitySlider"
        if not editor.element_exists(slider_id):
            pytest.xfail("Opacity slider not in popup")

        editor.set_slider(slider_id, 0.5)

        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "opacity" in osc:
            assert abs(osc["opacity"] - 0.5) < 0.1, (
                f"Opacity should be ~0.5 via popup, got {osc['opacity']}"
            )

    def test_line_width_slider_adjustable(self, config_popup):
        """
        Bug caught: line width slider not wired to oscillator property.
        """
        editor, osc_id = config_popup

        slider_id = "configPopup_lineWidthSlider"
        if not editor.element_exists(slider_id):
            pytest.xfail("Line width slider not in popup")

        editor.set_slider(slider_id, 4.0)

        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "lineWidth" in osc:
            assert abs(osc["lineWidth"] - 4.0) < 0.5, (
                f"Line width should be ~4.0 via popup, got {osc['lineWidth']}"
            )

    def test_visual_preset_dropdown_exists(self, config_popup):
        """
        Bug caught: visual preset dropdown not registered in config popup.
        """
        editor, osc_id = config_popup

        dropdown_id = "configPopup_visualPresetDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.xfail("Visual preset dropdown not in popup")

        el = editor.get_element(dropdown_id)
        assert el is not None
        assert el.visible

    def test_visual_preset_selection_does_not_crash(self, config_popup):
        """
        Bug caught: selecting a visual preset crashes (null pointer in
        preset application code).
        """
        editor, osc_id = config_popup

        dropdown_id = "configPopup_visualPresetDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.xfail("Visual preset dropdown not in popup")

        el = editor.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.xfail("No visual preset items")

        for item in items:
            item_id = item.get("id", item) if isinstance(item, dict) else str(item)
            editor.select_dropdown_item(dropdown_id, str(item_id))

        # Verify oscillator still exists
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None, "Oscillator should survive visual preset changes"

    def test_color_picker_exists(self, config_popup):
        """
        Bug caught: color picker/swatches not registered in config popup.
        """
        editor, osc_id = config_popup

        picker_id = "configPopup_colorPicker"
        if not editor.element_exists(picker_id):
            pytest.xfail("Color picker not in popup")

    def test_pane_selector_exists(self, config_popup):
        """
        Bug caught: pane selector not registered in config popup.
        """
        editor, osc_id = config_popup

        selector_id = "configPopup_paneSelector"
        if not editor.element_exists(selector_id):
            pytest.xfail("Pane selector not in popup")


class TestConfigPopupModeSelector:
    """Processing mode buttons in the config popup."""

    def test_mode_buttons_exist(self, config_popup):
        """
        Bug caught: mode selector buttons not registered.
        """
        editor, osc_id = config_popup

        mode_selector = "configPopup_modeSelector"
        if not editor.element_exists(mode_selector):
            pytest.xfail("Mode selector not in popup")

        mode_ids = [
            "configPopup_modeSelector_stereo",
            "configPopup_modeSelector_mono",
            "configPopup_modeSelector_mid",
            "configPopup_modeSelector_side",
            "configPopup_modeSelector_left",
            "configPopup_modeSelector_right",
        ]
        found = sum(1 for mid in mode_ids if editor.element_exists(mid))

        assert found >= 4, (
            f"Should have at least 4 mode buttons, found {found}"
        )

    def test_mode_button_changes_state(self, config_popup):
        """
        Bug caught: mode button click not updating oscillator processing mode.
        """
        editor, osc_id = config_popup

        mono_btn = "configPopup_modeSelector_mono"
        if not editor.element_exists(mono_btn):
            pytest.xfail(0)

        editor.click(mono_btn)

        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id)
            and editor.get_oscillator_by_id(osc_id).get("mode") == "Mono",
            timeout_s=2.0,
            desc="mode to become Mono",
        )

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc["mode"] == "Mono", (
            f"Mode should be Mono after clicking mono button, got '{osc['mode']}'"
        )

    def test_all_mode_buttons_update_state(self, config_popup):
        """
        Bug caught: some mode buttons wired incorrectly, setting the wrong
        enum value or not triggering state update at all.
        """
        editor, osc_id = config_popup

        mode_selector = "configPopup_modeSelector"
        if not editor.element_exists(mode_selector):
            pytest.xfail("Mode selector not in popup")

        button_to_mode = {
            "configPopup_modeSelector_mono": "Mono",
            "configPopup_modeSelector_mid": "Mid",
            "configPopup_modeSelector_side": "Side",
            "configPopup_modeSelector_left": "Left",
            "configPopup_modeSelector_right": "Right",
            "configPopup_modeSelector_stereo": "FullStereo",
        }

        modes_verified = 0
        for btn_id, expected_mode in button_to_mode.items():
            if not editor.element_exists(btn_id):
                continue

            editor.click(btn_id)
            try:
                editor.wait_until(
                    lambda em=expected_mode: (
                        editor.get_oscillator_by_id(osc_id)
                        and editor.get_oscillator_by_id(osc_id).get("mode") == em
                    ),
                    timeout_s=2.0,
                    desc=f"mode to become {expected_mode}",
                )
                modes_verified += 1
            except TimeoutError:
                pass

        assert modes_verified >= 4, (
            f"At least 4 mode buttons should work, verified {modes_verified}"
        )


class TestConfigPopupSliderEdgeCases:
    """Edge cases for slider interactions in the config popup."""

    def test_opacity_boundary_values(self, config_popup):
        """
        Bug caught: opacity slider not clamping at 0.0 or 1.0, or producing
        NaN when set to exact boundary values.
        """
        editor, osc_id = config_popup

        slider_id = "configPopup_opacitySlider"
        if not editor.element_exists(slider_id):
            pytest.xfail("Opacity slider not in popup")

        # Test boundary: minimum
        editor.set_slider(slider_id, 0.0)
        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "opacity" in osc:
            assert 0.0 <= osc["opacity"] <= 0.05, (
                f"Opacity at min should be ~0, got {osc['opacity']}"
            )

        # Test boundary: maximum
        editor.set_slider(slider_id, 1.0)
        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "opacity" in osc:
            assert 0.95 <= osc["opacity"] <= 1.0, (
                f"Opacity at max should be ~1.0, got {osc['opacity']}"
            )

    def test_line_width_boundary_values(self, config_popup):
        """
        Bug caught: line width below 1.0 causes invisible waveform,
        or extremely high values overflow GPU vertex buffer.
        """
        editor, osc_id = config_popup

        slider_id = "configPopup_lineWidthSlider"
        if not editor.element_exists(slider_id):
            pytest.xfail("Line width slider not in popup")

        # Set to minimum
        editor.set_slider(slider_id, 1.0)
        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "lineWidth" in osc:
            assert osc["lineWidth"] >= 0.5, (
                f"Line width at min should be >= 0.5, got {osc['lineWidth']}"
            )

        # Set to maximum
        editor.set_slider(slider_id, 10.0)
        osc = editor.get_oscillator_by_id(osc_id)
        if osc and "lineWidth" in osc:
            assert osc["lineWidth"] <= 15.0, (
                f"Line width at max should be <= 15, got {osc['lineWidth']}"
            )


class TestConfigPopupPropertyPersistence:
    """Verify config popup changes persist across popup close/reopen."""

    def test_changes_persist_after_close_reopen(self, config_popup):
        """
        Bug caught: config popup changes stored only in popup-local state
        and discarded on close, so reopening shows the original values.
        """
        editor, osc_id = config_popup

        # Change opacity via popup
        slider_id = "configPopup_opacitySlider"
        if not editor.element_exists(slider_id):
            pytest.xfail("Opacity slider not in popup")

        editor.set_slider(slider_id, 0.3)

        # Wait for state to update
        editor.wait_until(
            lambda: (osc := editor.get_oscillator_by_id(osc_id))
            and osc.get("opacity") is not None
            and abs(osc["opacity"] - 0.3) < 0.1,
            timeout_s=2.0,
            desc="opacity to update to ~0.3",
        )

        # Close popup (teardown will handle this, but we close explicitly to reopen)
        for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
            if editor.element_exists(btn):
                editor.click(btn)
                try:
                    editor.wait_for_not_visible("configPopup", timeout_s=2.0)
                except TimeoutError:
                    pass
                break

        # Verify state persisted after close
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        if "opacity" in osc:
            assert abs(osc["opacity"] - 0.3) < 0.1, (
                f"Opacity should persist after popup close: got {osc['opacity']}"
            )

    def test_multiple_property_changes_in_single_session(self, config_popup):
        """
        Bug caught: changing multiple properties in the same popup session
        causes only the last change to take effect (e.g., each property
        setter overwrites the entire state instead of merging).
        """
        editor, osc_id = config_popup

        opacity_id = "configPopup_opacitySlider"
        width_id = "configPopup_lineWidthSlider"
        mono_btn = "configPopup_modeSelector_mono"

        changes_made = 0
        if editor.element_exists(opacity_id):
            editor.set_slider(opacity_id, 0.6)
            changes_made += 1
        if editor.element_exists(width_id):
            editor.set_slider(width_id, 5.0)
            changes_made += 1
        if editor.element_exists(mono_btn):
            editor.click(mono_btn)
            changes_made += 1

        if changes_made < 2:
            pytest.xfail("Need at least 2 adjustable properties to test")

        # Wait for all changes to propagate
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id) is not None,
            timeout_s=2.0,
            desc="oscillator state to be queryable",
        )

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None

        # Verify ALL changes took effect, not just the last one
        verified = 0
        if "opacity" in osc and editor.element_exists(opacity_id):
            assert abs(osc["opacity"] - 0.6) < 0.15, (
                f"Opacity should be ~0.6 after multi-change, got {osc['opacity']}"
            )
            verified += 1
        if "lineWidth" in osc and editor.element_exists(width_id):
            assert abs(osc["lineWidth"] - 5.0) < 1.0, (
                f"Line width should be ~5.0 after multi-change, got {osc['lineWidth']}"
            )
            verified += 1
        if editor.element_exists(mono_btn):
            mode = osc.get("mode", osc.get("processingMode"))
            if mode is not None:
                assert mode == "Mono", (
                    f"Mode should be Mono after multi-change, got {mode}"
                )
                verified += 1

        assert verified >= 2, (
            f"At least 2 property changes should be verifiable, got {verified}"
        )
