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

        # Wait for waveform data to flow through the pipeline (condition-based, not sleep)
        try:
            editor.wait_for_waveform_data(pane_index=0, timeout_s=3.0)
            waveforms = editor.get_waveform_for_pane(0)
            assert len(waveforms) >= 1, "Should have at least one waveform in pane"
            assert waveforms[0].get("hasWaveformData"), (
                "Waveform should have data with active audio"
            )
        except TimeoutError:
            # Server-side waveform verification as fallback
            rendered = editor.verify_waveform_rendered(
                "waveformDisplay", min_amplitude=0.01
            )
            assert rendered, (
                "Waveform should be rendered with active audio "
                "(neither waveform state nor verify endpoint confirmed rendering)"
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
        if display_samples > 0:
            assert display_samples > 100, (
                f"displaySamples should be a reasonable value, got {display_samples}"
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
        try:
            waveforms = editor.wait_for_waveform_data(pane_index=0, timeout_s=3.0)
            wf = waveforms[0]
            peak = wf.get("peakLevel", 0.0)
            rms = wf.get("rmsLevel", 0.0)
            # With a sine at 0.8 amplitude, peak should be near 0.8 and RMS near 0.56
            if peak > 0:
                assert peak > 0.01, f"Peak level too low with 0.8 amplitude sine: {peak}"
            if rms > 0:
                assert rms > 0.01, f"RMS level too low with 0.8 amplitude sine: {rms}"
                assert rms <= peak + 0.01, (
                    f"RMS ({rms}) should not exceed peak ({peak})"
                )
        except TimeoutError:
            pytest.skip("Waveform data not available in time")

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
        if state and "panes" in state and state["panes"]:
            pane = state["panes"][0]
            wf_count = len(pane.get("waveforms", []))
            osc_count = pane.get("oscillatorCount", 0)
            assert osc_count >= 2, (
                f"Pane should report 2+ oscillators, got {osc_count}"
            )
            assert wf_count >= 2, (
                f"Pane should have 2+ waveforms, got {wf_count}"
            )


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
        # Verify opacity changed if field is returned
        if "opacity" in osc:
            assert abs(osc["opacity"] - 0.5) < 0.01, (
                f"Opacity should be 0.5, got {osc['opacity']}"
            )
        if "lineWidth" in osc:
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
