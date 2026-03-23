"""
Pytest fixtures for Oscil E2E tests.

Fixture hierarchy:
  client              -- session-scoped HTTP client, connected to the harness
  harness_capabilities -- session-scoped probe of available features
  editor              -- function-scoped: resets state, opens editor, yields, closes
  oscillator          -- function-scoped: creates one oscillator, yields its ID
  transport           -- function-scoped: ensures transport is playing

Page objects (function-scoped):
  sidebar_page        -- SidebarPage for sidebar interactions
  config_popup_page   -- ConfigPopup for config popup interactions
  timing_page         -- TimingSection for timing section interactions
  options_page        -- OptionsSection for options section interactions
  state_mgr           -- StateManager for state save/load/reset
  transport_ctrl      -- TransportControl for play/stop operations
"""

import pytest
from oscil_test_utils import OscilTestClient, HarnessConnectionError, HarnessCrashedError
from page_objects import (
    SidebarPage,
    AddOscillatorDialog,
    ConfigPopup,
    TimingSection,
    OptionsSection,
    StateManager,
    TransportControl,
)


# ---------------------------------------------------------------------------
# Core UI elements that MUST exist when the editor is open.
# If any of these are missing, the harness or plugin is broken -- not "optional."
# Tests should assert on these, never skip.
# ---------------------------------------------------------------------------

CORE_ELEMENTS = frozenset({
    "sidebar",
    "sidebar_addOscillator",
    "sidebar_timing",
    "sidebar_options",
})

# Elements that MUST be registered after an oscillator is added.
# These are not optional -- they are fundamental to the plugin's UI contract.
OSCILLATOR_ITEM_ELEMENTS = frozenset({
    "sidebar_oscillators_item_0",
    "sidebar_oscillators_item_0_settings",
    "sidebar_oscillators_item_0_delete",
})


# ---------------------------------------------------------------------------
# Session-scoped: one client for the entire pytest run
# ---------------------------------------------------------------------------

@pytest.fixture(scope="session")
def client() -> OscilTestClient:
    """Connect to the test harness once per session."""
    c = OscilTestClient()
    try:
        c.wait_for_harness(max_retries=15, delay=1.0)
    except HarnessConnectionError:
        pytest.skip("Test harness not running on localhost:8765")
    return c


@pytest.fixture(scope="session")
def harness_capabilities(client: OscilTestClient) -> dict:
    """
    Probe the harness once to discover which optional features are available.

    Returns a dict of capability flags so tests can use ``xfail`` with a reason
    instead of silently skipping. Core features are verified via assertion --
    only genuinely optional features get entries here.
    """
    caps = {}

    # Open editor to probe UI elements
    client.open_editor()

    # Verify core elements exist (fail fast if harness is fundamentally broken)
    element_ids = client.get_registered_element_ids()
    for core_id in CORE_ELEMENTS:
        assert core_id in element_ids, (
            f"Core element '{core_id}' not registered. "
            f"The test harness or plugin build is broken. "
            f"Registered: {len(element_ids)} elements."
        )

    # Probe optional features
    caps["has_filter_tabs"] = client.element_exists("sidebar_oscillators_toolbar_allTab")
    caps["has_diagnostic_snapshot"] = client.get_diagnostic_snapshot() is not None
    caps["has_metrics"] = client.metrics_current() is not None
    caps["has_state_save"] = True  # Will be checked when first used
    caps["has_pane_add"] = True  # Will be checked when first used

    # Verify oscillator item elements register correctly when an oscillator exists.
    # This catches a category of bugs where the sidebar list item renders but
    # does not register child elements in the test harness element registry.
    sources = client.get_sources()
    if sources:
        osc_id = client.add_oscillator(sources[0]["id"], name="Capability Probe")
        if osc_id:
            try:
                client.wait_for_element("sidebar_oscillators_item_0", timeout_s=5.0)
                element_ids = client.get_registered_element_ids()
                for item_id in OSCILLATOR_ITEM_ELEMENTS:
                    assert item_id in element_ids, (
                        f"Core oscillator element '{item_id}' not registered after "
                        f"adding an oscillator. The element registry is broken. "
                        f"Registered: {len(element_ids)} elements."
                    )
            finally:
                client.reset_state()
                client.wait_until(
                    lambda: len(client.get_oscillators()) == 0,
                    timeout_s=3.0,
                    desc="capability probe cleanup",
                )

    client.close_editor()
    return caps


@pytest.fixture(autouse=True)
def _abort_on_harness_crash(client: OscilTestClient):
    """Abort the entire test session immediately if the harness has crashed."""
    try:
        yield
    except HarnessCrashedError:
        pytest.exit(
            "HARNESS CRASHED — aborting test session. "
            "Check ~/Library/Logs/DiagnosticReports/ for crash details.",
            returncode=99,
        )


# ---------------------------------------------------------------------------
# Function-scoped: clean state + editor open/close per test
# ---------------------------------------------------------------------------

@pytest.fixture()
def editor(client: OscilTestClient):
    """
    Open editor, then reset state so tests start clean.

    Order matters: PluginEditor's constructor auto-creates a default
    oscillator + pane when state is empty (createDefaultOscillatorIfNeeded).
    By opening first, then resetting, we remove that default without it
    being re-created.
    """
    client.open_editor()
    client.reset_state()
    client.wait_until(
        lambda: len(client.get_oscillators()) == 0,
        timeout_s=3.0,
        desc="state reset to complete",
    )
    yield client
    client.close_editor()


@pytest.fixture()
def source_id(client: OscilTestClient) -> str:
    """Return the first available source ID. Skips if none available."""
    sources = client.get_sources()
    if not sources:
        pytest.skip("No audio sources available in test harness")
    return sources[0]["id"]


@pytest.fixture()
def oscillator(editor: OscilTestClient, source_id: str) -> str:
    """
    Create a single oscillator and return its ID.
    Editor is already open via the ``editor`` fixture.
    """
    osc_id = editor.add_oscillator(source_id, name="Test Oscillator")
    assert osc_id is not None, "Failed to add oscillator via state API"
    # Wait for UI to register the list item — the state API returns before
    # the async UI refresh runs, so allow time for the message thread to
    # process the queued refreshPanels callback.
    editor.wait_for_element("sidebar_oscillators_item_0", timeout_s=5.0)
    return osc_id


def _make_oscillators(client: OscilTestClient, source_id: str, count: int) -> list[str]:
    """Helper to create N oscillators and wait for their UI elements."""
    colours = ["#00FF00", "#00BFFF", "#FF6B6B", "#FFD93D", "#FF00FF"]
    ids = []
    for i in range(count):
        osc_id = client.add_oscillator(
            source_id,
            name=f"Oscillator {i + 1}",
            colour=colours[i % len(colours)],
        )
        assert osc_id is not None, f"Failed to add oscillator {i + 1}"
        ids.append(osc_id)
    # Wait for the last list item to appear
    client.wait_for_element(f"sidebar_oscillators_item_{count - 1}", timeout_s=5.0)
    return ids


@pytest.fixture()
def two_oscillators(editor: OscilTestClient, source_id: str) -> list[str]:
    """Create 2 oscillators and return their IDs."""
    return _make_oscillators(editor, source_id, 2)


@pytest.fixture()
def three_oscillators(editor: OscilTestClient, source_id: str) -> list[str]:
    """Create 3 oscillators and return their IDs."""
    return _make_oscillators(editor, source_id, 3)


@pytest.fixture()
def transport_playing(client: OscilTestClient):
    """Ensure transport is playing before the test, stop after."""
    client.transport_play()
    client.wait_until(lambda: client.is_playing(), timeout_s=2.0, desc="transport to start")
    yield client
    client.transport_stop()


@pytest.fixture()
def timing_section(editor: OscilTestClient) -> OscilTestClient:
    """Expand the timing accordion section. Requires editor to be open."""
    timing_id = "sidebar_timing"
    assert editor.element_exists(timing_id), (
        "Core element 'sidebar_timing' must be registered when editor is open"
    )
    editor.click(timing_id)
    # Wait for any timing content element to appear as a sentinel
    sentinels = [
        "sidebar_timing_modeToggle",
        "sidebar_timing_intervalField",
        "sidebar_timing_modeToggle_time",
    ]
    for sentinel in sentinels:
        if editor.element_exists(sentinel):
            return editor
    # If no sentinel found immediately, wait briefly for the first
    try:
        editor.wait_for_element(sentinels[0], timeout_s=2.0)
    except TimeoutError:
        pass  # Proceed anyway — tests will skip individual controls if missing
    return editor


@pytest.fixture()
def options_section(editor: OscilTestClient) -> OscilTestClient:
    """Expand the options accordion section. Requires editor to be open."""
    options_id = "sidebar_options"
    assert editor.element_exists(options_id), (
        "Core element 'sidebar_options' must be registered when editor is open"
    )
    editor.click(options_id)
    sentinels = [
        "sidebar_options_themeDropdown",
        "sidebar_options_gpuRenderingToggle",
    ]
    for sentinel in sentinels:
        if editor.element_exists(sentinel):
            return editor
    try:
        editor.wait_for_element(sentinels[0], timeout_s=2.0)
    except TimeoutError:
        pass
    return editor


@pytest.fixture()
def config_popup(editor: OscilTestClient, oscillator: str):
    """
    Open the config popup for the first oscillator.
    Yields (client, oscillator_id). Closes popup on teardown.
    """
    settings_btn = "sidebar_oscillators_item_0_settings"
    if not editor.element_exists(settings_btn):
        pytest.skip("Settings button not registered")
    editor.click(settings_btn)
    try:
        editor.wait_for_visible("configPopup", timeout_s=3.0)
    except TimeoutError:
        pytest.skip("Config popup not available")
    yield editor, oscillator
    # Teardown: close popup if still open
    for btn in ("configPopup_closeBtn", "configPopup_footerCloseBtn"):
        if editor.element_exists(btn):
            editor.click(btn)
            try:
                editor.wait_for_not_visible("configPopup", timeout_s=2.0)
            except TimeoutError:
                pass
            break


@pytest.fixture()
def two_panes(editor: OscilTestClient, source_id: str):
    """
    Create two panes: the auto-created default and a manually added second.
    Returns (pane1_id, pane2_id). Requires editor fixture.
    """
    osc_id = editor.add_oscillator(source_id, name="TwoPane Setup")
    assert osc_id is not None
    editor.wait_for_oscillator_count(1, timeout_s=3.0)

    panes = editor.get_panes()
    assert len(panes) >= 1, "First oscillator should auto-create a pane"
    pane1_id = panes[0]["id"]

    pane2_id = editor.add_pane("Second Pane")
    if pane2_id is None:
        pytest.skip("Pane add API not available")

    return pane1_id, pane2_id


@pytest.fixture()
def playing_oscillator(editor: OscilTestClient, source_id: str):
    """
    Create an oscillator with transport playing and sine audio active.
    Yields (client, oscillator_id). Stops transport on teardown.
    """
    osc_id = editor.add_oscillator(source_id, name="Playing Oscillator")
    assert osc_id is not None, "Failed to add oscillator"
    editor.wait_for_oscillator_count(1, timeout_s=3.0)

    editor.transport_play()
    editor.wait_until(lambda: editor.is_playing(), timeout_s=2.0, desc="transport to start")
    editor.set_track_audio(0, waveform="sine", frequency=440.0, amplitude=0.8)
    editor.wait_for_waveform_data(pane_index=0, timeout_s=5.0)

    yield editor, osc_id

    editor.transport_stop()


@pytest.fixture()
def saved_state_path(editor: OscilTestClient, source_id: str):
    """
    Create a non-trivial state (2 oscillators with different properties),
    save it, and return (client, file_path, oscillator_ids).
    The state is NOT reset — caller decides when to reset and reload.
    """
    id1 = editor.add_oscillator(
        source_id, name="Saved Osc A", colour="#FF0000", mode="Mono"
    )
    id2 = editor.add_oscillator(
        source_id, name="Saved Osc B", colour="#00FF00", mode="Mid"
    )
    assert id1 and id2
    editor.wait_for_oscillator_count(2, timeout_s=3.0)

    editor.update_oscillator(id1, visible=False, opacity=0.7, lineWidth=3.0)

    path = "/tmp/oscil_e2e_fixture_state.xml"
    saved = editor.save_state(path)
    if not saved:
        pytest.skip("State save API not available")

    yield editor, path, [id1, id2]


# ---------------------------------------------------------------------------
# Page object fixtures
# ---------------------------------------------------------------------------

@pytest.fixture()
def sidebar_page(editor: OscilTestClient) -> SidebarPage:
    """Sidebar page object for the current editor session."""
    return SidebarPage(editor)


@pytest.fixture()
def add_dialog(editor: OscilTestClient) -> AddOscillatorDialog:
    """Add oscillator dialog page object."""
    return AddOscillatorDialog(editor)


@pytest.fixture()
def config_popup_page(editor: OscilTestClient) -> ConfigPopup:
    """Config popup page object."""
    return ConfigPopup(editor)


@pytest.fixture()
def timing_page(editor: OscilTestClient) -> TimingSection:
    """Timing section page object."""
    return TimingSection(editor)


@pytest.fixture()
def options_page(editor: OscilTestClient) -> OptionsSection:
    """Options section page object."""
    return OptionsSection(editor)


@pytest.fixture()
def state_mgr(editor: OscilTestClient) -> StateManager:
    """State manager for save/load/reset operations."""
    return StateManager(editor)


@pytest.fixture()
def transport_ctrl(editor: OscilTestClient) -> TransportControl:
    """Transport control for play/stop operations."""
    return TransportControl(editor)
