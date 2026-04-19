"""
E2E test for multi-track project save → close → reopen.

Reproduces the Bitwig save/reload scenario the user reported:

  1. Project has 2 tracks, each with a MultiScoper instance.
  2. On each instance the user adds oscillators, with track A's oscillator
     bound to track B's source (cross-instance reference).
  3. User saves the project, closes it, reopens it.

Before the fix: on reopen, every oscillator's sourceId was ephemeral —
neither Track A nor Track B persisted a stable identity, so cross-instance
bindings pointed at sourceIds that no longer existed. The editor then ran
``onSourcesChanged`` during construction and called ``clearSource`` on
every unresolved binding — leaving the list showing "Self" and rendering
no waveform.

After the fix: ``trackIdentifier_`` is persisted in the plugin's state
XML, and ``InstanceRegistry::insertNewSource`` derives the SourceId
deterministically from that identifier. Same persisted identifier across
sessions → same SourceId → cross-instance bindings resolve again.

Endpoints exercised
-------------------
  POST /project/save    — bundle every live track's state into one file
  POST /project/close   — destroy all TestTracks (runs plugin dtors, clears
                          the InstanceRegistry)
  POST /project/reopen  — recreate tracks from the bundle and restore each
                          one's state XML

Assertions
----------
  1. Bundle file contains one entry per pre-save track.
  2. After /project/close the DAW has zero active tracks and the shared
     InstanceRegistry is empty.
  3. After /project/reopen the track count matches the bundle, each track
     re-registers with the SAME SourceId as pre-close (proving stable
     identity), and oscillator bindings resolve (proving cross-instance
     references survived).
"""

from __future__ import annotations

import os
import tempfile

import pytest

from multiscoper_test_utils import MultiScoperTestClient


def _post(client: MultiScoperTestClient, path: str, body: dict | None = None) -> dict:
    resp = client._post_json(path, body or {})
    assert resp is not None, f"{path}: no response"
    assert resp.get("success"), f"{path} failed: {resp}"
    return resp.get("data", {})


@pytest.fixture
def bundle_path():
    """Temp file for the project bundle; removed after the test."""
    fd, path = tempfile.mkstemp(suffix=".multiscoper.json", prefix="e2e_project_")
    os.close(fd)
    os.unlink(path)  # /project/save creates it; we just want the path
    yield path
    if os.path.exists(path):
        os.unlink(path)


@pytest.fixture
def two_track_session(client: MultiScoperTestClient):
    """
    Put the harness into a known 2-track state, editor open on track 0.
    Yields the client. Tears every track down on exit so subsequent tests
    start clean — ``/project/close`` + an ``add_track`` baseline rebuild.
    """
    # Start from zero tracks so track indices are predictable.
    _post(client, "/project/close")
    assert client.get_tracks() == []

    t0 = client.add_track("Track A")
    t1 = client.add_track("Track B")
    assert t0 is not None and t1 is not None, "add_track must succeed"

    client.open_editor(track_id=0)
    yield client

    # Teardown: close project so the next fixture run starts from empty.
    _post(client, "/project/close")
    # Restore the baseline track the other tests rely on.
    client.add_track("Baseline")


class TestProjectSaveReload:
    def test_bundle_contains_one_entry_per_track(
        self, two_track_session: MultiScoperTestClient, bundle_path: str
    ):
        """``/project/save`` writes a JSON bundle whose ``tracks`` array
        has exactly one entry per live track, with ``xml`` populated."""
        data = _post(two_track_session, "/project/save", {"path": bundle_path})
        assert data.get("trackCount") == 2, data

        import json
        with open(bundle_path) as f:
            bundle = json.load(f)
        assert "tracks" in bundle
        assert len(bundle["tracks"]) == 2
        for entry in bundle["tracks"]:
            # JUCE prepends an XML declaration; match anywhere in the doc.
            assert "<MultiScoperState" in entry["xml"], entry["xml"][:200]
            # The fix persists the trackIdentifier in the XML — this is
            # what lets sessions recreate the same SourceIds.
            assert "trackIdentifier" in entry["xml"], entry["xml"][:200]

    def test_close_tears_down_every_track(
        self, two_track_session: MultiScoperTestClient, bundle_path: str
    ):
        """After /project/close the DAW has no live tracks and the shared
        InstanceRegistry is empty."""
        _post(two_track_session, "/project/save", {"path": bundle_path})
        data = _post(two_track_session, "/project/close")
        assert data.get("removed") == 2
        assert data.get("remaining") == 0
        assert two_track_session.get_tracks() == []
        # get_sources hits the shared registry — expected empty after close.
        assert two_track_session.get_sources() == []

    def test_reopen_restores_same_source_ids(
        self, two_track_session: MultiScoperTestClient, bundle_path: str
    ):
        """Reopening a saved bundle registers each track under the SAME
        SourceId it had before close. This is the invariant that lets
        cross-instance oscillator bindings survive a project reload."""
        tracks_before = two_track_session.get_tracks()
        assert len(tracks_before) == 2
        pre_source_ids = sorted(t["sourceId"] for t in tracks_before)

        _post(two_track_session, "/project/save", {"path": bundle_path})
        _post(two_track_session, "/project/close")
        _post(two_track_session, "/project/reopen", {"path": bundle_path})

        tracks_after = two_track_session.get_tracks()
        assert len(tracks_after) == 2, f"expected 2 tracks after reopen, got {tracks_after}"
        post_source_ids = sorted(t["sourceId"] for t in tracks_after)
        assert post_source_ids == pre_source_ids, (
            "SourceIds must match pre-close values — stable identity is the "
            "whole point of persisting trackIdentifier. "
            f"before={pre_source_ids} after={post_source_ids}"
        )

    def test_cross_instance_oscillator_binding_survives_reload(
        self, two_track_session: MultiScoperTestClient, bundle_path: str
    ):
        """End-to-end regression for the Bitwig bug. Track A has an
        oscillator bound to Track B's source. After save+close+reopen the
        binding must still resolve — the oscillator's sourceId must match
        Track B's (new, but deterministically equal) sourceId, and the
        registry must expose Track B under that id."""
        tracks = two_track_session.get_tracks()
        track_a_source = next(t["sourceId"] for t in tracks if t["index"] == 0)
        track_b_source = next(t["sourceId"] for t in tracks if t["index"] == 1)

        # Track 0's editor auto-creates a default oscillator on open; clear
        # so the test owns the state entirely.
        two_track_session.reset_track_state(0)
        two_track_session.wait_until(
            lambda: len(two_track_session.get_oscillators_for_track(0)) == 0,
            timeout_s=3.0,
            desc="track 0 oscillators cleared",
        )

        osc_id = two_track_session.add_oscillator_to_track(
            track_id=0, source_id=track_b_source, name="Cross-bound to B"
        )
        assert osc_id, "cross-bound oscillator must be created"

        pre_oscs = two_track_session.get_oscillators_for_track(0)
        assert len(pre_oscs) == 1, pre_oscs
        assert pre_oscs[0]["sourceId"] == track_b_source, (
            f"Pre-save sanity check: oscillator should reference Track B's source "
            f"({track_b_source}), got {pre_oscs[0]['sourceId']}"
        )

        _post(two_track_session, "/project/save", {"path": bundle_path})
        _post(two_track_session, "/project/close")
        _post(two_track_session, "/project/reopen", {"path": bundle_path})

        # Re-query: Track B's source must be registered, and Track A's
        # oscillator must still point at it.
        post_tracks = two_track_session.get_tracks()
        track_b_source_after = next(t["sourceId"] for t in post_tracks if t["index"] == 1)
        assert track_b_source_after == track_b_source, (
            "Track B's sourceId must be stable across save/reopen — otherwise the "
            "whole fix is meaningless."
        )

        post_oscs = two_track_session.get_oscillators_for_track(0)
        assert len(post_oscs) == 1, f"expected one oscillator after reopen, got {post_oscs}"
        assert post_oscs[0]["sourceId"] == track_b_source_after, (
            f"Cross-instance binding lost after reopen. "
            f"Expected sourceId={track_b_source_after} (Track B), "
            f"got {post_oscs[0]['sourceId']}. "
            f"This is the exact bug the user reported in Bitwig."
        )

        # Registry must expose Track B under the restored sourceId so the
        # UI resolves the binding (otherwise OscillatorListItem falls
        # through to the "Self" branch — the exact user-visible bug).
        sources = {s["id"]: s["name"] for s in two_track_session.get_sources()}
        assert track_b_source_after in sources, (
            f"Track B's source missing from registry after reopen: {sources}. "
            f"Without this entry the oscillator item would render as 'Self'."
        )
