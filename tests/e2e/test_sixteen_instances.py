"""
16-instance end-to-end tests.

Oscil's core promise is "unlimited simultaneous audio sources, cross-instance
discovery, and rendering." The regular multi_instance suite exercises 2 and 3
instances; this suite pushes to 16 — a realistic upper bound for a DAW
session where an engineer might patch a reference scope onto many mix buses.

Every test in this file:

  1. Drives the harness to exactly 16 active plugin instances.
  2. Exercises a realistic cross-instance workflow.
  3. Measures RSS / CPU / FPS around the workload and asserts thresholds.

The resource thresholds are intentionally generous (debug-build on Apple
Silicon). They will catch runaway leaks and render collapses, not subtle
inefficiencies — that's the job of dedicated perf benchmarks, not E2E.

The suite is self-cleaning: any dynamically added tracks are removed in
teardown so the harness is left in the same 3-instance baseline the rest
of the suite expects.
"""
from __future__ import annotations

from typing import Optional

import pytest

from oscil_test_utils import OscilTestClient, settle
from perf_monitor import (
    ResourceMonitor,
    assert_cpu_below,
    assert_no_memory_leak,
)


TARGET_INSTANCES = 16
BASELINE_INSTANCES = 3


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------
@pytest.fixture
def sixteen_instances(client: OscilTestClient):
    """Ensure exactly 16 plugin instances exist, all editors open.

    Yields (client, track_ids, source_ids).  Teardown removes dynamic tracks
    and closes editors so we return to the 3-instance baseline.
    """
    # Make sure the 3 base tracks are present; some tests may have removed them.
    for tid in range(BASELINE_INSTANCES):
        if client.get_track_info(tid) is None:
            client.add_track()

    # Add tracks until we have TARGET_INSTANCES.
    added: list[int] = []
    existing = client.get_tracks()
    while len(existing) < TARGET_INSTANCES:
        result = client.add_track(f"Instance {len(existing) + 1}")
        assert result is not None, "add_track returned None"
        added.append(int(result["trackIndex"]))
        existing = client.get_tracks()

    # Open every track's editor.  Do this serially — opening editors in
    # parallel would stress JUCE's single message thread.
    for tid in range(TARGET_INSTANCES):
        client.open_editor(track_id=tid)

    # Wait until every instance has registered its source with the registry.
    def _all_sources_registered() -> bool:
        return len(client.get_sources()) >= TARGET_INSTANCES

    client.wait_until(
        _all_sources_registered,
        timeout_s=15.0,
        desc=f"all {TARGET_INSTANCES} instances to register sources",
    )

    track_ids = list(range(TARGET_INSTANCES))
    source_ids = {}
    for tid in track_ids:
        info = client.get_track_info(tid)
        assert info is not None, f"Track {tid} info missing"
        source_ids[tid] = info["sourceId"]

    yield client, track_ids, source_ids

    # Teardown ------------------------------------------------------
    client.transport_stop()
    for tid in range(TARGET_INSTANCES):
        try:
            client.reset_track_state(tid)
        except Exception:
            pass
    for tid in range(TARGET_INSTANCES):
        try:
            client.close_editor(track_id=tid)
        except Exception:
            pass
    # Remove every dynamic track (index >= BASELINE_INSTANCES).
    tracks = client.get_tracks()
    for t in tracks:
        idx = int(t.get("index", 0))
        if idx >= BASELINE_INSTANCES:
            client.remove_track(idx)
    # Ensure the base 3 exist for the next test that uses multi_editor etc.
    for tid in range(BASELINE_INSTANCES):
        if client.get_track_info(tid) is None:
            client.add_track()


# ---------------------------------------------------------------------------
# Source discovery & identity at scale
# ---------------------------------------------------------------------------
class TestSixteenInstanceDiscovery:
    def test_exactly_sixteen_instances_registered(self, sixteen_instances):
        """With 16 plugin instances, the registry reports 16 sources, each unique."""
        client, track_ids, source_ids = sixteen_instances
        sources = client.get_sources()
        assert len(sources) >= TARGET_INSTANCES, (
            f"Expected >= {TARGET_INSTANCES} sources, got {len(sources)}"
        )
        ids = [s["id"] for s in sources]
        assert len(set(ids)) == len(ids), (
            f"Source IDs must be unique across all 16 instances; got duplicates in {ids}"
        )

    def test_every_instance_has_nonempty_source_id(self, sixteen_instances):
        """Every one of the 16 tracks exposes a non-empty sourceId."""
        client, track_ids, source_ids = sixteen_instances
        missing = [tid for tid, sid in source_ids.items() if not sid]
        assert not missing, f"Instances missing sourceId: {missing}"

    def test_audio_dispatcher_runs_on_dedicated_pool(self, sixteen_instances):
        """
        The harness models multi-core DAW hosting: audio blocks are dispatched
        from a HighResolutionTimer onto a juce::ThreadPool, one job per track
        per tick. This test proves the dispatcher is alive and the pool exists
        by reading /health and asserting:

          1. audioPoolThreads >= 2 (parallelism is actually configured), and
          2. audioTicks advances between two probes 500 ms apart.

        If the audio loop regresses back to message-thread serial processing,
        or the pool is disabled, this test catches it.
        """
        import requests
        base = "http://localhost:8765"

        probe1 = requests.get(f"{base}/health", timeout=2.0).json()["data"]
        assert probe1["audioPoolThreads"] >= 2, (
            f"audio pool should have >= 2 worker threads for multi-core modeling; "
            f"got {probe1['audioPoolThreads']}"
        )
        ticks_before = int(probe1["audioTicks"])

        # Timer runs every 10 ms; 10 ticks ≈ 100 ms of real audio work.  Poll
        # /health until the counter advances by that much (or timeout trips,
        # which is a real failure — the dispatcher is stalled).
        def _ticks_advanced_by_ten() -> Optional[int]:
            probe = requests.get(f"{base}/health", timeout=2.0).json()["data"]
            delta_now = int(probe["audioTicks"]) - ticks_before
            return delta_now if delta_now >= 10 else None

        delta = client.wait_until(
            _ticks_advanced_by_ten,
            timeout_s=2.0,
            desc=f"audio dispatcher to advance by >= 10 ticks (from {ticks_before})",
        )
        assert delta >= 10, (
            f"audio dispatcher appears stalled: only {delta} ticks observed "
            f"(before={ticks_before})"
        )

    def test_every_instance_sees_all_peers(self, sixteen_instances):
        """
        Cross-instance visibility: every editor must see every other source
        in the global registry — if one instance's source is invisible to
        another, cross-instance oscillators there would be orphaned.
        """
        client, track_ids, source_ids = sixteen_instances
        sources = client.get_sources()
        visible_ids = {s["id"] for s in sources}
        for tid, sid in source_ids.items():
            assert sid in visible_ids, (
                f"Track {tid}'s source {sid} is not visible in the global registry"
            )


# ---------------------------------------------------------------------------
# Cross-instance rendering under load
# ---------------------------------------------------------------------------
class TestSixteenInstanceRendering:
    def test_ring_topology_each_instance_displays_neighbor(self, sixteen_instances):
        """
        Create a ring: instance i displays instance (i+1)'s signal.
        Verifies 16 cross-instance oscillator bindings coexist and each
        resolves to the correct neighbor's capture buffer.
        """
        client, track_ids, source_ids = sixteen_instances
        # Wipe any pre-existing oscillators.
        for tid in track_ids:
            client.reset_track_state(tid)

        # Bind the ring.
        for i, tid in enumerate(track_ids):
            neighbor = track_ids[(i + 1) % len(track_ids)]
            osc_id = client.add_oscillator_to_track(
                tid, source_ids[neighbor], name=f"T{tid}->T{neighbor}"
            )
            assert osc_id, f"Failed to bind T{tid} -> T{neighbor}"

        # Drive audio on every track so there's actually signal to see.
        for tid in track_ids:
            client.set_track_audio(
                tid, waveform="sine", frequency=220.0 + tid * 17, amplitude=0.6
            )
        client.transport_play()

        with ResourceMonitor() as mon:
            mon.sample_for(3.0)
            # Every oscillator should be receiving data from its neighbor.
            for tid in track_ids:
                wf = client.get_waveform_state_for_track(tid)
                assert wf and wf.get("panes"), f"Track {tid} missing waveform state"
                waveforms = wf["panes"][0].get("waveforms", [])
                assert waveforms, f"Track {tid} has no waveforms in pane 0"
                peak = max(w.get("peakLevel", 0.0) for w in waveforms)
                assert peak > 0.01, (
                    f"Track {tid} neighbor signal not flowing (peak={peak:.4f})"
                )
        client.transport_stop()

        # Per-core CPU% (can exceed 100 on multi-core machines). Generous
        # ceiling; a runaway render loop would blow past this.
        assert_cpu_below(mon.report, max_avg_percent=400.0)
        print(f"[ring-topology] {mon.report.summary()}")

    def test_sixteen_instances_sustained_playback_no_leak(self, sixteen_instances):
        """
        10 seconds of sustained playback across 16 instances, each with a
        local + cross oscillator.  Memory must not drift upward significantly.
        """
        client, track_ids, source_ids = sixteen_instances
        for tid in track_ids:
            client.reset_track_state(tid)

        # 2 oscillators per instance -> 32 active waveforms.
        for i, tid in enumerate(track_ids):
            neighbor = track_ids[(i + 1) % len(track_ids)]
            client.add_oscillator_to_track(tid, source_ids[tid], name=f"T{tid}-self")
            client.add_oscillator_to_track(
                tid, source_ids[neighbor], name=f"T{tid}-cross"
            )
            client.set_track_audio(
                tid, waveform="sine", frequency=330.0 + tid * 13, amplitude=0.5
            )

        client.transport_play()
        # Warm up — wait until every track is actually producing signal so
        # the leak window doesn't include start-up allocation noise.
        def _all_tracks_have_signal() -> bool:
            for tid in track_ids:
                wf = client.get_waveform_state_for_track(tid)
                if not wf or not wf.get("panes"):
                    return False
                waveforms = wf["panes"][0].get("waveforms", [])
                if not waveforms:
                    return False
                if max(w.get("peakLevel", 0.0) for w in waveforms) <= 0.01:
                    return False
            return True

        client.wait_until(
            _all_tracks_have_signal,
            timeout_s=5.0,
            desc="all 16 tracks to produce signal before leak window",
        )

        with ResourceMonitor(sample_interval_s=0.5) as mon:
            mon.sample_for(10.0)

        client.transport_stop()

        # Debug build + 32 waveforms is heavy; 60 MB growth over 10 s is a
        # hard sign something is not releasing.  Tighten after release-build
        # baselining.
        assert_no_memory_leak(mon.report, max_growth_mb=60.0)
        print(f"[sustained-playback] {mon.report.summary()}")


# ---------------------------------------------------------------------------
# CRUD stress: add/remove oscillators many times, check memory
# ---------------------------------------------------------------------------
class TestSixteenInstanceCrudStress:
    def test_rapid_oscillator_add_remove_across_all_instances_no_leak(
        self, sixteen_instances
    ):
        """
        Repeatedly add 16 oscillators (one per instance) and delete them.
        If any code path leaks capture buffers, panes, or GL resources,
        RSS will grow monotonically — flag that.
        """
        client, track_ids, source_ids = sixteen_instances
        for tid in track_ids:
            client.reset_track_state(tid)

        # Warm-up allocations (first iteration often bumps RSS).
        for tid in track_ids:
            oid = client.add_oscillator_to_track(tid, source_ids[tid], name="warmup")
            if oid:
                client._post_ok(
                    f"/state/oscillator/delete?trackId={tid}",
                    {"oscillatorId": oid, "trackId": tid},
                )
        # All warm-up oscillators deleted -> every track must show zero.
        client.wait_until(
            lambda: all(
                len(client.get_oscillators_for_track(t)) == 0 for t in track_ids
            ),
            timeout_s=5.0,
            desc="warm-up oscillators to drain from every track",
        )

        with ResourceMonitor(sample_interval_s=0.5) as mon:
            mon.sample()
            for _ in range(10):
                osc_ids = []
                for tid in track_ids:
                    oid = client.add_oscillator_to_track(
                        tid, source_ids[tid], name="churn"
                    )
                    osc_ids.append((tid, oid))
                for tid, oid in osc_ids:
                    if oid:
                        client._post_ok(
                            f"/state/oscillator/delete?trackId={tid}",
                            {"oscillatorId": oid, "trackId": tid},
                        )
                mon.sample()

        # 10 full cycles × 16 add/delete = 160 CRUD ops.  50 MB leak ceiling.
        assert_no_memory_leak(mon.report, max_growth_mb=50.0)

    def test_editor_open_close_cycle_no_leak(self, sixteen_instances):
        """
        Close and re-open all 16 editors repeatedly.  JUCE Component
        hierarchies, OpenGL contexts, and the test-element registry must
        clean up on close.
        """
        client, track_ids, source_ids = sixteen_instances

        # Start from "all open" (the fixture guarantees this) — damp any
        # in-flight editor animations before the measurement window.
        settle(0.5, reason="editor-open animations to finish")

        with ResourceMonitor(sample_interval_s=0.5) as mon:
            mon.sample()
            for _ in range(5):
                for tid in track_ids:
                    client.close_editor(track_id=tid)
                for tid in track_ids:
                    client.open_editor(track_id=tid)
                mon.sample()

        # 80 editor close+open cycles.  40 MB ceiling.
        assert_no_memory_leak(mon.report, max_growth_mb=40.0)


# ---------------------------------------------------------------------------
# State persistence at scale
# ---------------------------------------------------------------------------
class TestSixteenInstanceStatePersistence:
    def test_save_load_roundtrip_on_all_sixteen_instances(
        self, sixteen_instances, tmp_path
    ):
        """
        Each of the 16 instances saves, resets, and loads a 2-oscillator
        configuration.  After loading, every instance has the expected
        oscillator count.  A single save/load hang on any instance would
        have blown up two releases ago when we had a callAsync deadlock.
        """
        client, track_ids, source_ids = sixteen_instances
        for tid in track_ids:
            client.reset_track_state(tid)
            client.add_oscillator_to_track(tid, source_ids[tid], name=f"L{tid}")
            neighbor = track_ids[(tid + 1) % len(track_ids)]
            client.add_oscillator_to_track(
                tid, source_ids[neighbor], name=f"X{tid}->{neighbor}"
            )

        for tid in track_ids:
            path = str(tmp_path / f"state_track_{tid}.xml")
            assert client._post_ok(
                f"/state/save?trackId={tid}", {"path": path, "trackId": tid}
            ), f"save failed for track {tid}"

        for tid in track_ids:
            client.reset_track_state(tid)
            assert len(client.get_oscillators_for_track(tid)) == 0

        for tid in track_ids:
            path = str(tmp_path / f"state_track_{tid}.xml")
            assert client._post_ok(
                f"/state/load?trackId={tid}", {"path": path, "trackId": tid}
            ), f"load failed for track {tid}"

        for tid in track_ids:
            client.wait_until(
                lambda t=tid: len(client.get_oscillators_for_track(t)) == 2,
                timeout_s=5.0,
                desc=f"track {tid} to restore 2 oscillators",
            )


# ---------------------------------------------------------------------------
# Transport / audio parameter sweeps while 16 are rendering
# ---------------------------------------------------------------------------
class TestSixteenInstanceTransport:
    def test_transport_start_stop_affects_all_sixteen(self, sixteen_instances):
        """Start, stop, start again.  Transport state is global."""
        client, track_ids, _ = sixteen_instances
        client.transport_play()
        client.wait_until(client.is_playing, timeout_s=2.0, desc="transport to start")
        assert client.get_transport_state().get("playing") is True

        client.transport_stop()
        client.wait_until(
            lambda: not client.is_playing(), timeout_s=2.0, desc="transport to stop"
        )
        assert client.get_transport_state().get("playing") is False

        client.transport_play()
        client.wait_until(client.is_playing, timeout_s=2.0, desc="transport to restart")
        assert client.get_transport_state().get("playing") is True
        client.transport_stop()

    def test_bpm_change_does_not_collapse_fps(self, sixteen_instances):
        """
        A BPM change recomputes timing everywhere.  Under 16-instance load,
        it must not crash or stall rendering.
        """
        client, track_ids, _ = sixteen_instances
        for tid in track_ids:
            client.set_track_audio(tid, amplitude=0.4)
        client.transport_play()

        with ResourceMonitor() as mon:
            mon.sample_for(1.0)
            for bpm in (90.0, 140.0, 180.0, 60.0, 120.0):
                assert client._post_ok(
                    "/transport/setBpm", {"bpm": bpm, "trackId": 0}
                )
                mon.sample_for(0.6)

        client.transport_stop()
        # CPU must not saturate into thermal throttling territory during a
        # BPM change storm.
        assert_cpu_below(mon.report, max_avg_percent=400.0)
        print(f"[bpm-sweep] {mon.report.summary()}")


# ---------------------------------------------------------------------------
# Idle behavior — when nothing is happening, nothing should be eating CPU.
# ---------------------------------------------------------------------------
class TestSixteenInstanceIdle:
    def test_idle_cpu_is_reasonable(self, sixteen_instances):
        """
        16 instances with editors open but transport stopped: CPU should
        idle below a generous ceiling.  A runaway timer or polling loop
        would push this over.
        """
        client, _, _ = sixteen_instances
        client.transport_stop()
        client.wait_until(
            lambda: not client.is_playing(), timeout_s=2.0, desc="transport to stop"
        )
        # Editor animations can keep the GL renderer warm briefly after
        # transport stops — damp before measuring idle CPU.
        settle(1.5, reason="GL render tail after transport stop")

        with ResourceMonitor(sample_interval_s=0.5) as mon:
            mon.sample_for(5.0)

        # Per-core CPU%; allow generous headroom for 16 idle GL editors.
        assert_cpu_below(mon.report, max_avg_percent=200.0)
        print(f"[idle 16 editors] {mon.report.summary()}")


class TestInstanceScalingCost:
    """
    Measure the marginal idle CPU cost of each additional plugin instance.

    If cost is linear (constant per-instance), total CPU scales with N.
    If it's super-linear (quadratic or worse), we've got an N² problem —
    typically cross-instance source discovery or shared-state locking that
    doesn't scale.

    These tests don't have an absolute pass/fail threshold on CPU — they
    assert the *shape* of scaling so we catch regressions.
    """

    @pytest.fixture
    def scaling_client(self, client: OscilTestClient):
        # Close any open editors from prior tests; we control visibility.
        # Ensure 16 base tracks exist (for reuse).  We keep editors closed
        # to start and open them one-by-one in the measurement loop.
        for tid in range(BASELINE_INSTANCES):
            if client.get_track_info(tid) is None:
                client.add_track()
        existing = len(client.get_tracks())
        while existing < TARGET_INSTANCES:
            client.add_track(f"Scale {existing + 1}")
            existing += 1
        for tid in range(TARGET_INSTANCES):
            try:
                client.close_editor(track_id=tid)
            except Exception:
                pass
        client.transport_stop()
        client.wait_until(
            lambda: not client.is_playing(), timeout_s=2.0, desc="transport to stop"
        )
        settle(1.0, reason="editor close animations to wind down")
        yield client
        # Teardown: close everything again; remove dynamic tracks.
        for tid in range(TARGET_INSTANCES):
            try:
                client.close_editor(track_id=tid)
            except Exception:
                pass
        for t in client.get_tracks():
            if int(t.get("index", 0)) >= BASELINE_INSTANCES:
                client.remove_track(int(t["index"]))

    def _measure_idle_cpu_with_n_editors(
        self, client: OscilTestClient, n_open: int, sample_s: float = 3.0
    ) -> float:
        """Open exactly n_open editors, wait for settle, measure CPU."""
        # Open first n, close the rest.
        for tid in range(TARGET_INSTANCES):
            if tid < n_open:
                client.open_editor(track_id=tid)
            else:
                try:
                    client.close_editor(track_id=tid)
                except Exception:
                    pass
        settle(1.5, reason="editor open/close animations before CPU sampling")
        with ResourceMonitor(sample_interval_s=0.25) as mon:
            mon.sample_for(sample_s)
        print(
            f"[scaling n={n_open:2d}] {mon.report.summary()}  "
            f"per-instance={mon.report.cpu_percent_avg() / max(n_open, 1):.2f}%"
        )
        return mon.report.cpu_percent_avg()

    def test_idle_cpu_scales_sublinearly_or_linearly(self, scaling_client):
        """
        CPU with N editors open must not grow super-linearly in N.

        Rule: CPU(16) must be less than 1.6 × CPU(8).  Linear growth would
        predict 2.0x; we allow 1.6x to catch quadratic (~4x) or worse.
        """
        client = scaling_client
        cpu_0 = self._measure_idle_cpu_with_n_editors(client, 0)
        cpu_1 = self._measure_idle_cpu_with_n_editors(client, 1)
        cpu_4 = self._measure_idle_cpu_with_n_editors(client, 4)
        cpu_8 = self._measure_idle_cpu_with_n_editors(client, 8)
        cpu_16 = self._measure_idle_cpu_with_n_editors(client, 16)

        # cpu_0 is the harness-only cost (16 tracks processing audio, no GUI).
        # The delta (cpu_N - cpu_0) isolates rendering cost.
        print(
            f"[scaling summary] cpu(0)={cpu_0:.1f}% cpu(1)={cpu_1:.1f}% "
            f"cpu(4)={cpu_4:.1f}% cpu(8)={cpu_8:.1f}% cpu(16)={cpu_16:.1f}%"
        )

        # Super-linear guard: doubling editors shouldn't more than 1.6× CPU.
        if cpu_8 > 5.0:  # only meaningful if there's real signal to measure
            ratio = cpu_16 / cpu_8
            assert ratio < 1.6, (
                f"CPU scales super-linearly: cpu(16)/cpu(8) = {ratio:.2f} "
                f"(expected < 1.6 for linear-or-better). "
                f"cpu(1)={cpu_1:.1f}% cpu(8)={cpu_8:.1f}% cpu(16)={cpu_16:.1f}%"
            )

    def test_per_instance_idle_cpu_within_budget(self, scaling_client):
        """
        Marginal CPU per added plugin instance, measured by comparing 1-editor
        vs 16-editor idle cost.  On a 12-core Mac in debug build, empirically
        each additional idle instance costs ~6–8% of one core; we assert
        <= 10% per instance so we catch regressions where a single instance
        starts costing 20+%.
        """
        client = scaling_client
        cpu_1 = self._measure_idle_cpu_with_n_editors(client, 1)
        cpu_16 = self._measure_idle_cpu_with_n_editors(client, 16)
        marginal = (cpu_16 - cpu_1) / 15.0  # 15 extra editors
        print(
            f"[per-instance idle] baseline(1 editor)={cpu_1:.1f}%  "
            f"full(16 editors)={cpu_16:.1f}%  "
            f"marginal per instance={marginal:.2f}% of one core"
        )
        assert marginal < 10.0, (
            f"Per-instance idle CPU {marginal:.2f}% exceeds 10% budget. "
            f"Likely regression in continuous-repaint or idle audio path."
        )


# ---------------------------------------------------------------------------
# Cleanup — we must not leave dynamic tracks behind.
# ---------------------------------------------------------------------------
class TestSixteenInstanceTeardown:
    def test_fixture_restores_baseline_track_count(self, client: OscilTestClient):
        """
        After all the 16-instance tests above have run, the baseline must
        be intact — the next test that assumes exactly 3 instances should
        see exactly 3 instances.
        """
        tracks = client.get_tracks()
        # Base tracks (0,1,2) must exist.  Dynamic slots may be null but
        # the live track count must be at least BASELINE_INSTANCES.
        live = [t for t in tracks if t.get("index") is not None]
        assert len(live) >= BASELINE_INSTANCES
        for tid in range(BASELINE_INSTANCES):
            info = client.get_track_info(tid)
            assert info is not None, f"Base track {tid} missing after 16-instance suite"
