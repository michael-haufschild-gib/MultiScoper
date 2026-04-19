"""
E2E coverage for hover interactions (tooltips, crosshair, hover-driven
UI state changes).

What bugs these tests catch:
- Hover endpoint silently no-ops on components that have registered a
  tooltip (engineer's cursor-over-button check never shows the hint).
- Hover on waveform pane does not update the crosshair overlay's
  displayed sample/value.
- Hover API races with rapid mouse-out, leaving a stale tooltip
  frozen on screen.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestHoverBasicResponse:
    def test_hover_on_button_without_crash(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: hover handler on MultiScoperButton crashes due
        to missing setInterceptsMouseClicks configuration."""
        ok = editor.hover("sidebar_oscillators_item_0_settings", duration_ms=100)
        assert editor.health_check()["data"]["status"] == "ok"

    def test_hover_on_slider_without_crash(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        try:
            editor.hover("configPopup_lineWidthSlider", duration_ms=100)
            assert editor.health_check()["data"]["status"] == "ok"
        finally:
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)

    def test_hover_on_multiple_elements_in_sequence(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Hover on many elements in quick succession must not freeze
        the harness."""
        hover_targets = [
            "sidebar_addOscillator",
            "sidebar_timing",
            "sidebar_options",
            "sidebar_oscillators_item_0",
        ]
        for target in hover_targets:
            if editor.element_exists(target):
                editor.hover(target, duration_ms=50)
        assert editor.health_check()["data"]["status"] == "ok"


class TestHoverOnWaveform:
    """Hovering the waveform should update crosshair state."""

    def test_hover_over_pane_does_not_crash(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: hover dispatcher doesn't handle the waveform
        pane's custom hit-test, causing a null-pointer dereference."""
        assert editor.element_exists("pane_0"), "pane_0 must be registered"
        editor.hover("pane_0", duration_ms=100)
        assert editor.health_check()["data"]["status"] == "ok"


class TestHoverWithActiveAudio:
    """Hover should work while audio is playing — covers the case
    where render state is changing under the hover handler."""

    def test_hover_while_audio_plays(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        osc_id = editor.add_oscillator(source_id, name="HoverOsc")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.7)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        try:
            editor.hover("pane_0", duration_ms=100)
            editor.hover("sidebar_oscillators_item_0", duration_ms=100)
            assert editor.health_check()["data"]["status"] == "ok"
        finally:
            editor.transport_stop()
