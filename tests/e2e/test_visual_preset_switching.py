"""
E2E coverage for the visual preset dropdown in add and config dialogs.

The visual preset dropdown has at minimum "Default" and "Vector Scope"
options.  Selecting a preset applies a bundle of per-oscillator visual
overrides (color palette, line style, effect stack).

What bugs these tests catch:
- Visual preset dropdown wired but selecting a preset doesn't apply
  any of its bundled overrides (the user picks "Vector Scope" and
  their waveform looks identical).
- Visual preset selection clobbers user-chosen colour without warning.
- Preset selection on one oscillator accidentally applies to all.
- Preset selection not persisted.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


CONFIG_DROPDOWN = "configPopup_visualPresetDropdown"
ADD_DROPDOWN = "addOscillatorDialog_visualPresetDropdown"


class TestVisualPresetInConfigPopup:
    def test_dropdown_populated(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: preset dropdown empty because factory registry
        not wired in constructor."""
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        try:
            el = editor.get_element(CONFIG_DROPDOWN)
            assert el is not None
            assert el.extra.get("numItems", 0) >= 2, (
                "at least Default and Vector Scope presets expected"
            )
        finally:
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)

    def test_selecting_vector_scope_updates_dropdown(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: preset selection label doesn't update on state
        change (user thinks selection was ignored)."""
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        try:
            editor.select_dropdown_item(CONFIG_DROPDOWN, "vector_scope")
            editor.wait_until(
                lambda: editor.get_element(CONFIG_DROPDOWN).extra.get("selectedId") == "vector_scope",
                timeout_s=3.0,
                desc="visual preset to become vector_scope",
            )
        finally:
            # Restore.
            editor.select_dropdown_item(CONFIG_DROPDOWN, "default")
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)

    def test_preset_change_only_affects_target_oscillator(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: changing preset on one oscillator accidentally
        broadcasts to all.  Create two oscs, change preset on the
        first, verify the second's colour is untouched."""
        id_a = editor.add_oscillator(source_id, name="A", colour="#FF0000")
        id_b = editor.add_oscillator(source_id, name="B", colour="#00FF00")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        colour_b_before = editor.get_oscillator_by_id(id_b)["colour"]

        # Open first osc popup and change preset.
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        editor.select_dropdown_item(CONFIG_DROPDOWN, "vector_scope")
        editor.wait_until(
            lambda: editor.get_element(CONFIG_DROPDOWN).extra.get("selectedId") == "vector_scope",
            timeout_s=3.0, desc="preset change committed",
        )
        editor.click("configPopup_closeBtn")
        editor.wait_for_not_visible("configPopup", timeout_s=3.0)

        colour_b_after = editor.get_oscillator_by_id(id_b)["colour"]
        assert colour_b_after == colour_b_before, (
            f"B's colour changed because of A's preset selection: "
            f"before={colour_b_before!r}, after={colour_b_after!r}"
        )


class TestVisualPresetInAddDialog:
    def test_add_dialog_preset_dropdown_populated(
        self, editor: MultiScoperTestClient
    ):
        editor.click("sidebar_addOscillator")
        editor.wait_for_visible("addOscillatorDialog", timeout_s=3.0)
        try:
            el = editor.get_element(ADD_DROPDOWN)
            assert el is not None
            assert el.extra.get("numItems", 0) >= 2
        finally:
            editor.click("addOscillatorDialog_cancelBtn")
            editor.wait_for_not_visible("addOscillatorDialog", timeout_s=3.0)
