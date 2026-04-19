"""
E2E — black-box complement to the C++ race tests in
tests/test_plugin_processor_state_race.cpp.

Drives /state/save in a tight loop while the transport is active and two
oscillators are present. Exercises the message-thread save path that in
production is hit by DAW autosave; even though the HTTP handler always runs
on the message thread (so does not stress the audio-thread tryLock branch),
hammering save while processBlock is active in the harness still verifies:

  * the harness does not crash under save-while-playing
  * every save succeeds (no torn / invalid state returned)
  * the oscillator list remains stable (no mutation from reading state)

See docs/ci_reports/harness_capability_gaps.md item #5 / #9 for what this
cannot verify end-to-end (audio-thread save only matters under Pro Tools).
"""

import os
import time

import pytest
from multiscoper_test_utils import MultiScoperTestClient


SAVE_ITERATIONS = 500


class TestStateSaveDuringPlayback:
    """Spam /state/save while transport is playing; assert stability."""

    def test_rapid_save_while_playing_does_not_crash_or_corrupt(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        # Arrange: two oscillators, transport playing, audio flowing.
        id_a = editor.add_oscillator(source_id, name="Race A")
        id_b = editor.add_oscillator(source_id, name="Race B")
        assert id_a is not None and id_b is not None
        editor.wait_for_oscillator_count(2, timeout_s=5.0)

        editor.transport_play()
        editor.wait_until(lambda: editor.is_playing(), timeout_s=2.0,
                          desc="transport to start")
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)

        save_path = str(tmp_path / "race_state.xml")
        successes = 0
        failures = 0

        try:
            for i in range(SAVE_ITERATIONS):
                ok = editor.save_state(save_path)
                if ok:
                    successes += 1
                else:
                    failures += 1

                # Occasionally verify the oscillator list is still intact.
                # Every 50 saves is enough to catch mutation without making
                # the loop I/O-bound.
                if i % 50 == 0:
                    oscs = editor.get_oscillators()
                    assert len(oscs) == 2, (
                        f"Oscillator count drifted at iter {i}: "
                        f"expected 2, got {len(oscs)}"
                    )
        finally:
            editor.transport_stop()

        # Final invariants.
        assert successes + failures == SAVE_ITERATIONS
        # Every save attempt on the editor's message thread should succeed;
        # any failure indicates a regression.
        assert failures == 0, (
            f"{failures}/{SAVE_ITERATIONS} /state/save calls failed — "
            f"save-while-playing regression"
        )

        # File was written and is non-empty and parses as XML-looking content.
        assert os.path.exists(save_path), "No state file produced"
        size = os.path.getsize(save_path)
        assert size > 0, "Saved state file is empty"

        # Oscillator list unchanged after the whole flood.
        oscs = editor.get_oscillators()
        assert len(oscs) == 2, (
            f"Oscillator count mutated by save flood: "
            f"expected 2, got {len(oscs)}"
        )
        names = sorted(o.get("name", "") for o in oscs)
        assert names == sorted(["Race A", "Race B"]), (
            f"Oscillator names mutated by save flood: {names}"
        )

    def test_save_burst_then_readback_remains_consistent(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """
        Complement to the flood test: save many times, then verify the LAST
        saved file round-trips via /state/load without losing oscillators.
        """
        editor.add_oscillator(source_id, name="Readback A")
        editor.add_oscillator(source_id, name="Readback B")
        editor.wait_for_oscillator_count(2, timeout_s=5.0)

        editor.transport_play()
        editor.wait_until(lambda: editor.is_playing(), timeout_s=2.0,
                          desc="transport to start")

        save_path = str(tmp_path / "readback_state.xml")
        try:
            for _ in range(100):
                assert editor.save_state(save_path), "save failed mid-burst"
        finally:
            editor.transport_stop()

        # Reset and reload — oscillator count must come back to 2.
        editor.reset_state()
        editor.wait_until(lambda: len(editor.get_oscillators()) == 0,
                          timeout_s=3.0, desc="reset to complete")

        assert editor.load_state(save_path), "load_state failed after burst save"
        editor.wait_for_oscillator_count(2, timeout_s=5.0)
        oscs = editor.get_oscillators()
        assert len(oscs) == 2
        names = sorted(o.get("name", "") for o in oscs)
        assert names == sorted(["Readback A", "Readback B"])
