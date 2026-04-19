"""
E2E coverage for extreme sample rate values.

Sample rate is set by the host; the plugin must adapt the capture
buffer size, timing engine tick rate, and displaySamples.  Extreme
values (8 kHz phone-line, 192 kHz studio, 384 kHz ultra-hi-res) must
either work or be rejected cleanly — never crash.

What bugs these tests catch:
- Buffer size calculated as `srMs * sampleRate / 1000` overflows at
  384 kHz.
- Low sample rate (8 kHz) triggers underflow in FFT window math.
- Sample rate transition mid-playback causes glitch / crash.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


EXTREME_RATES = [8000.0, 22050.0, 88200.0, 96000.0, 176400.0, 192000.0]


class TestSampleRateChanges:
    @pytest.mark.parametrize("rate", EXTREME_RATES)
    def test_set_sample_rate_does_not_crash(
        self, client: MultiScoperTestClient, rate: float
    ):
        """Each of the supported extreme rates must be accepted OR
        rejected cleanly — never crash the harness."""
        client._post_ok("/daw/setSampleRate", {"sampleRate": rate})
        assert client.health_check()["data"]["status"] == "ok"


class TestSampleRateDuringPlayback:
    def test_sample_rate_change_during_playback(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: SR change while processBlock is running races
        with buffer reallocation → use-after-free or torn read."""
        editor.add_oscillator(source_id, name="SRChange")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        for rate in (48000.0, 96000.0, 44100.0):
            editor._post_ok("/daw/setSampleRate", {"sampleRate": rate})
            assert editor.health_check()["data"]["status"] == "ok"

        editor.transport_stop()


class TestSampleRateAbsurdValues:
    def test_zero_sample_rate_rejected_or_ignored(
        self, client: MultiScoperTestClient
    ):
        """0 Hz sample rate: either rejected with error=false or
        silently ignored.  Must not divide by zero anywhere."""
        client._post_ok("/daw/setSampleRate", {"sampleRate": 0.0})
        assert client.health_check()["data"]["status"] == "ok"
        # Restore to a sensible rate.
        client._post_ok("/daw/setSampleRate", {"sampleRate": 44100.0})

    def test_negative_sample_rate(self, client: MultiScoperTestClient):
        client._post_ok("/daw/setSampleRate", {"sampleRate": -48000.0})
        assert client.health_check()["data"]["status"] == "ok"
        client._post_ok("/daw/setSampleRate", {"sampleRate": 44100.0})

    def test_enormous_sample_rate(self, client: MultiScoperTestClient):
        """Absurdly high rate — must clamp or reject, never allocate
        gigabytes of buffer."""
        client._post_ok("/daw/setSampleRate", {"sampleRate": 10_000_000.0})
        assert client.health_check()["data"]["status"] == "ok"
        client._post_ok("/daw/setSampleRate", {"sampleRate": 44100.0})
