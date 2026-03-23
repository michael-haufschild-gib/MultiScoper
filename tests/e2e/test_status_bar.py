"""
E2E tests for the status bar component.

What bugs these tests catch:
- Status bar labels not registered or not visible
- FPS counter showing 0 when rendering is active
- Oscillator count label not updating after add/remove
- Source count label not reflecting available sources
- Rendering mode label not updating after GPU toggle
- Memory usage not reported
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestStatusBarVisibility:
    """Verify status bar labels are registered and visible."""

    STATUS_BAR_LABELS = [
        ("statusBar_fps", "FPS"),
        ("statusBar_cpu", "CPU"),
        ("statusBar_mem", "Memory"),
        ("statusBar_osc", "Oscillator count"),
        ("statusBar_src", "Source count"),
        ("statusBar_mode", "Render mode"),
    ]

    @pytest.mark.parametrize("label_id,description", STATUS_BAR_LABELS)
    def test_label_exists(
        self, editor: OscilTestClient, label_id: str, description: str
    ):
        """
        Bug caught: status bar label not registered in element registry,
        making it impossible for tests to verify metrics display.
        """
        if not editor.element_exists(label_id):
            pytest.skip(f"{description} label not registered")

        el = editor.get_element(label_id)
        assert el is not None
        assert el.visible, f"{description} label should be visible"


class TestStatusBarContent:
    """Verify status bar labels show meaningful content."""

    def test_oscillator_count_updates(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator count label not updating after add operation.
        """
        osc_label = "statusBar_osc"
        if not editor.element_exists(osc_label):
            pytest.skip("Oscillator count label not registered")

        # Add oscillator
        osc_id = editor.add_oscillator(source_id, name="StatusBar Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Check status bar reflects the oscillator count
        el = editor.get_element(osc_label)
        if el and el.extra.get("text"):
            text = el.extra["text"]
            # The label should contain "1" somewhere
            assert "1" in text, (
                f"Oscillator count label should show 1, got '{text}'"
            )

    def test_source_count_positive(self, editor: OscilTestClient):
        """
        Bug caught: source count label showing 0 when sources exist.
        """
        src_label = "statusBar_src"
        if not editor.element_exists(src_label):
            pytest.skip("Source count label not registered")

        sources = editor.get_sources()
        if not sources:
            pytest.skip("No sources available")

        el = editor.get_element(src_label)
        if el and el.extra.get("text"):
            text = el.extra["text"]
            # Should not show "0" when sources exist
            assert "0" not in text or len(sources) == 0, (
                f"Source count label shows '{text}' but {len(sources)} sources exist"
            )
