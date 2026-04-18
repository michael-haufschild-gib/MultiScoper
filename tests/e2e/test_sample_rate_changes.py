"""
E2E coverage for mid-session sample-rate changes (Primitive 2.1).

Background
----------
Real DAWs reload the audio device at a new sample rate — 44.1k sessions
opened on a 48k interface, studio rigs sweeping between 44.1k and 192k
for mastering, etc.  `OscilPluginProcessor::prepareToPlay` is documented
to be idempotent and is called every time the host reconfigures, but the
old harness called prepareToPlay exactly once per track lifetime
(`TestTrack.cpp:76`) so the reconfigure path was untested.

These tests drive the new `POST /daw/sampleRate` endpoint to sweep through
the common consumer-and-mastering rates and assert that:

  1. No crash occurs across the sweep
  2. The oscillator list stays intact (prepareToPlay must not re-register
     the source with a fresh id)
  3. Sources remain visible in the global registry
  4. Waveform data continues to flow after each change

Bug caught
----------
If a future change drops the `deferRegistration` updateSource branch (see
`PluginProcessor.cpp:130`), the second SR change would lose the source
from the registry.  That would fail `test_source_registered_across_sweep`
immediately.
"""

from __future__ import annotations

import pytest

from oscil_test_utils import OscilTestClient


SWEEP_RATES: list[float] = [44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0]
DEFAULT_RATE = 44100.0


def _set_sample_rate(client: OscilTestClient, rate: float) -> dict:
    """Drive the new /daw/sampleRate endpoint and return its data payload."""
    resp = client._post_json("/daw/sampleRate", {"rate": rate})
    assert resp is not None, f"Harness did not respond to sample rate change to {rate}"
    assert resp.get("success"), f"Sample rate change to {rate} failed: {resp}"
    return resp.get("data", {})


@pytest.fixture(autouse=True, scope="module")
def _restore_sample_rate(client: OscilTestClient):
    """Restore the DAW to 44.1 kHz after every module in this file.

    Leaving the harness at 192 kHz drives downstream suites (particularly
    test_sixteen_instances) into multi-hundred-MB-per-instance allocations
    that have been observed to crash the process with a mutex EINVAL.  Each
    sample-rate test module cleans up after itself.
    """
    yield
    try:
        _set_sample_rate(client, DEFAULT_RATE)
    except AssertionError:
        # Best-effort teardown; a secondary failure must not mask the
        # primary test failure it is cleaning up after.
        pass


class TestSampleRateSweep:
    """Drive SR sweeps across the full range hosts report in the wild."""

    def test_sweep_common_rates_no_crash(self, editor: OscilTestClient, source_id: str):
        """
        Sweep every common sample rate after adding one oscillator, then
        verify the oscillator still exists after every transition.  A crash
        during prepareToPlay would tear down the harness connection and
        raise HarnessCrashedError from the first post-change call.
        """
        osc_id = editor.add_oscillator(source_id, name="SR Sweep Oscillator")
        assert osc_id is not None, "Failed to create baseline oscillator"

        # Ensure transport is running so per-change waveform data is a
        # meaningful signal, not an always-empty reply.
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(),
            timeout_s=5.0,
            desc="transport to be playing before SR sweep",
        )

        for rate in SWEEP_RATES:
            data = _set_sample_rate(editor, rate)
            assert data.get("rate") == pytest.approx(rate), (
                f"Harness accepted rate {rate} but reports {data.get('rate')}"
            )

            # Give the reconfigure a chance to resume and confirm the
            # oscillator survived.  wait_until is polling; no sleep.
            oscs_after = editor.wait_until(
                lambda: editor.get_oscillators() if editor.get_oscillators() else None,
                timeout_s=5.0,
                desc=f"oscillator list non-empty after SR={rate}",
            )
            osc_ids = [o.get("id") for o in oscs_after]
            assert osc_id in osc_ids, (
                f"Oscillator {osc_id} disappeared after SR change to {rate}. "
                f"List after change: {osc_ids}"
            )

    def test_source_registered_across_sweep(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: `deferRegistration` on subsequent prepareToPlay calls
        silently drops the updateSource branch, leaving the source absent
        from the registry.  If this regresses, the source list below will
        shrink after the first SR change.
        """
        baseline_sources = {s["id"] for s in editor.get_sources()}
        assert source_id in baseline_sources, "Precondition: source must exist"

        for rate in SWEEP_RATES:
            _set_sample_rate(editor, rate)
            # Registry update is async (callAsync inside prepareToPlay);
            # poll rather than assert immediately.
            def check_source_present():
                ids = {s["id"] for s in editor.get_sources()}
                return source_id in ids

            editor.wait_until(
                check_source_present,
                timeout_s=5.0,
                desc=f"source {source_id} to remain registered at SR={rate}",
            )

    def test_waveform_resumes_after_each_change(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        hasWaveformData coming back true proves the capture buffer was
        reconfigured correctly and processBlock is succeeding at the new
        rate.  Flat false across the sweep would indicate a sample-rate
        mismatch between SharedCaptureBuffer and the incoming data.
        """
        osc_id = editor.add_oscillator(source_id, name="SR Waveform Check")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=5.0, desc="transport playing"
        )

        for rate in SWEEP_RATES:
            _set_sample_rate(editor, rate)
            # The waveform polling window is generous — some rates need
            # several blocks before the downsampled buffer reports data.
            editor.wait_until(
                lambda: any(
                    w.get("hasWaveformData")
                    for w in editor.get_waveform_for_pane(0)
                ),
                timeout_s=8.0,
                desc=f"waveform data to resume at SR={rate}",
            )


class TestSampleRateValidation:
    """Reject invalid sample rates without crashing."""

    def test_reject_zero(self, client: OscilTestClient):
        """Sanity: 0 Hz is invalid and must be rejected with a 400."""
        resp = client._post("/daw/sampleRate", {"rate": 0})
        assert resp.status_code == 400, (
            f"Zero SR should return 400, got {resp.status_code}: {resp.text}"
        )

    def test_reject_negative(self, client: OscilTestClient):
        resp = client._post("/daw/sampleRate", {"rate": -44100.0})
        assert resp.status_code == 400

    def test_reject_missing_field(self, client: OscilTestClient):
        """Body without 'rate' should 400, not 500."""
        resp = client._post("/daw/sampleRate", {})
        assert resp.status_code == 400
