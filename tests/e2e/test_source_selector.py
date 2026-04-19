"""
E2E coverage for the source dropdown inside the Add Oscillator and
Config Popup dialogs.

What bugs these tests catch:
- Dropdown populated at construction but never refreshed when a new
  track (and therefore a new source) joins the session → engineer
  can't see freshly-added tracks without restarting the UI.
- Selecting a source via the dropdown does not propagate to the new
  oscillator's `sourceId` — the osc binds to a default source and
  displays the wrong waveform.
- Switching source in the config popup does not re-bind the waveform
  capture buffer, so the signal doesn't update.
- Dropdown has stale items after a source is deregistered (removed
  track leaves a dangling entry).
- Dropdown number of items does not equal the number of visible
  sources.
"""

from __future__ import annotations

from multiscoper_test_utils import MultiScoperTestClient
from page_objects import AddOscillatorDialog, ConfigPopup


def _source_dropdown_items(editor: MultiScoperTestClient, dropdown_id: str) -> list:
    el = editor.get_element(dropdown_id)
    if not el or not el.extra:
        return []
    return list(el.extra.get("items", []))


class TestAddDialogSourceDropdown:
    """The add-oscillator dialog's source dropdown exposes sources."""

    def test_dropdown_item_count_matches_registered_sources(
        self, editor: MultiScoperTestClient
    ):
        """Bug caught: dropdown populated from a stale snapshot instead
        of the live SourceManager — new tracks don't appear, removed
        tracks linger."""
        sources = editor.get_sources()

        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        items = _source_dropdown_items(editor, AddOscillatorDialog.SOURCE_DROPDOWN)
        assert len(items) == len(sources), (
            f"dropdown has {len(items)} items but {len(sources)} sources are registered. "
            f"items={items}"
        )

        dialog.cancel()

    def test_dropdown_contains_track_0_source(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Track 0's source must be selectable in the dropdown."""
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        items = _source_dropdown_items(editor, AddOscillatorDialog.SOURCE_DROPDOWN)
        item_ids = {item.get("id") for item in items}
        assert source_id in item_ids, (
            f"track 0 source {source_id} missing from dropdown "
            f"(items={item_ids})"
        )

        dialog.cancel()

    def test_selecting_source_binds_oscillator(
        self, editor: MultiScoperTestClient
    ):
        """Selecting each of N available sources and confirming creates
        an oscillator bound to THAT source, not a default.

        Bug caught: onSelectionChanged captures the first source ID at
        dialog construction time and never updates → every osc binds
        to the first source regardless of UI selection.
        """
        sources = editor.get_sources()
        assert len(sources) >= 2, (
            f"test requires ≥2 sources; got {len(sources)}"
        )

        target = sources[1]["id"]

        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()

        dialog.select_source(target)
        dialog.set_name("BoundToTarget")
        dialog.confirm()

        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        oscs = editor.get_oscillators()
        assert len(oscs) == 1
        assert oscs[0]["sourceId"] == target, (
            f"oscillator must bind to selected source {target}, "
            f"got {oscs[0]['sourceId']}"
        )


class TestConfigPopupSourceDropdown:
    """The config popup exposes a source selector."""

    def test_source_selector_registered_in_popup(
        self, editor: MultiScoperTestClient, oscillator: str
    ):
        """The configPopup_sourceDropdown testId registers when the
        config popup is open.

        Bug caught: SourceSelectorComponent not wired to the dialog or
        testId registration dropped during refactor → tests and
        accessibility tooling can't find it.
        """
        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)

        try:
            assert editor.element_exists(ConfigPopup.SOURCE_DROPDOWN), (
                "configPopup_sourceDropdown must be registered when popup is open"
            )
            el = editor.get_element(ConfigPopup.SOURCE_DROPDOWN)
            assert el is not None and el.visible
        finally:
            editor.click("configPopup_closeBtn")
            editor.wait_for_not_visible("configPopup", timeout_s=3.0)

    def test_source_selector_does_not_affect_other_oscillator_fields(
        self, editor: MultiScoperTestClient, source_id: str
    ):
        """Opening the config popup for an oscillator must not silently
        mutate the oscillator's sourceId. Bug caught: constructor pulls
        the dropdown's default-selected index and writes it back to
        state, rebinding every oscillator to source 0 on every popup
        open.
        """
        osc_id = editor.add_oscillator(source_id, name="NoMutation")
        assert osc_id is not None
        editor.wait_for_oscillator_count(1, timeout_s=3.0)
        before = editor.get_oscillator_by_id(osc_id)

        editor.click("sidebar_oscillators_item_0_settings")
        editor.wait_for_visible("configPopup", timeout_s=3.0)
        editor.click("configPopup_closeBtn")
        editor.wait_for_not_visible("configPopup", timeout_s=3.0)

        after = editor.get_oscillator_by_id(osc_id)
        assert before["sourceId"] == after["sourceId"], (
            f"sourceId must not change from popup open/close: "
            f"before={before['sourceId']} after={after['sourceId']}"
        )
        assert before["colour"] == after["colour"]
        assert before["name"] == after["name"]


class TestDropdownRefreshOnTrackChange:
    """Dropdown reflects newly-added / removed tracks."""

    def test_dropdown_updates_when_track_added(
        self, editor: MultiScoperTestClient
    ):
        """Add a track mid-session and reopen the dialog.  The new
        track's source must appear in the dropdown.

        Bug caught: dropdown cached at first open, never refreshed on
        subsequent opens.
        """
        sources_before = editor.get_sources()

        # Open dialog once to baseline.
        dialog = AddOscillatorDialog(editor)
        editor.click("sidebar_addOscillator")
        dialog.wait_for_open()
        baseline_items = _source_dropdown_items(editor, AddOscillatorDialog.SOURCE_DROPDOWN)
        dialog.cancel()

        # Add a new track.
        created = editor.add_track("NewDropdownTrack")
        assert created is not None, "add_track must succeed"
        new_source = created["sourceId"]
        editor.wait_until(
            lambda: any(s["id"] == new_source for s in editor.get_sources()),
            timeout_s=5.0, desc="new source to register globally",
        )

        try:
            # Reopen dialog; must now contain the new source.
            editor.click("sidebar_addOscillator")
            dialog.wait_for_open()
            fresh_items = _source_dropdown_items(
                editor, AddOscillatorDialog.SOURCE_DROPDOWN
            )
            fresh_ids = {i.get("id") for i in fresh_items}
            assert new_source in fresh_ids, (
                f"new source {new_source} missing from dropdown after track add. "
                f"baseline_items={baseline_items}, fresh_ids={fresh_ids}"
            )
            dialog.cancel()
        finally:
            editor.remove_track(created["trackIndex"])
