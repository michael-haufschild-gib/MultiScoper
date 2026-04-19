"""
E2E coverage for the plugin editor's detach/reattach lifecycle.

A hosting DAW may close the editor window (detach) while keeping the
plugin processor alive, then later re-open it (reattach).  State must
survive the cycle.

What bugs these tests catch:
- Detach frees UI components but the processor keeps a dangling
  reference; reattach crashes with a use-after-free.
- Reattach rebuilds UI with default state instead of reading saved
  state.
- Multiple detach calls (e.g., via rapid window-close clicks)
  produce a UAF in the editor's listener registration.
"""

from __future__ import annotations

from multiscoper_test_utils import MultiScoperTestClient


class TestDetachReattachBasics:
    def test_detach_reattach_preserves_oscillator(
        self, client: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: detaching the editor clears oscillator state
        (editor owns state by mistake)."""
        client.open_editor(track_id=0)
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state empty",
        )
        client.add_oscillator(source_id, name="SurvivorOsc", colour="#FF00FF")
        client.wait_for_oscillator_count(1, timeout_s=3.0)

        # Detach via harness.
        assert client._post_ok("/track/0/detachEditor", {}), (
            "/track/0/detachEditor must succeed"
        )

        # Oscillator state persists despite editor detach.
        oscs = client.get_oscillators()
        assert len(oscs) == 1, (
            f"oscillator state must survive detach, got {len(oscs)}"
        )
        assert oscs[0]["name"] == "SurvivorOsc"

        # Reattach.
        ok = client._post_ok("/track/0/reattachEditor", {})
        assert ok, "reattach must succeed"
        client.wait_until(
            lambda: client.element_exists("sidebar"),
            timeout_s=5.0,
            desc="sidebar to re-register after reattach",
        )

        # State still intact after reattach.
        oscs = client.get_oscillators()
        assert len(oscs) == 1
        assert oscs[0]["name"] == "SurvivorOsc"

    def test_multiple_detaches_are_idempotent(
        self, client: MultiScoperTestClient
    ):
        """Rapid detach × 5 must not crash."""
        client.open_editor(track_id=0)
        for _ in range(5):
            client._post_ok("/track/0/detachEditor", {})
        assert client.health_check()["data"]["status"] == "ok"

        # Reattach to leave the session in a usable state.
        client._post_ok("/track/0/reattachEditor", {})


class TestReattachWithExistingState:
    def test_reattach_rebuilds_ui_from_state(
        self, client: MultiScoperTestClient, source_id: str
    ):
        """After reattach, every oscillator in state has a matching
        sidebar list item."""
        client.open_editor(track_id=0)
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="empty state",
        )
        for i in range(3):
            client.add_oscillator(source_id, name=f"ReattachOsc{i}")
        client.wait_for_oscillator_count(3, timeout_s=3.0)

        assert client._post_ok("/track/0/detachEditor", {}), (
            "/track/0/detachEditor must succeed"
        )
        assert client._post_ok("/track/0/reattachEditor", {}), (
            "/track/0/reattachEditor must succeed"
        )

        client.wait_until(
            lambda: client.element_exists("sidebar_oscillators_item_2"),
            timeout_s=5.0,
            desc="last list item to re-register after reattach",
        )

        for i in range(3):
            assert client.element_exists(f"sidebar_oscillators_item_{i}"), (
                f"item {i} must re-register after reattach"
            )
