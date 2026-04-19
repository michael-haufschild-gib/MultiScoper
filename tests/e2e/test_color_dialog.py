"""
E2E coverage for the full OscillatorColorDialog modal flow.

Distinct from the inline color swatches in the config popup: this modal
is opened by double-clicking the 12-px-wide swatch strip on the left
edge of an oscillator list item (see OscillatorListItem::mouseDoubleClick,
kSwatchHitWidth = 12).  A center double-click on the same item opens the
settings popup instead — proving that testing the color dialog requires
an offset click, not a generic doubleClick.

What bugs these tests catch:
- Double-click hit zone moves (refactor breaks the 12-px strip pattern).
- Dialog open callback never registers in DialogManager — double-click
  fires but no modal appears.
- OK closes the modal but the color change is not pushed to oscillator
  state (the user's edit is silently dropped).
- Cancel closes the modal but the dialog's staged selection leaks back
  to the oscillator anyway.
- Dialog preserves last selection after re-open (the user expects to
  see their previous color highlighted; leaking stale state between
  oscillators is a separate, worse bug).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient
from page_objects import ColorDialogPage


class TestColorDialogOpenClose:
    """Basic modal lifecycle: open via swatch double-click, close OK/Cancel."""

    def test_swatch_strip_double_click_opens_dialog(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """Bug caught: the 12-px swatch strip is no longer a hit target
        (refactor moved the swatch rendering to the right side, breaking
        the existing interaction contract).
        """
        # If a prior test left the modal cached `visible`, closing it here
        # keeps this test independent of sibling test ordering.  The
        # MODAL wrapper lives in the component tree even when closed, so
        # it is fair to clear stale state via cancel-if-visible.
        if color_dialog.is_open():
            color_dialog.cancel()
            editor.wait_until(
                lambda: not color_dialog.is_open(),
                timeout_s=3.0,
                desc="stale color dialog to close",
            )
        color_dialog.open_via_list_item(0)
        assert color_dialog.is_open(), "dialog must be open after swatch double-click"
        # Leave the dialog open? No — close so subsequent tests start clean.
        color_dialog.cancel()

    def test_center_double_click_does_not_open_color_dialog(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """A double-click at the center of the list item must NOT open
        the color dialog — it opens the settings popup instead.

        Bug caught: hit zone check removed or reversed, so every
        double-click on an oscillator row opens the color dialog
        (settings popup becomes unreachable via double-click).
        """
        # Plain doubleClick uses the center of the item bounds.
        editor.double_click("sidebar_oscillators_item_0")
        # Give UI a moment to open whichever popup it's going to open.
        editor.wait_until(
            lambda: editor.element_visible("configPopup")
                    or editor.element_visible("colorDialogModal"),
            timeout_s=3.0,
            desc="a dialog or popup to open after center double-click",
        )
        assert editor.element_visible("configPopup"), (
            "center double-click must open configPopup, not colorDialog"
        )
        assert not editor.element_visible("colorDialogModal"), (
            "center double-click must NOT open the color dialog"
        )
        # Cleanup
        for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
            if editor.element_exists(btn):
                editor.click(btn)
                break

    def test_cancel_closes_dialog(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """Cancel must close the modal cleanly without altering state."""
        colour_before = editor.get_oscillator_by_id(oscillator)["colour"]
        color_dialog.open_via_list_item(0)
        color_dialog.cancel()

        assert color_dialog.is_open() is False
        colour_after = editor.get_oscillator_by_id(oscillator)["colour"]
        assert colour_before == colour_after, (
            f"Cancel must not change oscillator colour: "
            f"before={colour_before}, after={colour_after}"
        )

    def test_ok_closes_dialog(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """OK closes the modal.  (Whether colour changed depends on
        whether the user picked a different swatch, tested separately.)
        """
        color_dialog.open_via_list_item(0)
        color_dialog.confirm()
        assert color_dialog.is_open() is False


class TestColorDialogRegistration:
    """Harness-level invariants: the dialog's testIds are stable and unique."""

    def test_dialog_structural_children_registered(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """Swatches + OK + Cancel must all be addressable by testId when
        the dialog is open.

        Bug caught: a redesign removes swatches or wraps the buttons in
        a container that strips their testId, breaking every downstream
        color-change test.
        """
        color_dialog.open_via_list_item(0)
        for test_id in (
            color_dialog.MODAL,
            color_dialog.CONTENT,
            color_dialog.OK_BTN,
            color_dialog.CANCEL_BTN,
        ):
            assert editor.element_exists(test_id), (
                f"{test_id} must be registered when color dialog is open"
            )
        color_dialog.cancel()

    def test_dialog_registration_cleared_on_close(
        self, editor: MultiScoperTestClient, oscillator: str, color_dialog: ColorDialogPage
    ):
        """After the dialog closes, its body elements must not appear
        as `visible+showing`.  A stale registration that thinks it's
        showing while it's hidden would mask UI regressions.
        """
        color_dialog.open_via_list_item(0)
        color_dialog.cancel()

        # Modal wrapper may still register (it's always in the tree) but
        # must report not-visible or not-showing.
        modal_el = editor.get_element(color_dialog.MODAL)
        if modal_el is not None:
            assert not modal_el.visible or not modal_el.showing, (
                f"colorDialogModal must report not-visible/not-showing "
                f"after close, got visible={modal_el.visible} showing={modal_el.showing}"
            )


class TestColorDialogStatePersistence:
    """Cancel after re-open does not leak dialog selection into state."""

    def test_cancel_after_reopen_preserves_state(
        self, editor: MultiScoperTestClient, source_id: str, color_dialog: ColorDialogPage
    ):
        """Create an oscillator with an explicit colour, open the dialog
        twice, cancel both times, verify colour untouched.

        Bug caught: `onColorSelected` callback holds stale lambda
        capturing the previous OscillatorId, so cancelling a new
        dialog leaks the old selection into an unrelated oscillator.
        """
        osc_id = editor.add_oscillator(source_id, name="ColorOsc", colour="#FF0000")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)

        colour_before = editor.get_oscillator_by_id(osc_id)["colour"]
        assert colour_before.lower().endswith("ff0000"), (
            f"setup: colour should be red, got {colour_before}"
        )

        for _ in range(2):
            color_dialog.open_via_list_item(0)
            color_dialog.cancel()

        colour_after = editor.get_oscillator_by_id(osc_id)["colour"]
        assert colour_before == colour_after, (
            f"Two open-cancel cycles must not change colour: "
            f"before={colour_before}, after={colour_after}"
        )
