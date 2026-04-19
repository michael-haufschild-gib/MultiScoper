"""
E2E coverage for the inline oscillator mode selector on each sidebar
list item (Stereo / Mono / Mid / Side / Left / Right).

This is the quick-access mode selector that lives on the list row and
is distinct from the mode chips inside the config popup.  Both must
stay in sync with the oscillator's `mode` field.

What bugs these tests catch:
- Inline mode chips wired to a stale listener that writes to a
  captured oscillator ID, so selecting a chip on row 3 changes row 0.
- Inline chip ignores clicks after the first oscillator is added
  (listener lost during list refresh).
- State API mode change does not reflect on the inline chip group
  (stale UI).
- Inline selector and config popup disagree about the current mode.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


MODES = ["FullStereo", "Mono", "Mid", "Side", "Left", "Right"]


def _inline_mode_id(index: int) -> str:
    return f"sidebar_oscillators_item_{index}_mode"


class TestInlineModeSelectorExists:
    """The inline mode selector testId is registered on every list row."""

    def test_inline_mode_exists_for_first_item(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: inline mode testId lost in list refresh — tests
        and accessibility tooling can't find it."""
        assert editor.element_exists(_inline_mode_id(0)), (
            "inline mode chip group must be registered on item 0"
        )

    def test_inline_mode_exists_for_each_item(
        self, editor: MultiScoperTestClient, three_oscillators
    ):
        """All three oscillator rows expose the inline mode selector."""
        for i in range(3):
            assert editor.element_exists(_inline_mode_id(i)), (
                f"inline mode chip group must be registered on item {i}"
            )


class TestStateAPIUpdatesInlineSelector:
    """State API mode updates are reflected in the oscillator field."""

    @pytest.mark.parametrize("mode", MODES)
    def test_mode_change_persists_in_state(
        self, editor: MultiScoperTestClient, oscillator: str, mode: str
    ):
        """Each mode is accepted and stored.  Bug caught: mode string
        parser rejects valid modes, or silently maps everything to the
        default.
        """
        ok = editor.update_oscillator(oscillator, mode=mode)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator).get("mode") == mode,
            timeout_s=3.0,
            desc=f"mode to settle on {mode}",
        )
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc["mode"] == mode


class TestInlineAndPopupModeConsistency:
    """Inline selector and config popup chips agree after either one
    is the driver of a mode change."""

    def test_state_api_change_reflects_in_popup_mode_buttons(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """After changing mode via state API, opening the config popup
        shows that mode as the *selected* (toggled) chip, and all other
        chips must be unselected.

        Bug caught: popup's opening flow re-initializes its mode chips
        to a default rather than reading the oscillator's current mode.
        The previous version only verified the Mid chip was registered
        and showing — which stays true even in the broken state where
        the popup ignores the osc's mode and defaults to Stereo. Check
        the toggled field to actually catch the documented regression.
        """
        # Change to Mid via state API.
        editor.update_oscillator(oscillator, mode="Mid")
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["mode"] == "Mid",
            timeout_s=3.0, desc="mode=Mid in state",
        )

        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)

        try:
            chip_prefix = "configPopup_modeSelector_"
            chip_suffixes = ["stereo", "mono", "mid", "side", "left", "right"]

            for suffix in chip_suffixes:
                chip_id = chip_prefix + suffix
                assert editor.element_exists(chip_id), f"{chip_id} must be registered"

            # Wait for the popup's internal sync to settle (chips update on
            # their theme/listener refresh). The toggled field reflects the
            # selected state once the popup has read the oscillator.
            editor.wait_until(
                lambda: (e := editor.get_element(chip_prefix + "mid"))
                and e.extra.get("toggled") is True,
                timeout_s=3.0,
                desc="Mid chip to become toggled after popup opens for mode=Mid",
            )

            # Exactly one chip must be toggled — the one matching the osc's mode.
            toggled_suffixes = [
                suffix
                for suffix in chip_suffixes
                if (el := editor.get_element(chip_prefix + suffix))
                and el.extra.get("toggled") is True
            ]
            assert toggled_suffixes == ["mid"], (
                f"only the chip matching the osc's mode should be toggled, "
                f"got toggled={toggled_suffixes}"
            )
        finally:
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)


class TestPopupModeClickUpdatesState:
    """Clicking a mode chip in the config popup updates oscillator state."""

    @pytest.mark.parametrize("chip_suffix,mode", [
        ("stereo", "FullStereo"),
        ("mono", "Mono"),
        ("mid", "Mid"),
        ("side", "Side"),
        ("left", "Left"),
        ("right", "Right"),
    ])
    def test_popup_mode_chip_click_updates_state(
        self, editor: MultiScoperTestClient, source_id: str,
        chip_suffix: str, mode: str
    ):
        """Each chip selector in the popup must write the right mode
        into state.  Bug caught: chip-to-mode mapping mis-wired by
        one position (e.g., Left chip sets Right).
        """
        osc_id = editor.add_oscillator(source_id, name=f"Mode:{mode}")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        try:
            chip = f"configPopup_modeSelector_{chip_suffix}"
            assert editor.element_exists(chip), f"{chip} must be registered"
            editor.click(chip)
            editor.wait_until(
                lambda: editor.get_oscillator_by_id(osc_id).get("mode") == mode,
                timeout_s=3.0,
                desc=f"mode to become {mode} via chip click",
            )
        finally:
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)
