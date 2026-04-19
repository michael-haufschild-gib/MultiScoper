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
from multiscoper_test_utils import MultiScoperTestClient
from page_objects import SidebarPage


class TestAccordion:
    """Accordion section expand/collapse behavior."""

    def test_timing_section_expand_reveals_content(
        self, editor: MultiScoperTestClient, sidebar_page: SidebarPage
    ):
        """
        Bug caught: accordion click handler not toggling content visibility.
        """
        sidebar_page.expand_timing()

        # Look for a content element that should appear
        content_elements = [
            "sidebar_timing_modeToggle",
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

    def test_timing_section_collapse_hides_content(
        self, editor: MultiScoperTestClient, sidebar_page: SidebarPage
    ):
        """
        Bug caught: accordion not collapsing on second click.
        PRODUCTION BUG: accordion child elements remain visible/registered
        after collapse click — setVisible(false) not called on children.
        """
        # Ensure expanded
        sidebar_page.expand_timing()
        content_id = None
        for eid in ["sidebar_timing_modeToggle", "sidebar_timing_intervalField"]:
            if editor.element_exists(eid):
                content_id = eid
                break
        if content_id is None:
            pytest.fail("No timing content element found after expand")

        editor.wait_for_visible(content_id, timeout_s=2.0)

        # Collapse
        sidebar_page.collapse_timing()
        editor.wait_for_not_visible(content_id, timeout_s=2.0)

    def test_options_section_expand(
        self, editor: MultiScoperTestClient, sidebar_page: SidebarPage
    ):
        """
        Bug caught: options section not wired to accordion.
        """
        sidebar_page.expand_options()

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
        self, editor: MultiScoperTestClient, two_oscillators, sidebar_page: SidebarPage
    ):
        """
        Bug caught: click handler not setting selection state, or expanded
        height calculation wrong.
        """
        # Force item 0 to unselected first by selecting item 1 so the test's
        # "click should expand" assertion has a meaningful starting state.
        # Otherwise item 0 might already be selected (fixture-dependent) and
        # the click is a no-op.
        sidebar_page.select_oscillator(1)

        item0 = sidebar_page.item_id(0)
        el_before = editor.get_element(item0)
        assert el_before is not None, "List item 0 must be registered"

        height_before = el_before.height

        sidebar_page.select_oscillator(0)
        # Old version caught TimeoutError and silently passed, then asserted
        # only "element still exists" — allowing any regression in the
        # expand-on-select wiring to slip through. Require that item 0's
        # height actually grows after the click. If the UI does not expand
        # selected items (product decision drift), this test SHOULD fail
        # and the docstring above needs updating accordingly.
        editor.wait_until(
            lambda: (e := editor.get_element(item0)) and e.height > height_before,
            timeout_s=2.0,
            desc="item 0 to expand after selection",
        )

        el_after = editor.get_element(item0)
        assert el_after is not None
        assert el_after.height > height_before, (
            f"Selected item 0 should be taller than when unselected; "
            f"before={height_before}, after={el_after.height}"
        )

    def test_selecting_different_item_switches_expansion(
        self, editor: MultiScoperTestClient, two_oscillators, sidebar_page: SidebarPage
    ):
        """
        Bug caught: multi-select not deselecting previous item — both rows
        remain expanded at their selected height. The old version only
        asserted "item 1 is visible", which was true even in the broken
        state and couldn't detect the bug it claimed to cover.
        """
        sidebar_page.select_oscillator(0)
        editor.wait_until(
            lambda: (e := editor.get_element(sidebar_page.item_id(0))) and e.height > 0,
            timeout_s=2.0,
            desc="item 0 to settle after selection",
        )
        item0_selected_height = editor.get_element(sidebar_page.item_id(0)).height

        sidebar_page.select_oscillator(1)
        editor.wait_until(
            lambda: (e := editor.get_element(sidebar_page.item_id(1)))
            and e.height >= item0_selected_height,
            timeout_s=2.0,
            desc="item 1 to reach selected height after switching",
        )

        el0 = editor.get_element(sidebar_page.item_id(0))
        el1 = editor.get_element(sidebar_page.item_id(1))
        assert el0 is not None and el1 is not None and el1.visible
        # Item 1 is now selected, so it MUST be the tall one. Item 0 must
        # have collapsed back — if both are at the selected height, the
        # list is multi-expanding (the actual bug).
        assert el1.height >= item0_selected_height, (
            f"Newly-selected item 1 must be expanded (got {el1.height}); "
            f"previously-selected item 0 was {item0_selected_height}"
        )
        assert el0.height < item0_selected_height, (
            f"Previously-selected item 0 must collapse when item 1 is selected; "
            f"still at height {el0.height} (selected height was {item0_selected_height})"
        )


class TestListItemButtons:
    """Verify list item action buttons are present and sized correctly."""

    BUTTONS = [
        ("sidebar_oscillators_item_0_delete", "Delete"),
        ("sidebar_oscillators_item_0_settings", "Settings"),
        ("sidebar_oscillators_item_0_vis_btn", "Visibility"),
    ]

    @pytest.mark.parametrize("btn_id,label", BUTTONS)
    def test_button_exists_and_has_size(
        self, editor: MultiScoperTestClient, oscillator: str, btn_id: str, label: str
    ):
        """
        Bug caught: button not rendered, or has zero width/height due to
        layout calculation error.
        """
        # Delete and Settings buttons are core -- they MUST exist.
        # Visibility button is also expected but may have a variant name.
        if btn_id.endswith("_vis_btn") and not editor.element_exists(btn_id):
            alt_id = btn_id.replace("_vis_btn", "_vis_toggle")
            if editor.element_exists(alt_id):
                btn_id = alt_id
            else:
                pytest.fail(f"{label} button not registered (tried _vis_btn and _vis_toggle)")
        elif not editor.element_exists(btn_id):
            assert False, f"{label} button '{btn_id}' must be registered"

        el = editor.get_element(btn_id)
        assert el is not None
        assert el.visible, f"{label} button must be visible"
        assert el.width > 0, f"{label} button has zero width"
        assert el.height > 0, f"{label} button has zero height"


class TestSidebarResize:
    """Sidebar resize via drag handle."""

    def test_drag_changes_width(self, editor: MultiScoperTestClient):
        """
        Bug caught: resize handle not wired, or drag delta not applied to
        sidebar width constraint.
        """
        handle_id = "sidebar_resizeHandle"
        if not editor.element_exists(handle_id):
            pytest.fail("Resize handle not registered")

        sidebar_before = editor.get_element("sidebar")
        assert sidebar_before is not None, "Sidebar element must be registered"
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
            pytest.fail("Sidebar width did not change -- resize may not be supported")

        sidebar_after = editor.get_element("sidebar")
        assert sidebar_after.width != width_before, (
            f"Expected width to change from {width_before}"
        )


class TestOscillatorReorder:
    """Drag-to-reorder oscillator list items."""

    def test_reorder_via_api(self, editor: MultiScoperTestClient, two_oscillators):
        """
        Bug caught: reorder API not updating order indices, or UI not
        reflecting new order after drag.
        """
        oscs_before = editor.get_oscillators()
        id_order_before = [o["id"] for o in oscs_before]

        success = editor.reorder_oscillators(0, 1)
        if not success:
            pytest.fail("Reorder API not available")

        oscs_after = editor.get_oscillators()
        id_order_after = [o["id"] for o in oscs_after]

        assert id_order_after != id_order_before, (
            "Oscillator order should change after reorder"
        )
        assert set(id_order_after) == set(id_order_before), (
            "Reorder should not add or remove oscillators"
        )
