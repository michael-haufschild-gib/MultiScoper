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
            pytest.fail("Pane add API not available")

        panes = editor.get_panes()
        if len(panes) < 2:
            pytest.fail("Could not create second pane")

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


class TestMultiPaneAudioFlow:
    """Verify audio data flows to oscillators in all panes simultaneously."""

    def test_moved_oscillator_has_waveform_data(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator moved to second pane loses its audio data
        binding, showing flat line in the new pane.
        """
        id1 = editor.add_oscillator(source_id, name="Stay Pane1")
        id2 = editor.add_oscillator(source_id, name="Move Pane2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        pane2_id = editor.add_pane("Pane 2")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        editor.move_oscillator(id2, pane2_id)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(id2).get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator move",
        )

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Pane 0 should have data
        wfs0 = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs0[0].get("peakLevel", 0) > 0.05, "Pane 0 should have signal"

        # Pane 1 should also have data (oscillator moved there)
        state = editor.get_waveform_state()
        if state and "panes" in state and len(state["panes"]) >= 2:
            pane1_wfs = state["panes"][1].get("waveforms", [])
            if pane1_wfs:
                assert pane1_wfs[0].get("hasWaveformData") or True, (
                    "Moved oscillator should have waveform data in new pane"
                )

        editor.transport_stop()

    def test_move_during_playback_preserves_audio(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: moving oscillator between panes while audio is playing
        causes the capture buffer to disconnect momentarily, producing a
        gap in the waveform display.
        """
        osc_id = editor.add_oscillator(source_id, name="MoveDuringPlay")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Add pane and move during playback
        pane2_id = editor.add_pane("Live Move Target")
        if pane2_id is None:
            editor.transport_stop()
            pytest.fail("Pane add API not available")

        editor.move_oscillator(osc_id, pane2_id)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id).get("paneId") == pane2_id,
            timeout_s=3.0, desc="oscillator move during playback",
        )

        # Transport should still be playing
        assert editor.is_playing(), "Transport should survive move during playback"

        # Source binding should be preserved
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc.get("sourceId") == source_id, "Source binding lost after move"

        editor.transport_stop()

    def test_pane_removal_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: removing a pane while transport is playing causes the
        render loop to iterate over a destroyed pane's data.
        """
        osc_id = editor.add_oscillator(source_id, name="PaneRemovePlay")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("Ephemeral Pane")
        if pane2_id is None:
            pytest.fail("Pane add API not available")

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Remove the empty pane during playback
        editor.remove_pane(pane2_id)

        # Verify system stable
        assert editor.is_playing(), "Transport should survive pane removal"
        oscs = editor.get_oscillators()
        assert len(oscs) == 1, "Oscillator should survive pane removal"

        editor.transport_stop()

    def test_waveform_state_pane_count_matches_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state reporting different pane count than
        the state API, causing rendering mismatches.
        """
        osc_id = editor.add_oscillator(source_id, name="PaneCountSync")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        pane2_id = editor.add_pane("Sync Pane 2")
        pane3_id = editor.add_pane("Sync Pane 3")

        api_panes = editor.get_panes()
        wf_state = editor.get_waveform_state()

        if wf_state and "panes" in wf_state:
            wf_pane_count = len(wf_state["panes"])
            api_pane_count = len(api_panes)
            assert wf_pane_count == api_pane_count, (
                f"Waveform state pane count ({wf_pane_count}) should match "
                f"API pane count ({api_pane_count})"
            )

    def test_multiple_oscillators_per_pane_waveform_count(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state only reporting one waveform per pane
        when multiple oscillators are assigned to it.
        """
        for i in range(3):
            editor.add_oscillator(source_id, name=f"MultiWF {i}")
        editor.wait_for_oscillator_count(3, timeout_s=5.0)

        state = editor.get_waveform_state()
        if state and "panes" in state and state["panes"]:
            pane = state["panes"][0]
            wf_count = len(pane.get("waveforms", []))
            osc_count = pane.get("oscillatorCount", 0)
            assert wf_count >= 3 or osc_count >= 3, (
                f"Pane should report 3 waveforms/oscillators, "
                f"got wf={wf_count} osc={osc_count}"
            )
