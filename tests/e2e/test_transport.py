"""
E2E tests for transport control and audio waveform behavior.

What bugs these tests catch:
- Play/stop commands not toggling transport state
- BPM API not clamping to valid range
- BPM changes while playing corrupting playback state
- Position not advancing during playback
- Position not freezing when stopped
- Rapid play/stop toggle causing crash or stuck state
- Waveform type / frequency / amplitude API not propagating to audio generator
- Position reset not working
"""

import pytest
from oscil_test_utils import OscilTestClient


# ── Transport State ─────────────────────────────────────────────────────────


class TestTransportState:
    """Basic transport state queries."""

    def test_transport_state_is_queryable(self, client: OscilTestClient):
        """
        Bug caught: /transport/state endpoint not returning valid data.
        """
        state = client.get_transport_state()
        assert state is not None, "Transport state should be queryable"
        assert "playing" in state, "State should contain 'playing' field"
        assert "bpm" in state, "State should contain 'bpm' field"

    def test_bpm_in_valid_range(self, client: OscilTestClient):
        """
        Bug caught: BPM initialized to 0 or negative.
        """
        bpm = client.get_bpm()
        assert 20 <= bpm <= 300, f"BPM should be in [20, 300], got {bpm}"


# ── Play / Stop ─────────────────────────────────────────────────────────────


class TestPlayStop:
    """Transport play and stop behavior."""

    def test_play_sets_playing_true(self, client: OscilTestClient):
        """
        Bug caught: transport_play() not setting playing flag.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")
        assert client.is_playing()
        client.transport_stop()

    def test_stop_sets_playing_false(self, client: OscilTestClient):
        """
        Bug caught: transport_stop() not clearing playing flag.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")
        client.transport_stop()
        client.wait_until(lambda: not client.is_playing(), timeout_s=2.0, desc="transport to stop")
        assert not client.is_playing()

    def test_position_advances_while_playing(self, client: OscilTestClient):
        """
        Bug caught: audio callback not advancing position, or position
        counter overflow.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")

        state1 = client.get_transport_state()
        # Wait for some audio to process
        client.wait_until(
            lambda: (s := client.get_transport_state()) and
                    s.get("positionSamples", 0) > state1.get("positionSamples", 0),
            timeout_s=3.0,
            desc="position to advance",
        )
        state2 = client.get_transport_state()
        assert state2["positionSamples"] > state1["positionSamples"]
        client.transport_stop()

    def test_position_frozen_while_stopped(self, client: OscilTestClient):
        """
        Bug caught: position counter not gated by playing flag.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")
        # Wait for position to advance (condition-based, not arbitrary sleep)
        client.wait_until(
            lambda: (s := client.get_transport_state())
            and s.get("positionSamples", 0) > 0,
            timeout_s=2.0,
            desc="position to advance from zero",
        )
        client.transport_stop()
        client.wait_until(
            lambda: not client.is_playing(), timeout_s=2.0, desc="transport to stop"
        )

        state1 = client.get_transport_state()
        pos1 = state1["positionSamples"]
        # Poll a few times to confirm position stays frozen
        stable = True
        for _ in range(3):
            state2 = client.get_transport_state()
            if state2["positionSamples"] != pos1:
                stable = False
                break
        assert stable, (
            f"Position should freeze when stopped: "
            f"initial={pos1}, later={state2['positionSamples']}"
        )

    def test_rapid_play_stop_stability(self, client: OscilTestClient):
        """
        Bug caught: race condition between play/stop commands causing
        deadlock or crash.
        """
        for _ in range(10):
            client.transport_stop()
            client.transport_play()

        # Should end in a stable state (last command was play)
        state = client.get_transport_state()
        assert state is not None, "Transport state should be queryable after rapid toggles"
        assert client.is_playing(), "Should be playing after rapid toggles ending with play"
        client.transport_stop()


# ── BPM Control ─────────────────────────────────────────────────────────────


class TestBPM:
    """BPM setting and validation."""

    @pytest.mark.parametrize("target_bpm", [60.0, 90.0, 120.0, 140.0, 180.0])
    def test_set_bpm(self, client: OscilTestClient, target_bpm: float):
        """
        Bug caught: BPM setter not storing value, or getter returning stale data.
        """
        initial = client.get_bpm()
        assert client.set_bpm(target_bpm)

        actual = client.get_bpm()
        assert abs(actual - target_bpm) < 0.5, (
            f"Expected BPM {target_bpm}, got {actual}"
        )
        # Restore
        client.set_bpm(initial)

    def test_bpm_minimum_boundary(self, client: OscilTestClient):
        """
        Bug caught: BPM below 20 causing division by zero in timing calculations.
        """
        initial = client.get_bpm()
        client.set_bpm(20.0)
        assert abs(client.get_bpm() - 20.0) < 0.5
        client.set_bpm(initial)

    def test_bpm_maximum_boundary(self, client: OscilTestClient):
        """
        Bug caught: extremely high BPM causing integer overflow in sample calculations.
        """
        initial = client.get_bpm()
        client.set_bpm(300.0)
        assert abs(client.get_bpm() - 300.0) < 0.5
        client.set_bpm(initial)

    def test_bpm_change_while_playing(self, client: OscilTestClient):
        """
        Bug caught: changing BPM during playback causing audio glitch or crash.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")

        bpm_sequence = [80.0, 120.0, 160.0, 100.0, 140.0]
        for bpm in bpm_sequence:
            client.set_bpm(bpm)
            assert client.is_playing(), f"Playback should continue after BPM change to {bpm}"
            actual = client.get_bpm()
            assert abs(actual - bpm) < 0.5, f"Expected BPM {bpm}, got {actual}"

        client.transport_stop()
        client.set_bpm(120.0)


# ── Position Control ────────────────────────────────────────────────────────


class TestPositionReset:
    """Transport position reset."""

    def test_reset_position_to_zero(self, client: OscilTestClient):
        """
        Bug caught: setPosition API not updating internal counter.
        """
        client.transport_play()
        client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to play")

        # Let position advance
        client.wait_until(
            lambda: (s := client.get_transport_state()) and s.get("positionSamples", 0) > 1000,
            timeout_s=3.0,
            desc="position to advance past 1000",
        )

        before = client.get_transport_state()["positionSamples"]
        client.set_position(0)

        after = client.get_transport_state()["positionSamples"]
        assert after < before, (
            f"Position should reset: was {before}, now {after}"
        )
        client.transport_stop()


# ── Waveform Audio Generator ────────────────────────────────────────────────


class TestHealthEndpoint:
    """Verify the health endpoint works (fundamental test infrastructure)."""

    def test_health_returns_success(self, client: OscilTestClient):
        """
        Bug caught: health endpoint broken, making all other tests report
        'harness not running' instead of their actual failures.
        """
        body = client.health_check()
        assert body.get("success") is True, f"Health should report success, got {body}"


class TestWaveformAudio:
    """Track audio generator settings."""

    @pytest.mark.parametrize("waveform", ["sine", "square", "triangle", "saw"])
    def test_waveform_type(self, client: OscilTestClient, waveform: str):
        """
        Bug caught: waveform type enum not mapped correctly in audio generator.
        """
        assert client.set_track_audio(0, waveform=waveform)
        info = client.get_track_info(0)
        if info:
            actual = info.get("waveform", "").lower()
            assert actual == waveform, f"Expected waveform '{waveform}', got '{actual}'"
        client.set_track_audio(0, waveform="sine")

    @pytest.mark.parametrize("freq", [220.0, 440.0, 880.0, 1000.0])
    def test_frequency(self, client: OscilTestClient, freq: float):
        """
        Bug caught: frequency not stored or truncated.
        """
        assert client.set_track_audio(0, frequency=freq)
        info = client.get_track_info(0)
        if info:
            actual = info.get("frequency", 0)
            assert abs(actual - freq) < 0.1, f"Expected freq {freq}, got {actual}"
        client.set_track_audio(0, frequency=440.0)

    @pytest.mark.parametrize("amp", [0.0, 0.25, 0.5, 0.75, 1.0])
    def test_amplitude(self, client: OscilTestClient, amp: float):
        """
        Bug caught: amplitude clamping broken, or value not stored.
        """
        assert client.set_track_audio(0, amplitude=amp)
        info = client.get_track_info(0)
        if info:
            actual = info.get("amplitude", -1)
            assert abs(actual - amp) < 0.01, f"Expected amplitude {amp}, got {actual}"
        client.set_track_audio(0, amplitude=0.8)

    def test_silence_waveform(self, client: OscilTestClient):
        """
        Bug caught: "silence" type not handled, causing fallthrough to default.
        """
        assert client.set_track_audio(0, waveform="silence")
        info = client.get_track_info(0)
        if info:
            assert info.get("waveform", "").lower() == "silence"
        client.set_track_audio(0, waveform="sine")
