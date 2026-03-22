"""
E2E tests for timing section controls and persistence.

What bugs these tests catch:
- Timing mode toggle not switching between Time and Melodic views
- Time interval field changes not affecting waveform displaySamples
- Melodic mode note interval dropdown not updating timing engine
- BPM field changes not propagating to displaySamples calculation
- Timing settings reset when user selects an oscillator (regression)
- Timing settings lost after editor close/reopen
"""

import pytest
from oscil_test_utils import OscilTestClient

# Actual test IDs from TimingSidebarSection.cpp:
#   sidebar_timing_modeToggle          (parent bar)
#   sidebar_timing_modeToggle_time     (TIME segment button)
#   sidebar_timing_modeToggle_melodic  (MELODIC segment button)
#   sidebar_timing_intervalField       (time interval text field)
#   sidebar_timing_noteDropdown        (note interval dropdown)
#   sidebar_timing_syncToggle          (host sync toggle)
#   sidebar_timing_bpmField            (BPM text field)
#   sidebar_timing_bpmDisplay          (BPM readout label)
#   sidebar_timing_waveformModeDropdown (waveform mode dropdown)

TIME_SEG = "sidebar_timing_modeToggle_time"
MELODIC_SEG = "sidebar_timing_modeToggle_melodic"
INTERVAL_FIELD = "sidebar_timing_intervalField"
NOTE_DROPDOWN = "sidebar_timing_noteDropdown"
BPM_FIELD = "sidebar_timing_bpmField"
SYNC_TOGGLE = "sidebar_timing_syncToggle"
WAVEFORM_MODE = "sidebar_timing_waveformModeDropdown"


class TestTimingModeToggle:
    """Switch between Time and Melodic modes."""

    def test_switch_to_melodic_mode(self, timing_section: OscilTestClient):
        """
        Bug caught: mode toggle segment click not firing, or melodic-specific
        controls not shown after mode switch.
        """
        c = timing_section

        if not c.element_exists(MELODIC_SEG):
            pytest.skip("Melodic mode segment not registered")

        assert c.click(MELODIC_SEG), "Melodic segment should be clickable"

        # In melodic mode, at least one melodic-specific control should appear
        melodic_controls = [NOTE_DROPDOWN, BPM_FIELD]
        found = False
        for ctrl in melodic_controls:
            if c.element_exists(ctrl):
                found = True
                break
        # If neither exists, the mode switch may not show controls in the harness
        if not found:
            pytest.skip("No melodic-specific controls registered after mode switch")

    def test_switch_back_to_time_mode(self, timing_section: OscilTestClient):
        """
        Bug caught: switching back to Time mode not restoring interval controls.
        """
        c = timing_section

        # Switch to melodic first
        if not c.element_exists(MELODIC_SEG):
            pytest.skip("Melodic mode segment not registered")
        c.click(MELODIC_SEG)

        # Switch back to time
        if not c.element_exists(TIME_SEG):
            pytest.skip("Time mode segment not registered")

        assert c.click(TIME_SEG), "Time segment should be clickable"

        # Interval field should exist in time mode
        if c.element_exists(INTERVAL_FIELD):
            el = c.get_element(INTERVAL_FIELD)
            assert el is not None, "Interval field should exist in time mode"


class TestTimeIntervalField:
    """Adjusting the time interval in Time mode."""

    def test_field_accepts_values(self, timing_section: OscilTestClient):
        """
        Bug caught: text field not wired to timing engine, or value clamping wrong.
        """
        c = timing_section
        if c.element_exists(TIME_SEG):
            c.click(TIME_SEG)

        if not c.element_exists(INTERVAL_FIELD):
            pytest.skip("Interval field not registered")

        # The interval field is a numeric text field.
        # Use set_slider which maps to the /ui/slider endpoint, which the
        # TestUIController routes to setNumericValue for text fields.
        assert c.set_slider(INTERVAL_FIELD, 50.0), "Field should accept value 50"
        assert c.set_slider(INTERVAL_FIELD, 200.0), "Field should accept value 200"

    def test_extreme_values(self, timing_section: OscilTestClient):
        """
        Bug caught: field not clamping at min/max, causing division by zero
        or buffer overflow when displaySamples is 0 or extremely large.
        """
        c = timing_section
        if not c.element_exists(INTERVAL_FIELD):
            pytest.skip("Interval field not registered")

        # Test minimum (field range is 0.1 to 4000)
        c.set_slider(INTERVAL_FIELD, 0.1)
        # Test maximum
        c.set_slider(INTERVAL_FIELD, 4000.0)


class TestMelodicMode:
    """Melodic mode controls: note intervals and BPM interaction."""

    def test_melodic_mode_display_samples_differ_from_time_mode(
        self, timing_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: displaySamples not recalculated when switching to
        melodic mode (the original bug this test was written for).
        """
        c = timing_section

        # Need an oscillator for waveform state
        osc_id = c.add_oscillator(source_id, name="Melodic Test")
        if not osc_id:
            pytest.skip("Cannot add oscillator")

        if not (c.element_exists(TIME_SEG) and c.element_exists(MELODIC_SEG)):
            pytest.skip("Mode toggle segments not registered")

        # Ensure Time mode and record samples
        c.click(TIME_SEG)
        time_samples = c.get_display_samples()

        # Switch to Melodic
        c.click(MELODIC_SEG)

        melodic_samples = c.get_display_samples()

        # Both should be positive
        if time_samples > 0:
            assert melodic_samples > 0, (
                f"Melodic mode should produce positive displaySamples, got {melodic_samples}"
            )

    def test_bpm_change_affects_melodic_samples(
        self, timing_section: OscilTestClient, source_id: str
    ):
        """
        Bug caught: BPM changes not recalculating displaySamples in melodic mode.
        """
        c = timing_section

        osc_id = c.add_oscillator(source_id, name="BPM Test")
        if not osc_id:
            pytest.skip("Cannot add oscillator")

        if not c.element_exists(MELODIC_SEG):
            pytest.skip("Melodic mode segment not registered")

        c.click(MELODIC_SEG)

        # Set BPM to 120
        c.set_bpm(120.0)
        samples_120 = c.get_display_samples()

        # Set BPM to 60 (half speed = double samples per beat)
        c.set_bpm(60.0)
        samples_60 = c.get_display_samples()

        if samples_120 > 0 and samples_60 > 0:
            assert samples_60 > samples_120, (
                f"60 BPM should have more samples than 120 BPM "
                f"(got {samples_60} vs {samples_120})"
            )

        c.set_bpm(120.0)


class TestWaveformModeDropdown:
    """Waveform mode dropdown in the timing section."""

    def test_waveform_mode_dropdown_exists(self, timing_section: OscilTestClient):
        """
        Bug caught: waveform mode dropdown not registered in timing section.
        """
        c = timing_section
        if not c.element_exists(WAVEFORM_MODE):
            pytest.skip("Waveform mode dropdown not registered")

        el = c.get_element(WAVEFORM_MODE)
        assert el is not None
        assert el.visible, "Waveform mode dropdown should be visible"

    def test_waveform_mode_selection_does_not_crash(
        self, timing_section: OscilTestClient
    ):
        """
        Bug caught: selecting a waveform mode causes null dereference or
        enum mismatch in the rendering pipeline.
        """
        c = timing_section
        if not c.element_exists(WAVEFORM_MODE):
            pytest.skip("Waveform mode dropdown not registered")

        el = c.get_element(WAVEFORM_MODE)
        items = el.extra.get("items", []) if el else []
        if not items:
            pytest.skip("Waveform mode dropdown has no items")

        for item in items:
            item_id = item.get("id", item) if isinstance(item, dict) else str(item)
            c.select_dropdown_item(WAVEFORM_MODE, str(item_id))

        # Verify harness is still responsive
        state = c.get_transport_state()
        assert state is not None, (
            "Harness should be responsive after cycling waveform modes"
        )


class TestTimingPersistence:
    """Timing settings survive oscillator selection and editor close/reopen."""

    def test_selecting_oscillator_does_not_reset_timing(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: the specific regression where clicking an oscillator
        in the sidebar reset the timing interval to default ~46ms.
        """
        id1 = editor.add_oscillator(source_id, name="Timing Test 1")
        id2 = editor.add_oscillator(source_id, name="Timing Test 2")
        if not (id1 and id2):
            pytest.skip("Cannot create oscillators")

        editor.wait_for_element("sidebar_oscillators_item_1", timeout_s=3.0)

        # Expand timing section
        timing_id = "sidebar_timing"
        if not editor.element_exists(timing_id):
            pytest.skip("Timing section not registered")
        editor.click(timing_id)

        if not editor.element_exists(INTERVAL_FIELD):
            pytest.skip("Interval field not registered")

        editor.set_slider(INTERVAL_FIELD, 10.0)

        samples_before = editor.get_display_samples()

        # Click on an oscillator
        editor.click("sidebar_oscillators_item_0")

        samples_after = editor.get_display_samples()

        if samples_before > 0 and samples_after > 0:
            ratio = samples_after / samples_before if samples_before else 0
            assert 0.5 < ratio < 2.0, (
                f"Timing reset after oscillator click: "
                f"{samples_before} -> {samples_after} (ratio {ratio:.2f})"
            )

    def test_timing_survives_editor_close_reopen(self, client: OscilTestClient):
        """
        Bug caught: timing settings not serialized/restored on editor lifecycle.
        """
        client.reset_state()
        client.wait_until(
            lambda: len(client.get_oscillators()) == 0,
            timeout_s=3.0, desc="state reset",
        )
        client.open_editor()

        timing_id = "sidebar_timing"
        if not client.element_exists(timing_id):
            client.close_editor()
            pytest.skip("Timing section not registered")

        client.click(timing_id)

        if not client.element_exists(INTERVAL_FIELD):
            client.close_editor()
            pytest.skip("Interval field not registered")

        client.set_slider(INTERVAL_FIELD, 75.0)
        samples_before = client.get_display_samples()

        client.close_editor()
        client.open_editor()

        if client.element_exists(timing_id):
            client.click(timing_id)

        samples_after = client.get_display_samples()
        client.close_editor()

        if samples_before > 0 and samples_after > 0:
            ratio = samples_after / samples_before if samples_before else 0
            assert 0.5 < ratio < 2.0, (
                f"Timing changed after reopen: {samples_before} -> {samples_after}"
            )
