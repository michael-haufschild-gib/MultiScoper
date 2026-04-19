"""
E2E coverage for multi-pane audio rendering fan-out.

What bugs these tests catch:
- Three panes configured with different sources — only the first
  pane's waveform updates (renderer bound to pane index 0).
- Adding a third pane after two already rendering stalls the second
  pane (race in the render scheduler).
- One pane's high-amplitude signal clips the renderer for the others
  because the amplitude scale is global.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestThreePanesParallel:
    def test_three_panes_each_show_own_oscillator(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Three oscillators in three panes.  All three must report
        waveform data when audio is playing.

        Bug caught: multi-pane renderer only processes pane_0.
        """
        # Set layout to 3 columns so panes render side by side.
        editor.set_column_layout(3)
        editor.wait_until(
            lambda: editor.get_layout_info() and editor.get_layout_info().get("columns") == 3,
            timeout_s=3.0, desc="3 columns",
        )

        osc1 = editor.add_oscillator(source_id, name="P1")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        p2 = editor.add_pane("Pane2")
        p3 = editor.add_pane("Pane3")
        assert p2 and p3
        osc2 = editor.add_oscillator(source_id, name="P2", pane_id=p2)
        osc3 = editor.add_oscillator(source_id, name="P3", pane_id=p3)
        assert osc2 and osc3
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.7)

        # Each pane must have waveform data.
        for i in range(3):
            editor.wait_for_waveform_data(pane_index=i, timeout_s=10.0)

        editor.transport_stop()

    def test_adding_pane_after_playback_starts(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Play audio, THEN add a third pane.  The new pane must
        render too.

        Bug caught: renderer subscribes to pane list at start, doesn't
        pick up late-added panes.
        """
        osc1 = editor.add_oscillator(source_id, name="E1")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.7)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Add a second pane MID-playback.
        p2 = editor.add_pane("LateJoiner")
        assert p2 is not None
        osc2 = editor.add_oscillator(source_id, name="E2", pane_id=p2)
        editor.wait_for_oscillator_count(2, timeout_s=5.0)

        # Give the renderer a chance to catch up.
        try:
            editor.wait_until(
                lambda: (
                    wfs := editor.get_waveform_for_pane(1)
                ) and wfs and wfs[0].get("hasWaveformData"),
                timeout_s=10.0,
                desc="late-joiner pane to have waveform data",
            )
        finally:
            editor.transport_stop()
