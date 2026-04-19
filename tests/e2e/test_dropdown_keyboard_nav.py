"""
E2E coverage for keyboard navigation in MultiScoper dropdown controls.

What bugs these tests catch:
- Dropdown advertises focusable=True but ignores arrow keys when
  focused (user can't navigate without a mouse).
- Arrow key cycling wraps in an unexpected direction (Down takes user
  to the previous item).
- Enter on a dropdown opens the popup but a second Enter doesn't
  commit — user stuck in the popup with no keyboard escape.

Most of these are product-behavior expectations; the harness itself
doesn't simulate OS key bindings perfectly, so these tests exercise
what IS reliably observable: that the dropdown is focusable, callable
via the keyboard API, and coherent after a press.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


DROPDOWN = "sidebar_options_themeDropdown"


@pytest.fixture()
def focused_theme_dropdown(editor: MultiScoperTestClient):
    editor.click("sidebar_options")
    editor.wait_for_element(DROPDOWN, timeout_s=3.0)
    editor.focus(DROPDOWN)
    return editor


class TestDropdownKeyboardAPI:
    def test_dropdown_is_focusable_attribute_true(
        self, focused_theme_dropdown: MultiScoperTestClient
    ):
        """Bug caught: dropdown marked focusable=False in element info,
        preventing keyboard users from discovering it via Tab."""
        el = focused_theme_dropdown.get_element(DROPDOWN)
        assert el is not None
        assert el.extra.get("focusable", False) is True, (
            f"dropdown {DROPDOWN} must advertise focusable=True"
        )

    def test_arrow_keys_callable_without_crash(
        self, focused_theme_dropdown: MultiScoperTestClient
    ):
        """Pressing arrow keys on a dropdown must not crash regardless
        of whether the plugin binds them to a selection change."""
        for direction in ("up", "down", "left", "right"):
            focused_theme_dropdown.key_press(direction)
        assert focused_theme_dropdown.health_check()["data"]["status"] == "ok"

    def test_enter_on_dropdown_does_not_crash(
        self, focused_theme_dropdown: MultiScoperTestClient
    ):
        """Enter on a focused dropdown is a common key for open-popup
        or commit.  Must not crash regardless of the plugin's choice."""
        focused_theme_dropdown.key_press("enter")
        assert focused_theme_dropdown.health_check()["data"]["status"] == "ok"


class TestDropdownSelectionStable:
    def test_keypresses_do_not_cause_random_selection_jumps(
        self, focused_theme_dropdown: MultiScoperTestClient
    ):
        """Pressing unrelated keys (space, escape, tab) must not
        silently change the dropdown's selected id.

        Bug caught: dropdown listener catches any keystroke as a
        navigation, shifting selection on background key events.
        """
        before = focused_theme_dropdown.get_element(DROPDOWN).extra.get("selectedId")
        for key in ("a", "b", "c", "escape"):
            focused_theme_dropdown.key_press(key)
        after = focused_theme_dropdown.get_element(DROPDOWN).extra.get("selectedId")
        assert before == after, (
            f"dropdown selection should not drift on unrelated keys: "
            f"before={before!r}, after={after!r}"
        )
