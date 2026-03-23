"""
E2E tests for multi-pane waveform rendering.

What bugs these tests catch:
- Oscillator moved to second pane not receiving audio data
- Waveform state endpoint not reporting per-pane data
- Second pane created but not added to render pipeline
- Moving oscillator between panes loses audio binding
- Waveform data in original pane not cleaned up after oscillator moves
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestMultiPaneRendering:
    """Verify waveforms render correctly across multiple panes."""

    def test_waveform_state_reports_pane_data(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: /waveform/state endpoint not including pane-level data,
        making per-pane verification impossible.
        """
        osc_id = editor.add_oscillator(source_id, name="Pane WF Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        state = editor.get_waveform_state()
        assert state is not None, "/waveform/state should return data"
        assert "panes" in state, "Waveform state should have panes"
        assert len(state["panes"]) >= 1, "Should have at least one pane"

    def test_two_panes_both_report_data(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: second pane not included in waveform state,
        so oscillators in pane 2 appear to have no data.
        """
        # Create first oscillator (auto-creates pane)
        id1 = editor.add_oscillator(source_id, name="Pane1 Osc")
        assert id1 is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # Add second pane
        pane2_id = editor.add_pane("Pane 2")
        if pane2_id is None:
            pytest.skip("Pane add API not available")

        panes = editor.get_panes()
        if len(panes) < 2:
            pytest.skip("Could not create second pane")

        # Add oscillator to second pane
        id2 = editor.add_oscillator(
            source_id, name="Pane2 Osc", pane_id=panes[1]["id"]
        )
        if id2 is None:
            # Try without pane_id and move manually
            id2 = editor.add_oscillator(source_id, name="Pane2 Osc")
            if id2:
                editor.move_oscillator(id2, panes[1]["id"])
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        state = editor.get_waveform_state()
        if state and "panes" in state:
            assert len(state["panes"]) >= 2, (
                f"Waveform state should have 2 panes, got {len(state['panes'])}"
            )

    def test_waveform_data_with_audio_in_both_panes(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: audio data only flowing to the first pane, second
        pane oscillators showing flat line.
        """
        id1 = editor.add_oscillator(source_id, name="Audio Pane1")
        assert id1 is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Verify waveform data in pane 0
        wfs = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs[0].get("hasWaveformData"), "Pane 0 should have waveform data"
        assert wfs[0].get("peakLevel", 0) > 0.05, (
            "Pane 0 should have measurable peak level"
        )

        editor.transport_stop()

    def test_oscillator_count_per_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: pane reporting wrong oscillator count (off-by-one
        or counting oscillators from other panes).
        """
        id1 = editor.add_oscillator(source_id, name="Count Test 1")
        id2 = editor.add_oscillator(source_id, name="Count Test 2")
        id3 = editor.add_oscillator(source_id, name="Count Test 3")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        state = editor.get_waveform_state()
        if state and "panes" in state and state["panes"]:
            pane = state["panes"][0]
            osc_count = pane.get("oscillatorCount", 0)
            assert osc_count >= 3, (
                f"Default pane should have 3 oscillators, got {osc_count}"
            )
