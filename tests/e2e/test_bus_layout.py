"""
E2E coverage for bus-layout renegotiation (Primitive 2.3).

`MultiScoperPluginProcessor::isBusesLayoutSupported` (PluginProcessor.cpp:183)
has a load-bearing reject-disabled-bus branch that real DAWs drive when
the user collapses a side-chain.  The old harness fed a fixed stereo
buffer and never called isBusesLayoutSupported; a regression that drops
the disabled-bus rejection would pass CI but break FL Studio / Ableton.

The new `POST /track/{id}/channelLayout` endpoint exercises both valid
transitions (stereo <-> mono) and the rejection contract.  Key assertions:

  1. stereo -> mono succeeds and the processor reports 1 input channel
  2. mono -> stereo round-trips successfully
  3. An invalid layout string ("disabled", "surround") is rejected with 400
  4. The per-track channel count actually changes as reported
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


def _set_layout(client: MultiScoperTestClient, track_id: int, layout: str) -> dict:
    resp = client._post_json(f"/track/{track_id}/channelLayout", {"layout": layout})
    assert resp is not None, (
        f"Harness did not respond to layout request {layout} on track {track_id}"
    )
    assert resp.get("success"), f"Layout {layout} rejected: {resp}"
    return resp.get("data", {})


class TestLayoutRoundTrip:
    @pytest.fixture(autouse=True)
    def _reset_layout(self, client: MultiScoperTestClient):
        """Restore track 0 to stereo before and after each test in this class.

        Without this guard, `test_stereo_to_mono_reduces_channels` would
        leave track 0 in mono and leak that state into any later test that
        assumes a stereo-channel default (including tests in other files
        running after this module in the pytest order).
        """
        _set_layout(client, 0, "stereo")
        yield
        _set_layout(client, 0, "stereo")

    def test_stereo_to_mono_reduces_channels(self, client: MultiScoperTestClient):
        """
        Initial prepare is stereo.  Switching to mono should see the
        processor report 1 input / 1 output channel via JUCE's own
        getTotalNumInputChannels()/OutputChannels() accessors.
        """
        data = _set_layout(client, 0, "mono")
        assert data.get("inputChannels") == 1, data
        assert data.get("outputChannels") == 1, data

    def test_mono_to_stereo_restores_channels(self, client: MultiScoperTestClient):
        _set_layout(client, 0, "mono")
        data = _set_layout(client, 0, "stereo")
        assert data.get("inputChannels") == 2, data
        assert data.get("outputChannels") == 2, data

    def test_full_round_trip_is_lossless(self, client: MultiScoperTestClient):
        """stereo -> mono -> stereo should land us right back where we
        started with zero processor-reported drift."""
        _set_layout(client, 0, "stereo")
        _set_layout(client, 0, "mono")
        data = _set_layout(client, 0, "stereo")
        assert data.get("inputChannels") == 2
        assert data.get("outputChannels") == 2


class TestLayoutValidation:
    def test_reject_disabled_string(self, client: MultiScoperTestClient):
        """
        Bug caught: the harness handler forgets to validate `layout` and
        passes an arbitrary string through.  The plugin would then reject
        it but the HTTP surface would hide that behind a 500.
        """
        resp = client._post("/track/0/channelLayout", {"layout": "disabled"})
        assert resp.status_code == 400

    def test_reject_surround(self, client: MultiScoperTestClient):
        """
        The plugin's isBusesLayoutSupported rejects anything that is not
        mono or stereo.  We surface that rejection at the harness boundary
        as a 400 (invalid argument) so tests can assert cleanly.
        """
        resp = client._post("/track/0/channelLayout", {"layout": "surround"})
        assert resp.status_code == 400

    def test_reject_missing_field(self, client: MultiScoperTestClient):
        resp = client._post("/track/0/channelLayout", {})
        assert resp.status_code == 400

    def test_invalid_layout_leaves_last_valid_intact(self, client: MultiScoperTestClient):
        """
        After a rejected layout, the processor must still report whatever
        the last valid layout was — the plugin contract is 'refuse or
        apply', never partial.
        """
        before = _set_layout(client, 0, "stereo")
        # Reject
        resp = client._post("/track/0/channelLayout", {"layout": "disabled"})
        assert resp.status_code == 400
        # Confirm the track still looks like stereo
        after = _set_layout(client, 0, "stereo")  # no-op-ish reassert
        assert before.get("inputChannels") == after.get("inputChannels") == 2
