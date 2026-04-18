"""
E2E coverage for per-track InstanceRegistry isolation (Stream 2.9).

Oscil's headline multi-instance feature relies on cross-instance source
discovery via a process-wide PluginFactory singleton. Under Logic Pro's
AU sandbox each plugin runs in its OWN process, so the singleton is
actually per-process — cross-discovery silently does not work.

Without this test the in-process harness structurally cannot reproduce
that failure mode: every track sees every other track's source through
the shared singleton.

POST /daw/isolatedRegistries {"enabled": true} tells TestDAW to give
every subsequently-added track its OWN InstanceRegistry instead of the
factory singleton's. The plugin processor gets that registry injected
via PluginProcessorConfig. Cross-instance discovery then fails exactly
the way it would in Logic AU.

This test:
  1. Confirms baseline behaviour (shared registry) — each track sees
     every other track's source.
  2. Enables isolation, adds fresh tracks, confirms each track's
     registry contains only its OWN source (the AU-sandbox failure
     mode is now reproducible in harness).
  3. Disables isolation and confirms subsequent adds are back on the
     shared registry.
"""

from __future__ import annotations

import pytest

from oscil_test_utils import OscilTestClient


def _set_isolation(client: OscilTestClient, enabled: bool) -> dict:
    resp = client._post_json("/daw/isolatedRegistries", {"enabled": enabled})
    assert resp is not None and resp.get("success"), f"isolation toggle failed: {resp}"
    return resp.get("data", {})


def _add_track(client: OscilTestClient, name: str) -> int:
    resp = client._post_json("/daw/track/add", {"name": name})
    assert resp is not None and resp.get("success"), f"add_track failed: {resp}"
    return int(resp["data"]["trackIndex"])


def _remove_track(client: OscilTestClient, index: int) -> None:
    resp = client._post_json("/daw/track/remove", {"trackIndex": index})
    assert resp is not None and resp.get("success"), f"remove_track failed: {resp}"


def _track_sources(client: OscilTestClient, index: int) -> list[dict]:
    """Query the registry visible to a specific track."""
    r = client._get(f"/track/{index}/sources")
    assert r.status_code == 200, f"GET /track/{index}/sources -> {r.status_code}: {r.text}"
    body = r.json()
    assert body.get("success"), body
    return list(body.get("data", []))


class TestSharedRegistryBaseline:
    """Without isolation, every track sees every other track's source."""

    def test_baseline_tracks_share_registry(self, client: OscilTestClient):
        # Make sure isolation is OFF (default state).
        _set_isolation(client, False)

        # The 3 baseline tracks already exist. Each should see all 3.
        for track_index in (0, 1, 2):
            sources = _track_sources(client, track_index)
            ids = {s["id"] for s in sources}
            assert len(ids) >= 3, (
                f"Track {track_index} sees only {len(ids)} sources "
                f"(expected >= 3 because registry is shared): {sorted(ids)}"
            )


class TestIsolatedRegistry:
    """With isolation on, each newly-added track sees only its own source."""

    def test_isolation_toggle_reflects_state(self, client: OscilTestClient):
        data_on = _set_isolation(client, True)
        assert data_on.get("isolatedRegistries") is True
        data_off = _set_isolation(client, False)
        assert data_off.get("isolatedRegistries") is False

    def test_new_tracks_have_private_registry(self, client: OscilTestClient):
        """
        The core regression test.  Under isolation, adding two new tracks
        must NOT expose their sources to each other — because their
        registries are per-track, not shared.  This reproduces the
        Logic AU sandbox failure mode.
        """
        # Note the baseline tracks (and their registry) from the run.
        baseline_track_indices = [t["index"] for t in client.get_tracks()]

        _set_isolation(client, True)
        try:
            idx_a = _add_track(client, "IsolatedA")
            idx_b = _add_track(client, "IsolatedB")

            # Each isolated track's registry sees exactly 1 source: its own.
            sources_a = _track_sources(client, idx_a)
            sources_b = _track_sources(client, idx_b)

            assert len(sources_a) == 1, (
                f"IsolatedA should see only its OWN source under "
                f"registry isolation, got {len(sources_a)}: "
                f"{[s['id'] for s in sources_a]}"
            )
            assert len(sources_b) == 1, (
                f"IsolatedB should see only its OWN source under "
                f"registry isolation, got {len(sources_b)}"
            )
            # And the two registries must not overlap — isolation means
            # A's source id is absent from B's registry.
            ids_a = {s["id"] for s in sources_a}
            ids_b = {s["id"] for s in sources_b}
            assert not (ids_a & ids_b), (
                f"Isolated registries should not share source ids: "
                f"A={ids_a} B={ids_b}"
            )

            # Pre-existing (baseline) tracks keep their shared registry
            # and should continue to see the baseline set.  They do NOT
            # gain visibility into the new isolated tracks.
            for shared_track_index in baseline_track_indices:
                shared_sources = _track_sources(client, shared_track_index)
                shared_ids = {s["id"] for s in shared_sources}
                assert not (shared_ids & ids_a), (
                    f"Baseline track {shared_track_index} leaked "
                    f"IsolatedA's source into the shared registry: "
                    f"{shared_ids & ids_a}"
                )
        finally:
            # Tear down the isolated tracks and turn isolation off.
            tracks = client.get_tracks()
            for t in tracks:
                if t.get("name", "").startswith("Isolated"):
                    _remove_track(client, t["index"])
            _set_isolation(client, False)

    def test_isolation_off_restores_shared_visibility(self, client: OscilTestClient):
        """Turning isolation off and adding a new track puts it back on
        the shared registry — same source visibility as baseline."""
        _set_isolation(client, False)
        try:
            idx = _add_track(client, "SharedAgain")
            sources = _track_sources(client, idx)
            assert len(sources) >= 4, (
                f"Track added with isolation OFF should see all baseline "
                f"sources plus its own (>=4), got {len(sources)}"
            )
        finally:
            tracks = client.get_tracks()
            for t in tracks:
                if t.get("name") == "SharedAgain":
                    _remove_track(client, t["index"])


class TestValidation:
    def test_missing_enabled_rejects(self, client: OscilTestClient):
        r = client._post("/daw/isolatedRegistries", {})
        assert r.status_code == 400

    def test_nonexistent_track_sources_returns_404(self, client: OscilTestClient):
        r = client._get("/track/9999/sources")
        assert r.status_code == 404
