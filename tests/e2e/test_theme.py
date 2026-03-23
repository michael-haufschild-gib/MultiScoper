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
            pytest.fail("Theme dropdown not registered")

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
            pytest.fail("Theme dropdown not registered")

        current, num, items = _get_theme_info(c)
        if num is None or num < 2:
            pytest.fail("Need at least 2 themes to test selection")

        # Pick a theme that is NOT the current selection
        new_theme = None
        for item in items:
            tid = _item_id(item)
            if tid and tid != current:
                new_theme = tid
                break

        if new_theme is None:
            pytest.fail("Could not find an alternative theme")

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
            pytest.fail("Theme dropdown not registered")

        _, _, items = _get_theme_info(c)
        if not items:
            pytest.fail("No theme items to test")

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
            pytest.fail("Options section not registered")
        client.click(options_id)

        dropdown_id = "sidebar_options_themeDropdown"
        if not client.element_exists(dropdown_id):
            client.close_editor()
            pytest.fail("Theme dropdown not registered")

        current, num, items = _get_theme_info(client)
        if num is None or num < 2:
            client.close_editor()
            pytest.fail("Need at least 2 themes")

        # Select a non-default theme
        target = None
        for item in items:
            tid = _item_id(item)
            if tid and tid != current:
                target = tid
                break
        if not target:
            client.close_editor()
            pytest.fail("No alternative theme found")

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


class TestThemeDuringPlayback:
    """Theme interactions while audio is active."""

    def test_theme_change_during_active_rendering(
        self, options_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: theme change while waveforms are actively rendering
        causes render pipeline to use stale color references.
        """
        c = options_section
        osc_id = c.add_oscillator(source_id, name="ThemePlayback")
        assert osc_id is not None
        c.wait_for_oscillator_count(1, timeout_s=3.0)

        c.transport_play()
        c.wait_until(lambda: c.is_playing(), timeout_s=2.0, desc="transport playing")
        c.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        c.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            c.transport_stop()
            pytest.fail("Theme dropdown not registered")

        _, _, items = _get_theme_info(c)
        if len(items) < 2:
            c.transport_stop()
            pytest.fail("Need 2+ themes")

        # Change theme during playback
        tid = _item_id(items[-1])
        c.select_dropdown_item(dropdown_id, tid)

        # Verify still playing and stable
        assert c.is_playing(), "Transport should survive theme change"
        oscs = c.get_oscillators()
        assert len(oscs) == 1, "Oscillator should survive theme change"

        c.transport_stop()

    def test_rapid_theme_switching_stability(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: rapidly cycling themes causes resource leak or crash
        due to theme resources not being released before new theme loads.
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.fail("Theme dropdown not registered")

        _, _, items = _get_theme_info(c)
        if len(items) < 2:
            pytest.fail("Need 2+ themes")

        # Cycle through all themes 3 times
        for _ in range(3):
            for item in items:
                tid = _item_id(item)
                if tid:
                    c.select_dropdown_item(dropdown_id, tid)

        # System should be stable
        state = c.get_transport_state()
        assert state is not None, "Harness should survive rapid theme cycling"

    def test_theme_survives_state_save_load(
        self, options_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: theme selection not included in state XML, reverting
        to default after save/load roundtrip.
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.fail("Theme dropdown not registered")

        current, _, items = _get_theme_info(c)
        if len(items) < 2:
            pytest.fail("Need 2+ themes")

        # Select non-default theme
        target = None
        for item in items:
            tid = _item_id(item)
            if tid and tid != current:
                target = tid
                break
        if not target:
            pytest.fail("No alternative theme")

        c.select_dropdown_item(dropdown_id, target)
        c.wait_until(
            lambda: _get_theme_info(c)[0] == target,
            timeout_s=2.0, desc="theme applied",
        )

        # Need an oscillator for state save
        osc_id = c.add_oscillator(source_id, name="ThemeSaveLoad")
        c.wait_for_oscillator_count(1, timeout_s=3.0)

        path = "/tmp/oscil_e2e_theme_save.xml"
        saved = c.save_state(path)
        if not saved:
            pytest.fail("State save not available")

        c.reset_state()
        c.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = c.load_state(path)
        assert loaded, "State load should succeed"
        c.wait_for_oscillator_count(1, timeout_s=5.0)

        # Re-expand options to check theme
        options_id = "sidebar_options"
        if c.element_exists(options_id):
            c.click(options_id)

        restored, _, _ = _get_theme_info(c)
        if restored:
            assert restored == target, (
                f"Theme should survive save/load: expected '{target}', got '{restored}'"
            )

    def test_theme_dropdown_item_count_nonzero(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: theme dropdown reports numItems=0 even though
        ThemeManager has themes registered.
        """
        c = options_section
        dropdown_id = "sidebar_options_themeDropdown"
        if not c.element_exists(dropdown_id):
            pytest.fail("Theme dropdown not registered")

        el = c.get_element(dropdown_id)
        assert el is not None
        num_items = el.extra.get("numItems", 0)
        items = el.extra.get("items", [])
        assert num_items > 0, f"numItems should be > 0, got {num_items}"
        assert len(items) > 0, f"items list should be non-empty"
        assert len(items) == num_items, (
            f"items count ({len(items)}) should match numItems ({num_items})"
        )
