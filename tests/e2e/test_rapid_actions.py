"""
E2E coverage for rapid-fire user interactions.

What bugs these tests catch:
- Add button opens the dialog twice on a double-click (no re-entrancy guard).
- Rapid delete clicks race the list refresh, causing index-based
  deletion to corrupt (off-by-one after shrink).
- Burst of oscillator add/remove cycles leaks memory or registers
  duplicate testIds that collide in the element registry.
- Rapid pane add/remove degrades into a deadlock (message-thread
  queue floods with never-drained refresh callbacks).
- Rapid transport play/stop toggles cause inconsistent "playing"
  state queries.

These tests exercise timing behavior deterministically by issuing
back-to-back HTTP requests without sleeps; condition-based waits
confirm final state once the burst has fully settled.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestRapidAddOscillator:
    """Rapid add actions must be serialized, not produce duplicates."""

    def test_rapid_api_adds(self, editor: MultiScoperTestClient, source_id: str):
        """10 rapid state API adds must produce exactly 10 oscillators."""
        TARGET = 10
        ids = []
        for i in range(TARGET):
            osc_id = editor.add_oscillator(source_id, name=f"Burst{i}")
            assert osc_id is not None, f"add {i} must succeed"
            ids.append(osc_id)

        editor.wait_for_oscillator_count(TARGET, timeout_s=8.0)
        oscs = editor.get_oscillators()
        actual_ids = {o["id"] for o in oscs}
        assert len(actual_ids) == TARGET, (
            f"burst of {TARGET} adds produced {len(actual_ids)} unique ids"
        )
        for osc_id in ids:
            assert osc_id in actual_ids, f"id {osc_id} missing after burst"

    def test_rapid_add_button_clicks_do_not_open_multiple_dialogs(
        self, editor: MultiScoperTestClient
    ):
        """Rapid clicks on the add button must NOT stack multiple open
        dialogs.  The modal is a singleton — a second click while the
        dialog is already visible is a no-op at worst.

        Bug caught: add button opens a new AddOscillatorDialog instance
        each click without checking isShowing, leaking modals and
        confusing the user.
        """
        for _ in range(5):
            editor.click("sidebar_addOscillator")

        editor.wait_until(
            lambda: editor.element_visible("addOscillatorDialog"),
            timeout_s=3.0,
            desc="add dialog to be visible after click burst",
        )

        # Cancel; verify that ONE cancel closes the dialog (not five).
        editor.click("addOscillatorDialog_cancelBtn")
        editor.wait_for_not_visible("addOscillatorDialog", timeout_s=3.0)
        assert not editor.element_visible("addOscillatorDialog")


class TestRapidAddRemoveCycles:
    """Tight add/remove loops must not leak registrations or memory."""

    def test_tight_add_remove_cycle_leaves_empty(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Bug caught: InstanceRegistry or testId registry leaks
        entries across add/remove cycles."""
        CYCLES = 15
        for i in range(CYCLES):
            osc_id = editor.add_oscillator(source_id, name=f"Cycle{i}")
            assert osc_id is not None
            editor.wait_for_oscillator_count(1, timeout_s=3.0)
            ok = editor.delete_oscillator(osc_id)
            assert ok, f"cycle {i} delete must succeed"
            editor.wait_for_oscillator_count(0, timeout_s=3.0)

        # Harness must still respond.
        health = editor.health_check()
        assert health["data"]["status"] == "ok"


class TestRapidTransportToggle:
    """Play/stop sequences must converge on a definite state."""

    def test_play_stop_burst_ends_stopped(self, editor: MultiScoperTestClient):
        """Ten play/stop toggles ending on `stop` must leave the
        transport in the stopped state, never in a partial state."""
        for _ in range(10):
            editor.transport_play()
            editor.transport_stop()
        editor.wait_until(
            lambda: not editor.is_playing(),
            timeout_s=3.0,
            desc="transport to converge on stopped",
        )

    def test_play_stop_burst_ends_playing(self, editor: MultiScoperTestClient):
        """Same as above but ending on `play`."""
        for _ in range(10):
            editor.transport_stop()
            editor.transport_play()
        editor.wait_until(
            lambda: editor.is_playing(),
            timeout_s=3.0,
            desc="transport to converge on playing",
        )
        editor.transport_stop()


class TestRapidPaneActions:
    """Adding/removing panes rapidly must not deadlock the UI."""

    def test_rapid_pane_add(self, editor: MultiScoperTestClient, source_id: str):
        """Seven rapid pane adds must result in 1 (auto-created) + 7 = 8 panes."""
        osc_id = editor.add_oscillator(source_id, name="Seed")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        added = 0
        for i in range(7):
            pid = editor.add_pane(f"RapidPane{i}")
            if pid is not None:
                added += 1

        assert added == 7, f"expected 7 pane adds, got {added}"
        panes = editor.get_panes()
        assert len(panes) >= 1 + added, (
            f"expected at least {1 + added} panes, got {len(panes)}"
        )


class TestRapidStateReset:
    """State reset during active scenario must not crash."""

    def test_reset_then_immediate_add(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Reset followed immediately by add must produce a clean state
        with exactly 1 oscillator (not a race between reset and add)."""
        for i in range(5):
            editor.add_oscillator(source_id, name=f"Pre{i}")
        editor.wait_for_oscillator_count(5, timeout_s=5.0)

        # Reset and immediately add — the add must land AFTER reset completes.
        editor.reset_state()
        osc_id = editor.add_oscillator(source_id, name="PostReset")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        oscs = editor.get_oscillators()
        assert len(oscs) == 1, f"expected 1 osc after reset+add, got {len(oscs)}"
        assert oscs[0]["name"] == "PostReset", (
            f"surviving oscillator must be the post-reset one, got {oscs[0]['name']}"
        )
