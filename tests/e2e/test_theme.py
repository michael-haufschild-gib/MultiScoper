"""
E2E tests for theme switching functionality.

What bugs these tests catch:
- Theme dropdown not populated with available themes
- Theme selection not updating the selectedId in dropdown state
- Selecting a theme that crashes or produces rendering errors
- Theme not persisting after editor close/reopen
- Theme dropdown showing wrong selected theme after roundtrip
"""

import pytest
from oscil_test_utils import OscilTestClient


def _get_theme_info(client: OscilTestClient):
    """Get theme dropdown info: items list and selected ID."""
    el = client.get_element("sidebar_options_themeDropdown")
    if not el or not el.extra:
        return None, None, []
    items = el.extra.get("items", [])
    selected = el.extra.get("selectedId", "")
    num = el.extra.get("numItems", 0)
    return selected, num, items


def _item_id(item):
    """Extract the id from a theme dropdown item (dict or string)."""
    if isinstance(item, dict):
        return item.get("id", "")
    return str(item)


class TestThemeDropdown:
    """Theme dropdown population and selection."""

    def test_dropdown_has_themes(self, options_section: OscilTestClient):
        """
        Bug caught: theme dropdown not populated (ThemeManager not wired).
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.skip("Theme dropdown not registered")

        selected, num, items = _get_theme_info(c)
        assert num is not None and num > 0, (
            f"Theme dropdown should have at least 1 theme, got {num}"
        )

    def test_selecting_different_theme_updates_state(self, options_section: OscilTestClient):
        """
        Bug caught: dropdown selection handler not calling ThemeManager::setTheme.
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.skip("Theme dropdown not registered")

        current, num, items = _get_theme_info(c)
        if num is None or num < 2:
            pytest.skip("Need at least 2 themes to test selection")

        # Pick a theme that is NOT the current selection
        new_theme = None
        for item in items:
            tid = _item_id(item)
            if tid and tid != current:
                new_theme = tid
                break

        if new_theme is None:
            pytest.skip("Could not find an alternative theme")

        c.select_dropdown_item(dropdown_id, new_theme)

        # Verify the dropdown reports the new selection
        def selection_changed():
            sel, _, _ = _get_theme_info(c)
            return sel == new_theme
        c.wait_until(selection_changed, timeout_s=3.0, desc="theme selection to update")

        sel_after, _, _ = _get_theme_info(c)
        assert sel_after == new_theme, (
            f"Expected theme '{new_theme}', got '{sel_after}'"
        )

    def test_all_themes_selectable_without_error(self, options_section: OscilTestClient):
        """
        Bug caught: specific theme crashing on apply (e.g., missing color key,
        nil dereference in theme parser).
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.skip("Theme dropdown not registered")

        _, _, items = _get_theme_info(c)
        if not items:
            pytest.skip("No theme items to test")

        failures = []
        for item in items:
            tid = _item_id(item)
            if not tid:
                continue
            success = c.select_dropdown_item(dropdown_id, tid)
            if not success:
                failures.append(tid)

        assert not failures, f"Failed to select themes: {failures}"


class TestThemePersistence:
    """Theme survives editor close/reopen."""

    def test_theme_persists_across_editor_lifecycle(self, client: OscilTestClient):
        """
        Bug caught: theme selection not serialized to plugin state, so it
        reverts to default on reopen.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )
        client.open_editor()

        # Expand options
        options_id = "sidebar_options"
        if not client.element_exists(options_id):
            client.close_editor()
            pytest.skip("Options section not registered")
        client.click(options_id)

        dropdown_id = "sidebar_options_themeDropdown"
        if not client.element_exists(dropdown_id):
            client.close_editor()
            pytest.skip("Theme dropdown not registered")

        current, num, items = _get_theme_info(client)
        if num is None or num < 2:
            client.close_editor()
            pytest.skip("Need at least 2 themes")

        # Select a non-default theme
        target = None
        for item in items:
            tid = _item_id(item)
            if tid and tid != current:
                target = tid
                break
        if not target:
            client.close_editor()
            pytest.skip("No alternative theme found")

        client.select_dropdown_item(dropdown_id, target)
        client.wait_until(
            lambda: _get_theme_info(client)[0] == target,
            timeout_s=2.0, desc="theme applied",
        )

        # Close and reopen
        client.close_editor()
        client.open_editor()

        # Re-expand options
        if client.element_exists(options_id):
            client.click(options_id)

        restored, _, _ = _get_theme_info(client)
        client.close_editor()

        assert restored == target, (
            f"Theme should persist: expected '{target}', got '{restored}'"
        )
