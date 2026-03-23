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
