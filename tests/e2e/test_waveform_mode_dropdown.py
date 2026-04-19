"""
E2E coverage for the timing section's waveform mode dropdown.

Options: Free Running / Restart on Play / Restart on Note.

What bugs these tests catch:
- Dropdown populated but selection change does not propagate to the
  timing engine (waveform keeps running in "Free Running" mode no
  matter what's selected).
- Waveform mode lost across state save/load.
- All options selectable (no disabled fallback that silently clamps
  to the first option).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


DROPDOWN = "sidebar_timing_waveformModeDropdown"
MODES = ["Free Running", "Restart on Play", "Restart on Note"]


@pytest.fixture()
def expanded_timing(editor: MultiScoperTestClient):
    """Expand the timing section so dropdowns are reachable."""
    editor.click("sidebar_timing")
    editor.wait_for_element(DROPDOWN, timeout_s=3.0)
    return editor


class TestDropdownPopulation:
    def test_dropdown_has_expected_modes(self, expanded_timing: MultiScoperTestClient):
        el = expanded_timing.get_element(DROPDOWN)
        assert el is not None
        items = [i.get("id") for i in el.extra.get("items", [])]
        for mode in MODES:
            assert mode in items, f"{mode!r} missing from dropdown: {items}"

    def test_dropdown_has_exactly_three_items(
        self, expanded_timing: MultiScoperTestClient
    ):
        """Bug caught: silent duplication of an item, or a hidden
        fourth mode leaking from a debug build."""
        el = expanded_timing.get_element(DROPDOWN)
        assert el is not None
        num = el.extra.get("numItems", 0)
        assert num == 3, f"expected 3 modes, got {num}"


class TestSelectionUpdatesDropdown:
    @pytest.mark.parametrize("mode", MODES)
    def test_selecting_each_mode_updates_dropdown_label(
        self, expanded_timing: MultiScoperTestClient, mode: str
    ):
        """Bug caught: selection handler not wired, dropdown label
        never updates."""
        expanded_timing.select_dropdown_item(DROPDOWN, mode)
        expanded_timing.wait_until(
            lambda m=mode: expanded_timing.get_element(DROPDOWN).extra.get("selectedId") == m,
            timeout_s=3.0, desc=f"dropdown to show selected={mode}",
        )
        # Post-wait assertion so the contract is visible without
        # tracing wait_until's TimeoutError semantics.
        assert expanded_timing.get_element(DROPDOWN).extra.get("selectedId") == mode


class TestWaveformModePersistence:
    def test_mode_survives_save_load_roundtrip(
        self, expanded_timing: MultiScoperTestClient, tmp_path
    ):
        """Bug caught: waveform mode not serialized, or deserializer
        defaults to Free Running regardless of saved value."""
        expanded_timing.select_dropdown_item(DROPDOWN, "Restart on Play")
        expanded_timing.wait_until(
            lambda: expanded_timing.get_element(DROPDOWN).extra.get("selectedId") == "Restart on Play",
            timeout_s=3.0, desc="mode set to Restart on Play",
        )

        path = str(tmp_path / "wfm_state.xml")
        assert expanded_timing.save_state(path)

        expanded_timing.reset_state()
        expanded_timing.wait_until(
            lambda: len(expanded_timing.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )

        assert expanded_timing.load_state(path)
        # Re-expand timing section after state reload.
        expanded_timing.click("sidebar_timing")
        expanded_timing.wait_for_element(DROPDOWN, timeout_s=3.0)

        expanded_timing.wait_until(
            lambda: expanded_timing.get_element(DROPDOWN).extra.get("selectedId") == "Restart on Play",
            timeout_s=3.0, desc="mode to round-trip as Restart on Play",
        )
        # Explicit final assertion.
        assert expanded_timing.get_element(DROPDOWN).extra.get("selectedId") == "Restart on Play"
