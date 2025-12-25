"""
pytest fixtures for Oscil E2E testing.

This module provides fixtures for:
- Test harness lifecycle management
- OscilClient initialization
- LogAnalyzer setup
- Common test setup/teardown
"""

import pytest
import subprocess
import time
import os
import signal
from pathlib import Path
from typing import Generator, Optional

from .oscil_client import OscilClient, OscilClientError
from .oscil_test_utils import OscilTestClient
from .log_analyzer import LogAnalyzer
from .assertions import OscilAssertions


# ============================================================================
# Configuration
# ============================================================================

# Path to the test harness binary
TEST_HARNESS_PATH = Path("/Users/Spare/Documents/code/MultiScoper/build/Test/OscilTestHarness_artefacts/Debug/OscilTestHarness.app/Contents/MacOS/OscilTestHarness")

# Alternative paths to try
ALTERNATIVE_HARNESS_PATHS = [
    Path("/Users/Spare/Documents/code/MultiScoper/build/Test/OscilTestHarness_artefacts/Release/OscilTestHarness.app/Contents/MacOS/OscilTestHarness"),
    Path("/Users/Spare/Documents/code/MultiScoper/cmake-build-debug/Test/OscilTestHarness_artefacts/Debug/OscilTestHarness.app/Contents/MacOS/OscilTestHarness"),
]

# HTTP server configuration
DEFAULT_PORT = 8765
DEFAULT_TIMEOUT = 30.0

# Debug log path
DEBUG_LOG_PATH = Path("/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log")


def find_harness_binary() -> Optional[Path]:
    """Find the test harness binary."""
    if TEST_HARNESS_PATH.exists():
        return TEST_HARNESS_PATH
    
    for path in ALTERNATIVE_HARNESS_PATHS:
        if path.exists():
            return path
    
    return None


# ============================================================================
# Fixtures
# ============================================================================

@pytest.fixture(scope="session")
def harness_binary() -> Optional[Path]:
    """Get the path to the test harness binary."""
    # First check if harness is already running
    try:
        client = OscilClient(timeout=2.0)
        if client.ping():
            return None  # Already running, no binary needed
    except Exception:
        pass
    
    binary = find_harness_binary()
    if not binary:
        pytest.skip("Test harness binary not found. Build with -DOSCIL_BUILD_TEST_HARNESS=ON")
    return binary


@pytest.fixture(scope="session")
def test_harness(harness_binary: Optional[Path]) -> Generator[Optional[subprocess.Popen], None, None]:
    """
    Start the test harness for the entire test session.
    
    This fixture starts the OscilTestHarness process and waits for it to be ready.
    The process is terminated when the test session ends.
    If the harness is already running, it uses the existing instance.
    """
    # Check if harness is already running
    client = OscilClient(timeout=2.0)
    if client.ping():
        # Already running, yield None to indicate we didn't start it
        print("Test harness already running, using existing instance")
        yield None
        return
    
    # Start the test harness
    env = os.environ.copy()
    env["OSCIL_TEST_MODE"] = "1"
    
    process = subprocess.Popen(
        [str(harness_binary)],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # Wait for the harness to be ready
    client = OscilClient(timeout=DEFAULT_TIMEOUT)
    if not client.wait_for_ready(timeout=DEFAULT_TIMEOUT):
        process.terminate()
        process.wait(timeout=5)
        pytest.fail("Test harness failed to start")
    
    # Wait a bit more for UI to fully initialize
    time.sleep(1.0)
    
    yield process
    
    # Cleanup: terminate the process
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


@pytest.fixture(scope="function")
def oscil_client(test_harness) -> Generator[OscilClient, None, None]:
    """
    Get an OscilClient connected to the test harness.
    
    This fixture provides a fresh client for each test and ensures
    the harness is ready before running the test.
    """
    c = OscilClient()
    
    # Verify connection
    if not c.wait_for_ready(timeout=5.0):
        pytest.fail("Cannot connect to test harness")
    
    yield c
    
    c.close()


@pytest.fixture(scope="function")
def client(test_harness) -> Generator[OscilTestClient, None, None]:
    """
    Get an OscilTestClient connected to the test harness.
    
    This is the primary fixture used by most E2E tests.
    It provides assertion methods and full API support.
    """
    c = OscilTestClient(strict_mode=True)
    
    # Verify connection
    if not c.wait_for_harness(max_retries=5, delay=0.5):
        pytest.fail("Cannot connect to test harness")
    
    yield c


@pytest.fixture(scope="function")
def clean_state(client: OscilTestClient) -> OscilTestClient:
    """
    Ensure a clean plugin state before each test.
    
    This fixture resets the plugin state and clears the debug log.
    """
    # Reset plugin state
    client.reset_state()
    
    # Ensure editor is open
    client.open_editor()
    
    # Wait for reset to complete
    time.sleep(0.5)
    
    return client


@pytest.fixture(scope="function")
def analyzer() -> Generator[LogAnalyzer, None, None]:
    """
    Get a LogAnalyzer for the current test.
    
    This fixture clears the log before the test and provides
    access to log analysis after actions are taken.
    """
    analyzer = LogAnalyzer()
    analyzer.clear()
    
    yield analyzer


@pytest.fixture(scope="function")
def assertions(analyzer: LogAnalyzer) -> OscilAssertions:
    """
    Get an OscilAssertions instance for the current test.
    """
    return OscilAssertions(analyzer)


@pytest.fixture(scope="function")
def setup_oscillator(clean_state: OscilTestClient) -> tuple[OscilTestClient, str]:
    """
    Set up a basic oscillator for testing.
    
    Returns the client and the oscillator ID.
    """
    result = clean_state.add_oscillator("input_1_2", name="Test Oscillator")
    osc_id = result.get("data", {}).get("id", result.get("data", {}).get("oscillatorId", ""))
    time.sleep(0.5)  # Wait for UI update
    return clean_state, osc_id


@pytest.fixture(scope="function")
def setup_two_oscillators(clean_state: OscilTestClient) -> tuple[OscilTestClient, str, str]:
    """
    Set up two oscillators for testing.
    
    Returns the client and both oscillator IDs.
    """
    result1 = clean_state.add_oscillator("input_1_2", name="Oscillator 1")
    result2 = clean_state.add_oscillator("input_1_2", name="Oscillator 2")
    
    osc_id_1 = result1.get("data", {}).get("id", result1.get("data", {}).get("oscillatorId", ""))
    osc_id_2 = result2.get("data", {}).get("id", result2.get("data", {}).get("oscillatorId", ""))
    
    time.sleep(0.5)  # Wait for UI update
    return clean_state, osc_id_1, osc_id_2


@pytest.fixture(scope="function")
def gpu_enabled(clean_state: OscilTestClient) -> OscilTestClient:
    """
    Ensure GPU rendering is enabled.
    """
    # Use transport/state endpoint or similar for GPU control
    try:
        import requests
        requests.post(f"{clean_state.base_url}/state/gpu", json={"enabled": True}, timeout=2.0)
    except:
        pass
    time.sleep(0.3)
    return clean_state


@pytest.fixture(scope="function")
def cpu_mode(clean_state: OscilTestClient) -> OscilTestClient:
    """
    Ensure CPU rendering is enabled (GPU disabled).
    """
    try:
        import requests
        requests.post(f"{clean_state.base_url}/state/gpu", json={"enabled": False}, timeout=2.0)
    except:
        pass
    time.sleep(0.3)
    return clean_state


# ============================================================================
# Markers
# ============================================================================

def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers", "gpu: marks tests that require GPU rendering"
    )
    config.addinivalue_line(
        "markers", "visual: marks visual regression tests"
    )


# ============================================================================
# Hooks
# ============================================================================

def pytest_collection_modifyitems(config, items):
    """Modify test collection based on markers and options."""
    # Add slow marker to long-running tests
    for item in items:
        if "visual" in item.keywords:
            item.add_marker(pytest.mark.slow)


# ============================================================================
# Utility Fixtures
# ============================================================================

@pytest.fixture
def wait_for_render():
    """Helper to wait for rendering to complete."""
    def _wait(seconds: float = 0.5):
        time.sleep(seconds)
    return _wait


@pytest.fixture
def screenshot_dir(tmp_path: Path) -> Path:
    """Get a temporary directory for screenshots."""
    screenshots = tmp_path / "screenshots"
    screenshots.mkdir(exist_ok=True)
    return screenshots


@pytest.fixture
def baseline_dir() -> Path:
    """Get the baseline directory for visual regression tests."""
    baselines = Path("/Users/Spare/Documents/code/MultiScoper/tests/e2e/baselines")
    baselines.mkdir(exist_ok=True)
    return baselines

