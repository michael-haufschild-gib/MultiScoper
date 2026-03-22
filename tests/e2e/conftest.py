"""
Pytest fixtures for Oscil E2E tests.

Fixture hierarchy:
  client          -- session-scoped HTTP client, connected to the harness
  editor          -- function-scoped: resets state, opens editor, yields, closes
  oscillator      -- function-scoped: creates one oscillator, yields its ID
  transport       -- function-scoped: ensures transport is playing
"""

import pytest
from oscil_test_utils import OscilTestClient, HarnessConnectionError


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
    if not editor.element_exists(timing_id):
        pytest.skip("Timing section not registered")
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
    if not editor.element_exists(options_id):
        pytest.skip("Options section not registered")
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
