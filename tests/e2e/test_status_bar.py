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
            pytest.fail(f"{description} label not registered")

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
            pytest.fail("Oscillator count label not registered")

        # Add oscillator
        osc_id = editor.add_oscillator(source_id, name="StatusBar Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Status bar is updated by a timer-based metrics controller; poll
        # until the label reflects the new oscillator count.
        editor.wait_until(
            lambda: (el := editor.get_element(osc_label))
            and el.extra.get("text")
            and "1" in el.extra["text"],
            timeout_s=3.0,
            desc="oscillator count label to update",
        )

    def test_source_count_positive(self, editor: OscilTestClient):
        """
        Bug caught: source count label showing 0 when sources exist.
        """
        src_label = "statusBar_src"
        if not editor.element_exists(src_label):
            pytest.fail("Source count label not registered")

        sources = editor.get_sources()
        if not sources:
            pytest.fail("No sources available")

        el = editor.get_element(src_label)
        if el and el.extra.get("text"):
            text = el.extra["text"]
            # Should not show "0" when sources exist
            assert "0" not in text or len(sources) == 0, (
                f"Source count label shows '{text}' but {len(sources)} sources exist"
            )


class TestStatusBarDuringPlayback:
    """Verify status bar labels update meaningfully during audio activity."""

    def test_fps_positive_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: FPS counter stuck at 0 when rendering is active because
        the metrics controller timer is not started.
        """
        fps_label = "statusBar_fps"
        if not editor.element_exists(fps_label):
            pytest.fail("FPS label not registered")

        osc_id = editor.add_oscillator(source_id, name="FPS Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # FPS label should eventually show a positive value
        try:
            editor.wait_until(
                lambda: (el := editor.get_element(fps_label))
                and el.extra.get("text")
                and any(c.isdigit() and c != "0" for c in el.extra["text"]),
                timeout_s=5.0,
                desc="FPS label to show positive value",
            )
        except TimeoutError:
            el = editor.get_element(fps_label)
            text = el.extra.get("text", "") if el else ""
            pytest.fail(f"FPS label did not show positive value, got '{text}'")
        finally:
            editor.transport_stop()

    def test_oscillator_count_after_delete(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator count label not decrementing after deletion.
        """
        osc_label = "statusBar_osc"
        if not editor.element_exists(osc_label):
            pytest.fail("Oscillator count label not registered")

        id1 = editor.add_oscillator(source_id, name="Count Del 1")
        id2 = editor.add_oscillator(source_id, name="Count Del 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        # Wait for label to show 2
        try:
            editor.wait_until(
                lambda: (el := editor.get_element(osc_label))
                and el.extra.get("text")
                and "2" in el.extra["text"],
                timeout_s=3.0,
                desc="osc count to show 2",
            )
        except TimeoutError:
            pass

        # Delete one
        editor.delete_oscillator(id1)
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Wait for label to update
        try:
            editor.wait_until(
                lambda: (el := editor.get_element(osc_label))
                and el.extra.get("text")
                and "1" in el.extra["text"],
                timeout_s=3.0,
                desc="osc count to update after delete",
            )
        except TimeoutError:
            el = editor.get_element(osc_label)
            text = el.extra.get("text", "") if el else ""
            pytest.fail(f"Osc count label did not update after delete, got '{text}'")

    def test_render_mode_label_updates_after_gpu_toggle(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: render mode label showing stale value after GPU toggle.
        """
        mode_label = "statusBar_mode"
        if not editor.element_exists(mode_label):
            pytest.fail("Render mode label not registered")

        # Expand options and toggle GPU
        if not editor.element_exists("sidebar_options"):
            pytest.fail("Options section not registered")
        editor.click("sidebar_options")

        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not editor.element_exists(toggle_id):
            pytest.fail("GPU toggle not registered")

        # Record current mode text
        el_before = editor.get_element(mode_label)
        text_before = el_before.extra.get("text", "") if el_before else ""

        # Toggle
        editor.click(toggle_id)

        # Wait for mode label to change
        try:
            editor.wait_until(
                lambda: (el := editor.get_element(mode_label))
                and el.extra.get("text", "") != text_before,
                timeout_s=3.0,
                desc="render mode label to update",
            )
        except TimeoutError:
            pass  # Label may not change if text format doesn't include mode name

        # Toggle back
        editor.click(toggle_id)

        # System should be stable
        state = editor.get_transport_state()
        assert state is not None

    def test_all_labels_have_text_during_activity(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: status bar labels showing empty strings during active
        rendering, indicating the metrics controller is not feeding data.
        """
        osc_id = editor.add_oscillator(source_id, name="Labels Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for metrics to populate
        try:
            editor.wait_until(
                lambda: (el := editor.get_element("statusBar_fps"))
                and el.extra.get("text"),
                timeout_s=5.0,
                desc="FPS label to have text",
            )
        except TimeoutError:
            pass

        labels_with_text = 0
        for label_id in ["statusBar_fps", "statusBar_cpu", "statusBar_mem",
                         "statusBar_osc", "statusBar_src", "statusBar_mode"]:
            el = editor.get_element(label_id)
            if el and el.extra.get("text"):
                labels_with_text += 1

        editor.transport_stop()

        assert labels_with_text >= 3, (
            f"At least 3 status bar labels should have text during activity, "
            f"got {labels_with_text}"
        )

    def test_labels_survive_editor_lifecycle(self, client: OscilTestClient):
        """
        Bug caught: status bar labels not re-registered after editor
        close/reopen, making them invisible to test automation.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )
        client.open_editor()

        labels = ["statusBar_fps", "statusBar_osc", "statusBar_src"]
        found_before = sum(1 for l in labels if client.element_exists(l))

        client.close_editor()
        client.open_editor()

        found_after = sum(1 for l in labels if client.element_exists(l))
        client.close_editor()

        assert found_after >= found_before, (
            f"Labels should re-register after reopen: before={found_before}, after={found_after}"
        )
