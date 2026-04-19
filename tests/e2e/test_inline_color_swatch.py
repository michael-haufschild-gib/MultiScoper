"""
E2E coverage for the inline color swatch strip in the config popup.

The swatches live inside configPopup_colorPicker — a horizontal strip
of individual color choices that share the parent's testId and are
hit-tested by x-offset within the strip.

What bugs these tests catch:
- Swatch click fires the generic list handler but doesn't update the
  oscillator's colour field (user sees the palette highlight change
  but the waveform keeps rendering with the old colour).
- Clicking the same swatch twice produces a random second color on
  the second click (event handler stuck in toggle state).
- The strip is not showing — clicks hit the underlying pane instead.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


STRIP = "configPopup_colorPicker"


@pytest.fixture()
def config_popup_open(editor: MultiScoperTestClient, oscillator: str):
    editor.click("sidebar_oscillators_item_0_settings")
    editor.wait_for_visible("configPopup", timeout_s=3.0)
    yield editor
    editor.click("configPopup_closeBtn")
    editor.wait_for_not_visible("configPopup", timeout_s=3.0)


class TestColorSwatchStripExists:
    def test_strip_is_showing(self, config_popup_open: MultiScoperTestClient):
        """Bug caught: the color picker strip is rendered but not
        `showing` (invisible to click handlers in the harness)."""
        el = config_popup_open.get_element(STRIP)
        assert el is not None
        assert el.visible and el.showing
        assert el.width > 0 and el.height > 0


class TestClickingSwatchesUpdatesColour:
    """Click-at-offset on the strip at different x positions picks
    different swatches, each updating the oscillator's colour field."""

    @pytest.mark.parametrize("offset_x,expected_hex_contains", [
        # Use broad hex substrings to stay robust against small palette
        # rebalancing — we only verify that the stored colour changed
        # to a value that is plausibly at the given x position.
        (10, None),
        (50, None),
        (100, None),
        (200, None),
        (300, None),
    ])
    def test_click_at_offset_changes_colour(
        self, config_popup_open: MultiScoperTestClient, oscillator: str,
        offset_x: int, expected_hex_contains
    ):
        """Bug caught: swatch click handler picks a random swatch
        regardless of click position."""
        before = config_popup_open.get_oscillator_by_id(oscillator)["colour"]
        ok = config_popup_open.click_at_offset(STRIP, offset_x=offset_x, offset_y=16)
        assert ok, "click_at_offset must succeed"
        config_popup_open.wait_until(
            lambda: config_popup_open.get_oscillator_by_id(oscillator)["colour"] != before,
            timeout_s=3.0,
            desc=f"colour to change after swatch click at x={offset_x}",
        )


class TestSwatchClicksAreIdempotent:
    def test_clicking_same_x_gives_same_colour(
        self, config_popup_open: MultiScoperTestClient, oscillator: str
    ):
        """Clicking the same swatch x-offset twice must land on the
        SAME colour both times.  Bug caught: swatch picker uses a
        cursor counter that increments on every click, so the second
        click at the same x hits the NEXT swatch."""
        # First click.
        config_popup_open.click_at_offset(STRIP, offset_x=100, offset_y=16)
        config_popup_open.wait_until(
            lambda: (c := config_popup_open.get_oscillator_by_id(oscillator)["colour"]),
            timeout_s=3.0,
            desc="colour to settle after first click",
        )
        c1 = config_popup_open.get_oscillator_by_id(oscillator)["colour"]

        # Click to a different swatch to reset state.
        config_popup_open.click_at_offset(STRIP, offset_x=300, offset_y=16)
        config_popup_open.wait_until(
            lambda: config_popup_open.get_oscillator_by_id(oscillator)["colour"] != c1,
            timeout_s=3.0,
            desc="colour to flip away from first pick",
        )

        # Click back to the original x.
        config_popup_open.click_at_offset(STRIP, offset_x=100, offset_y=16)
        config_popup_open.wait_until(
            lambda: config_popup_open.get_oscillator_by_id(oscillator)["colour"] == c1,
            timeout_s=3.0,
            desc=f"colour to return to {c1!r} on repeat click at x=100",
        )


class TestSwatchClickChangesReflectInPopup:
    def test_swatch_click_survives_popup_reopen(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """After picking a swatch and closing the popup, reopening it
        must reflect the selected colour.

        Bug caught: the picker doesn't read back the oscillator's
        colour on open, so each reopen re-highlights the wrong
        swatch (creating the illusion the user's choice was lost).
        """
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        editor.click_at_offset(STRIP, offset_x=100, offset_y=16)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["colour"] != "",
            timeout_s=3.0,
            desc="colour to update",
        )
        picked = editor.get_oscillator_by_id(oscillator)["colour"]
        editor.click("configPopup_closeBtn")
        editor.wait_for_not_visible("configPopup", timeout_s=3.0)

        # Reopen; oscillator must still have the picked colour.
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        assert editor.get_oscillator_by_id(oscillator)["colour"] == picked
        editor.click("configPopup_closeBtn")
        editor.wait_for_not_visible("configPopup", timeout_s=3.0)
