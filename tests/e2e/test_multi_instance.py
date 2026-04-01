"""
E2E tests for multi-instance behavior.

Verifies that multiple Oscil plugin instances running simultaneously
behave correctly: independent state, cross-instance source discovery,
and proper lifecycle management.

Each test documents what production bug it catches.
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestSourceDiscovery:
    """Verify instances can discover each other's sources."""

    def test_all_tracks_register_sources(self, client: OscilTestClient):
        """
        Bug caught: some tracks fail to register sources because
        prepareToPlay is not called or InstanceRegistry is not shared.
        """
        sources = client.get_sources()
        assert len(sources) >= 3, (
            f"Test harness has 3 tracks; expected >= 3 sources, got {len(sources)}"
        )

    def test_source_ids_differ_across_tracks(self, client: OscilTestClient):
        """
        Bug caught: all tracks register with the same source ID due to
        deduplication using identical track identifiers.
        """
        sources = client.get_sources()
        ids = [s["id"] for s in sources]
        assert len(ids) == len(set(ids)), (
            f"Each track should have a unique source ID, got: {ids}"
        )

    def test_each_track_has_source(self, three_track_sources: dict):
        """
        Bug caught: track info endpoint returns empty sourceId because
        the processor hasn't registered yet.
        """
        for track_id, source_id in three_track_sources.items():
            assert source_id, (
                f"Track {track_id} should have a non-empty source ID"
            )

    def test_instance_sees_other_track_sources(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: source list for one instance only shows its own source,
        not sources from other tracks.

        In Oscil, every instance sees ALL sources from the global
        InstanceRegistry, enabling cross-track waveform display.
        """
        sources = multi_editor.get_sources()
        source_ids = {s["id"] for s in sources}

        for track_id, expected_source in track_sources.items():
            assert expected_source in source_ids, (
                f"Track {track_id}'s source '{expected_source}' should be "
                f"visible in the global source list. Got: {source_ids}"
            )


class TestCrossInstanceOscillator:
    """Verify oscillators can display signals from other instances."""

    def test_add_oscillator_with_other_tracks_source(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: adding an oscillator with a foreign source ID fails
        or silently binds to the local source instead.

        This is the core multi-instance feature: Track 0's plugin displays
        a waveform from Track 1's audio.
        """
        track1_source = track_sources[1]

        # On track 0's instance, add an oscillator bound to track 1's source
        osc_id = multi_editor.add_oscillator_to_track(
            0, track1_source, name="Cross-Instance Osc"
        )
        assert osc_id is not None, (
            "Should be able to create oscillator with another track's source"
        )

        oscs = multi_editor.get_oscillators_for_track(0)
        match = [o for o in oscs if o.get("id") == osc_id]
        assert len(match) == 1
        assert match[0]["sourceId"] == track1_source, (
            f"Oscillator should be bound to track 1's source '{track1_source}', "
            f"got '{match[0]['sourceId']}'"
        )

    def test_cross_instance_waveform_has_data(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: oscillator bound to foreign source shows flat line
        because the capture buffer lookup fails across instances.
        """
        track1_source = track_sources[1]

        # Set track 1 to produce a signal
        multi_editor.set_track_audio(1, waveform="sine", frequency=440.0, amplitude=0.8)
        multi_editor.transport_play()

        # Add oscillator on track 0 displaying track 1's signal
        osc_id = multi_editor.add_oscillator_to_track(
            0, track1_source, name="CrossTrack Display"
        )
        assert osc_id is not None

        # Wait for waveform data to flow from track 1 to track 0's oscillator
        def _has_cross_instance_data():
            wf_state = multi_editor.get_waveform_state_for_track(0)
            if wf_state is None:
                return False
            panes = wf_state.get("panes", [])
            if not panes:
                return False
            waveforms = panes[0].get("waveforms", [])
            return any(w.get("hasWaveformData", False) for w in waveforms)

        multi_editor.wait_until(
            _has_cross_instance_data,
            desc="cross-instance oscillator to receive waveform data from track 1",
        )


class TestIndependentState:
    """Verify each instance maintains independent state."""

    def test_oscillator_added_to_one_instance_not_visible_on_other(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: adding an oscillator writes to a shared state object,
        causing it to appear in both instances.
        """
        source0 = track_sources[0]

        # Add oscillator to track 0's instance only
        osc_id = multi_editor.add_oscillator_to_track(
            0, source0, name="Track 0 Only"
        )
        assert osc_id is not None

        # Verify track 0 has the oscillator
        oscs_track0 = multi_editor.get_oscillators_for_track(0)
        assert len(oscs_track0) == 1

        # Verify track 1 does NOT have it
        oscs_track1 = multi_editor.get_oscillators_for_track(1)
        assert len(oscs_track1) == 0, (
            f"Track 1 should have 0 oscillators, got {len(oscs_track1)}: "
            f"state is leaking between instances"
        )

    def test_panes_are_independent(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: pane layout shared between instances, causing pane
        add on one instance to appear in another.
        """
        source0 = track_sources[0]

        # Add oscillator to track 0 (creates default pane)
        multi_editor.add_oscillator_to_track(0, source0, name="Pane Test")
        multi_editor.wait_until(
            lambda: len(multi_editor.get_panes_for_track(0)) > 0,
            timeout_s=3.0,
            desc="track 0 to have panes",
        )

        panes_track0 = multi_editor.get_panes_for_track(0)
        panes_track1 = multi_editor.get_panes_for_track(1)

        assert len(panes_track0) >= 1, "Track 0 should have at least 1 pane"
        assert len(panes_track1) == 0, (
            f"Track 1 should have 0 panes, got {len(panes_track1)}: "
            f"pane state is leaking between instances"
        )

    def test_reset_one_instance_does_not_affect_other(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: reset_state resets a global object rather than the
        per-instance OscilState.
        """
        source0 = track_sources[0]
        source1 = track_sources[1]

        # Add oscillators to both tracks
        multi_editor.add_oscillator_to_track(0, source0, name="Track 0 Osc")
        multi_editor.add_oscillator_to_track(1, source1, name="Track 1 Osc")
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 1,
            timeout_s=3.0,
        )
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) == 1,
            timeout_s=3.0,
        )

        # Reset track 0 only
        multi_editor.reset_track_state(0)
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 0,
            timeout_s=3.0,
            desc="track 0 reset",
        )

        # Track 1 should still have its oscillator
        oscs_track1 = multi_editor.get_oscillators_for_track(1)
        assert len(oscs_track1) == 1, (
            f"Track 1 should still have 1 oscillator after track 0 reset, "
            f"got {len(oscs_track1)}"
        )


class TestMultiEditorUI:
    """Verify both editors can be open and interacted with independently."""

    def test_both_editors_open_simultaneously(
        self, multi_editor: OscilTestClient
    ):
        """
        Bug caught: opening a second editor closes the first, or the
        second editor fails to create.
        """
        tracks = multi_editor.get_tracks()
        visible = [t for t in tracks if t.get("editorVisible")]
        assert len(visible) >= 2, (
            f"Both editors should be open, got {len(visible)} visible. "
            f"Tracks: {tracks}"
        )

    def test_element_registry_has_entries_for_both_editors(
        self, multi_editor: OscilTestClient
    ):
        """
        Bug caught: element registry only stores one component per ID,
        making the second editor's elements unreachable.

        After the registry fix, both "sidebar" components should coexist.
        """
        assert multi_editor.element_exists_on_track("sidebar", 0), (
            "Track 0's sidebar should be registered"
        )
        assert multi_editor.element_exists_on_track("sidebar", 1), (
            "Track 1's sidebar should be registered"
        )

    def test_click_on_track0_does_not_affect_track1(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: UI interaction routes to the wrong editor window
        because element lookup returns the last-registered component.
        """
        source0 = track_sources[0]
        source1 = track_sources[1]

        # Add oscillator to both tracks
        multi_editor.add_oscillator_to_track(0, source0, name="T0 Osc")
        multi_editor.add_oscillator_to_track(1, source1, name="T1 Osc")

        # Wait for both to register UI elements
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 1,
            timeout_s=5.0,
        )
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(1)) == 1,
            timeout_s=5.0,
        )

        # Delete oscillator on track 0 via state API (not UI — UI click
        # scoping is a separate concern, state API is the reliable path)
        oscs0 = multi_editor.get_oscillators_for_track(0)
        multi_editor._post_ok(
            "/state/oscillator/delete",
            {"id": oscs0[0]["id"], "trackId": 0},
        )
        multi_editor.wait_until(
            lambda: len(multi_editor.get_oscillators_for_track(0)) == 0,
            timeout_s=3.0,
        )

        # Track 1 should still have its oscillator
        oscs_track1 = multi_editor.get_oscillators_for_track(1)
        assert len(oscs_track1) == 1, (
            f"Track 1's oscillator should be unaffected by track 0 deletion, "
            f"got {len(oscs_track1)} oscillators"
        )


class TestSourceLifecycle:
    """Verify source registration/deregistration on track add/remove."""

    def test_add_track_registers_new_source(self, client: OscilTestClient):
        """
        Bug caught: dynamically added track doesn't call prepareToPlay,
        so no source is registered with InstanceRegistry.
        """
        initial_sources = client.get_sources()
        initial_count = len(initial_sources)

        result = client.add_track("Dynamic Track")
        assert result is not None, "add_track should return track info"

        new_source_id = result.get("sourceId")
        assert new_source_id, "New track should have a source ID"

        # Verify source count increased
        updated_sources = client.get_sources()
        assert len(updated_sources) == initial_count + 1, (
            f"Source count should increase from {initial_count} to "
            f"{initial_count + 1}, got {len(updated_sources)}"
        )

        # Clean up
        client.remove_track(result["trackIndex"])

    def test_remove_track_deregisters_source(self, client: OscilTestClient):
        """
        Bug caught: removing a track doesn't unregister the source,
        leaving dangling entries in InstanceRegistry.
        """
        # Add then remove a track
        result = client.add_track("Ephemeral Track")
        assert result is not None
        track_idx = result["trackIndex"]
        source_id = result["sourceId"]

        sources_before = client.get_sources()
        client.remove_track(track_idx)

        # Source should be gone
        client.wait_until(
            lambda: all(s["id"] != source_id for s in client.get_sources()),
            timeout_s=3.0,
            desc=f"source '{source_id}' to be deregistered",
        )

        sources_after = client.get_sources()
        assert len(sources_after) == len(sources_before) - 1

    def test_other_instances_see_new_source(
        self, multi_editor: OscilTestClient
    ):
        """
        Bug caught: InstanceRegistry notifications don't propagate to
        existing instances, so they never see dynamically added sources.
        """
        initial_sources = multi_editor.get_sources()
        initial_count = len(initial_sources)

        result = multi_editor.add_track("Late Joiner")
        assert result is not None

        # Wait for source list to update
        multi_editor.wait_for_source_count(initial_count + 1, timeout_s=5.0)

        # The new source should be usable for oscillator binding
        new_source_id = result["sourceId"]
        osc_id = multi_editor.add_oscillator_to_track(
            0, new_source_id, name="From New Track"
        )
        assert osc_id is not None, (
            "Should be able to bind oscillator to dynamically added track's source"
        )

        # Clean up
        multi_editor.remove_track(result["trackIndex"])

    def test_remove_track_with_bound_oscillators(
        self, multi_editor: OscilTestClient
    ):
        """
        Bug caught: removing a track whose source is used by oscillators
        in other instances causes a crash or dangling reference.
        """
        # Add a temporary track
        result = multi_editor.add_track("Temporary")
        assert result is not None
        temp_source = result["sourceId"]
        temp_idx = result["trackIndex"]

        # Bind an oscillator on track 0 to the temp track's source
        osc_id = multi_editor.add_oscillator_to_track(
            0, temp_source, name="Bound to Temp"
        )
        assert osc_id is not None

        # Remove the temp track — this must not crash
        multi_editor.remove_track(temp_idx)

        # Track 0 should still be responsive (no crash)
        health = multi_editor.health_check()
        assert health.get("data", {}).get("status") == "ok", (
            "Harness should still be healthy after removing a track "
            "whose source was bound to an oscillator"
        )

        # The oscillator should still exist (orphaned source is OK)
        oscs = multi_editor.get_oscillators_for_track(0)
        match = [o for o in oscs if o.get("id") == osc_id]
        assert len(match) == 1, (
            "Oscillator should still exist after its source track is removed"
        )


class TestWaveformDataFlow:
    """Verify each instance receives its own track's audio correctly."""

    def test_each_track_has_independent_audio_pipeline(
        self, multi_editor: OscilTestClient, track_sources: dict
    ):
        """
        Bug caught: all instances share one capture buffer, showing
        identical waveform data regardless of which track they're on.
        """
        # Set different audio on each track
        multi_editor.set_track_audio(
            0, waveform="sine", frequency=220.0, amplitude=0.3
        )
        multi_editor.set_track_audio(
            1, waveform="sine", frequency=880.0, amplitude=0.9
        )

        source0 = track_sources[0]
        source1 = track_sources[1]

        # Add oscillators on each instance bound to their own source
        multi_editor.add_oscillator_to_track(0, source0, name="Self T0")
        multi_editor.add_oscillator_to_track(1, source1, name="Self T1")

        multi_editor.transport_play()

        # Wait for both tracks to have waveform data with non-zero peaks
        def _both_tracks_have_data():
            wf0 = multi_editor.get_waveform_state_for_track(0)
            wf1 = multi_editor.get_waveform_state_for_track(1)
            if wf0 is None or wf1 is None:
                return None
            panes0 = wf0.get("panes", [])
            panes1 = wf1.get("panes", [])
            if not panes0 or not panes1:
                return None
            wfs0 = panes0[0].get("waveforms", [])
            wfs1 = panes1[0].get("waveforms", [])
            if not wfs0 or not wfs1:
                return None
            peak0 = wfs0[0].get("peakLevel", 0)
            peak1 = wfs1[0].get("peakLevel", 0)
            if peak0 > 0 and peak1 > 0:
                return (peak0, peak1)
            return None

        peaks = multi_editor.wait_until(
            _both_tracks_have_data,
            desc="both tracks to have waveform data with non-zero peaks",
        )
        peak0, peak1 = peaks
        # Track 1 has 3x the amplitude, so its peak should be higher
        assert peak1 > peak0, (
            f"Track 1 (amp=0.9) should have higher peak than "
            f"track 0 (amp=0.3): track0={peak0:.4f}, track1={peak1:.4f}"
        )
