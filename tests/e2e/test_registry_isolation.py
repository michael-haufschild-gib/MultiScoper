"""
E2E coverage for per-track InstanceRegistry isolation (Stream 2.9).

MultiScoper's headline multi-instance feature relies on cross-instance source
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

from multiscoper_test_utils import MultiScoperTestClient


def _set_isolation(client: MultiScoperTestClient, enabled: bool) -> dict:
    resp = client._post_json("/daw/isolatedRegistries", {"enabled": enabled})
    assert resp is not None and resp.get("success"), f"isolation toggle failed: {resp}"
    return resp.get("data", {})


def _add_track(client: MultiScoperTestClient, name: str) -> int:
    resp = client._post_json("/daw/track/add", {"name": name})
    assert resp is not None and resp.get("success"), f"add_track failed: {resp}"
    return int(resp["data"]["trackIndex"])


def _remove_track(client: MultiScoperTestClient, index: int) -> None:
    resp = client._post_json("/daw/track/remove", {"trackIndex": index})
    assert resp is not None and resp.get("success"), f"remove_track failed: {resp}"


def _track_sources(client: MultiScoperTestClient, index: int) -> list[dict]:
    """Query the registry visible to a specific track."""
    r = client._get(f"/track/{index}/sources")
    assert r.status_code == 200, f"GET /track/{index}/sources -> {r.status_code}: {r.text}"
    body = r.json()
    assert body.get("success"), body
    return list(body.get("data", []))


class TestSharedRegistryBaseline:
    """Without isolation, every track sees every other track's source.

    Tests in this class are self-contained: they create their own tracks
    (rather than relying on the 3 baseline tracks having survived prior
    tests with their shared-registry state intact).  A prior test that
    removed and re-added a baseline track with `isolatedRegistries` toggled
    ON would leave that track with a private registry that this test was
    previously vulnerable to.
    """

    def test_baseline_tracks_share_registry(self, client: MultiScoperTestClient):
        # Make sure isolation is OFF before creating fresh tracks.
        _set_isolation(client, False)

        # Create three fresh tracks rather than trust the pre-existing ones.
        added = []
        try:
            added.append(_add_track(client, "SharedA"))
            added.append(_add_track(client, "SharedB"))
            added.append(_add_track(client, "SharedC"))

            # Each freshly added track is on the shared registry — so each
            # must see ALL three fresh sources (at minimum).
            fresh_sources_required = set()
            for idx in added:
                info = client.get_track_info(idx)
                assert info is not None, f"track {idx} must exist after add"
                fresh_sources_required.add(info["sourceId"])

            for track_index in added:
                sources = _track_sources(client, track_index)
                ids = {s["id"] for s in sources}
                missing = fresh_sources_required - ids
                assert not missing, (
                    f"Track {track_index} is missing fresh shared sources "
                    f"{missing}; sees only {sorted(ids)}"
                )
        finally:
            for idx in reversed(added):
                _remove_track(client, idx)


class TestIsolatedRegistry:
    """With isolation on, each newly-added track sees only its own source."""

    def test_isolation_toggle_reflects_state(self, client: MultiScoperTestClient):
        data_on = _set_isolation(client, True)
        assert data_on.get("isolatedRegistries") is True
        data_off = _set_isolation(client, False)
        assert data_off.get("isolatedRegistries") is False

    def test_new_tracks_have_private_registry(self, client: MultiScoperTestClient):
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

    def test_isolation_off_restores_shared_visibility(self, client: MultiScoperTestClient):
        """Turning isolation off and adding a new track puts it back on
        the shared registry.  Two tracks added while isolation is OFF
        must each see BOTH sources (they share).

        Previously this test counted baseline tracks (>=4 sources), which
        is fragile when earlier tests removed baselines.  The stable
        invariant is: two fresh add-off tracks see each other.
        """
        _set_isolation(client, False)
        added = []
        try:
            a = _add_track(client, "SharedAgainA")
            b = _add_track(client, "SharedAgainB")
            added = [a, b]

            info_a = client.get_track_info(a)
            info_b = client.get_track_info(b)
            assert info_a is not None and info_b is not None
            id_a = info_a["sourceId"]
            id_b = info_b["sourceId"]

            sources_a = {s["id"] for s in _track_sources(client, a)}
            sources_b = {s["id"] for s in _track_sources(client, b)}
            assert id_a in sources_a, "A must see its own source"
            assert id_b in sources_a, "A must see B's source (shared registry)"
            assert id_a in sources_b, "B must see A's source (shared registry)"
            assert id_b in sources_b, "B must see its own source"
        finally:
            for idx in reversed(added):
                _remove_track(client, idx)


class TestValidation:
    def test_missing_enabled_rejects(self, client: MultiScoperTestClient):
        r = client._post("/daw/isolatedRegistries", {})
        assert r.status_code == 400

    def test_nonexistent_track_sources_returns_404(self, client: MultiScoperTestClient):
        r = client._get("/track/9999/sources")
        assert r.status_code == 404
