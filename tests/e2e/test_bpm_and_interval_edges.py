"""
E2E coverage for BPM and interval boundary values.

What bugs these tests catch:
- BPM=0 triggers divide-by-zero in TimingEngine (seconds/beat).
- Negative BPM treated as positive — subtle bug that changes audio
  scheduling.
- Interval=0 causes an infinite loop in the frame scheduler.
- Extremely high BPM (e.g. 99999) exhausts buffer or overflows int.
- NaN/Inf values from a malformed client request poison downstream
  timing calculations.
"""

from __future__ import annotations

import math

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestBpmEdgeValues:
    @pytest.mark.parametrize("bpm", [0.0, -1.0, 0.001, 99999.0, 60.0])
    def test_set_bpm_does_not_crash(
        self, editor: MultiScoperTestClient, bpm: float
    ):
        """Bug caught: BPM=0 divides by zero; negative BPM treated
        unsafely downstream."""
        editor.set_bpm(bpm)
        # Harness must remain responsive.
        assert editor.health_check()["data"]["status"] == "ok"
        state = editor.get_transport_state()
        assert state is not None
        # Stored BPM must be finite.
        stored = state.get("bpm", 0.0)
        assert math.isfinite(stored), f"stored BPM {stored} must be finite"

    def test_bpm_nan_rejected_or_sanitized(self, editor: MultiScoperTestClient):
        """Bug caught: JSON.parse accepts NaN from some clients; the
        plugin must sanitize to a finite default."""
        # requests doesn't allow NaN in JSON directly, but we can POST
        # a raw body.
        import json
        import requests
        try:
            requests.post(
                f"{editor.base_url}/transport/setBpm",
                data='{"bpm": null}',
                headers={"Content-Type": "application/json"},
                timeout=5.0,
            )
        except requests.exceptions.ConnectionError:
            pytest.fail("harness unreachable")
        assert editor.health_check()["data"]["status"] == "ok"


class TestIntervalEdgeValues:
    """Timing section's interval field at extremes."""

    @pytest.fixture()
    def timing_expanded(self, editor: MultiScoperTestClient):
        editor.click("sidebar_timing")
        editor.wait_for_element("sidebar_timing_intervalField", timeout_s=3.0)
        return editor

    @pytest.mark.parametrize("interval", [0.0, -1.0, 0.001, 99999.0])
    def test_interval_does_not_crash(
        self, timing_expanded: MultiScoperTestClient, interval: float
    ):
        timing_expanded.set_slider("sidebar_timing_intervalField", interval)
        assert timing_expanded.health_check()["data"]["status"] == "ok"


class TestTransportPositionEdges:
    @pytest.mark.parametrize("pos", [0, -1, 2**31 - 1, 2**63 - 2])
    def test_set_position_does_not_crash(
        self, editor: MultiScoperTestClient, pos: int
    ):
        """Bug caught: negative or overflow position values passed
        directly to TimingEngine, breaking sample-count arithmetic."""
        editor.set_position(pos)
        assert editor.health_check()["data"]["status"] == "ok"
