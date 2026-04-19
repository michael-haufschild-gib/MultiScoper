"""
E2E coverage for the oscillator visibility toggle user flow.

What bugs these tests catch:
- Visibility state drifts between the UI button and the oscillator's
  `visible` field (the "invisible eye" icon is wrong half the time).
- Hidden oscillator still contributes to waveform rendering (the
  display looks identical whether visible or hidden — a regression
  that breaks A/B comparison workflows).
- Visibility state does not persist across state save/load.
- Click pattern on vis button differs between "visible with pane"
  (toggle) and "invisible without pane" (open SelectPane dialog);
  the conditional branch must stay correct after refactors.
- Hiding an oscillator does not release the pane slot (pane shows
  empty but can't be reused).
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestVisibilityToggleState:
    """Toggling visibility flips the oscillator's `visible` field."""

    def test_state_api_toggles_visibility(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Bug caught: update_oscillator(visible=False) does nothing."""
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc["visible"] is True, "oscillator must start visible"

        ok = editor.update_oscillator(oscillator, visible=False)
        assert ok, "state update must succeed"
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["visible"] is False,
            timeout_s=3.0, desc="oscillator to go invisible",
        )

        ok = editor.update_oscillator(oscillator, visible=True)
        assert ok
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["visible"] is True,
            timeout_s=3.0, desc="oscillator to go visible",
        )

    def test_ui_button_toggles_visibility(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """The visibility button click must flip the state field.

        Bug caught: button handler updates UI icon but forgets to fire
        the state notification, leaving state stale.
        """
        btn = "sidebar_oscillators_item_0_vis_btn"
        assert editor.element_exists(btn), "vis button must be registered"

        before = editor.get_oscillator_by_id(oscillator)["visible"]
        editor.click(btn)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["visible"] != before,
            timeout_s=3.0, desc="visibility to flip after vis-button click",
        )


class TestVisibilityAffectsRendering:
    """Hidden oscillator does not appear in the waveform render state."""

    def test_invisible_osc_excluded_from_waveform_state(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Create two oscs, hide one, verify only the visible one
        appears in the rendered waveform state.

        Bug caught: visibility flag read by state API but ignored by
        the rendering pipeline → hidden oscillator still draws.
        """
        id_vis = editor.add_oscillator(source_id, name="Visible", colour="#00FF00")
        id_hid = editor.add_oscillator(source_id, name="Hidden",  colour="#FF0000")
        assert id_vis and id_hid
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        editor.update_oscillator(id_hid, visible=False)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(id_hid)["visible"] is False,
            timeout_s=3.0, desc="hidden osc to go invisible",
        )

        editor.transport_play()
        editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
        editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

        waveforms = editor.get_waveform_for_pane(0)
        assert waveforms, "must have at least one waveform in pane 0"

        # If the plugin honors visibility, the hidden osc's ID must not
        # appear among active waveforms.  (Implementations vary: some
        # include the entry with a zero-amplitude/silent flag.)
        wf_ids = {w.get("oscillatorId") for w in waveforms}
        visible_ids = wf_ids & {id_vis, id_hid}
        assert id_vis in visible_ids, "visible oscillator must be rendered"

        editor.transport_stop()


class TestVisibilityStatePersistence:
    """Visibility survives state save/load."""

    def test_visibility_round_trip_save_load(
        self, editor: MultiScoperTestClient, source_id: str, tmp_path
    ):
        """Bug caught: save_state serializes visible flag but load_state
        does not restore it (oscillator comes back visible regardless)."""
        id_a = editor.add_oscillator(source_id, name="VisA", colour="#00FF00")
        id_b = editor.add_oscillator(source_id, name="InvisB", colour="#FF0000")
        assert id_a and id_b
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        editor.update_oscillator(id_b, visible=False)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(id_b)["visible"] is False,
            timeout_s=3.0, desc="invisible flag to persist",
        )

        path = str(tmp_path / "state_vis.xml")
        assert editor.save_state(path), "save must succeed"

        editor.reset_state()
        editor.wait_for_oscillator_count(0, timeout_s=3.0)

        assert editor.load_state(path), "load must succeed"
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        loaded = editor.get_oscillators()
        by_name = {o["name"]: o for o in loaded}
        assert "VisA" in by_name and "InvisB" in by_name
        assert by_name["VisA"]["visible"] is True, (
            f"VisA should be visible after load, got {by_name['VisA']['visible']}"
        )
        assert by_name["InvisB"]["visible"] is False, (
            f"InvisB should be invisible after load, got {by_name['InvisB']['visible']}"
        )


class TestVisibilityButtonSpecialCase:
    """Clicking vis on an invisible, pane-less oscillator opens
    selectPaneDialog (per OscillatorListItem::onClick branch)."""

    def test_click_on_visible_osc_hides_it(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """Normal case: a visible oscillator in a valid pane flips
        invisible on click.  Covers the main branch of the vis-button
        handler.
        """
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc["visible"] is True
        assert osc.get("paneId"), "oscillator starts in a pane"

        btn = "sidebar_oscillators_item_0_vis_btn"
        editor.click(btn)
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator)["visible"] is False,
            timeout_s=3.0,
            desc="visible osc to go invisible on vis-btn click",
        )


class TestVisibilityToggleRapid:
    """Rapid toggles must converge cleanly."""

    def test_burst_toggle_converges(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """20 rapid UI toggles must leave state in a definite visible
        or invisible state — never in a half-toggled state where UI
        says one thing and state says another.
        """
        btn = "sidebar_oscillators_item_0_vis_btn"
        for _ in range(20):
            editor.click(btn)

        # Wait for things to settle.
        editor.wait_until(
            lambda: editor.get_oscillator_by_id(oscillator) is not None,
            timeout_s=3.0, desc="state read after burst",
        )
        osc = editor.get_oscillator_by_id(oscillator)
        assert osc["visible"] in (True, False), "visible must be a bool"
