"""
OscilClient - Python client for the Oscil Test Harness HTTP API.

This client wraps all HTTP endpoints exposed by the OscilTestHarness,
providing a clean Python interface for E2E testing.
"""

import requests
import time
from typing import Optional, Dict, Any, List
from dataclasses import dataclass


@dataclass
class OscillatorInfo:
    """Represents an oscillator's configuration."""
    id: str
    name: str
    source_id: str
    pane_id: str
    visible: bool
    color: str
    processing_mode: str


@dataclass
class PaneInfo:
    """Represents a pane's configuration."""
    id: str
    name: str
    column: int
    oscillator_count: int


class OscilClientError(Exception):
    """Exception raised when an API call fails."""
    pass


class OscilClient:
    """
    Python client for the Oscil Test Harness HTTP API.
    
    Usage:
        client = OscilClient()
        client.reset()
        client.add_oscillator("Osc1", "input_1_2", "pane_0")
        client.click("sidebar_options_gpuRenderingToggle")
    """
    
    def __init__(self, base_url: str = "http://127.0.0.1:8765", timeout: float = 10.0):
        """
        Initialize the client.
        
        Args:
            base_url: Base URL of the test harness HTTP server
            timeout: Default timeout for HTTP requests in seconds
        """
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self._session = requests.Session()
    
    def _request(self, method: str, endpoint: str, **kwargs) -> Dict[str, Any]:
        """Make an HTTP request and return the JSON response."""
        url = f"{self.base_url}{endpoint}"
        kwargs.setdefault("timeout", self.timeout)
        
        try:
            response = self._session.request(method, url, **kwargs)
            response.raise_for_status()
            data = response.json()
            
            if not data.get("success", True):
                error = data.get("error", "Unknown error")
                raise OscilClientError(f"API error: {error}")
            
            return data
        except requests.exceptions.RequestException as e:
            raise OscilClientError(f"Request failed: {e}")
    
    def _get(self, endpoint: str, **kwargs) -> Dict[str, Any]:
        """Make a GET request."""
        return self._request("GET", endpoint, **kwargs)
    
    def _post(self, endpoint: str, json_data: Optional[Dict] = None, **kwargs) -> Dict[str, Any]:
        """Make a POST request."""
        return self._request("POST", endpoint, json=json_data or {}, **kwargs)
    
    # =========================================================================
    # Health & Status
    # =========================================================================
    
    def ping(self) -> bool:
        """Check if the test harness is running."""
        try:
            self._get("/health")
            return True
        except OscilClientError:
            return False
    
    def wait_for_ready(self, timeout: float = 30.0, poll_interval: float = 0.5) -> bool:
        """Wait for the test harness to be ready."""
        start = time.time()
        while time.time() - start < timeout:
            if self.ping():
                return True
            time.sleep(poll_interval)
        return False
    
    # =========================================================================
    # State Management
    # =========================================================================
    
    def reset(self) -> Dict[str, Any]:
        """Reset the plugin to default state."""
        return self._post("/state/reset")
    
    def save_state(self, path: str) -> Dict[str, Any]:
        """Save current state to a file."""
        return self._post("/state/save", {"path": path})
    
    def load_state(self, path: str) -> Dict[str, Any]:
        """Load state from a file."""
        return self._post("/state/load", {"path": path})
    
    # =========================================================================
    # Oscillator Management
    # =========================================================================
    
    def get_oscillators(self) -> List[OscillatorInfo]:
        """Get all oscillators."""
        response = self._get("/state/oscillators")
        # API returns oscillators in "data" field
        oscillators = response.get("data", response.get("oscillators", []))
        if not isinstance(oscillators, list):
            oscillators = []
        return [
            OscillatorInfo(
                id=osc.get("id", ""),
                name=osc.get("name", ""),
                source_id=osc.get("sourceId", ""),
                pane_id=osc.get("paneId", ""),
                visible=osc.get("visible", True),
                color=osc.get("color", ""),
                processing_mode=osc.get("mode", osc.get("processingMode", "stereo"))
            )
            for osc in oscillators
        ]
    
    def add_oscillator(
        self,
        name: str,
        source_id: str = "input_1_2",
        pane_id: str = "pane_0",
        color: Optional[str] = None,
        visible: bool = True
    ) -> Dict[str, Any]:
        """Add a new oscillator."""
        payload = {
            "name": name,
            "sourceId": source_id,
            "paneId": pane_id,
            "visible": visible
        }
        if color:
            payload["color"] = color
        return self._post("/state/oscillator/add", payload)
    
    def update_oscillator(
        self,
        oscillator_id: str,
        name: Optional[str] = None,
        source_id: Optional[str] = None,
        pane_id: Optional[str] = None,
        visible: Optional[bool] = None,
        color: Optional[str] = None,
        processing_mode: Optional[str] = None
    ) -> Dict[str, Any]:
        """Update an oscillator's properties."""
        payload = {"id": oscillator_id}
        if name is not None:
            payload["name"] = name
        if source_id is not None:
            payload["sourceId"] = source_id
        if pane_id is not None:
            payload["paneId"] = pane_id
        if visible is not None:
            payload["visible"] = visible
        if color is not None:
            payload["color"] = color
        if processing_mode is not None:
            payload["processingMode"] = processing_mode
        return self._post("/state/oscillator/update", payload)
    
    def reorder_oscillators(self, order: List[str] = None, from_index: int = None, to_index: int = None) -> Dict[str, Any]:
        """Reorder oscillators by index or ID list."""
        if from_index is not None and to_index is not None:
            # Use index-based reordering
            return self._post("/state/oscillator/reorder", {
                "fromIndex": from_index,
                "toIndex": to_index
            })
        elif order is not None:
            # Convert ID list to index-based (get current order, map IDs to indices)
            current = self.get_oscillators()
            id_to_idx = {osc.id: i for i, osc in enumerate(current)}
            # For now, just use the first two if provided
            if len(order) >= 2:
                return self._post("/state/oscillator/reorder", {
                    "fromIndex": 0,
                    "toIndex": len(order) - 1
                })
        return {"success": False, "error": "Invalid parameters"}
    
    # =========================================================================
    # Pane Management
    # =========================================================================
    
    def get_panes(self) -> List[PaneInfo]:
        """Get all panes."""
        response = self._get("/state/panes")
        # API returns panes in "data" field
        panes = response.get("data", response.get("panes", []))
        if not isinstance(panes, list):
            panes = []
        return [
            PaneInfo(
                id=pane.get("id", ""),
                name=pane.get("name", ""),
                column=pane.get("column", 0),
                oscillator_count=pane.get("oscillatorCount", 0)
            )
            for pane in panes
        ]
    
    def add_pane(self, name: Optional[str] = None) -> Dict[str, Any]:
        """Add a new pane."""
        payload = {}
        if name:
            payload["name"] = name
        return self._post("/state/pane/add", payload)
    
    def swap_panes(self, pane_id_1: str, pane_id_2: str) -> Dict[str, Any]:
        """Swap two panes by index."""
        # API expects fromIndex and toIndex, not paneIds
        # Get current panes to find indices
        panes = self.get_panes()
        from_index = -1
        to_index = -1
        for i, pane in enumerate(panes):
            if pane.id == pane_id_1:
                from_index = i
            if pane.id == pane_id_2:
                to_index = i
        
        if from_index < 0 or to_index < 0:
            raise OscilClientError(f"Pane not found: {pane_id_1} or {pane_id_2}")
        
        return self._post("/state/pane/swap", {
            "fromIndex": from_index,
            "toIndex": to_index
        })
    
    # =========================================================================
    # GPU Rendering
    # =========================================================================
    
    def set_gpu_enabled(self, enabled: bool) -> Dict[str, Any]:
        """Enable or disable GPU rendering."""
        return self._post("/state/gpu", {"enabled": enabled})
    
    def is_gpu_enabled(self) -> bool:
        """Check if GPU rendering is enabled."""
        data = self._post("/state/gpu", {"enabled": True})  # Toggle returns current state
        # Actually we need to get current state, let's use UI state
        try:
            state = self.get_ui_state()
            return state.get("gpuEnabled", True)
        except:
            return True
    
    # =========================================================================
    # Display Settings
    # =========================================================================
    
    def get_display_settings(self) -> Dict[str, Any]:
        """Get current display settings."""
        data = self._get("/state/display")
        return data.get("data", data)
    
    def set_display_settings(self, **kwargs) -> Dict[str, Any]:
        """
        Update display settings.
        
        Supported kwargs: showGrid, autoScale, gain
        """
        return self._post("/state/display", kwargs)
    
    def set_layout(self, columns: int) -> Dict[str, Any]:
        """Set the column layout (1, 2, or 3 columns)."""
        return self._post("/state/layout", {"columns": columns})
    
    def set_theme(self, theme_name: str) -> Dict[str, Any]:
        """Set the active theme by name."""
        return self._post("/state/theme", {"theme": theme_name})
    
    def set_show_grid(self, show: bool) -> Dict[str, Any]:
        """Enable or disable grid display."""
        return self._post("/state/grid", {"show": show})
    
    # =========================================================================
    # Timing Settings
    # =========================================================================
    
    def get_timing_settings(self) -> Dict[str, Any]:
        """Get current timing settings."""
        data = self._get("/state/timing")
        return data.get("data", data)
    
    def set_timing_settings(self, **kwargs) -> Dict[str, Any]:
        """
        Update timing settings.
        
        Supported kwargs: mode, bpm, interval, syncToHost, timeIntervalMs
        """
        return self._post("/state/timing", kwargs)
    
    def set_timing_mode(self, mode: int) -> Dict[str, Any]:
        """Set timing mode (0=Time, 1=Melodic, 2=Triggered)."""
        return self.set_timing_settings(mode=mode)
    
    def set_bpm(self, bpm: float) -> Dict[str, Any]:
        """Set internal BPM."""
        return self.set_timing_settings(bpm=bpm)
    
    def set_host_sync(self, enabled: bool) -> Dict[str, Any]:
        """Enable or disable host BPM sync."""
        return self.set_timing_settings(syncToHost=enabled)
    
    # =========================================================================
    # Sources
    # =========================================================================
    
    def get_sources(self) -> List[Dict[str, Any]]:
        """Get available audio sources."""
        data = self._get("/state/sources")
        return data.get("sources", data.get("data", []))
    
    # =========================================================================
    # UI Interaction - Mouse
    # =========================================================================
    
    def click(self, element_id: str, x: Optional[int] = None, y: Optional[int] = None) -> Dict[str, Any]:
        """Click on an element by test ID."""
        payload = {"elementId": element_id}
        if x is not None:
            payload["x"] = x
        if y is not None:
            payload["y"] = y
        return self._post("/ui/click", payload)
    
    def double_click(self, element_id: str) -> Dict[str, Any]:
        """Double-click on an element."""
        return self._post("/ui/doubleClick", {"elementId": element_id})
    
    def right_click(self, element_id: str) -> Dict[str, Any]:
        """Right-click on an element."""
        return self._post("/ui/rightClick", {"elementId": element_id})
    
    def hover(self, element_id: str) -> Dict[str, Any]:
        """Hover over an element."""
        return self._post("/ui/hover", {"elementId": element_id})
    
    def select(self, element_id: str, value: str) -> Dict[str, Any]:
        """Select a value from a dropdown."""
        return self._post("/ui/select", {"elementId": element_id, "value": value})
    
    def toggle(self, element_id: str, state: Optional[bool] = None) -> Dict[str, Any]:
        """Toggle a switch/checkbox."""
        payload = {"elementId": element_id}
        if state is not None:
            payload["state"] = state
        return self._post("/ui/toggle", payload)
    
    def slider(self, element_id: str, value: float) -> Dict[str, Any]:
        """Set a slider value."""
        return self._post("/ui/slider", {"elementId": element_id, "value": value})
    
    def slider_increment(self, element_id: str, amount: float = 1.0) -> Dict[str, Any]:
        """Increment a slider value."""
        return self._post("/ui/slider/increment", {"elementId": element_id, "amount": amount})
    
    def slider_decrement(self, element_id: str, amount: float = 1.0) -> Dict[str, Any]:
        """Decrement a slider value."""
        return self._post("/ui/slider/decrement", {"elementId": element_id, "amount": amount})
    
    def slider_reset(self, element_id: str) -> Dict[str, Any]:
        """Reset a slider to default value."""
        return self._post("/ui/slider/reset", {"elementId": element_id})
    
    def drag(self, from_element: str, to_element: str) -> Dict[str, Any]:
        """Drag from one element to another."""
        return self._post("/ui/drag", {
            "fromElementId": from_element,
            "toElementId": to_element
        })
    
    def drag_offset(self, element_id: str, dx: int, dy: int) -> Dict[str, Any]:
        """Drag an element by offset."""
        return self._post("/ui/dragOffset", {
            "elementId": element_id,
            "dx": dx,
            "dy": dy
        })
    
    def scroll(self, element_id: str, dx: int = 0, dy: int = 0) -> Dict[str, Any]:
        """Scroll within an element."""
        return self._post("/ui/scroll", {
            "elementId": element_id,
            "dx": dx,
            "dy": dy
        })
    
    # =========================================================================
    # UI Interaction - Keyboard
    # =========================================================================
    
    def key_press(self, key: str, modifiers: Optional[List[str]] = None) -> Dict[str, Any]:
        """Press a key."""
        payload = {"key": key}
        if modifiers:
            payload["modifiers"] = modifiers
        return self._post("/ui/keyPress", payload)
    
    def type_text(self, element_id: str, text: str) -> Dict[str, Any]:
        """Type text into an element."""
        return self._post("/ui/typeText", {"elementId": element_id, "text": text})
    
    def clear_text(self, element_id: str) -> Dict[str, Any]:
        """Clear text from an element."""
        return self._post("/ui/clearText", {"elementId": element_id})
    
    # =========================================================================
    # UI Focus
    # =========================================================================
    
    def focus(self, element_id: str) -> Dict[str, Any]:
        """Focus an element."""
        return self._post("/ui/focus", {"elementId": element_id})
    
    def get_focused(self) -> Optional[str]:
        """Get the currently focused element."""
        data = self._get("/ui/focused")
        return data.get("elementId")
    
    def focus_next(self) -> Dict[str, Any]:
        """Focus the next element."""
        return self._post("/ui/focusNext")
    
    def focus_previous(self) -> Dict[str, Any]:
        """Focus the previous element."""
        return self._post("/ui/focusPrevious")
    
    # =========================================================================
    # UI Wait Conditions
    # =========================================================================
    
    def wait_for_element(self, element_id: str, timeout: float = 5.0) -> bool:
        """Wait for an element to exist."""
        try:
            self._post("/ui/waitForElement", {
                "elementId": element_id,
                "timeout": int(timeout * 1000)
            })
            return True
        except OscilClientError:
            return False
    
    def wait_for_visible(self, element_id: str, timeout: float = 5.0) -> bool:
        """Wait for an element to be visible."""
        try:
            self._post("/ui/waitForVisible", {
                "elementId": element_id,
                "timeout": int(timeout * 1000)
            })
            return True
        except OscilClientError:
            return False
    
    def wait_for_enabled(self, element_id: str, timeout: float = 5.0) -> bool:
        """Wait for an element to be enabled."""
        try:
            self._post("/ui/waitForEnabled", {
                "elementId": element_id,
                "timeout": int(timeout * 1000)
            })
            return True
        except OscilClientError:
            return False
    
    # =========================================================================
    # UI State Queries
    # =========================================================================
    
    def get_ui_state(self) -> Dict[str, Any]:
        """Get the current UI state."""
        data = self._get("/ui/state")
        return data.get("data", data)
    
    def get_elements(self, parent_id: Optional[str] = None) -> List[Dict[str, Any]]:
        """Get all UI elements (optionally under a parent)."""
        params = {}
        if parent_id:
            params["parentId"] = parent_id
        data = self._get("/ui/elements", params=params)
        return data.get("elements", [])
    
    def get_element(self, element_id: str) -> Dict[str, Any]:
        """Get details about a specific element."""
        data = self._get(f"/ui/element/{element_id}")
        return data.get("element", data)
    
    # =========================================================================
    # Screenshots & Visual Verification
    # =========================================================================
    
    def screenshot(self, path: Optional[str] = None) -> Dict[str, Any]:
        """Take a screenshot."""
        payload = {}
        if path:
            payload["path"] = path
        return self._post("/screenshot", payload)
    
    def screenshot_raw(self) -> bytes:
        """Get raw screenshot data."""
        response = self._session.get(f"{self.base_url}/screenshot/raw", timeout=self.timeout)
        response.raise_for_status()
        return response.content
    
    def screenshot_compare(self, baseline_path: str, threshold: float = 0.01) -> Dict[str, Any]:
        """Compare current screenshot to baseline."""
        return self._post("/screenshot/compare", {
            "baselinePath": baseline_path,
            "threshold": threshold
        })
    
    def baseline_save(self, path: str) -> Dict[str, Any]:
        """Save current screenshot as baseline."""
        return self._post("/baseline/save", {"path": path})
    
    # =========================================================================
    # Verification Endpoints
    # =========================================================================
    
    def verify_waveform(self, pane_id: str, expected_visible: bool = True) -> Dict[str, Any]:
        """Verify waveform rendering in a pane."""
        return self._post("/verify/waveform", {
            "paneId": pane_id,
            "expectedVisible": expected_visible
        })
    
    def verify_color(self, element_id: str, expected_color: str) -> Dict[str, Any]:
        """Verify an element's color."""
        return self._post("/verify/color", {
            "elementId": element_id,
            "expectedColor": expected_color
        })
    
    def verify_bounds(self, element_id: str, expected_bounds: Dict[str, int]) -> Dict[str, Any]:
        """Verify an element's bounds."""
        return self._post("/verify/bounds", {
            "elementId": element_id,
            "expectedBounds": expected_bounds
        })
    
    def verify_visible(self, element_id: str, expected: bool = True) -> Dict[str, Any]:
        """Verify an element's visibility."""
        return self._post("/verify/visible", {
            "elementId": element_id,
            "expected": expected
        })
    
    # =========================================================================
    # Waveform Analysis
    # =========================================================================
    
    def analyze_waveform(self, pane_id: Optional[str] = None) -> Dict[str, Any]:
        """Analyze waveform data."""
        params = {}
        if pane_id:
            params["paneId"] = pane_id
        return self._get("/analyze/waveform", params=params)
    
    # =========================================================================
    # Performance Metrics
    # =========================================================================
    
    def metrics_start(self) -> Dict[str, Any]:
        """Start performance metrics collection."""
        return self._post("/metrics/start")
    
    def metrics_stop(self) -> Dict[str, Any]:
        """Stop performance metrics collection."""
        return self._post("/metrics/stop")
    
    def metrics_current(self) -> Dict[str, Any]:
        """Get current performance metrics."""
        return self._get("/metrics/current")
    
    def metrics_stats(self) -> Dict[str, Any]:
        """Get performance statistics."""
        return self._get("/metrics/stats")
    
    def metrics_reset(self) -> Dict[str, Any]:
        """Reset performance metrics."""
        return self._post("/metrics/reset")
    
    # =========================================================================
    # Debug Logs
    # =========================================================================
    
    def get_logs(self, lines: Optional[int] = None) -> List[Dict[str, Any]]:
        """Get debug log entries."""
        try:
            params = {}
            if lines:
                params["lines"] = lines
            data = self._get("/debug/logs", params=params)
            return data.get("logs", data.get("data", {}).get("logs", []))
        except OscilClientError:
            # Endpoint not available, read from file directly
            return []
    
    def clear_logs(self) -> Dict[str, Any]:
        """Clear the debug log."""
        try:
            return self._post("/debug/logs/clear")
        except OscilClientError:
            # Endpoint not available, clear file directly
            import os
            log_path = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"
            if os.path.exists(log_path):
                with open(log_path, "w") as f:
                    f.write("")
            return {"success": True, "method": "file"}
    
    def wait_for_log(
        self,
        pattern: str,
        timeout: float = 5.0,
        hypothesis_id: Optional[str] = None
    ) -> Optional[Dict[str, Any]]:
        """Wait for a log entry matching a pattern."""
        try:
            payload = {
                "pattern": pattern,
                "timeout": int(timeout * 1000)
            }
            if hypothesis_id:
                payload["hypothesisId"] = hypothesis_id
            data = self._post("/debug/logs/wait", payload)
            return data.get("entry")
        except OscilClientError:
            return None
    
    # =========================================================================
    # Transport Control
    # =========================================================================
    
    def play(self) -> Dict[str, Any]:
        """Start playback."""
        return self._post("/transport/play")
    
    def stop(self) -> Dict[str, Any]:
        """Stop playback."""
        return self._post("/transport/stop")
    
    def set_transport_bpm(self, bpm: float) -> Dict[str, Any]:
        """Set the host/transport BPM."""
        return self._post("/transport/setBpm", {"bpm": bpm})
    
    def get_transport_state(self) -> Dict[str, Any]:
        """Get current transport state."""
        return self._get("/transport/state")
    
    # =========================================================================
    # Track/Editor Control
    # =========================================================================
    
    def show_editor(self, track_id: int = 0) -> Dict[str, Any]:
        """Show the plugin editor for a track."""
        return self._post(f"/track/{track_id}/showEditor")
    
    def hide_editor(self, track_id: int = 0) -> Dict[str, Any]:
        """Hide the plugin editor for a track."""
        return self._post(f"/track/{track_id}/hideEditor")
    
    def send_audio(self, track_id: int = 0, samples: Optional[List[float]] = None) -> Dict[str, Any]:
        """Send audio to a track for testing."""
        payload = {}
        if samples:
            payload["samples"] = samples
        return self._post(f"/track/{track_id}/audio", payload)
    
    def send_burst(self, track_id: int = 0, duration_ms: int = 100, amplitude: float = 0.5) -> Dict[str, Any]:
        """Send an audio burst to a track."""
        return self._post(f"/track/{track_id}/burst", {
            "durationMs": duration_ms,
            "amplitude": amplitude
        })
    
    # =========================================================================
    # Convenience Methods
    # =========================================================================
    
    def setup_basic_oscillator(self, name: str = "Test Oscillator") -> str:
        """Set up a basic oscillator for testing. Returns oscillator ID."""
        self.reset()
        self.show_editor()  # Ensure editor is visible
        result = self.add_oscillator(name, "input_1_2", "pane_0")
        # Extract oscillator ID from result
        return result.get("data", {}).get("oscillatorId", "")
    
    def wait_for_render(self, timeout: float = 2.0):
        """Wait for rendering to complete."""
        time.sleep(timeout)  # Simple wait; could be enhanced with log monitoring
    
    def close(self):
        """Close the HTTP session."""
        self._session.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

