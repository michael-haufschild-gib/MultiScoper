"""
E2E coverage for the inline mode chip group on each sidebar list row.

`sidebar_oscillators_item_N_mode` is a SegmentedButtonBar with six
chips (Stereo/Mono/Mid/Side/Left/Right).  The chips share the parent's
testId and are hit-tested by x-offset.

What bugs these tests catch:
- Mode chip x-offset hit-test off by one, so clicking the leftmost
  chip picks up the second chip.
- Chip click updates the list row's visual highlight but the
  oscillator's `mode` field stays the same.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestModeChipClicksViaOffset:
    def test_chip_clicks_at_various_offsets_change_mode(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: chip group hit test maps all offsets to the
        same chip (the click handler grabs the first chip on
        any click)."""
        osc_id = editor.add_oscillator(source_id, name="ChipTest", mode="FullStereo")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        mode_id = "sidebar_oscillators_item_0_mode"
        el = editor.get_element(mode_id)
        assert el is not None and el.width > 0, (
            f"{mode_id} must be visible with non-zero width; "
            "ensure the sidebar is wide enough and the row is selected"
        )

        # Click at various offsets and observe whether the oscillator
        # mode changes.  Across 6 chips in a ~180 px width, offsets
        # of 15, 45, 75, 105, 135, 165 should each hit a distinct
        # chip.  We record the set of modes reached to prove the
        # offset routing is not a no-op.
        seen_modes: set = set()
        for x in (15, 45, 75, 105, 135, 165):
            if x >= el.width:
                break
            editor.click_at_offset(mode_id, offset_x=x, offset_y=max(5, el.height // 2))
            editor.wait_until(
                lambda: editor.get_oscillator_by_id(osc_id) is not None,
                timeout_s=2.0,
                desc="state readable",
            )
            mode_now = editor.get_oscillator_by_id(osc_id).get("mode")
            if mode_now:
                seen_modes.add(mode_now)

        # 6 offsets on 6 chips must reach at least 4 distinct modes.
        # The old `>= 2` bound passed even when 5 of 6 offsets routed
        # to the same chip — far too loose for "offsets don't map to a
        # single chip". 4 gives slack for (a) the chip-width math
        # rounding two adjacent offsets into the same chip and (b) the
        # model sometimes dedup'ing repeated same-chip clicks.
        assert len(seen_modes) >= 4, (
            f"inline mode chip offsets didn't route to enough distinct chips "
            f"(saw {len(seen_modes)}): {seen_modes}"
        )


class TestModeChipAffectsWaveformRendering:
    def test_mode_change_via_chip_affects_peak(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: chip click updates oscillator.mode but the
        waveform renderer reads the mode only at construction.
        Changing mid-playback would leave peak_before == peak_after.

        The test harness generates stereo test audio where right =
        0.8 * left. FullStereo keeps per-channel peak (peakLeft ≈ |L|);
        Mono mixes the channels (peakLeft of the mono result ≈ mean
        of |L| and |R|). The peaks MUST differ when the processing
        chain genuinely switches — if they match, the renderer is
        still running the old pipeline.
        """
        osc_id = editor.add_oscillator(source_id, name="ChipRender", mode="FullStereo")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        # Let the peak settle before sampling — peak readings right after
        # transport_play/set_track_audio can be transient.
        editor.wait_until(
            lambda: editor.get_waveform_for_pane(0)[0].get("peakLevelLeft", 0) > 0.1,
            timeout_s=3.0,
            desc="waveform peak to reach steady state under FullStereo",
        )
        peak_before = editor.get_waveform_for_pane(0)[0].get("peakLevelLeft", 0)

        # Switch mode via state API (offset clicks are tested above).
        editor.update_oscillator(osc_id, mode="Mono")
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(osc_id).get("mode") == "Mono",
            timeout_s=3.0,
            desc="mode to become Mono",
        )

        # Wait for the new mode's processing to propagate through the
        # waveform analyzer — a few audio frames is enough. Use a
        # condition rather than a blind sleep: the peak must either
        # have changed by a detectable amount, or stay locked.
        def peak_changed() -> bool:
            p = editor.get_waveform_for_pane(0)[0].get("peakLevelLeft", 0)
            return abs(p - peak_before) > 0.01

        try:
            editor.wait_until(peak_changed, timeout_s=3.0, desc="peak to shift after mode change")
        except Exception:
            pytest.fail(
                f"Peak did not change after FullStereo→Mono — the renderer may "
                f"have cached the initial mode. peak_before={peak_before}"
            )

        peak_after = editor.get_waveform_for_pane(0)[0].get("peakLevelLeft", 0)
        assert 0.0 <= peak_after <= 1.0, (
            f"peak after mode switch out of bounds: {peak_after}"
        )
        assert abs(peak_after - peak_before) > 0.01, (
            f"peakLeft must differ between FullStereo and Mono; "
            f"before={peak_before}, after={peak_after}"
        )

        editor.transport_stop()
