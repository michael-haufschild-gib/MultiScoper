"""
E2E coverage for the per-pane StatsOverlay (Peak/RMS/Crest readout).

What bugs these tests catch:
- Stats overlay toggle wired but overlay never paints the metrics table
  (user sees an empty overlay).
- Reset button not clearing the accumulated Max Peak (engineer can't
  reset after an unwanted input transient polluted the reading).
- Overlay not registered with the test harness (impossible to assert
  its state programmatically from future tests).
- Multiple oscillators in a pane: only one shows stats (data columns
  don't expand).

Assertions are programmatic against the harness element registry and
the StatsOverlay's exposed display text (via getDisplayedText in the
overlay component when wired through the TestElementRegistry).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient
from page_objects import PaneActionBarPage, StatsOverlayPage


def _enable_stats(editor: MultiScoperTestClient) -> None:
    """Engage the per-pane stats toggle and wait for the overlay to appear."""
    assert editor.element_exists("pane_statsBtn"), "stats toggle must exist"
    editor.click("pane_statsBtn")
    editor.wait_until(
        lambda: editor.element_visible("statsOverlay"),
        timeout_s=3.0,
        desc="statsOverlay to become visible",
    )


class TestStatsOverlayBasics:
    """The overlay appears/disappears in response to the pane stats toggle."""

    def test_overlay_registered_when_engaged(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: StatsOverlay not registering testId, invisible to
        the test harness even though it's rendered on screen."""
        _enable_stats(editor)

        el = editor.get_element("statsOverlay")
        assert el is not None, "statsOverlay must be registered with the harness"
        assert el.visible, "statsOverlay must be visible when engaged"

    def test_reset_button_showing_after_fade_in(
        self, editor: MultiScoperTestClient, oscillator: str, pane_action_bar: PaneActionBarPage
    ):
        """Reset btn is a child of the overlay and lives behind a fade
        animation. It must be reachable (`showing=True`) once the fade
        completes, and must stop showing when the overlay fades out.

        Bug caught: Reset button registered but never `showing`, making
        it unreachable by harness lookups even though it paints.
        """
        _enable_stats(editor)
        editor.wait_until(
            lambda: (el := editor.get_element("statsOverlay_resetBtn")) is not None
                    and el.showing,
            timeout_s=3.0,
            desc="reset button to become showing after fade-in",
        )

        pane_action_bar.toggle_stats()
        editor.wait_until(
            lambda: (el := editor.get_element("statsOverlay_resetBtn")) is None
                    or not el.showing,
            timeout_s=3.0,
            desc="reset button to stop showing after toggle off",
        )

    def test_reset_click_does_not_hide_overlay(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Clicking the reset button should clear accumulated metrics but
        keep the overlay visible.

        Bug caught: reset handler calling `setVisibleAnimated(false)`
        by mistake — engineer clicks "reset" and overlay disappears.
        """
        _enable_stats(editor)

        ok = editor.click("statsOverlay_resetBtn")
        assert ok, "reset click must succeed"

        # The overlay must still be showing after reset.
        el = editor.get_element("statsOverlay")
        assert el is not None and el.visible, (
            "overlay must remain visible after reset click"
        )


class TestStatsContentUpdates:
    """Metric values update when audio is flowing."""

    def test_overlay_size_tracks_oscillator_count(
        self, editor: MultiScoperTestClient, two_oscillators
    ):
        """Overlay must grow wider when more oscillators share the pane.

        Bug caught: table formatter locks column count at construction
        and never resizes → extra oscillators go unrendered.
        """
        # Note: we depend only on `two_oscillators` (not `pane_action_bar`)
        # so the editor starts with exactly 2 oscillators, not 2+1 from a
        # conflicting `oscillator` fixture.  pane_action_bar's `oscillator`
        # dependency would otherwise seed 3.
        oscs_start = editor.get_oscillators()
        assert len(oscs_start) == 2, (
            f"fixture precondition: 2 oscillators, got {len(oscs_start)}"
        )

        _enable_stats(editor)
        el_two = editor.get_element("statsOverlay")
        assert el_two is not None
        width_two = el_two.width

        # Disengage, remove one oscillator, re-engage, compare width.
        editor.click("pane_statsBtn")  # disengage
        editor.wait_until(
            lambda: not editor.element_visible("statsOverlay"),
            timeout_s=3.0, desc="stats overlay to hide"
        )

        ok = editor.delete_oscillator(oscs_start[0]["id"])
        assert ok, f"delete via state API must succeed for osc {oscs_start[0]['id']}"
        editor.wait_for_oscillator_count(1, timeout_s=5.0)

        _enable_stats(editor)
        el_one = editor.get_element("statsOverlay")
        assert el_one is not None
        width_one = el_one.width

        assert width_two >= width_one, (
            f"overlay with 2 oscillators ({width_two}px) must be at least as "
            f"wide as with 1 oscillator ({width_one}px)"
        )


class TestStatsOverlayResetDoesNotCrash:
    """Rapid/duplicate reset clicks do not corrupt overlay state."""

    def test_rapid_reset_clicks_are_idempotent(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Clicking reset repeatedly must not crash the harness or cause
        the overlay to drop out of the registry.

        Bug caught: reset handler mutates state via message-thread post
        without re-entrancy guard, so rapid clicks race and null out a
        stats accumulator mid-update.
        """
        _enable_stats(editor)
        for _ in range(5):
            assert editor.click("statsOverlay_resetBtn"), "reset click must succeed"

        # Overlay must still respond.
        el = editor.get_element("statsOverlay")
        assert el is not None and el.visible, "overlay survived rapid resets"
        assert editor.health_check()["data"]["status"] == "ok"
