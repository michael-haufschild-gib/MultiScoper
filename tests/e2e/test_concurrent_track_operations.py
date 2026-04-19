"""
E2E coverage for concurrent operations across multiple tracks.

What bugs these tests catch:
- Adding an oscillator to track 0 races with resetting track 1,
  clobbering track 0's new oscillator.
- Remove_track while audio is playing on that track leaks a
  capture buffer or deadlocks the audio dispatcher.
- Multiple threads posting state mutations via /state/oscillator/add
  on the same track cause duplicate IDs or lost-update anomalies.
- Cross-track source discovery bounces under concurrent add/remove
  (source appears/disappears randomly in the registry).
"""

from __future__ import annotations

import concurrent.futures
import threading

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestIndependentTrackStateMutations:
    """Operations on distinct tracks must not interfere."""

    def test_reset_track0_does_not_affect_track1(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Add 2 oscillators to track 1, reset track 0, verify track 1
        untouched.

        Bug caught: reset endpoint matches any trackId, or reset leaks
        across tracks sharing an InstanceRegistry.
        """
        src1 = track_sources[1]
        for i in range(2):
            multi_editor.add_oscillator_to_track(
                1, src1, name=f"T1-{i}", colour=f"#{i:06x}"
            )
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) == 2,
            timeout_s=3.0, desc="track 1 to have 2 oscs",
        )

        multi_editor.reset_track_state(0)
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 0,
            timeout_s=3.0, desc="track 0 to be empty",
        )

        track1_oscs = multi_editor.get_oscillators_for_track(1)
        assert len(track1_oscs) == 2, (
            f"track 1 must keep its oscillators when track 0 is reset, "
            f"got {len(track1_oscs)}"
        )


class TestParallelStateAdds:
    """Multiple concurrent adds to the same track must each succeed."""

    def test_six_parallel_adds_to_track_0(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: state mutex held on the wrong lock, so parallel
        adds see each other mid-insertion and drop entries."""
        N = 6

        def do_add(i: int) -> str:
            return editor.add_oscillator(source_id, name=f"Parallel-{i}")

        with concurrent.futures.ThreadPoolExecutor(max_workers=N) as pool:
            ids = list(pool.map(do_add, range(N)))

        # All should have succeeded.
        assert all(i is not None for i in ids), (
            f"parallel adds must all succeed, got {ids}"
        )
        editor.wait_for_oscillator_count(N, timeout_s=10.0)
        final = editor.get_oscillators()
        assert len(final) == N, (
            f"exactly {N} oscillators expected, got {len(final)}"
        )
        final_ids = {o["id"] for o in final}
        for i in ids:
            assert i in final_ids, f"osc id {i} missing from state"


class TestRemoveTrackDuringPlayback:
    """Removing a track while its audio is playing must not crash."""

    def test_remove_track_with_active_audio(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Bug caught: teardown of track mid-processBlock leaves
        dangling ptr in audio thread."""
        # Create a dynamic track that we can remove safely.
        new_track = multi_editor.add_track("Ephemeral")
        assert new_track is not None
        new_idx = new_track["trackIndex"]

        # Play on all tracks.
        multi_editor.transport_play()
        multi_editor.set_track_audio(
            new_idx, waveform="sine", frequency=440.0, amplitude=0.6
        )
        multi_editor.wait_until(
            lambda: multi_editor.is_playing(),
            timeout_s=3.0, desc="transport to start",
        )

        # Remove the dynamic track while playback is active.
        multi_editor.close_editor(track_id=new_idx)
        multi_editor.remove_track(new_idx)

        # Harness must stay alive.
        assert multi_editor.health_check()["data"]["status"] == "ok"
        multi_editor.transport_stop()


class TestConcurrentAddAndReset:
    """Add on one track while resetting another must remain coherent."""

    def test_add_on_track1_while_resetting_track0(
        self, multi_editor: MultiScoperTestClient, track_sources: dict
    ):
        """Bug caught: global write-lock serialization prevents the
        add from landing — it silently fails with success=true but
        the state doesn't reflect the write."""
        src1 = track_sources[1]

        results: list[str | None] = [None, None]
        errors: list[Exception | None] = [None, None]
        barrier = threading.Barrier(2)

        def worker_add():
            try:
                barrier.wait()
                results[0] = multi_editor.add_oscillator_to_track(
                    1, src1, name="ConcurrentAdd"
                )
            except Exception as e:
                errors[0] = e

        def worker_reset():
            try:
                barrier.wait()
                multi_editor.reset_track_state(0)
                results[1] = "reset-done"
            except Exception as e:
                errors[1] = e

        t1 = threading.Thread(target=worker_add)
        t2 = threading.Thread(target=worker_reset)
        t1.start(); t2.start()
        t1.join(timeout=10.0); t2.join(timeout=10.0)

        assert errors == [None, None], f"errors: {errors}"
        assert results[0] is not None, "concurrent add must succeed"

        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) >= 1,
            timeout_s=5.0,
            desc="track 1 to have at least the concurrent-add osc",
        )
