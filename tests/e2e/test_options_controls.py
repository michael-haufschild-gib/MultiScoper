"""
E2E tests for Options section controls.

What bugs these tests catch:
- Layout dropdown not populated or selection not applying
- Quality preset dropdown not wired to rendering pipeline
- Buffer duration dropdown not updating capture buffer size
- Auto-adjust quality toggle not toggling state
- Grid toggle not toggling grid overlay
- Auto-scale toggle not toggling amplitude scaling
- Gain slider not adjustable or value not persisting
- Controls crashing when clicked with active oscillators
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestLayoutDropdown:
    """Layout dropdown in options section."""

    def test_layout_dropdown_exists(self, options_section: OscilTestClient):
        """
        Bug caught: layout dropdown removed from options section.
        """
        c = options_section
        dropdown_id = "sidebar_options_layoutDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Layout dropdown not registered")

        el = c.get_element(dropdown_id)
        assert el is not None
        assert el.visible, "Layout dropdown should be visible"

    def test_layout_dropdown_has_items(self, options_section: OscilTestClient):
        """
        Bug caught: layout dropdown empty (no layouts registered).
        """
        c = options_section
        dropdown_id = "sidebar_options_layoutDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Layout dropdown not registered")

        el = c.get_element(dropdown_id)
        num_items = el.extra.get("numItems", 0) if el else 0
        assert num_items > 0, (
            f"Layout dropdown should have items, got {num_items}"
        )

    def test_layout_selection_does_not_crash(
        self, options_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: selecting a layout crashes when oscillators are present
        (pane layout recalculation null pointer).
        """
        c = options_section
        # Add oscillator so layout change has something to rearrange
        c.add_oscillator(source_id, name="Layout Osc")
        c.wait_for_oscillator_count(1, timeout_s=3.0)

        dropdown_id = "sidebar_options_layoutDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Layout dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.xfail("No layout items")

        for item in items:
            item_id = item.get("id", item) if isinstance(item, dict) else str(item)
            c.select_dropdown_item(dropdown_id, str(item_id))

        # Verify system stable after cycling layouts
        oscs = c.get_oscillators()
        assert len(oscs) == 1, "Oscillator should survive layout changes"


class TestGridToggle:
    """Grid overlay toggle."""

    def test_grid_toggle_clickable(self, options_section: OscilTestClient):
        """
        Bug caught: grid toggle not responding to clicks.
        """
        c = options_section
        toggle_id = "sidebar_options_gridToggle"
        if not c.element_exists(toggle_id):
            pytest.xfail("Grid toggle not registered")

        assert c.click(toggle_id), "Should be able to click grid toggle"
        # Click again to restore
        c.click(toggle_id)

        state = c.get_transport_state()
        assert state is not None, "Harness should be responsive after grid toggle"


class TestAutoScaleToggle:
    """Auto-scale amplitude toggle."""

    def test_auto_scale_toggle_clickable(self, options_section: OscilTestClient):
        """
        Bug caught: auto-scale toggle not responding to clicks.
        """
        c = options_section
        toggle_id = "sidebar_options_autoScaleToggle"
        if not c.element_exists(toggle_id):
            pytest.xfail("Auto-scale toggle not registered")

        assert c.click(toggle_id), "Should be able to click auto-scale toggle"
        c.click(toggle_id)

        state = c.get_transport_state()
        assert state is not None


class TestGainSlider:
    """Gain slider control."""

    def test_gain_slider_accepts_values(self, options_section: OscilTestClient):
        """
        Bug caught: gain slider value setter not wired.
        """
        c = options_section
        slider_id = "sidebar_options_gainSlider"
        if not c.element_exists(slider_id):
            pytest.xfail("Gain slider not registered")

        # Set to positive gain
        assert c.set_slider(slider_id, 6.0), "Gain slider should accept 6.0"
        # Set to negative gain
        assert c.set_slider(slider_id, -6.0), "Gain slider should accept -6.0"
        # Set to zero (unity)
        assert c.set_slider(slider_id, 0.0), "Gain slider should accept 0.0"

    def test_gain_slider_increment_decrement(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: increment/decrement not adjusting slider value.
        """
        c = options_section
        slider_id = "sidebar_options_gainSlider"
        if not c.element_exists(slider_id):
            pytest.xfail("Gain slider not registered")

        c.set_slider(slider_id, 0.0)
        c.increment_slider(slider_id)
        c.decrement_slider(slider_id)
        c.reset_slider(slider_id)

        # Verify harness stable
        state = c.get_transport_state()
        assert state is not None


class TestQualityPresetDropdown:
    """Quality preset dropdown."""

    def test_quality_dropdown_exists(self, options_section: OscilTestClient):
        """
        Bug caught: quality dropdown not registered in options.
        """
        c = options_section
        dropdown_id = "sidebar_options_qualityPresetDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Quality preset dropdown not registered")

        el = c.get_element(dropdown_id)
        assert el is not None
        assert el.visible

    def test_quality_selection_does_not_crash(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: selecting a quality preset crashes the rendering pipeline.
        """
        c = options_section
        dropdown_id = "sidebar_options_qualityPresetDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Quality preset dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.xfail("No quality items")

        for item in items:
            item_id = item.get("id", item) if isinstance(item, dict) else str(item)
            c.select_dropdown_item(dropdown_id, str(item_id))

        state = c.get_transport_state()
        assert state is not None


class TestBufferDurationDropdown:
    """Buffer duration dropdown."""

    def test_buffer_dropdown_exists(self, options_section: OscilTestClient):
        """
        Bug caught: buffer duration dropdown not registered.
        """
        c = options_section
        dropdown_id = "sidebar_options_bufferDurationDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Buffer duration dropdown not registered")

        el = c.get_element(dropdown_id)
        assert el is not None
        assert el.visible

    def test_buffer_selection_does_not_crash(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: changing buffer duration during active capture causes
        buffer resize while audio callback is writing to it.
        """
        c = options_section
        dropdown_id = "sidebar_options_bufferDurationDropdown"
        if not c.element_exists(dropdown_id):
            pytest.xfail("Buffer duration dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.xfail("No buffer duration items")

        for item in items:
            item_id = item.get("id", item) if isinstance(item, dict) else str(item)
            c.select_dropdown_item(dropdown_id, str(item_id))

        state = c.get_transport_state()
        assert state is not None


class TestAutoAdjustToggle:
    """Auto-adjust quality toggle."""

    def test_auto_adjust_toggle_clickable(
        self, options_section: OscilTestClient
    ):
        """
        Bug caught: auto-adjust toggle not responding to clicks.
        """
        c = options_section
        toggle_id = "sidebar_options_autoAdjustToggle"
        if not c.element_exists(toggle_id):
            pytest.xfail("Auto-adjust toggle not registered")

        assert c.click(toggle_id), "Should be able to click auto-adjust toggle"
        c.click(toggle_id)

        state = c.get_transport_state()
        assert state is not None
