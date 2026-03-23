"""
Page Objects for Oscil E2E Tests

Encapsulates UI navigation, element interactions, and state verification
into reusable objects that make tests read as user intent rather than
HTTP API calls.

Each page object:
  - Navigates to its page/view (including required state setup)
  - Exposes locators for interactive and assertable elements
  - Provides action methods (fill, click, select)
  - Provides assertion helpers (verify content loaded, verify state)
"""

import pytest
from oscil_test_utils import OscilTestClient, ElementInfo
from typing import Optional


class SidebarPage:
    """Encapsulates sidebar interactions: oscillator list, sections, resize."""

    def __init__(self, client: OscilTestClient):
        self.c = client

    # ── Locators ──────────────────────────────────────────────────

    SIDEBAR = "sidebar"
    ADD_BUTTON = "sidebar_addOscillator"
    TIMING_SECTION = "sidebar_timing"
    OPTIONS_SECTION = "sidebar_options"
    RESIZE_HANDLE = "sidebar_resizeHandle"

    def item_id(self, index: int) -> str:
        return f"sidebar_oscillators_item_{index}"

    def item_delete_id(self, index: int) -> str:
        return f"sidebar_oscillators_item_{index}_delete"

    def item_settings_id(self, index: int) -> str:
        return f"sidebar_oscillators_item_{index}_settings"

    def item_vis_id(self, index: int) -> str:
        return f"sidebar_oscillators_item_{index}_vis_btn"

    def filter_tab_id(self, name: str) -> str:
        return f"sidebar_oscillators_toolbar_{name}Tab"

    # ── Actions ───────────────────────────────────────────────────

    def assert_visible(self):
        """Assert sidebar is visible with non-zero dimensions."""
        el = self.c.get_element(self.SIDEBAR)
        assert el is not None, "Sidebar element must be registered"
        assert el.visible, "Sidebar must be visible"
        assert el.width > 0 and el.height > 0, (
            f"Sidebar must have non-zero size, got {el.width}x{el.height}"
        )

    def click_add(self):
        """Click the add oscillator button."""
        assert self.c.element_exists(self.ADD_BUTTON), (
            "Add oscillator button must be registered"
        )
        return self.c.click(self.ADD_BUTTON)

    def select_oscillator(self, index: int):
        """Click an oscillator list item to select it."""
        item = self.item_id(index)
        assert self.c.element_exists(item), (
            f"Oscillator list item {index} must be registered"
        )
        self.c.click(item)

    def delete_oscillator(self, index: int):
        """Click the delete button on an oscillator list item."""
        btn = self.item_delete_id(index)
        assert self.c.element_exists(btn), (
            f"Delete button for item {index} must be registered"
        )
        self.c.click(btn)
        # Handle confirmation dialog if present
        if self.c.element_exists("confirmDeleteDialog"):
            self.c.click("confirmDeleteDialog_confirm")

    def open_settings(self, index: int):
        """Click the settings button to open the config popup."""
        btn = self.item_settings_id(index)
        assert self.c.element_exists(btn), (
            f"Settings button for item {index} must be registered"
        )
        self.c.click(btn)
        self.c.wait_for_visible("configPopup", timeout_s=3.0)

    def toggle_visibility(self, index: int):
        """Click the visibility toggle on a list item."""
        btn = self.item_vis_id(index)
        if not self.c.element_exists(btn):
            btn = f"sidebar_oscillators_item_{index}_vis_toggle"
        assert self.c.element_exists(btn), (
            f"Visibility button for item {index} must be registered"
        )
        self.c.click(btn)

    def expand_timing(self):
        """Expand the timing accordion section."""
        assert self.c.element_exists(self.TIMING_SECTION), (
            "Timing section must be registered"
        )
        self.c.click(self.TIMING_SECTION)

    def expand_options(self):
        """Expand the options accordion section."""
        assert self.c.element_exists(self.OPTIONS_SECTION), (
            "Options section must be registered"
        )
        self.c.click(self.OPTIONS_SECTION)


class AddOscillatorDialog:
    """Encapsulates the Add Oscillator dialog interactions."""

    DIALOG = "addOscillatorDialog"
    SOURCE_DROPDOWN = "addOscillatorDialog_sourceDropdown"
    NAME_FIELD = "addOscillatorDialog_nameField"
    COLOR_PICKER = "addOscillatorDialog_colorPicker"
    PANE_SELECTOR = "addOscillatorDialog_paneSelector"
    OK_BUTTON = "addOscillatorDialog_okBtn"
    CANCEL_BUTTON = "addOscillatorDialog_cancelBtn"
    CLOSE_BUTTON = "addOscillatorDialog_closeBtn"

    def __init__(self, client: OscilTestClient):
        self.c = client

    def wait_for_open(self, timeout_s: float = 3.0):
        """Wait for dialog to be visible."""
        self.c.wait_for_visible(self.DIALOG, timeout_s=timeout_s)

    def wait_for_closed(self, timeout_s: float = 3.0):
        """Wait for dialog to disappear."""
        self.c.wait_for_not_visible(self.DIALOG, timeout_s=timeout_s)

    def is_open(self) -> bool:
        return self.c.element_visible(self.DIALOG)

    def select_source(self, source_id: str):
        """Select a source in the dropdown."""
        if self.c.element_exists(self.SOURCE_DROPDOWN):
            self.c.select_dropdown_item(self.SOURCE_DROPDOWN, source_id)

    def set_name(self, name: str):
        """Set the oscillator name."""
        if self.c.element_exists(self.NAME_FIELD):
            self.c.clear_text(self.NAME_FIELD)
            self.c.type_text(self.NAME_FIELD, name)

    def confirm(self):
        """Click OK to create the oscillator."""
        self.c.click(self.OK_BUTTON)
        self.wait_for_closed()

    def cancel(self):
        """Cancel the dialog."""
        for btn_id in (self.CANCEL_BUTTON, self.CLOSE_BUTTON):
            if self.c.element_exists(btn_id):
                self.c.click(btn_id)
                self.wait_for_closed()
                return
        raise AssertionError("No cancel/close button found on add dialog")


class ConfigPopup:
    """Encapsulates the oscillator config popup interactions."""

    POPUP = "configPopup"
    NAME_FIELD = "configPopup_nameField"
    SOURCE_DROPDOWN = "configPopup_sourceDropdown"
    MODE_SELECTOR = "configPopup_modeSelector"
    OPACITY_SLIDER = "configPopup_opacitySlider"
    LINE_WIDTH_SLIDER = "configPopup_lineWidthSlider"
    COLOR_PICKER = "configPopup_colorPicker"
    PANE_SELECTOR = "configPopup_paneSelector"
    VISUAL_PRESET = "configPopup_visualPresetDropdown"
    CLOSE_BUTTON = "configPopup_closeBtn"
    FOOTER_CLOSE = "configPopup_footerCloseBtn"

    MODE_BUTTONS = {
        "stereo": "FullStereo",
        "mono": "Mono",
        "mid": "Mid",
        "side": "Side",
        "left": "Left",
        "right": "Right",
    }

    def __init__(self, client: OscilTestClient):
        self.c = client

    def is_open(self) -> bool:
        return self.c.element_visible(self.POPUP)

    def wait_for_open(self, timeout_s: float = 3.0):
        self.c.wait_for_visible(self.POPUP, timeout_s=timeout_s)

    def close(self):
        """Close the popup via available close button."""
        for btn_id in (self.CLOSE_BUTTON, self.FOOTER_CLOSE):
            if self.c.element_exists(btn_id):
                self.c.click(btn_id)
                try:
                    self.c.wait_for_not_visible(self.POPUP, timeout_s=2.0)
                except TimeoutError:
                    pass
                return

    def set_name(self, name: str):
        """Change the oscillator name via the popup."""
        assert self.c.element_exists(self.NAME_FIELD), (
            "Name field must exist in config popup"
        )
        self.c.clear_text(self.NAME_FIELD)
        self.c.type_text(self.NAME_FIELD, name)

    def set_mode(self, mode_suffix: str) -> bool:
        """Click a mode button. Returns True if button exists and was clicked."""
        btn_id = f"{self.MODE_SELECTOR}_{mode_suffix}"
        if not self.c.element_exists(btn_id):
            return False
        self.c.click(btn_id)
        return True

    def set_opacity(self, value: float):
        """Set opacity slider value."""
        assert self.c.element_exists(self.OPACITY_SLIDER), (
            "Opacity slider must exist in config popup"
        )
        self.c.set_slider(self.OPACITY_SLIDER, value)

    def set_line_width(self, value: float):
        """Set line width slider value."""
        assert self.c.element_exists(self.LINE_WIDTH_SLIDER), (
            "Line width slider must exist in config popup"
        )
        self.c.set_slider(self.LINE_WIDTH_SLIDER, value)

    def has_element(self, element_id: str) -> bool:
        return self.c.element_exists(element_id)


class TimingSection:
    """Encapsulates timing section interactions."""

    TIME_SEG = "sidebar_timing_modeToggle_time"
    MELODIC_SEG = "sidebar_timing_modeToggle_melodic"
    INTERVAL_FIELD = "sidebar_timing_intervalField"
    NOTE_DROPDOWN = "sidebar_timing_noteDropdown"
    BPM_FIELD = "sidebar_timing_bpmField"
    SYNC_TOGGLE = "sidebar_timing_syncToggle"
    WAVEFORM_MODE = "sidebar_timing_waveformModeDropdown"

    def __init__(self, client: OscilTestClient):
        self.c = client

    def switch_to_time(self):
        """Switch to Time mode."""
        assert self.c.element_exists(self.TIME_SEG), (
            "Time mode segment must be registered"
        )
        self.c.click(self.TIME_SEG)

    def switch_to_melodic(self):
        """Switch to Melodic mode."""
        assert self.c.element_exists(self.MELODIC_SEG), (
            "Melodic mode segment must be registered"
        )
        self.c.click(self.MELODIC_SEG)

    def set_interval(self, value: float):
        """Set the time interval field value."""
        assert self.c.element_exists(self.INTERVAL_FIELD), (
            "Interval field must be registered"
        )
        self.c.set_slider(self.INTERVAL_FIELD, value)

    def has_element(self, element_id: str) -> bool:
        return self.c.element_exists(element_id)


class OptionsSection:
    """Encapsulates options section interactions."""

    THEME_DROPDOWN = "sidebar_options_themeDropdown"
    GPU_TOGGLE = "sidebar_options_gpuRenderingToggle"
    LAYOUT_DROPDOWN = "sidebar_options_layoutDropdown"
    QUALITY_DROPDOWN = "sidebar_options_qualityPresetDropdown"
    BUFFER_DROPDOWN = "sidebar_options_bufferDurationDropdown"
    GRID_TOGGLE = "sidebar_options_gridToggle"
    AUTO_SCALE_TOGGLE = "sidebar_options_autoScaleToggle"
    AUTO_ADJUST_TOGGLE = "sidebar_options_autoAdjustToggle"
    GAIN_SLIDER = "sidebar_options_gainSlider"

    def __init__(self, client: OscilTestClient):
        self.c = client

    def get_theme_info(self):
        """Get theme dropdown info: (selected_id, num_items, items_list)."""
        el = self.c.get_element(self.THEME_DROPDOWN)
        if not el or not el.extra:
            return None, None, []
        return (
            el.extra.get("selectedId", ""),
            el.extra.get("numItems", 0),
            el.extra.get("items", []),
        )

    def select_theme(self, theme_id: str):
        """Select a theme from the dropdown."""
        self.c.select_dropdown_item(self.THEME_DROPDOWN, theme_id)

    def has_element(self, element_id: str) -> bool:
        return self.c.element_exists(element_id)


class StateManager:
    """Encapsulates state save/load/reset operations with assertions."""

    def __init__(self, client: OscilTestClient):
        self.c = client

    def save_state(self, path: str):
        """Save state and assert success."""
        result = self.c.save_state(path)
        assert result, f"State save to '{path}' must succeed"

    def load_state(self, path: str):
        """Load state and assert success."""
        result = self.c.load_state(path)
        assert result, f"State load from '{path}' must succeed"

    def reset_and_wait(self, timeout_s: float = 3.0):
        """Reset state and wait for empty oscillator list."""
        self.c.reset_state()
        self.c.wait_for_oscillator_count(0, timeout_s=timeout_s)

    def save_load_roundtrip(self, path: str, expected_osc_count: int):
        """Save, reset, load, and wait for expected oscillator count."""
        self.save_state(path)
        self.reset_and_wait()
        self.load_state(path)
        return self.c.wait_for_oscillator_count(
            expected_osc_count, timeout_s=5.0
        )


class TransportControl:
    """Encapsulates transport play/stop operations."""

    def __init__(self, client: OscilTestClient):
        self.c = client

    def play_and_wait(self, timeout_s: float = 2.0):
        """Start transport and wait for playing state."""
        self.c.transport_play()
        self.c.wait_until(
            lambda: self.c.is_playing(),
            timeout_s=timeout_s,
            desc="transport to start playing",
        )

    def stop_and_wait(self, timeout_s: float = 2.0):
        """Stop transport and wait for stopped state."""
        self.c.transport_stop()
        self.c.wait_until(
            lambda: not self.c.is_playing(),
            timeout_s=timeout_s,
            desc="transport to stop",
        )

    def play_audio(self, waveform: str = "sine", frequency: float = 440.0,
                   amplitude: float = 0.8, track: int = 0):
        """Start transport and set audio generator parameters."""
        self.play_and_wait()
        self.c.set_track_audio(
            track, waveform=waveform, frequency=frequency, amplitude=amplitude
        )

    def assert_playing(self):
        assert self.c.is_playing(), "Transport must be playing"

    def assert_stopped(self):
        assert not self.c.is_playing(), "Transport must be stopped"


def require_element(client: OscilTestClient, element_id: str, context: str = ""):
    """Assert an element exists. Use instead of skip for required elements."""
    msg = f"Element '{element_id}' must be registered"
    if context:
        msg += f" ({context})"
    assert client.element_exists(element_id), msg


def xfail_if_missing(client: OscilTestClient, element_id: str, reason: str = ""):
    """Mark test as xfail if an optional element is missing."""
    if not client.element_exists(element_id):
        pytest.xfail(reason or f"Optional element '{element_id}' not registered")
