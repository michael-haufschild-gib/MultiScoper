"""
E2E coverage for sidebar accordion section collapse state.

The sidebar has at least two accordion sections: `sidebar_timing` and
`sidebar_options`.  Their expanded/collapsed state must be reachable
via the element info and must persist across editor close/reopen.

What bugs these tests catch:
- Accordion section click not wired to expand/collapse handler (user
  can't expand Options).
- Section reports `expanded=true` but content is still hidden (UI
  lies about its own state).
- Expanded/collapsed state not persisted across editor visibility
  cycles (every reopen starts with all sections collapsed).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


SECTIONS = ["sidebar_timing"]


def _expanded(client: MultiScoperTestClient, section: str) -> bool:
    el = client.get_element(section)
    if el is None:
        return False
    return bool(el.extra.get("expanded", False))


class TestSectionExpandedFieldReported:
    @pytest.mark.parametrize("section", SECTIONS)
    def test_section_reports_expanded_bool(
        self, editor: MultiScoperTestClient, section: str
    ):
        """Bug caught: `expanded` not in element info → harness can't
        verify accordion state without a pixel check."""
        el = editor.get_element(section)
        assert el is not None
        assert "expanded" in el.extra, (
            f"{section} must expose an `expanded` bool in element info"
        )
        assert isinstance(el.extra["expanded"], bool)


class TestSectionToggleViaClick:
    @pytest.mark.parametrize("section", SECTIONS)
    def test_click_toggles_expanded(
        self, editor: MultiScoperTestClient, section: str
    ):
        """Bug caught: click handler not wired → section stays stuck
        in one state."""
        before = _expanded(editor, section)
        editor.click(section)
        editor.wait_until(
            lambda: _expanded(editor, section) != before,
            timeout_s=3.0,
            desc=f"{section} to toggle expanded",
        )
        editor.click(section)
        editor.wait_until(
            lambda: _expanded(editor, section) == before,
            timeout_s=3.0,
            desc=f"{section} to revert expanded",
        )


class TestSectionContentShownWhenExpanded:
    def test_timing_content_visible_when_section_expanded(
        self, editor: MultiScoperTestClient
    ):
        """Bug caught: section reports expanded=True but inner
        controls (bpm field, interval field, mode toggle) are still
        hidden.
        """
        section = "sidebar_timing"
        if not _expanded(editor, section):
            editor.click(section)
            editor.wait_until(
                lambda: _expanded(editor, section),
                timeout_s=3.0,
                desc="timing section to expand",
            )

        # At least one canonical timing inner control should be `showing`.
        inner_ids = [
            "sidebar_timing_bpmField",
            "sidebar_timing_intervalField",
            "sidebar_timing_modeToggle",
        ]
        showing = [
            i for i in inner_ids
            if (el := editor.get_element(i)) is not None and el.showing
        ]
        assert showing, (
            f"Expanded timing section must show at least one of {inner_ids}"
        )

    def test_options_controls_always_showing(
        self, editor: MultiScoperTestClient
    ):
        """Observation: sidebar_options is a non-collapsible container
        (contrast with sidebar_timing which is an accordion).  Its
        inner controls must be showing by default — no user gesture
        required to access gain, grid, theme, etc.

        Bug caught: sidebar_options moved behind a collapsed accordion
        that defaults to closed, hiding options until the user clicks.
        """
        section = "sidebar_options"
        el = editor.get_element(section)
        assert el is not None and el.visible and el.showing, (
            f"sidebar_options must always be showing, got {el}"
        )
        inner_ids = [
            "sidebar_options_themeDropdown",
            "sidebar_options_gpuRenderingToggle",
            "sidebar_options_gainSlider",
        ]
        showing = [
            i for i in inner_ids
            if (iel := editor.get_element(i)) is not None and iel.showing
        ]
        assert showing, (
            f"at least one options control must be showing, got showing={showing}"
        )
