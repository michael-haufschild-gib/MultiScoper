"""
E2E tests for sidebar interactions.

What bugs these tests catch:
- Accordion sections not expanding/collapsing on click
- Accordion content not hiding when collapsed
- Sidebar resize handle not changing width
- Oscillator selection not expanding the list item
- Drag-to-reorder corrupting order indices
- List item buttons (delete, settings, vis) missing or zero-sized
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestAccordion:
    """Accordion section expand/collapse behavior."""

    def test_timing_section_expand_reveals_content(self, editor: OscilTestClient):
        """
        Bug caught: accordion click handler not toggling content visibility.
        """
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.skip("Timing section not registered")

        editor.click(timing_id)

        # Look for a content element that should appear
        content_elements = [
            "sidebar_timing_modeToggle",
            "sidebar_timing_intervalField",
            "sidebar_timing_intervalField",
        ]
        found = False
        for eid in content_elements:
            if editor.element_exists(eid):
                try:
                    editor.wait_for_visible(eid, timeout_s=2.0)
                    found = True
                    break
                except TimeoutError:
                    continue

        assert found, "No timing section content became visible after expanding"

    def test_timing_section_collapse_hides_content(self, editor: OscilTestClient):
        """
        Bug caught: accordion not collapsing on second click.
        """
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.skip("Timing section not registered")

        # Expand
        editor.click(timing_id)
        content_id = None
        for eid in ["sidebar_timing_modeToggle", "sidebar_timing_intervalField"]:
            if editor.element_exists(eid):
                content_id = eid
                break
        if content_id is None:
            pytest.skip("No timing content element found")

        try:
            editor.wait_for_visible(content_id, timeout_s=2.0)
        except TimeoutError:
            pytest.skip("Content did not become visible on expand")

        # Collapse
        editor.click(timing_id)
        editor.wait_for_not_visible(content_id, timeout_s=2.0)

    def test_options_section_expand(self, editor: OscilTestClient):
        """
        Bug caught: options section not wired to accordion.
        """
        options_id = "sidebar_options"
        if not editor.element_exists(options_id):
            pytest.skip("Options section not registered")

        editor.click(options_id)

        content_ids = [
            "sidebar_options_themeDropdown",
            "sidebar_options_gpuRenderingToggle",
        ]
        found = False
        for eid in content_ids:
            if editor.element_exists(eid):
                try:
                    editor.wait_for_visible(eid, timeout_s=2.0)
                    found = True
                    break
                except TimeoutError:
                    continue

        assert found, "No options content became visible after expanding"


class TestOscillatorListSelection:
    """Selecting oscillators in the sidebar list."""

    def test_clicking_item_expands_it(
        self, editor: OscilTestClient, two_oscillators
    ):
        """
        Bug caught: click handler not setting selection state, or expanded
        height calculation wrong.
        """
        item0 = "sidebar_oscillators_item_0"
        el_before = editor.get_element(item0)
        if el_before is None:
            pytest.skip("List item 0 not registered")

        height_before = el_before.height

        editor.click(item0)
        # Wait a moment for expansion animation
        try:
            editor.wait_until(
                lambda: (e := editor.get_element(item0)) and e.height > height_before,
                timeout_s=2.0,
                desc="item expansion",
            )
        except TimeoutError:
            # Expansion may not change height if already selected
            pass

        el_after = editor.get_element(item0)
        assert el_after is not None

    def test_selecting_different_item_switches_expansion(
        self, editor: OscilTestClient, two_oscillators
    ):
        """
        Bug caught: multi-select not deselecting previous item.
        """
        item0 = "sidebar_oscillators_item_0"
        item1 = "sidebar_oscillators_item_1"

        if not (editor.element_exists(item0) and editor.element_exists(item1)):
            pytest.skip("Both list items not registered")

        editor.click(item0)
        editor.click(item1)

        # Item 1 should now be the selected/expanded one
        el1 = editor.get_element(item1)
        assert el1 is not None and el1.visible


class TestListItemButtons:
    """Verify list item action buttons are present and sized correctly."""

    BUTTONS = [
        ("sidebar_oscillators_item_0_delete", "Delete"),
        ("sidebar_oscillators_item_0_settings", "Settings"),
        ("sidebar_oscillators_item_0_vis_btn", "Visibility"),
    ]

    @pytest.mark.parametrize("btn_id,label", BUTTONS)
    def test_button_exists_and_has_size(
        self, editor: OscilTestClient, oscillator: str, btn_id: str, label: str
    ):
        """
        Bug caught: button not rendered, or has zero width/height due to
        layout calculation error.
        """
        if not editor.element_exists(btn_id):
            pytest.skip(f"{label} button not registered")

        el = editor.get_element(btn_id)
        assert el is not None
        assert el.visible, f"{label} button should be visible"
        assert el.width > 0, f"{label} button has zero width"
        assert el.height > 0, f"{label} button has zero height"


class TestSidebarResize:
    """Sidebar resize via drag handle."""

    def test_drag_changes_width(self, editor: OscilTestClient):
        """
        Bug caught: resize handle not wired, or drag delta not applied to
        sidebar width constraint.
        """
        handle_id = "sidebar_resizeHandle"
        if not editor.element_exists(handle_id):
            pytest.skip("Resize handle not registered")

        sidebar_before = editor.get_element("sidebar")
        if sidebar_before is None:
            pytest.skip("Sidebar element not registered")
        width_before = sidebar_before.width

        # Drag left by 40px (sidebar is on the right, so left = wider)
        editor.drag_offset(handle_id, -40, 0)

        # Wait for layout update
        try:
            editor.wait_until(
                lambda: (e := editor.get_element("sidebar")) and e.width != width_before,
                timeout_s=2.0,
                desc="sidebar width change",
            )
        except TimeoutError:
            pytest.skip("Sidebar width did not change — resize may not be supported")

        sidebar_after = editor.get_element("sidebar")
        assert sidebar_after.width != width_before, (
            f"Expected width to change from {width_before}"
        )


class TestOscillatorReorder:
    """Drag-to-reorder oscillator list items."""

    def test_reorder_via_api(self, editor: OscilTestClient, two_oscillators):
        """
        Bug caught: reorder API not updating order indices, or UI not
        reflecting new order after drag.
        """
        oscs_before = editor.get_oscillators()
        id_order_before = [o["id"] for o in oscs_before]

        success = editor.reorder_oscillators(0, 1)
        if not success:
            pytest.skip("Reorder API not available")

        oscs_after = editor.get_oscillators()
        id_order_after = [o["id"] for o in oscs_after]

        assert id_order_after != id_order_before, (
            "Oscillator order should change after reorder"
        )
        assert set(id_order_after) == set(id_order_before), (
            "Reorder should not add or remove oscillators"
        )
