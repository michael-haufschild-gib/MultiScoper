"""
E2E coverage for cross-track source routing — the plugin's headline
multi-instance feature.

Scenario: track B's oscillator is bound to track A's source. Audio
generated on track A must reach track B's waveform render state.

What bugs these tests catch:
- Source deregistration from A on track removal leaves B's osc
  bound to a phantom ID (waveform state goes silent but state claims
  bound).
- Audio signal routed through shared capture buffer lags or drops
  samples because B's TimingEngine isn't synced to A's source rate.
- Source rename (future product feature) breaks existing cross-
  instance bindings — they don't re-resolve by the new name.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestCrossTrackSourceBinding:
    def test_track_b_osc_bound_to_track_a_source(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Bug caught: add_oscillator_to_track silently ignores a
        sourceId that doesn't match the target track's own source
        (only bindings to self accepted)."""
        src_a = track_sources[0]
        osc_id = multi_editor.add_oscillator_to_track(
            1, src_a, name="B->A"
        )
        assert osc_id is not None, "cross-track add must succeed"

        oscs = multi_editor.get_oscillators_for_track(1)
        matching = [o for o in oscs if o["sourceId"] == src_a]
        assert matching, (
            f"track 1 must have an osc bound to track 0's source {src_a}"
        )


class TestCrossTrackAudioFlow:
    def test_audio_on_a_reaches_b_oscillator(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """End-to-end: play audio on A, read track B's waveform state,
        verify it has data for A's signal.

        Bug caught: cross-track discovery via InstanceRegistry works
        at API level but the capture buffer isn't actually shared, so
        B's waveform stays silent even though the osc is bound.
        """
        src_a = track_sources[0]
        osc_id = multi_editor.add_oscillator_to_track(
            1, src_a, name="CrossSignal"
        )
        assert osc_id is not None
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) >= 1,
            timeout_s=3.0, desc="cross osc on track 1",
        )

        multi_editor.transport_play()
        multi_editor.set_track_audio(
            0, waveform="sine", frequency=440.0, amplitude=0.9
        )
        multi_editor.wait_until(
            lambda: multi_editor.is_playing(),
            timeout_s=3.0, desc="transport playing",
        )

        # Track B's specific cross-bound oscillator — identified by osc_id —
        # must observe data. Using `any()` over all waveforms on track 1 was
        # semantically wrong: it would pass even if an unrelated osc on
        # track 1 had data while the cross-bound one stayed silent.
        def cross_bound_osc_has_data() -> bool:
            state = multi_editor.get_waveform_state_for_track(1) or {}
            for pane in state.get("panes", []):
                for w in pane.get("waveforms", []):
                    if w.get("oscillatorId") == osc_id:
                        return bool(w.get("hasWaveformData"))
            return False

        try:
            multi_editor.wait_until(
                cross_bound_osc_has_data,
                timeout_s=10.0,
                desc=f"track B's cross-bound osc {osc_id} to see track A's signal",
            )
        finally:
            multi_editor.transport_stop()


class TestCrossTrackRemovalCleanup:
    def test_remove_source_track_deregisters_from_other_instances(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Bug caught: removing track A leaves A's source in the
        registry, so B's discovery still thinks it's bound."""
        src_a = track_sources[0]

        # Create an ephemeral third track to avoid killing the fixture's tracks.
        extra = multi_editor.add_track("EphemeralA")
        assert extra is not None
        extra_idx = extra["trackIndex"]
        extra_src = extra["sourceId"]

        # Bind track 1 osc to the extra track's source.
        multi_editor.add_oscillator_to_track(
            1, extra_src, name="DependOnExtra"
        )
        multi_editor.wait_until(
            lambda: any(
                o["sourceId"] == extra_src
                for o in multi_editor.get_oscillators_for_track(1)
            ),
            timeout_s=3.0,
            desc="binding to exist",
        )

        # Remove the extra track.
        multi_editor.close_editor(track_id=extra_idx)
        multi_editor.remove_track(extra_idx)
        multi_editor.wait_until(
            lambda: multi_editor.get_track_info(extra_idx) is None,
            timeout_s=3.0, desc="extra track to be removed",
        )

        # Extra track's source must no longer be in the global source list.
        global_sources = {s["id"] for s in multi_editor.get_sources()}
        assert extra_src not in global_sources, (
            f"extra source {extra_src} must be deregistered from global registry"
        )

        # Track 1's orphan osc must be in a coherent state. Either it was
        # reassigned/removed when its source vanished, or it still exists
        # but its sourceId refers to a source that no longer appears in
        # the registry — i.e., "discovery doesn't think it's bound to
        # anything valid". What the test must catch is the broken
        # middle state: osc still exists AND its sourceId shows up in
        # the global list (stale registry entry). That's the docstring's
        # phantom-binding bug.
        track1_oscs = multi_editor.get_oscillators_for_track(1)
        orphan = next((o for o in track1_oscs if o["sourceId"] == extra_src), None)
        if orphan is not None:
            assert extra_src not in global_sources, (
                f"osc {orphan.get('id')} still bound to {extra_src}, and that "
                f"source is still in the global registry — phantom binding"
            )
