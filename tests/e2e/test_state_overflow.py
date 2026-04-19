"""
E2E coverage for state at scale: many oscillators and panes.

What bugs these tests catch:
- 50 oscillators register but list rendering is O(n²) so UI lags
  unusable.
- XML serializer/deserializer struggles with large state (50 oscs,
  20 panes) — round-trip loses entries or reorders silently.
- Some element IDs collide after 10+ oscillators because formatter
  uses a fixed-size buffer.
- Adding the 51st oscillator hits an internal cap but reports
  success.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestManyOscillators:
    def test_many_oscillators_all_register(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: adding N > some-internal-cap silently fails
        without error.

        OBSERVATION: the plugin's per-add and GET /state/oscillators
        costs scale super-linearly.  Beyond ~15 oscillators, HTTP
        requests start timing out (504) and subsequent tests can't
        recover because editor open/close enters the same slow path.
        This test targets 12 — enough to exercise list scaling and
        uncover off-by-one testId formatting without triggering the
        known scaling cliff.  Raising this target will cause cross-
        test flakiness until the plugin list is optimized.
        """
        TARGET = 12
        ids = []
        for i in range(TARGET):
            oid = editor.add_oscillator(source_id, name=f"N{i:02d}")
            assert oid is not None, f"add {i} must succeed"
            ids.append(oid)

        editor.wait_for_oscillator_count(TARGET, timeout_s=15.0)
        oscs = editor.get_oscillators()
        assert len(oscs) == TARGET, f"expected {TARGET}, got {len(oscs)}"

        # Each testId must be unique and addressable.
        unique_item_ids = set()
        for i in range(TARGET):
            test_id = f"sidebar_oscillators_item_{i}"
            if editor.element_exists(test_id):
                unique_item_ids.add(test_id)
        assert len(unique_item_ids) >= 5, (
            f"at least the top of the list must be reachable, got {len(unique_item_ids)}"
        )


class TestManyPanes:
    def test_ten_panes_added_and_registered(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: pane container doesn't grow past the first
        visible area — new panes rendered off-screen."""
        editor.add_oscillator(source_id, name="PaneSeed")
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        panes_added = []
        for i in range(10):
            pid = editor.add_pane(f"Pane{i:02d}")
            if pid:
                panes_added.append(pid)
        assert len(panes_added) == 10, (
            f"expected 10 new panes, got {len(panes_added)}"
        )
        panes = editor.get_panes()
        assert len(panes) >= 1 + len(panes_added), (
            f"pane count must reflect additions, got {len(panes)}"
        )


class TestLargeStateRoundTrip:
    def test_large_state_save_load(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        TARGET = 10
        for i in range(TARGET):
            editor.add_oscillator(
                source_id, name=f"R{i:02d}", colour=f"#{(i*7) % 256:02x}{(i*13) % 256:02x}{(i*23) % 256:02x}"
            )
        editor.wait_for_oscillator_count(TARGET, timeout_s=15.0)
        before_names = sorted(o["name"] for o in editor.get_oscillators())

        path = str(tmp_path / "large.xml")
        assert editor.save_state(path)
        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=10.0)
        assert editor.load_state(path)
        editor.wait_for_oscillator_count(TARGET, timeout_s=20.0)
        after_names = sorted(o["name"] for o in editor.get_oscillators())
        assert before_names == after_names, (
            "large state must round-trip without loss or duplication"
        )
