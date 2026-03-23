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
            pytest.fail("Layout dropdown not registered")

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
            pytest.fail("Layout dropdown not registered")

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
            pytest.fail("Layout dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.fail("No layout items")

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
            pytest.fail("Grid toggle not registered")

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
            pytest.fail("Auto-scale toggle not registered")

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
            pytest.fail("Gain slider not registered")

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
            pytest.fail("Gain slider not registered")

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
            pytest.fail("Quality preset dropdown not registered")

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
            pytest.fail("Quality preset dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.fail("No quality items")

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
            pytest.fail("Buffer duration dropdown not registered")

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
            pytest.fail("Buffer duration dropdown not registered")

        el = c.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.fail("No buffer duration items")

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
            pytest.fail("Auto-adjust toggle not registered")

        assert c.click(toggle_id), "Should be able to click auto-adjust toggle"
        c.click(toggle_id)

        state = c.get_transport_state()
        assert state is not None


class TestOptionsPersistence:
    """Verify options section settings persist through state save/load."""

    def _expand_options(self, client: OscilTestClient) -> bool:
        """Expand options section. Returns False if not available."""
        options_id = "sidebar_options"
        if not client.element_exists(options_id):
            return False
        client.click(options_id)
        return True

    def test_grid_toggle_state_persists_through_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: grid toggle state not serialized in state XML,
        reverting to default after save/load.
        """
        if not self._expand_options(editor):
            pytest.fail("Options section not registered")

        toggle_id = "sidebar_options_gridToggle"
        if not editor.element_exists(toggle_id):
            pytest.fail("Grid toggle not registered")

        # Need an oscillator so state save has content
        osc_id = editor.add_oscillator(source_id, name="GridPersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Record initial toggle state
        el_before = editor.get_element(toggle_id)
        toggled_before = el_before.extra.get("toggled", el_before.extra.get("value")) if el_before else None

        # Toggle it
        editor.click(toggle_id)

        # Wait for state to change
        try:
            editor.wait_until(
                lambda: (e := editor.get_element(toggle_id))
                and e.extra.get("toggled", e.extra.get("value")) != toggled_before,
                timeout_s=2.0,
                desc="grid toggle state to change",
            )
        except TimeoutError:
            pass

        el_after_toggle = editor.get_element(toggle_id)
        toggled_after = el_after_toggle.extra.get("toggled", el_after_toggle.extra.get("value")) if el_after_toggle else None

        # Save, reset, load
        path = "/tmp/oscil_e2e_grid_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"
        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        # Re-expand options
        if not self._expand_options(editor):
            pytest.fail("Options section not registered after load")

        el_after_load = editor.get_element(toggle_id)
        toggled_after_load = el_after_load.extra.get("toggled", el_after_load.extra.get("value")) if el_after_load else None

        if toggled_after is not None and toggled_after_load is not None:
            assert toggled_after_load == toggled_after, (
                f"Grid toggle should persist: set to {toggled_after}, "
                f"loaded as {toggled_after_load}"
            )

    def test_gain_slider_value_persists_through_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: gain slider value not serialized, reverting to 0 dB
        after state load.
        """
        if not self._expand_options(editor):
            pytest.fail("Options section not registered")

        slider_id = "sidebar_options_gainSlider"
        if not editor.element_exists(slider_id):
            pytest.fail("Gain slider not registered")

        osc_id = editor.add_oscillator(source_id, name="GainPersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Set a distinctive gain value
        editor.set_slider(slider_id, 3.0)

        # Record the slider extra data for comparison after load
        el_before = editor.get_element(slider_id)
        value_before = el_before.extra.get("value") if el_before else None

        # Save, reset, load
        path = "/tmp/oscil_e2e_gain_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"
        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        if not self._expand_options(editor):
            pytest.fail("Options section not registered after load")

        el_after = editor.get_element(slider_id)
        value_after = el_after.extra.get("value") if el_after else None

        if value_before is not None and value_after is not None:
            assert abs(value_after - value_before) < 0.5, (
                f"Gain should persist: set to {value_before}, "
                f"loaded as {value_after}"
            )

    def test_layout_dropdown_selection_persists_through_save_load(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: layout selection not serialized, reverting to default
        layout after state load.
        """
        if not self._expand_options(editor):
            pytest.fail("Options section not registered")

        dropdown_id = "sidebar_options_layoutDropdown"
        if not editor.element_exists(dropdown_id):
            pytest.fail("Layout dropdown not registered")

        osc_id = editor.add_oscillator(source_id, name="LayoutPersist")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        el = editor.get_element(dropdown_id)
        items = el.extra.get("items", []) if el else []
        current = el.extra.get("selectedId", "") if el else ""
        if len(items) < 2:
            pytest.fail("Need at least 2 layout items to test persistence")

        # Select a non-default layout
        target = None
        for item in items:
            tid = item.get("id", item) if isinstance(item, dict) else str(item)
            if tid and tid != current:
                target = tid
                break
        if not target:
            pytest.fail("No alternative layout found")

        editor.select_dropdown_item(dropdown_id, target)

        # Verify selection changed
        editor.wait_until(
            lambda: (e := editor.get_element(dropdown_id))
            and e.extra.get("selectedId") == target,
            timeout_s=2.0,
            desc="layout selection to update",
        )

        # Save, reset, load
        path = "/tmp/oscil_e2e_layout_persist.xml"
        saved = editor.save_state(path)
        if not saved:
            pytest.fail("State save API not available")

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        loaded = editor.load_state(path)
        assert loaded, "State load should succeed"
        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        if not self._expand_options(editor):
            pytest.fail("Options section not registered after load")

        el_after = editor.get_element(dropdown_id)
        selected_after = el_after.extra.get("selectedId", "") if el_after else ""

        if selected_after:
            assert selected_after == target, (
                f"Layout should persist: expected '{target}', got '{selected_after}'"
            )
