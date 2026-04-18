"""
E2E coverage for mid-session buffer-size changes (Primitive 2.2).

Every DAW exposes a block-size setting that calls prepareToPlay at the
new size.  The old harness fixed bufferSize_ at 512 for every tick, so
the processor never observed the size change — any scratch buffer that
silently assumed the prepare-time size was not protected against the
following test.

The `POST /daw/bufferSize` endpoint under test sweeps every power of two
between 16 and 8192 inclusive and asserts:

  1. The harness reports the new size back
  2. The oscillator list survives every transition
  3. Waveform data resumes after each change (confirms processBlock is
     succeeding with the new N)
  4. Out-of-range sizes are rejected with a 400 (not a crash / not 500)
"""

from __future__ import annotations

import contextlib

import pytest

from oscil_test_utils import OscilTestClient


SWEEP_SIZES: list[int] = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
DEFAULT_BUFFER_SIZE = 512


def _set_buffer_size(client: OscilTestClient, size: int) -> dict:
    resp = client._post_json("/daw/bufferSize", {"size": size})
    assert resp is not None, f"Harness did not respond to buffer size {size}"
    assert resp.get("success"), f"Buffer size {size} rejected: {resp}"
    return resp.get("data", {})


@pytest.fixture(autouse=True, scope="module")
def _restore_buffer_size(client: OscilTestClient):
    """Restore the DAW to 512-sample blocks after every module in this file.

    Same rationale as the sample-rate-cleanup fixture: leaving the harness
    at 8192-sample blocks compounds memory pressure in downstream 16-
    instance scale tests."""
    yield
    with contextlib.suppress(AssertionError):
        _set_buffer_size(client, DEFAULT_BUFFER_SIZE)


class TestBufferSizeSweep:
    def test_sweep_power_of_two_sizes(
        self, editor: OscilTestClient, source_id: str
    ):
        osc_id = editor.add_oscillator(source_id, name="Buffer Sweep Oscillator")
        assert osc_id is not None

        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(),
            timeout_s=5.0,
            desc="transport playing before buffer sweep",
        )

        for size in SWEEP_SIZES:
            data = _set_buffer_size(editor, size)
            assert data.get("size") == size, (
                f"Harness accepted size {size} but reports {data.get('size')}"
            )

            oscs_after = editor.wait_until(
                lambda: editor.get_oscillators() if editor.get_oscillators() else None,
                timeout_s=5.0,
                desc=f"oscillator list non-empty after block size={size}",
            )
            assert osc_id in [o.get("id") for o in oscs_after], (
                f"Oscillator lost after buffer size change to {size}"
            )

    def test_waveform_resumes_after_each_size(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: any downstream code path that cached a pointer /
        length sized to the prepare-time block size would produce zero
        output after the first size change.  Waveform data resuming is a
        proxy for processBlock correctness.
        """
        editor.add_oscillator(source_id, name="Buffer Waveform Check")
        editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(), timeout_s=5.0, desc="transport playing"
        )

        for size in SWEEP_SIZES:
            _set_buffer_size(editor, size)
            editor.wait_until(
                lambda: any(
                    w.get("hasWaveformData")
                    for w in editor.get_waveform_for_pane(0)
                ),
                timeout_s=8.0,
                desc=f"waveform resumes at block size={size}",
            )


class TestBufferSizeValidation:
    def test_reject_below_minimum(self, client: OscilTestClient):
        """15 is one below the documented floor — must be rejected."""
        resp = client._post("/daw/bufferSize", {"size": 15})
        assert resp.status_code == 400, (
            f"Out-of-range size should 400, got {resp.status_code}"
        )

    def test_reject_above_maximum(self, client: OscilTestClient):
        resp = client._post("/daw/bufferSize", {"size": 9000})
        assert resp.status_code == 400

    def test_reject_missing_field(self, client: OscilTestClient):
        resp = client._post("/daw/bufferSize", {})
        assert resp.status_code == 400

    def test_reject_zero(self, client: OscilTestClient):
        resp = client._post("/daw/bufferSize", {"size": 0})
        assert resp.status_code == 400
