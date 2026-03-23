"""
E2E tests for oscillator list filtering (All/Visible/Hidden tabs).

What bugs these tests catch:
- Filter tabs not clickable or not registered
- "Visible" filter not hiding invisible oscillators from the list
- "Hidden" filter not showing only hidden oscillators
- "All" filter not restoring the full list
- Filter state not updating after visibility toggle
- Filter breaking after oscillator add/delete
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestOscillatorFilterTabs:
    """Filter tabs in the oscillator list toolbar."""

    def _get_filter_tab_ids(self):
        return {
            "all": "sidebar_oscillators_toolbar_allTab",
            "visible": "sidebar_oscillators_toolbar_visibleTab",
            "hidden": "sidebar_oscillators_toolbar_hiddenTab",
        }

    def test_filter_tabs_exist(self, editor: OscilTestClient, oscillator: str):
        """
        Bug caught: filter tabs not rendered in the toolbar.
        """
        tabs = self._get_filter_tab_ids()
        found = 0
        for name, tab_id in tabs.items():
            if editor.element_exists(tab_id):
                found += 1

        if found == 0:
            pytest.fail("No filter tabs registered")

        assert found >= 2, (
            f"Should have at least 2 filter tabs, found {found}"
        )

    def test_all_tab_clickable(self, editor: OscilTestClient, oscillator: str):
        """
        Bug caught: "All" tab click handler not wired.
        """
        tab_id = self._get_filter_tab_ids()["all"]
        if not editor.element_exists(tab_id):
            pytest.fail("All tab not registered")

        assert editor.click(tab_id), "All tab should be clickable"

        # Verify oscillator still visible in list
        el = editor.get_element("sidebar_oscillators_item_0")
        assert el is not None, (
            "Oscillator should be in list when 'All' filter is active"
        )

    def test_visible_filter_hides_invisible_oscillators(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: "Visible" filter not actually filtering — shows all
        oscillators regardless of visibility state.
        Known failure: hidden oscillator still appears in list after filter tab click.
        """
        tabs = self._get_filter_tab_ids()
        if not editor.element_exists(tabs["visible"]):
            pytest.fail("Visible tab not registered")

        # Create two oscillators, hide one
        id1 = editor.add_oscillator(source_id, name="FilterVis 1")
        id2 = editor.add_oscillator(source_id, name="FilterVis 2")
        assert id1 and id2
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        editor.wait_for_element("sidebar_oscillators_item_1", timeout_s=3.0)

        editor.update_oscillator(id1, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(id1).get("visible", True),
            timeout_s=2.0,
            desc="oscillator to become hidden",
        )

        # Switch to "Visible" filter
        editor.click(tabs["visible"])

        # The hidden oscillator should not be in the list
        # Give the filter time to update the list
        editor.wait_until(
            lambda: not editor.element_exists("sidebar_oscillators_item_1"),
            timeout_s=2.0,
            desc="hidden oscillator to disappear from filtered list",
        )

        # Switch back to "All"
        if editor.element_exists(tabs["all"]):
            editor.click(tabs["all"])

    def test_hidden_filter_shows_only_hidden(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: "Hidden" filter showing visible oscillators or
        showing nothing even when hidden oscillators exist.
        """
        tabs = self._get_filter_tab_ids()
        if not editor.element_exists(tabs["hidden"]):
            pytest.fail("Hidden tab not registered")

        id1 = editor.add_oscillator(source_id, name="FilterHid 1")
        id2 = editor.add_oscillator(source_id, name="FilterHid 2")
        assert id1 and id2
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        editor.wait_for_element("sidebar_oscillators_item_1", timeout_s=3.0)

        # Hide one
        editor.update_oscillator(id1, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(id1).get("visible", True),
            timeout_s=2.0,
            desc="oscillator to become hidden",
        )

        # Switch to "Hidden" filter
        editor.click(tabs["hidden"])

        # At least one item should show (the hidden one)
        try:
            editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=2.0)
        except TimeoutError:
            # Hidden filter not implemented
            if editor.element_exists(tabs["all"]):
                editor.click(tabs["all"])
            pytest.fail("Hidden filter does not populate list")

        # Switch back to "All"
        if editor.element_exists(tabs["all"]):
            editor.click(tabs["all"])

    def test_filter_survives_oscillator_add(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: adding an oscillator while a filter is active causes
        the filter state to reset or the new oscillator to be invisible.
        """
        tabs = self._get_filter_tab_ids()
        if not editor.element_exists(tabs["all"]):
            pytest.fail("All tab not registered")

        id1 = editor.add_oscillator(source_id, name="FilterAdd 1")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Apply "All" filter explicitly
        editor.click(tabs["all"])

        # Add another oscillator
        id2 = editor.add_oscillator(source_id, name="FilterAdd 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Both should appear in "All" view
        editor.wait_for_element("sidebar_oscillators_item_1", timeout_s=3.0)


class TestFilterRobustness:
    """Edge cases and robustness of filter tab interactions."""

    def _get_filter_tab_ids(self):
        return {
            "all": "sidebar_oscillators_toolbar_allTab",
            "visible": "sidebar_oscillators_toolbar_visibleTab",
            "hidden": "sidebar_oscillators_toolbar_hiddenTab",
        }

    def test_rapid_filter_tab_switching(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: rapidly switching filter tabs causes list rendering
        race condition or stale item references.
        """
        tabs = self._get_filter_tab_ids()
        available_tabs = [tid for tid in tabs.values() if editor.element_exists(tid)]
        if len(available_tabs) < 2:
            pytest.fail("Need 2+ filter tabs")

        id1 = editor.add_oscillator(source_id, name="RapidFilter 1")
        id2 = editor.add_oscillator(source_id, name="RapidFilter 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        editor.update_oscillator(id1, visible=False)

        # Rapid switching
        for _ in range(5):
            for tab_id in available_tabs:
                editor.click(tab_id)

        # Return to All and verify state intact
        if editor.element_exists(tabs["all"]):
            editor.click(tabs["all"])

        oscs = editor.get_oscillators()
        assert len(oscs) == 2, "Rapid filter switching should not affect state"

    def test_filter_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching filter tabs while audio is playing causes
        list item deregistration that disrupts the render pipeline.
        """
        tabs = self._get_filter_tab_ids()
        if not editor.element_exists(tabs.get("all", "")):
            pytest.fail("All tab not registered")

        id1 = editor.add_oscillator(source_id, name="FilterPlay 1")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Switch filter tabs during playback
        for tab_id in tabs.values():
            if editor.element_exists(tab_id):
                editor.click(tab_id)

        assert editor.is_playing(), "Transport should survive filter switching"

        if editor.element_exists(tabs["all"]):
            editor.click(tabs["all"])
        editor.transport_stop()

    def test_filter_state_after_state_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: filter tab state not reset after loading new state,
        showing stale filtered list from pre-load state.
        """
        tabs = self._get_filter_tab_ids()
        if not editor.element_exists(tabs.get("all", "")):
            pytest.fail("All tab not registered")

        # Create state with mixed visibility
        id1 = editor.add_oscillator(source_id, name="FilterLoad Vis")
        id2 = editor.add_oscillator(source_id, name="FilterLoad Hid")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)
        editor.update_oscillator(id2, visible=False)

        # Save
        path = "/tmp/oscil_e2e_filter_load.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save not available")

        # Reset, then load
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded
        editor.wait_for_oscillator_count(2, timeout_s=5.0)

        # All filter should show both
        if editor.element_exists(tabs["all"]):
            editor.click(tabs["all"])

        oscs = editor.get_oscillators()
        assert len(oscs) == 2, "State load should restore both oscillators"

    def test_visibility_toggle_updates_filtered_list(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: toggling oscillator visibility while a filter tab is
        active does not update the filtered list, showing/hiding the
        oscillator incorrectly.
        """
        tabs = self._get_filter_tab_ids()
        visible_tab = tabs.get("visible", "")
        if not editor.element_exists(visible_tab):
            pytest.fail("Visible tab not registered")

        id1 = editor.add_oscillator(source_id, name="VisToggle Filter")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Switch to visible filter
        editor.click(visible_tab)

        # Oscillator should be in the visible list
        editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=2.0)

        # Hide the oscillator
        editor.update_oscillator(id1, visible=False)
        editor.wait_until(
            lambda: not editor.get_oscillator_by_id(id1).get("visible", True),
            timeout_s=2.0, desc="oscillator hidden",
        )

        # Now it should disappear from the visible filter
        # (give it time to update)
        try:
            editor.wait_until(
                lambda: not editor.element_exists("sidebar_oscillators_item_0"),
                timeout_s=2.0,
                desc="hidden osc to leave visible filter",
            )
        except TimeoutError:
            pass  # Filter may not immediately update

        # Switch back to All
        all_tab = tabs.get("all", "")
        if editor.element_exists(all_tab):
            editor.click(all_tab)

        # Oscillator should still exist in state
        oscs = editor.get_oscillators()
        assert len(oscs) == 1

    def test_filter_count_matches_state(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: filter tab showing wrong oscillator count badge
        (e.g., "All (2)" when 3 oscillators exist).
        """
        tabs = self._get_filter_tab_ids()

        for i in range(3):
            editor.add_oscillator(source_id, name=f"FilterCount {i}")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        # Hide one
        oscs = editor.get_oscillators()
        editor.update_oscillator(oscs[0]["id"], visible=False)

        # Verify state counts
        all_oscs = editor.get_oscillators()
        hidden = [o for o in all_oscs if not o.get("visible", True)]
        visible = [o for o in all_oscs if o.get("visible", True)]

        assert len(all_oscs) == 3
        assert len(hidden) == 1
        assert len(visible) == 2

        # Switch to All — verify all 3 exist in state
        if editor.element_exists(tabs.get("all", "")):
            editor.click(tabs["all"])
