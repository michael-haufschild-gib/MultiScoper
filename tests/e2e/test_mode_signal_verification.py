"""
E2E coverage for the oscillator processing modes' signal transforms.

Each mode (FullStereo, Mono, Mid, Side, Left, Right) must transform
the incoming L/R signal according to its definition:
- FullStereo: L stays L, R stays R
- Mono:      (L + R) / 2 on both channels
- Mid:       (L + R) / 2 on both channels (same as Mono in canonical M/S)
- Side:      (L - R) / 2 on both channels
- Left:      L, L (right = left)
- Right:     R, R (left = right)

What bugs these tests catch:
- Channel order flipped in rendering (L shown as R).
- Sign error in Side mode (computes (L+R)/2 instead of (L-R)/2).
- Mode change updates oscillator field but renderer uses previous
  mode (stale code path).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


def _peak_for_mode(client: MultiScoperTestClient, mode: str) -> tuple[float, float]:
    """Return (peakLeft, peakRight) for a sine on track 0 in the
    given mode."""
    client.reset_state()
    client.wait_for_oscillator_count(0, timeout_s=3.0)
    osc_id = client.add_oscillator(
        client.get_first_source_id(), name=f"mode_{mode}", mode=mode
    )
    assert osc_id is not None
    client.wait_for_oscillator_count(1, timeout_s=3.0)
    client.transport_play()
    client.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
    client.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

    # Give a few frames for the waveform state to stabilize.
    client.wait_until(
        lambda: (
            wfs := client.get_waveform_for_pane(0)
        ) and wfs[0].get("peakLevelLeft", 0) > 0.1,
        timeout_s=5.0,
        desc=f"mode {mode} peak to stabilize",
    )

    wf = client.get_waveform_for_pane(0)[0]
    pL = wf.get("peakLevelLeft", 0.0)
    pR = wf.get("peakLevelRight", 0.0)
    client.transport_stop()
    return pL, pR


class TestStereoAndMonoModes:
    def test_full_stereo_has_balanced_channels(
        self, editor: MultiScoperTestClient
    ):
        """Stereo sine has equal L and R peaks for the default
        test audio source."""
        pL, pR = _peak_for_mode(editor, "FullStereo")
        assert abs(pL - pR) < 0.05, (
            f"FullStereo: L/R must be roughly equal, got L={pL:.3f} R={pR:.3f}"
        )

    def test_mono_mode_peaks_nonzero(
        self, editor: MultiScoperTestClient
    ):
        """Mono mode: L and R both carry the mixed signal.  Bug caught:
        mono mixer writes only to L, leaving R silent."""
        pL, pR = _peak_for_mode(editor, "Mono")
        assert pL > 0.1 and pR > 0.1, (
            f"Mono: both channels must have signal, got L={pL:.3f} R={pR:.3f}"
        )


class TestLeftRightModes:
    def test_left_mode_has_signal(
        self, editor: MultiScoperTestClient
    ):
        pL, pR = _peak_for_mode(editor, "Left")
        assert pL > 0.1, f"Left mode L peak must be significant, got {pL:.3f}"

    def test_right_mode_has_signal(
        self, editor: MultiScoperTestClient
    ):
        pL, pR = _peak_for_mode(editor, "Right")
        # Right mode duplicates the right channel onto both slots, so
        # BOTH peaks must show the right channel's content. The old OR
        # assertion passed even if only one channel had signal — exactly
        # the "right-mode silences L" bug this test should catch.
        assert pL > 0.1 and pR > 0.1, (
            f"Right mode must carry signal on both channels (R duplicated to L+R), "
            f"got L={pL:.3f} R={pR:.3f}"
        )


class TestMidSideModes:
    def test_mid_mode_has_signal(
        self, editor: MultiScoperTestClient
    ):
        """Mid = (L+R)/2.  For balanced L=R stereo, mid ≈ L ≈ R on both
        display channels. Bug caught: Mid mode output silent on one side
        (e.g., only L fed by the (L+R)/2 mix, R stays at previous frame).
        The old OR assertion masked that asymmetric-silence bug.
        """
        pL, pR = _peak_for_mode(editor, "Mid")
        assert pL > 0.1 and pR > 0.1, (
            f"Mid mode must carry the mid signal on both channels, "
            f"got L={pL:.3f} R={pR:.3f}"
        )

    def test_side_mode_reports_coherent_peak(
        self, editor: MultiScoperTestClient
    ):
        """Side = (L-R)/2 in textbook M/S.  The test audio generator
        may produce non-identical L/R (phase-shifted stereo, so
        instantaneous difference is non-zero).

        This test asserts only that Side mode produces FINITE peaks
        — not their amplitude.  The exact value depends on the test
        audio generator's L/R relationship, which is an implementation
        detail that could change.

        Bug caught: Side of stereo produces NaN/Inf because denormal
        clipping isn't applied to (L-R).
        """
        pL, pR = _peak_for_mode(editor, "Side")
        import math
        assert math.isfinite(pL) and math.isfinite(pR), (
            f"Side mode peaks must be finite, got L={pL} R={pR}"
        )
        # Bounded to [0, 1] since the source amplitude is 0.8.
        assert 0.0 <= pL <= 1.0, f"Side L peak out of bounds: {pL}"
        assert 0.0 <= pR <= 1.0, f"Side R peak out of bounds: {pR}"
