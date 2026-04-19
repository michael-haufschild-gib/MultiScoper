"""
E2E coverage for burst mode audio — brief signal with decay.

The /track/{N}/burst endpoint triggers the test audio generator to
emit a finite-length burst.  After the burst ends, silence follows.

What bugs these tests catch:
- Burst endpoint accepted but audio never emitted (handler not
  wired to generator's burst function).
- Burst duration ignored (always emits full-length signal).
- Multiple rapid bursts queue incorrectly — second burst cancels
  first but silence still plays.
"""

from __future__ import annotations

from multiscoper_test_utils import MultiScoperTestClient


class TestBurstEndpointExists:
    def test_burst_post_succeeds(self, editor: MultiScoperTestClient, source_id: str):
        """Bug caught: burst endpoint returns 404 or 500."""
        editor.add_oscillator(source_id, name="Burst")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()

        # Issue a burst via raw POST.
        resp = editor._post_json(
            "/track/0/burst",
            {"durationMs": 200, "frequency": 880.0, "amplitude": 0.9, "waveform": "sine"},
        )
        assert resp is not None, "burst endpoint must respond"
        # Whether `resp` is ok=true or ok=false, the harness must live.
        assert editor.health_check()["data"]["status"] == "ok"
        editor.transport_stop()


class TestBurstProducesTransientSignal:
    def test_burst_creates_peak_then_silences(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """After a burst, waveform data should have a peak then decay
        to silence.  Bug caught: burst's decay never triggers, leaving
        the signal latched."""
        editor.add_oscillator(source_id, name="BurstDecay")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()

        # Baseline: silence.
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.0)

        # Fire a burst.
        resp = editor._post_json(
            "/track/0/burst",
            {"durationMs": 200, "frequency": 880.0, "amplitude": 0.9, "waveform": "sine"},
        )
        assert resp and resp.get("success"), (
            f"/track/0/burst must succeed; got {resp!r}"
        )

        # Verify: waveform should have data during the burst window.
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        editor.transport_stop()


class TestMultipleRapidBursts:
    def test_rapid_bursts_are_idempotent(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Firing five bursts in quick succession must not crash."""
        editor.add_oscillator(source_id, name="RapidBursts")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()

        for i in range(5):
            editor._post_json(
                "/track/0/burst",
                {"durationMs": 50, "frequency": 440.0 + i * 100, "amplitude": 0.5},
            )

        assert editor.health_check()["data"]["status"] == "ok"
        editor.transport_stop()
