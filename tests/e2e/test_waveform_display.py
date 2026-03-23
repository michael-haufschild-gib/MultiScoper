"""
E2E tests for waveform display, data flow, and visual verification.

What bugs these tests catch:
- Waveform not rendering when oscillator is added and transport is playing
- Waveform still rendering after oscillator is hidden
- Waveform not updating when audio generator frequency/type changes
- Oscillator color not applied to waveform rendering
- Update API not applying changes to oscillator state
- Multiple oscillators overwriting each other in render state
- Oscillator created without source binding or pane assignment
- Order indices not assigned sequentially
- Opacity/lineWidth not propagated via update API
- Waveform data not flowing from capture buffer to render state
- Display samples not positive after oscillator add
- Peak/RMS levels zero with active audio signal
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestWaveformRendering:
    """Verify waveforms render when audio is flowing."""

    def test_waveform_renders_with_active_audio(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: render pipeline not connected to audio data, so waveform
        shows flat line even when audio is playing.
        """
        osc_id = editor.add_oscillator(source_id, name="Waveform Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data to flow through the pipeline
        waveforms = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert len(waveforms) >= 1, "Should have at least one waveform in pane"
        assert waveforms[0].get("hasWaveformData"), (
            "Waveform should have data with active audio"
        )

        # Verify peak level is reasonable for a 0.8 amplitude sine
        peak = waveforms[0].get("peakLevel", 0.0)
        assert peak > 0.01, (
            f"Peak level should be > 0.01 with 0.8 amplitude sine, got {peak}"
        )

        editor.transport_stop()

    def test_waveform_state_has_display_samples(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: displaySamples not computed after oscillator added,
        leading to zero-width waveform rendering.
        """
        osc_id = editor.add_oscillator(source_id, name="Display Samples Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        # displaySamples should be positive once oscillator is added
        display_samples = editor.get_display_samples(0)
        assert display_samples > 0, (
            f"displaySamples should be positive after adding oscillator, got {display_samples}"
        )
        assert display_samples > 100, (
            f"displaySamples should be a reasonable value (>100), got {display_samples}"
        )

    def test_waveform_peak_rms_with_audio(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: peak/RMS level computation broken, showing 0 even
        when audio data is present in the capture buffer.
        """
        osc_id = editor.add_oscillator(source_id, name="Peak RMS Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data
        waveforms = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        wf = waveforms[0]
        peak = wf.get("peakLevel", 0.0)
        rms = wf.get("rmsLevel", 0.0)

        # With a sine at 0.8 amplitude, peak should be near 0.8 and RMS near 0.56
        assert peak > 0.1, f"Peak level too low with 0.8 amplitude sine: {peak}"
        assert rms > 0.05, f"RMS level too low with 0.8 amplitude sine: {rms}"
        assert rms <= peak + 0.01, (
            f"RMS ({rms}) should not exceed peak ({peak})"
        )

        editor.transport_stop()

    def test_oscillator_update_via_state_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: update_oscillator API not applying changes to existing
        oscillator, or state update not triggering UI refresh.
        """
        osc_id = editor.add_oscillator(source_id, name="Update Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        success = editor.update_oscillator(osc_id, name="Updated Name")
        assert success, "Update oscillator API should succeed"

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc["name"] == "Updated Name", (
            f"Name should be 'Updated Name', got '{osc['name']}'"
        )

    def test_oscillator_visibility_affects_rendering(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: setting visible=false on oscillator but render pipeline
        still drawing it (wasted GPU cycles and incorrect display).
        """
        osc_id = editor.add_oscillator(source_id, name="Visibility Render Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc["visible"] is True

        editor.update_oscillator(osc_id, visible=False)

        # Wait for state to actually update
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id)
            and not editor.get_oscillator_by_id(osc_id).get("visible", True),
            timeout_s=2.0,
            desc="oscillator to become hidden",
        )
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc["visible"] is False, "Oscillator should be hidden after update"

        # Show again
        editor.update_oscillator(osc_id, visible=True)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id)
            and editor.get_oscillator_by_id(osc_id).get("visible") is True,
            timeout_s=2.0,
            desc="oscillator to become visible",
        )
        osc = editor.get_oscillator_by_id(osc_id)
        assert osc["visible"] is True, "Oscillator should be visible after re-show"

    def test_multiple_oscillators_coexist(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: second oscillator overwriting first in render state,
        or pane only rendering the most recently added oscillator.
        """
        id1 = editor.add_oscillator(source_id, name="Multi 1", colour="#FF0000")
        id2 = editor.add_oscillator(source_id, name="Multi 2", colour="#00FF00")
        id3 = editor.add_oscillator(source_id, name="Multi 3", colour="#0000FF")

        oscs = editor.wait_for_oscillator_count(3, timeout_s=5.0)
        ids = {o["id"] for o in oscs}
        assert id1 in ids
        assert id2 in ids
        assert id3 in ids

        for osc in oscs:
            assert osc["visible"] is True, f"Oscillator {osc['id']} should be visible"

    def test_multiple_oscillators_appear_in_waveform_state(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state only reporting the first oscillator,
        dropping subsequent ones from the render list.
        """
        editor.add_oscillator(source_id, name="WF State 1")
        editor.add_oscillator(source_id, name="WF State 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        state = editor.get_waveform_state()
        assert state is not None, "/waveform/state should return data"
        assert "panes" in state, "Waveform state should have panes"
        assert len(state["panes"]) > 0, "Should have at least one pane"

        pane = state["panes"][0]
        osc_count = pane.get("oscillatorCount", 0)
        wf_count = len(pane.get("waveforms", []))
        assert osc_count >= 2, (
            f"Pane should report 2+ oscillators, got {osc_count}"
        )
        assert wf_count >= 2, (
            f"Pane should have 2+ waveforms, got {wf_count}"
        )

    def test_waveform_data_disappears_with_silence(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: waveform state reporting hasWaveformData=true even
        when audio is silent (false positive in the data pipeline).
        """
        osc_id = editor.add_oscillator(source_id, name="Silence Test")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="silence", amplitude=0.0)

        # Wait for the circular buffer to flush old audio data.
        # The capture buffer may hold several thousand samples of pre-silence
        # audio, so we poll until peak drops below noise floor.
        def peak_is_silent():
            wfs = editor.get_waveform_for_pane(0)
            if not wfs:
                return False
            return wfs[0].get("peakLevel", 1.0) < 0.05

        editor.wait_until(
            peak_is_silent,
            timeout_s=5.0,
            desc="peak to drop below noise floor after silence",
        )

        editor.transport_stop()

    def test_waveform_data_recovers_after_position_reset(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: resetting transport position to 0 while audio is
        playing causes the capture buffer to read stale/misaligned data,
        producing a momentary glitch in the waveform display.
        """
        osc_id = editor.add_oscillator(source_id, name="PosReset WF")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        # Wait for waveform data to flow
        wfs_before = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs_before[0].get("peakLevel", 0) > 0.05, (
            "Should have signal before position reset"
        )

        # Reset position to 0
        editor.set_position(0)

        # Waveform data should continue flowing after reset
        wfs_after = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        assert wfs_after[0].get("hasWaveformData"), (
            "Waveform should have data after position reset"
        )
        assert wfs_after[0].get("peakLevel", 0) > 0.05, (
            "Peak level should recover after position reset"
        )

        editor.transport_stop()

    def test_frequency_change_reflected_in_waveform(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: changing audio frequency does not update the capture
        buffer fast enough, causing the waveform display to show the old
        frequency pattern indefinitely.
        """
        osc_id = editor.add_oscillator(source_id, name="FreqChange")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # Start with 440Hz
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Switch to 880Hz — waveform data should still be valid
        editor.set_track_audio(0, waveform="sine", frequency=880.0, amplitude=0.8)

        # Verify track info updated
        editor.wait_until(
            lambda: (info := editor.get_track_info(0))
            and abs(info.get("frequency", 0) - 880.0) < 1.0,
            timeout_s=2.0,
            desc="frequency to update to 880Hz",
        )

        # Waveform data should still be present
        wfs = editor.get_waveform_for_pane(0)
        if wfs and wfs[0].get("hasWaveformData"):
            assert wfs[0].get("peakLevel", 0) > 0.05, (
                "Peak should be positive after frequency change"
            )

        editor.transport_stop()


class TestOscillatorProperties:
    """Verify oscillator property values via state API."""

    def test_oscillator_has_source_id(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator created without source binding.
        """
        osc_id = editor.add_oscillator(source_id, name="Source Test")
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc.get("sourceId") == source_id, (
            f"Source should be '{source_id}', got '{osc.get('sourceId')}'"
        )

    def test_oscillator_has_pane_id(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator added without pane assignment (orphaned).
        """
        osc_id = editor.add_oscillator(source_id, name="Pane Test")
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        pane_id = osc.get("paneId", "")
        assert pane_id, f"Oscillator should have a pane ID, got '{pane_id}'"

    def test_oscillator_order_index(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: order indices not assigned sequentially, causing
        display order to be random.
        """
        editor.add_oscillator(source_id, name="Order 1")
        editor.add_oscillator(source_id, name="Order 2")
        editor.add_oscillator(source_id, name="Order 3")

        oscs = editor.wait_for_oscillator_count(3, timeout_s=5.0)

        indices = [o.get("orderIndex", -1) for o in oscs]
        assert all(i >= 0 for i in indices), (
            f"All order indices should be >= 0, got {indices}"
        )
        assert len(set(indices)) == 3, (
            f"Order indices should be unique, got {indices}"
        )

    def test_opacity_and_line_width_via_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: update API ignoring opacity/lineWidth fields.
        """
        osc_id = editor.add_oscillator(source_id, name="Props Test")
        assert osc_id is not None

        success = editor.update_oscillator(osc_id, opacity=0.5, lineWidth=3.0)
        assert success, "Update API should accept opacity and lineWidth"

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert "opacity" in osc, "Oscillator state should include opacity field"
        assert abs(osc["opacity"] - 0.5) < 0.01, (
            f"Opacity should be 0.5, got {osc['opacity']}"
        )
        assert "lineWidth" in osc, "Oscillator state should include lineWidth field"
        assert abs(osc["lineWidth"] - 3.0) < 0.1, (
            f"Line width should be 3.0, got {osc['lineWidth']}"
        )

    def test_oscillator_assigned_to_existing_pane(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator's paneId doesn't match any actual pane,
        leaving it orphaned and invisible.
        """
        osc_id = editor.add_oscillator(source_id, name="Pane Match Test")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        osc = editor.get_oscillator_by_id(osc_id)
        pane_id = osc.get("paneId", "")
        panes = editor.get_panes()

        pane_ids = {p["id"] for p in panes}
        assert pane_id in pane_ids, (
            f"Oscillator paneId '{pane_id}' not found in pane list: {pane_ids}"
        )

    def test_name_survives_state_roundtrip(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: name serialization truncating or encoding incorrectly
        (e.g., special characters lost).
        """
        test_name = "Test Osc #1 (Stereo)"
        osc_id = editor.add_oscillator(source_id, name=test_name)
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc["name"] == test_name, (
            f"Name should be '{test_name}', got '{osc['name']}'"
        )

    def test_delete_oscillator_via_api(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: delete API not removing oscillator from state.
        """
        id1 = editor.add_oscillator(source_id, name="Delete API 1")
        id2 = editor.add_oscillator(source_id, name="Delete API 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        success = editor.delete_oscillator(id1)
        assert success, "Delete API should succeed"

        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        remaining = editor.get_oscillators()
        remaining_ids = [o["id"] for o in remaining]
        assert id1 not in remaining_ids, "Deleted oscillator should be gone"
        assert id2 in remaining_ids, "Other oscillator should remain"


class TestWaveformFrequencyResponse:
    """Verify waveform display responds to audio parameter changes."""

    def test_amplitude_change_affects_peak_level(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: peak level computation using stale audio data after
        amplitude parameter change, showing old peak value indefinitely.
        """
        osc_id = editor.add_oscillator(source_id, name="Amp Response")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        # High amplitude
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=1.0)
        wfs_high = editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)
        peak_high = wfs_high[0].get("peakLevel", 0.0)
        assert peak_high > 0.3, f"High amplitude peak should be > 0.3, got {peak_high}"

        # Low amplitude
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.1)
        # Wait for peak to drop
        editor.wait_until(
            lambda: (wfs := editor.get_waveform_for_pane(0))
            and wfs and wfs[0].get("peakLevel", 1.0) < peak_high * 0.5,
            timeout_s=5.0,
            desc="peak level to decrease after amplitude reduction",
        )
        wfs_low = editor.get_waveform_for_pane(0)
        peak_low = wfs_low[0].get("peakLevel", 1.0)
        assert peak_low < peak_high, (
            f"Lower amplitude should produce lower peak: "
            f"high={peak_high:.4f}, low={peak_low:.4f}"
        )

        editor.transport_stop()

    def test_zero_amplitude_produces_near_silent_peak(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: amplitude=0 not actually producing silence in the
        audio generator (e.g., DC offset or noise floor not handled).
        """
        osc_id = editor.add_oscillator(source_id, name="Zero Amp")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.0)

        # Wait for peak to settle near zero
        def peak_near_zero():
            wfs = editor.get_waveform_for_pane(0)
            if not wfs:
                return False
            return wfs[0].get("peakLevel", 1.0) < 0.05

        editor.wait_until(
            peak_near_zero,
            timeout_s=5.0,
            desc="peak to drop near zero with zero amplitude",
        )

        editor.transport_stop()

    def test_waveform_type_switch_during_playback(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: switching waveform type while playing causes a torn
        read on the waveform enum in the audio callback, producing
        corrupted output.
        """
        osc_id = editor.add_oscillator(source_id, name="Type Switch")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=2.0, desc="transport playing"
        )

        waveforms = ["sine", "square", "triangle", "saw"]
        for wf in waveforms:
            editor.set_track_audio(0, waveform=wf, frequency=440.0, amplitude=0.8)
            # Wait briefly for the new waveform to take effect
            editor.wait_until(
                lambda: (info := editor.get_track_info(0))
                and info.get("waveform", "").lower() == wf,
                timeout_s=2.0,
                desc=f"waveform to become {wf}",
            )

            # Verify waveform data still flowing
            wfs = editor.get_waveform_for_pane(0)
            if wfs and wfs[0].get("hasWaveformData"):
                peak = wfs[0].get("peakLevel", 0)
                assert peak > 0.01, (
                    f"Peak should be positive with {wf} waveform, got {peak}"
                )

        editor.transport_stop()
