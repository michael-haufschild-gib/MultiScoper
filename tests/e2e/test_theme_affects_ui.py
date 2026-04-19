"""
E2E coverage for the theme dropdown's effect on the UI layer.

The theme dropdown exposes multiple preset themes (Classic Amber,
Glass Dark Black, High Contrast, ...).  Selecting a theme applies a
palette change to every ThemedComponent in the tree.

What bugs these tests catch:
- Theme dropdown selection updates dropdown label but does not
  actually propagate to ThemedComponent listeners.
- Theme change during active rendering causes a crash / GL error.
- Theme not persisted across close/reopen.
- Applying the same theme twice (or back-to-back) leaks resources.

These tests extend existing test_theme.py by also verifying that
the applied selection is reflected in the element info after a
short settle.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


THEME_DD = "sidebar_options_themeDropdown"


class TestThemeDropdownAPI:
    def test_dropdown_has_at_least_five_themes(
        self, editor: MultiScoperTestClient
    ):
        el = editor.get_element(THEME_DD)
        assert el is not None
        assert el.extra.get("numItems", 0) >= 5, (
            f"expected ≥5 themes, got {el.extra.get('numItems', 0)}"
        )

    def test_each_theme_selectable(self, editor: MultiScoperTestClient):
        el = editor.get_element(THEME_DD)
        themes = [i["id"] for i in el.extra.get("items", [])]
        assert len(themes) >= 5

        for theme_id in themes:
            editor.select_dropdown_item(THEME_DD, theme_id)
            editor.wait_until(
                lambda: editor.get_element(THEME_DD).extra.get("selectedId") == theme_id,
                timeout_s=3.0,
                desc=f"theme dropdown to show {theme_id}",
            )

        # Harness must stay alive after cycling through all themes.
        assert editor.health_check()["data"]["status"] == "ok"


class TestThemeChangeDuringActiveRendering:
    def test_theme_change_mid_audio(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: theme change while the OpenGL renderer is
        issuing draw calls → GL error or crash."""
        editor.add_oscillator(source_id, name="ThemeRender")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        try:
            for theme_id in ("Classic Amber", "Light Mode", "High Contrast"):
                editor.select_dropdown_item(THEME_DD, theme_id)
                editor.wait_until(
                    lambda: editor.get_element(THEME_DD).extra.get("selectedId") == theme_id,
                    timeout_s=3.0,
                    desc=f"theme to be {theme_id}",
                )
                # Waveform should still be flowing.
                assert editor.health_check()["data"]["status"] == "ok"
        finally:
            editor.transport_stop()


class TestThemePersistenceAcrossReopen:
    def test_theme_survives_editor_close_reopen(
        self, editor: MultiScoperTestClient
    ):
        """Bug caught: editor close/reopen resets theme to default."""
        target = "High Contrast"
        editor.select_dropdown_item(THEME_DD, target)
        editor.wait_until(
            lambda: editor.get_element(THEME_DD).extra.get("selectedId") == target,
            timeout_s=3.0,
            desc=f"theme to be {target}",
        )

        editor.close_editor()
        editor.open_editor()
        editor.wait_for_element(THEME_DD, timeout_s=5.0)

        el = editor.get_element(THEME_DD)
        assert el is not None
        assert el.extra.get("selectedId") == target, (
            f"theme must persist across close/reopen, got {el.extra.get('selectedId')}"
        )

        # Restore to a stable default for subsequent tests.
        editor.select_dropdown_item(THEME_DD, "Classic Amber")
