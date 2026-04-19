"""
E2E coverage for Escape-key dismissal of every modal dialog.

What bugs these tests catch:
- One of the modals swallows the Escape keypress (user is stuck until
  they find the Cancel button or reach for the mouse).
- Escape dismisses dialog but leaves its backdrop visible (UI feels
  frozen; subsequent clicks hit backdrop, not the underlying editor).
- Escape in add-dialog submits accidentally (creates an oscillator
  with defaults), masking a deeper Enter/Escape key handler mixup.
"""

from __future__ import annotations

import time

import pytest

from multiscoper_test_utils import MultiScoperTestClient


def _assert_stays_visible(
    editor: MultiScoperTestClient, element_id: str, duration_s: float = 0.3
) -> None:
    """Assert that ``element_id`` is visible for the entire ``duration_s`` window.

    ``wait_until(lambda: element_visible(x))`` returns at the *first* moment
    the predicate is true (usually t=0), so it cannot detect an asynchronous
    dismiss a few hundred ms later. We poll every 50 ms instead and fail the
    moment the element goes invisible — that's the actual invariant the
    "Escape must not dismiss this modal" tests want to verify.
    """
    steps = max(1, int(duration_s / 0.05))
    for i in range(steps):
        assert editor.element_visible(element_id), (
            f"{element_id} became invisible after {i * 0.05:.2f}s — "
            f"something dismissed it during the stays-visible window"
        )
        time.sleep(0.05)  # noqa: e2e-lint — stays-visible polling over duration D; wait_until returns at t=0 and can't prove survival


class TestEscapeDismissesAddDialog:
    def test_escape_closes_add_dialog(self, editor: MultiScoperTestClient):
        editor.click("sidebar_addOscillator")
        editor.wait_for_visible("addOscillatorDialog", timeout_s=3.0)

        initial_count = len(editor.get_oscillators())
        editor.key_press("escape")

        editor.wait_until(
            lambda: not editor.element_visible("addOscillatorDialog"),
            timeout_s=3.0,
            desc="add dialog to dismiss on Escape",
        )
        # Escape must NOT accidentally submit the form.
        assert len(editor.get_oscillators()) == initial_count, (
            "Escape must not create an oscillator"
        )


class TestEscapeOnConfigPopup:
    """The config popup's Escape-handling contract.

    Observation: the MultiScoperModal base class supports
    `closeOnEscape_` but this flag defaults to false and is not enabled
    for the config popup modal.  This test documents that behavior
    (Escape does NOT dismiss the config popup today) and verifies no
    side effects: the popup remains open and usable, state is
    unchanged, and the harness does not crash.

    A product decision to make Escape close this popup would flip the
    assertion in `test_escape_does_not_dismiss_config_popup` — fail
    it, then update to the matching positive assertion.
    """

    def test_escape_does_not_dismiss_config_popup(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        state_before = editor.get_oscillator_by_id(oscillator)

        editor.key_press("escape")
        # Poll for the full stays-visible window — `wait_until` returns at
        # the first truthy check (t=0), so it can't actually prove the
        # popup survived the Escape.
        _assert_stays_visible(editor, "configPopup")
        state_after = editor.get_oscillator_by_id(oscillator)
        assert state_before == state_after, (
            "Escape on config popup must not mutate oscillator state"
        )

        # Cleanup.
        editor.click("configPopup_closeBtn")
        editor.wait_for_not_visible("configPopup", timeout_s=3.0)


class TestEscapeOnColorDialog:
    """The color dialog's Escape-handling contract (see above)."""

    def test_escape_does_not_dismiss_color_dialog(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog
    ):
        color_dialog.open_via_list_item(0)
        state_before = editor.get_oscillator_by_id(oscillator)

        editor.key_press("escape")
        # Poll the stays-open predicate for a bounded window rather than
        # relying on wait_until (which would return on the first truthy check).
        steps = 6  # 6 × 50ms = 300ms
        for i in range(steps):
            assert color_dialog.is_open(), (
                f"color dialog closed after {i * 0.05:.2f}s — "
                f"Escape should not dismiss it"
            )
            time.sleep(0.05)  # noqa: e2e-lint — stays-open polling; proves dialog survives the full window
        state_after = editor.get_oscillator_by_id(oscillator)
        assert state_before == state_after

        # Cleanup.
        color_dialog.cancel()


class TestEscapeDoesNotAffectSidebar:
    """Escape on the plain sidebar (no dialog) must be a no-op."""

    def test_escape_without_modal_is_noop(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: Escape bound globally to "clear everything",
        deleting the selected oscillator by accident."""
        count_before = len(editor.get_oscillators())
        editor.key_press("escape")
        # Brief pause to let a hypothetical async "delete selected osc"
        # handler run; if Escape really is a no-op, count stays the same.
        # (Previous version used `wait_until(...) if False else None`, an
        # always-false conditional that made the whole call dead code.)
        time.sleep(0.1)  # noqa: e2e-lint — probes for absence of async side effect; no predicate to poll
        count_after = len(editor.get_oscillators())
        assert count_before == count_after, (
            f"Escape with no modal must not mutate state: "
            f"before={count_before}, after={count_after}"
        )
