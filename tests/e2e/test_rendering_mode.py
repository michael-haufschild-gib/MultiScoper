"""
E2E tests for GPU/Software rendering mode toggle.

What bugs these tests catch:
- GPU toggle not registered in test harness
- Toggle click not switching rendering backend
- Toggle state not persisting across editor close/reopen
- Switching rendering mode while oscillators are displayed causing crash
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestRenderingModeToggle:
    """GPU Acceleration toggle in Options section."""

    def test_toggle_exists_and_visible(self, options_section: OscilTestClient):
        """
        Bug caught: GPU toggle removed from options section layout.
        """
        c = options_section
        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not c.element_exists(toggle_id):
            pytest.fail("GPU rendering toggle not registered")

        el = c.get_element(toggle_id)
        assert el is not None
        assert el.visible, "GPU toggle should be visible when options expanded"

    def test_toggle_click_does_not_crash(self, options_section: OscilTestClient):
        """
        Bug caught: toggling GPU mode dereferences null OpenGL context.
        """
        c = options_section
        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not c.element_exists(toggle_id):
            pytest.fail("GPU rendering toggle not registered")

        # Click toggle (change mode)
        assert c.click(toggle_id), "Should be able to click GPU toggle"

        # Verify harness is still responsive
        state = c.get_transport_state()
        assert state is not None, "Harness should still be responsive after GPU toggle"

        # Click again (restore)
        assert c.click(toggle_id), "Should be able to toggle back"

        state = c.get_transport_state()
        assert state is not None, "Harness should still be responsive after restore"

    def test_toggle_with_oscillators_present(
        self, options_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching renderer while waveforms are actively drawing
        causes null pointer or use-after-free in render pipeline.
        """
        c = options_section

        # Add an oscillator so there's active rendering
        osc_id = c.add_oscillator(source_id, name="Render Toggle Test")
        if not osc_id:
            pytest.fail("Cannot add oscillator")

        # Ensure transport is playing for active rendering
        c.transport_play()
        c.wait_until(lambda: c.is_playing(), timeout_s=2.0, desc="transport playing")

        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not c.element_exists(toggle_id):
            c.transport_stop()
            pytest.fail("GPU rendering toggle not registered")

        # Toggle while rendering
        c.click(toggle_id)

        # Verify system is stable
        oscs = c.get_oscillators()
        assert len(oscs) >= 1, "Oscillators should still exist after toggle"
        assert c.is_playing(), "Transport should still be playing"

        # Toggle back
        c.click(toggle_id)

        c.transport_stop()

    def test_waveform_data_survives_gpu_toggle(
        self, options_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: toggling GPU mode during active audio causes the
        waveform render pipeline to lose its audio data binding — the
        oscillator exists and transport is playing, but the waveform
        shows a flat line because the new renderer was not connected
        to the capture buffer.
        """
        c = options_section

        osc_id = c.add_oscillator(source_id, name="WF GPU Toggle")
        assert osc_id is not None, "Must be able to add oscillator"
        c.wait_for_oscillator_count(1, timeout_s=3.0)

        c.transport_play()
        c.wait_until(lambda: c.is_playing(), timeout_s=2.0, desc="transport playing")
        c.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data to flow
        wfs_before = c.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        peak_before = wfs_before[0].get("peakLevel", 0)
        assert peak_before > 0.05, (
            f"Should have signal before GPU toggle, got peak={peak_before}"
        )

        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not c.element_exists(toggle_id):
            c.transport_stop()
            pytest.fail("GPU rendering toggle not registered")

        # Toggle GPU mode
        c.click(toggle_id)

        # After mode switch, waveform data must still flow.
        # The capture buffer is independent of the renderer, so
        # hasWaveformData and peakLevel should remain valid.
        wfs_after = c.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs_after[0].get("hasWaveformData"), (
            "Waveform data must survive GPU mode toggle"
        )
        peak_after = wfs_after[0].get("peakLevel", 0)
        assert peak_after > 0.05, (
            f"Peak level must remain positive after GPU toggle, got {peak_after}"
        )

        # Toggle back and verify again
        c.click(toggle_id)
        wfs_restored = c.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs_restored[0].get("peakLevel", 0) > 0.05, (
            "Peak level must remain positive after toggling GPU mode back"
        )

        c.transport_stop()

    def test_toggle_persists_across_editor_lifecycle(self, client: OscilTestClient):
        """
        Bug caught: rendering mode preference not serialized.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )
        client.open_editor()

        options_id = "sidebar_options"
        if not client.element_exists(options_id):
            client.close_editor()
            pytest.fail("Options section not registered")
        client.click(options_id)

        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not client.element_exists(toggle_id):
            client.close_editor()
            pytest.fail("GPU rendering toggle not registered")

        # Record initial toggle state
        el_before = client.get_element(toggle_id)
        toggled_before = el_before.extra.get("toggled", el_before.extra.get("value")) if el_before else None

        # Toggle once
        client.click(toggle_id)

        # Wait for toggle state to change
        try:
            client.wait_until(
                lambda: (e := client.get_element(toggle_id))
                and e.extra.get("toggled", e.extra.get("value")) != toggled_before,
                timeout_s=2.0,
                desc="toggle state to change",
            )
        except TimeoutError:
            pass  # Toggle may not report state in extra

        el_after_toggle = client.get_element(toggle_id)
        toggled_after_toggle = (
            el_after_toggle.extra.get("toggled", el_after_toggle.extra.get("value"))
            if el_after_toggle else None
        )

        # Close and reopen
        client.close_editor()
        client.open_editor()

        if client.element_exists(options_id):
            client.click(options_id)

        el_after_reopen = client.get_element(toggle_id)
        client.close_editor()

        assert el_after_reopen is not None, "Toggle should exist after reopen"

        # If toggle state is available, verify it persisted
        if toggled_after_toggle is not None and el_after_reopen:
            toggled_after_reopen = el_after_reopen.extra.get(
                "toggled", el_after_reopen.extra.get("value")
            )
            if toggled_after_reopen is not None:
                assert toggled_after_reopen == toggled_after_toggle, (
                    f"Toggle state should persist: "
                    f"expected {toggled_after_toggle}, got {toggled_after_reopen}"
                )

    def test_toggle_rapid_cycling_stability(self, options_section: OscilTestClient):
        """
        Bug caught: rapid GPU/Software toggling causing OpenGL context
        race condition or resource leak.
        """
        c = options_section
        toggle_id = "sidebar_options_gpuRenderingToggle"
        if not c.element_exists(toggle_id):
            pytest.fail("GPU rendering toggle not registered")

        for _ in range(6):
            c.click(toggle_id)

        # Verify system stable
        state = c.get_transport_state()
        assert state is not None, "Harness should survive rapid GPU toggle cycling"
        oscs = c.get_oscillators()
        assert isinstance(oscs, list), "State should be queryable after rapid toggles"
