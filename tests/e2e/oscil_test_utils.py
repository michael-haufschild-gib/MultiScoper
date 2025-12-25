"""
Oscil Test Harness Utilities
Provides common functions for E2E testing via the test harness HTTP API
"""

import requests
import time
import sys
import json
from typing import Optional, Dict, Any, List, Tuple
from dataclasses import dataclass
from enum import Enum


class TestResult(Enum):
    PASSED = "passed"
    FAILED = "failed"
    SKIPPED = "skipped"


@dataclass
class ElementInfo:
    """Information about a UI element"""
    element_id: str
    visible: bool
    enabled: bool
    bounds: Dict[str, int]
    showing: bool = False
    focused: bool = False
    extra: Dict[str, Any] = None

    @classmethod
    def from_response(cls, element_id: str, data: Dict) -> 'ElementInfo':
        return cls(
            element_id=element_id,
            visible=data.get('visible', False),
            enabled=data.get('enabled', True),
            bounds=data.get('bounds', {}),
            showing=data.get('showing', False),
            focused=data.get('focused', False),
            extra=data
        )


class OscilTestClient:
    """Client for interacting with the Oscil test harness"""

    DEFAULT_URL = "http://localhost:8765"
    DEFAULT_TIMEOUT = 10.0  # Increased for editor open operations

    def __init__(self, base_url: str = DEFAULT_URL, strict_mode: bool = True):
        """
        Initialize the test client.

        Args:
            base_url: URL of the test harness HTTP server
            strict_mode: If True, assertion failures raise AssertionError and stop execution
        """
        self.base_url = base_url
        self.timeout = self.DEFAULT_TIMEOUT
        self.strict_mode = strict_mode
        self._test_results: List[Tuple[str, TestResult, str]] = []
        self._editor_open = False

    # ==================== Health & Setup ====================

    def wait_for_harness(self, max_retries: int = 10, delay: float = 1.0) -> bool:
        """Wait for the test harness to be available"""
        for attempt in range(max_retries):
            try:
                response = requests.get(f"{self.base_url}/health", timeout=2.0)
                if response.status_code == 200:
                    print(f"[OK] Connected to test harness at {self.base_url}")
                    return True
            except requests.exceptions.ConnectionError:
                pass
            print(f"[...] Waiting for harness (attempt {attempt + 1}/{max_retries})")
            time.sleep(delay)
        print(f"[FAIL] Could not connect to test harness at {self.base_url}")
        return False

    def reset_state(self) -> bool:
        """Reset plugin to default state"""
        try:
            response = requests.post(
                f"{self.base_url}/state/reset",
                json={},  # httplib requires a body for POST requests
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Reset state failed: {e}")

    def reset(self) -> Dict:
        """Alias for reset_state that returns dict"""
        success = self.reset_state()
        return {"success": success}

    def open_editor(self, track_id: int = 0, wait_for_elements: bool = False, max_wait: float = 10.0) -> bool:
        """
        Open the plugin editor window and verify it loaded successfully.

        Args:
            track_id: Track index to open editor for
            wait_for_elements: If True, poll until UI elements are registered (default: False)
            max_wait: Maximum seconds to wait for elements

        Returns True if editor request was successful.
        Note: UI elements may not be registered if TestElementRegistry is not integrated.
        """
        try:
            response = requests.post(
                f"{self.base_url}/track/{track_id}/showEditor",
                json={},  # httplib requires a body for POST requests
                timeout=self.timeout
            )
            response_data = response.json()

            if response.status_code != 200 or not response_data.get('success', False):
                error_msg = response_data.get('error', 'Unknown error')
                print(f"[ERROR] Failed to request editor open: {error_msg}")
                self._editor_open = False
                return False

            data = response_data.get('data', {})

            # Editor opened or already open
            if data.get('alreadyOpen', False) or data.get('editorVisible', False):
                elements_registered = data.get('elementsRegistered', 0)
                print(f"[INFO] Editor ready: elements={elements_registered}")
                self._editor_open = True
                return True

            # Editor opening was requested asynchronously
            if not wait_for_elements:
                print("[INFO] Editor open requested (async)")
                self._editor_open = True
                return True

            # Poll for elements to be registered
            print("[INFO] Waiting for editor to be ready...")
            poll_interval = 0.2
            elapsed = 0.0
            while elapsed < max_wait:
                time.sleep(poll_interval)
                elapsed += poll_interval

                try:
                    elem_response = requests.get(
                        f"{self.base_url}/ui/elements",
                        timeout=2.0
                    )
                    if elem_response.status_code == 200:
                        elem_data = elem_response.json().get('data', {})
                        count = elem_data.get('count', 0)
                        if count > 0:
                            print(f"[OK] Editor ready with {count} UI elements (after {elapsed:.1f}s)")
                            self._editor_open = True
                            return True
                except Exception:
                    pass

            # Timeout - editor didn't become ready
            print(f"[ERROR] Editor failed to become ready after {max_wait}s (no UI elements registered)")
            print("[ERROR] This usually means TEST_HARNESS is not defined in the plugin build")
            self._editor_open = False
            return False

        except Exception as e:
            print(f"[ERROR] Open editor failed: {e}")
            self._editor_open = False
            return False

    def verify_editor_ready(self) -> bool:
        """Verify the editor is open and has UI elements registered."""
        try:
            response = requests.get(
                f"{self.base_url}/ui/elements",
                timeout=self.timeout
            )
            if response.status_code == 200:
                data = response.json().get('data', {})
                count = data.get('count', 0)
                if count > 0:
                    print(f"[OK] Editor ready with {count} UI elements")
                    return True
                else:
                    print(f"[ERROR] Editor has no UI elements registered")
                    return False
            return False
        except Exception as e:
            print(f"[ERROR] Verify editor ready failed: {e}")
            return False

    def close_editor(self, track_id: int = 0) -> bool:
        """Close the plugin editor window"""
        try:
            response = requests.post(
                f"{self.base_url}/track/{track_id}/hideEditor",
                json={},  # httplib requires a body for POST requests
                timeout=self.timeout
            )
            self._editor_open = False
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Close editor failed: {e}")
            return False

    # ==================== Element Queries ====================

    def get_element(self, element_id: str) -> Optional[ElementInfo]:
        """
        Get information about a UI element by its test ID.

        Returns None if:
        - Element doesn't exist (404 response)
        - Network error
        - Response contains error

        Returns ElementInfo only for valid element data.
        """
        try:
            response = requests.get(
                f"{self.base_url}/ui/element/{element_id}",
                timeout=self.timeout
            )

            # 404 means element not found
            if response.status_code == 404:
                return None

            if response.status_code == 200:
                response_data = response.json()

                # Check for success flag
                if not response_data.get('success', False):
                    return None

                data = response_data.get('data', {})

                # Check for error in data (shouldn't happen with 200, but be defensive)
                if 'error' in data:
                    return None

                return ElementInfo.from_response(element_id, data)

            return None
        except Exception as e:
            print(f"[ERROR] Get element '{element_id}' failed: {e}")
            return None

    def element_exists(self, element_id: str) -> bool:
        """Check if an element exists in the TestElementRegistry"""
        elem = self.get_element(element_id)
        return elem is not None

    def element_visible(self, element_id: str) -> bool:
        """Check if an element is visible"""
        elem = self.get_element(element_id)
        return elem is not None and elem.visible and elem.showing

    def wait_for_element(self, element_id: str, timeout_ms: int = 5000) -> bool:
        """Wait for an element to appear"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/waitForElement",
                json={"elementId": element_id, "timeoutMs": timeout_ms},
                timeout=(timeout_ms / 1000) + 1
            )
            return response.status_code == 200 and response.json().get('status') == 'success'
        except Exception as e:
            print(f"[ERROR] Wait for element '{element_id}' failed: {e}")
            return False

    def wait_for_visible(self, element_id: str, timeout_ms: int = 5000) -> bool:
        """Wait for an element to become visible"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/waitForVisible",
                json={"elementId": element_id, "timeoutMs": timeout_ms},
                timeout=(timeout_ms / 1000) + 1
            )
            return response.status_code == 200 and response.json().get('status') == 'success'
        except Exception as e:
            print(f"[ERROR] Wait for visible '{element_id}' failed: {e}")
            return False

    # ==================== UI Interactions ====================

    def click(self, element_id: str) -> bool:
        """Click on an element"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/click",
                json={"elementId": element_id},
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.1)  # Small delay after click
                return True
            print(f"[WARN] Click on '{element_id}' returned status {response.status_code}")
            return False
        except Exception as e:
            print(f"[ERROR] Click on '{element_id}' failed: {e}")
            return False

    def double_click(self, element_id: str) -> bool:
        """Double-click on an element"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/doubleClick",
                json={"elementId": element_id},
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.1)
                return True
            return False
        except Exception as e:
            print(f"[ERROR] Double-click on '{element_id}' failed: {e}")
            return False

    def type_text(self, element_id: str, text: str) -> bool:
        """Type text into a text field"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/typeText",
                json={"elementId": element_id, "text": text},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Type text in '{element_id}' failed: {e}")
            return False

    def clear_text(self, element_id: str) -> bool:
        """Clear text from a text field"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/clearText",
                json={"elementId": element_id},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Clear text in '{element_id}' failed: {e}")
            return False

    def select_dropdown_item(self, element_id: str, item_id: str) -> bool:
        """Select an item in a dropdown by item ID"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/select",
                json={"elementId": element_id, "itemId": item_id},
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.2)  # Wait for selection to apply
                return True
            return False
        except Exception as e:
            print(f"[ERROR] Select in '{element_id}' failed: {e}")
            return False

    def set_slider_value(self, element_id: str, value: float) -> bool:
        """Set a slider to a specific value"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/slider",
                json={"elementId": element_id, "value": value},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set slider '{element_id}' failed: {e}")
            return False

    def toggle(self, element_id: str, state: bool) -> bool:
        """Set a toggle button state"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/toggle",
                json={"elementId": element_id, "value": state},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Toggle '{element_id}' failed: {e}")
            return False

    def drag(self, from_element_id: str, to_element_id: str) -> bool:
        """Drag from one element to another"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/drag",
                json={"fromElementId": from_element_id, "toElementId": to_element_id},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Drag failed: {e}")
            return False

    def drag_offset(self, element_id: str, delta_x: int, delta_y: int) -> bool:
        """Drag an element by pixel offset"""
        try:
            response = requests.post(
                f"{self.base_url}/ui/dragOffset",
                json={"elementId": element_id, "deltaX": delta_x, "deltaY": delta_y},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Drag offset failed: {e}")
            return False

    # ==================== State Queries ====================

    def get_oscillators(self) -> List[Dict]:
        """Get list of oscillators"""
        try:
            response = requests.get(
                f"{self.base_url}/state/oscillators",
                timeout=self.timeout
            )
            if response.status_code == 200:
                data = response.json().get('data', [])
                # API returns data as a list directly
                if isinstance(data, list):
                    return data
                # Fallback for dict format
                return data.get('oscillators', []) if isinstance(data, dict) else []
            return []
        except Exception as e:
            print(f"[ERROR] Get oscillators failed: {e}")
            return []

    def get_panes(self) -> List[Dict]:
        """Get list of panes"""
        try:
            response = requests.get(
                f"{self.base_url}/state/panes",
                timeout=self.timeout
            )
            if response.status_code == 200:
                data = response.json().get('data', [])
                # API returns data as a list directly
                if isinstance(data, list):
                    return data
                # Fallback for dict format
                return data.get('panes', []) if isinstance(data, dict) else []
            return []
        except Exception as e:
            print(f"[ERROR] Get panes failed: {e}")
            return []

    def get_sources(self) -> List[Dict]:
        """Get list of available sources"""
        try:
            response = requests.get(
                f"{self.base_url}/state/sources",
                timeout=self.timeout
            )
            if response.status_code == 200:
                data = response.json().get('data', [])
                # API returns data as a list directly
                if isinstance(data, list):
                    return data
                # Fallback for dict format
                return data.get('sources', []) if isinstance(data, dict) else []
            return []
        except Exception as e:
            print(f"[ERROR] Get sources failed: {e}")
            return []

    def add_oscillator(self, source_id: str, pane_id: str = None,
                       name: str = "Test Oscillator", color: str = "#00FF00") -> Dict:
        """Add an oscillator via state API (for test setup)"""
        try:
            payload = {
                "sourceId": source_id,
                "name": name,
                "color": color
            }
            if pane_id:
                payload["paneId"] = pane_id

            response = requests.post(
                f"{self.base_url}/state/oscillator/add",
                json=payload,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Add oscillator failed: {e}")
            return {"success": False, "error": str(e)}

    def update_oscillator(self, oscillator_id: str, name: str = None, 
                          source_id: str = None, pane_id: str = None,
                          visible: bool = None, color: str = None,
                          processing_mode: str = None) -> Dict:
        """Update an oscillator via state API"""
        try:
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

            response = requests.post(
                f"{self.base_url}/state/oscillator/update",
                json=payload,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Update oscillator failed: {e}")
            return {"success": False, "error": str(e)}

    def reorder_oscillators(self, order_or_from_index, to_index: int = None) -> Dict:
        """
        Reorder oscillators.
        
        Can be called two ways:
        - reorder_oscillators([id1, id2, id3]) - set new order by ID list
        - reorder_oscillators(from_index, to_index) - move item from one index to another
        """
        try:
            if isinstance(order_or_from_index, list):
                # List of IDs - set new order
                response = requests.post(
                    f"{self.base_url}/state/oscillator/reorder",
                    json={"order": order_or_from_index},
                    timeout=self.timeout
                )
            else:
                # Two indices - move from one to another
                response = requests.post(
                    f"{self.base_url}/state/oscillator/reorder",
                    json={"fromIndex": order_or_from_index, "toIndex": to_index},
                    timeout=self.timeout
                )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Reorder oscillators failed: {e}")
            return {"success": False, "error": str(e)}

    def add_pane(self, name: str = None) -> Dict:
        """Add a new pane via state API"""
        try:
            payload = {}
            if name:
                payload["name"] = name
            
            response = requests.post(
                f"{self.base_url}/state/pane/add",
                json=payload,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Add pane failed: {e}")
            return {"success": False, "error": str(e)}

    def swap_panes(self, from_id_or_index, to_id_or_index) -> bool:
        """
        Swap two panes by ID or index.
        
        If strings are passed, they are treated as pane IDs.
        If ints are passed, they are treated as indices.
        """
        try:
            # If IDs (strings) are passed, convert to indices
            if isinstance(from_id_or_index, str) or isinstance(to_id_or_index, str):
                panes = self.get_panes()
                from_index = -1
                to_index = -1
                
                for i, pane in enumerate(panes):
                    pane_id = pane.get("id", "")
                    if pane_id == from_id_or_index:
                        from_index = i
                    if pane_id == to_id_or_index:
                        to_index = i
                
                if from_index < 0 or to_index < 0:
                    print(f"[ERROR] Pane not found: {from_id_or_index} or {to_id_or_index}")
                    return False
            else:
                from_index = from_id_or_index
                to_index = to_id_or_index
            
            response = requests.post(
                f"{self.base_url}/state/pane/swap",
                json={"fromIndex": from_index, "toIndex": to_index},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Swap panes failed: {e}")
            return False

    def set_gpu_enabled(self, enabled: bool) -> bool:
        """Enable or disable GPU rendering"""
        try:
            response = requests.post(
                f"{self.base_url}/state/gpu",
                json={"enabled": enabled},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set GPU enabled failed: {e}")
            return False

    def set_layout(self, columns: int) -> bool:
        """Set column layout (1, 2, or 3)"""
        try:
            response = requests.post(
                f"{self.base_url}/state/layout",
                json={"columns": columns},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set layout failed: {e}")
            return False

    def set_theme(self, theme_name: str) -> bool:
        """Set the current theme"""
        try:
            response = requests.post(
                f"{self.base_url}/state/theme",
                json={"theme": theme_name},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set theme failed: {e}")
            return False

    def set_show_grid(self, show: bool) -> bool:
        """Enable or disable grid display"""
        try:
            response = requests.post(
                f"{self.base_url}/state/grid",
                json={"show": show},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set show grid failed: {e}")
            return False

    def get_display_settings(self) -> Dict:
        """Get current display settings"""
        try:
            response = requests.get(
                f"{self.base_url}/state/display",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {}
        except Exception as e:
            print(f"[ERROR] Get display settings failed: {e}")
            return {}

    def get_timing_settings(self) -> Dict:
        """Get current timing settings"""
        try:
            response = requests.get(
                f"{self.base_url}/state/timing",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {}
        except Exception as e:
            print(f"[ERROR] Get timing settings failed: {e}")
            return {}

    def set_timing_settings(self, **kwargs) -> Dict:
        """Update timing settings"""
        try:
            response = requests.post(
                f"{self.base_url}/state/timing",
                json=kwargs,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Set timing settings failed: {e}")
            return {"success": False, "error": str(e)}

    def save_state(self, path: str) -> Dict:
        """Save current state to a file"""
        try:
            response = requests.post(
                f"{self.base_url}/state/save",
                json={"path": path},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Save state failed: {e}")
            return {"success": False, "error": str(e)}

    def load_state(self, path: str) -> Dict:
        """Load state from a file"""
        try:
            response = requests.post(
                f"{self.base_url}/state/load",
                json={"path": path},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json()
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Load state failed: {e}")
            return {"success": False, "error": str(e)}

    # ==================== Metrics Control ====================

    def start_metrics(self) -> Dict:
        """Start metrics collection"""
        try:
            response = requests.post(
                f"{self.base_url}/metrics/start",
                json={},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return {"success": True}
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Start metrics failed: {e}")
            return {"success": False, "error": str(e)}

    def stop_metrics(self) -> Dict:
        """Stop metrics collection"""
        try:
            response = requests.post(
                f"{self.base_url}/metrics/stop",
                json={},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return {"success": True}
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Stop metrics failed: {e}")
            return {"success": False, "error": str(e)}

    def get_metrics(self) -> Dict:
        """Get current metrics"""
        try:
            response = requests.get(
                f"{self.base_url}/metrics/current",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {}
        except Exception as e:
            print(f"[ERROR] Get metrics failed: {e}")
            return {}

    def get_current_metrics(self) -> Dict:
        """Alias for get_metrics"""
        return self.get_metrics()

    def get_metrics_stats(self) -> Dict:
        """Get metrics statistics"""
        try:
            response = requests.get(
                f"{self.base_url}/metrics/stats",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {}
        except Exception as e:
            print(f"[ERROR] Get metrics stats failed: {e}")
            return {}

    def reset_metrics(self) -> Dict:
        """Reset metrics"""
        try:
            response = requests.post(
                f"{self.base_url}/metrics/reset",
                json={},
                timeout=self.timeout
            )
            if response.status_code == 200:
                return {"success": True}
            return {"success": False, "error": f"Status code {response.status_code}"}
        except Exception as e:
            print(f"[ERROR] Reset metrics failed: {e}")
            return {"success": False, "error": str(e)}

    # Aliases for compatibility
    def metrics_start(self) -> Dict:
        """Alias for start_metrics"""
        return self.start_metrics()

    def metrics_stop(self) -> Dict:
        """Alias for stop_metrics"""
        return self.stop_metrics()

    def metrics_current(self) -> Dict:
        """Alias for get_metrics"""
        return self.get_metrics()

    def metrics_stats(self) -> Dict:
        """Alias for get_metrics_stats"""
        return self.get_metrics_stats()

    def metrics_reset(self) -> Dict:
        """Alias for reset_metrics"""
        return self.reset_metrics()

    # ==================== Transport Control ====================

    def transport_play(self) -> bool:
        """Start DAW playback"""
        try:
            response = requests.post(
                f"{self.base_url}/transport/play",
                json={},
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.1)  # Brief delay for state to update
                return True
            return False
        except Exception as e:
            print(f"[ERROR] Transport play failed: {e}")
            return False

    def transport_stop(self) -> bool:
        """Stop DAW playback"""
        try:
            response = requests.post(
                f"{self.base_url}/transport/stop",
                json={},
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.1)
                return True
            return False
        except Exception as e:
            print(f"[ERROR] Transport stop failed: {e}")
            return False

    def set_bpm(self, bpm: float) -> bool:
        """Set DAW tempo in BPM"""
        try:
            response = requests.post(
                f"{self.base_url}/transport/setBpm",
                json={"bpm": bpm},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set BPM failed: {e}")
            return False

    def get_transport_state(self) -> Optional[Dict]:
        """Get current transport state (playing, bpm, position)"""
        try:
            response = requests.get(
                f"{self.base_url}/transport/state",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return None
        except Exception as e:
            print(f"[ERROR] Get transport state failed: {e}")
            return None

    def is_playing(self) -> bool:
        """Check if DAW is currently playing"""
        state = self.get_transport_state()
        return state.get('playing', False) if state else False

    def get_bpm(self) -> float:
        """Get current BPM"""
        state = self.get_transport_state()
        return state.get('bpm', 120.0) if state else 120.0

    def set_position(self, samples: int) -> bool:
        """Set transport position in samples"""
        try:
            response = requests.post(
                f"{self.base_url}/transport/setPosition",
                json={"samples": samples},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set position failed: {e}")
            return False

    # ==================== Track Audio Control ====================

    def set_track_audio(self, track_id: int, waveform: str = "sine",
                        frequency: float = 440.0, amplitude: float = 0.8) -> bool:
        """Set audio generator settings for a track"""
        try:
            response = requests.post(
                f"{self.base_url}/track/{track_id}/audio",
                json={
                    "waveform": waveform,
                    "frequency": frequency,
                    "amplitude": amplitude
                },
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set track audio failed: {e}")
            return False

    def get_track_info(self, track_id: int) -> Optional[Dict]:
        """Get track information including audio generator state"""
        try:
            response = requests.get(
                f"{self.base_url}/track/{track_id}/info",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return None
        except Exception as e:
            print(f"[ERROR] Get track info failed: {e}")
            return None

    def set_track_burst(self, track_id: int, samples: int, waveform: str = None) -> bool:
        """Set track to burst mode (generate N samples then silence)"""
        try:
            payload = {"samples": samples}
            if waveform:
                payload["waveform"] = waveform
            response = requests.post(
                f"{self.base_url}/track/{track_id}/burst",
                json=payload,
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Set track burst failed: {e}")
            return False

    # ==================== Waveform Analysis ====================

    def verify_waveform(self, element_id: str, expected_type: str = None,
                        min_amplitude: float = None) -> Dict:
        """
        Verify waveform properties in a display element.
        Returns dict with 'pass' boolean and analysis results.
        """
        try:
            payload = {"elementId": element_id}
            if expected_type:
                payload["expectedType"] = expected_type
            if min_amplitude is not None:
                payload["minAmplitude"] = min_amplitude

            response = requests.post(
                f"{self.base_url}/verify/waveform",
                json=payload,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {"pass": False, "error": "Request failed"}
        except Exception as e:
            print(f"[ERROR] Verify waveform failed: {e}")
            return {"pass": False, "error": str(e)}

    def analyze_waveform(self, element_id: str = None, background_color: str = None) -> Dict:
        """Analyze waveform in display and return metrics"""
        try:
            params = {}
            if element_id:
                params["elementId"] = element_id
            if background_color:
                params["backgroundColor"] = background_color

            response = requests.get(
                f"{self.base_url}/analyze/waveform",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {})
            return {}
        except Exception as e:
            print(f"[ERROR] Analyze waveform failed: {e}")
            return {}

    # ==================== Screenshots & Verification ====================

    def take_screenshot(self, path: str, element_id: str = None) -> bool:
        """Take a screenshot"""
        try:
            payload = {"path": path}
            if element_id:
                payload["elementId"] = element_id

            response = requests.post(
                f"{self.base_url}/screenshot",
                json=payload,
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception as e:
            print(f"[ERROR] Screenshot failed: {e}")
            return False

    def verify_color(self, element_id: str, color: str, tolerance: int = 10) -> bool:
        """Verify an element contains the expected color"""
        try:
            response = requests.post(
                f"{self.base_url}/verify/color",
                json={
                    "elementId": element_id,
                    "color": color,
                    "tolerance": tolerance,
                    "mode": "contains"
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.json().get('data', {}).get('pass', False)
            return False
        except Exception as e:
            print(f"[ERROR] Verify color failed: {e}")
            return False

    # ==================== Visual Comparison Helpers ====================

    def save_baseline(self, name: str, element_id: str = None) -> bool:
        """
        Save a reference screenshot as a baseline for comparison.
        
        Args:
            name: Unique name for this baseline (e.g., "gain_default")
            element_id: Optional element to capture (full window if None)
        
        Returns:
            True if baseline saved successfully
        """
        import os
        baseline_dir = os.path.join(os.path.dirname(__file__), "baselines")
        os.makedirs(baseline_dir, exist_ok=True)
        path = os.path.join(baseline_dir, f"{name}.png")
        return self.take_screenshot(path, element_id)

    def compare_to_baseline(self, name: str, element_id: str = None, 
                            threshold: float = 0.95) -> Dict:
        """
        Compare current state to a saved baseline screenshot.
        
        Args:
            name: Name of the baseline to compare against
            element_id: Optional element to capture
            threshold: Similarity threshold (0.0-1.0, default 0.95 = 95% similar)
        
        Returns:
            Dict with 'match' (bool), 'similarity' (float), 'diff_path' (str if different)
        """
        import os
        baseline_dir = os.path.join(os.path.dirname(__file__), "baselines")
        baseline_path = os.path.join(baseline_dir, f"{name}.png")
        
        if not os.path.exists(baseline_path):
            print(f"[WARN] Baseline '{name}' not found, saving new baseline")
            self.save_baseline(name, element_id)
            return {"match": True, "similarity": 1.0, "new_baseline": True}
        
        try:
            response = requests.post(
                f"{self.base_url}/screenshot/compare",
                json={
                    "baselinePath": baseline_path,
                    "elementId": element_id,
                    "threshold": threshold
                },
                timeout=self.timeout
            )
            if response.status_code == 200:
                data = response.json().get('data', {})
                return {
                    "match": data.get('match', False),
                    "similarity": data.get('similarity', 0.0),
                    "diff_path": data.get('diffPath')
                }
            return {"match": False, "similarity": 0.0, "error": "Request failed"}
        except Exception as e:
            print(f"[ERROR] Compare to baseline failed: {e}")
            return {"match": False, "similarity": 0.0, "error": str(e)}

    def assert_waveform_visible(self, element_id: str, min_amplitude: float = 0.05,
                                 message: str = None) -> bool:
        """
        Assert that a waveform is being rendered in the specified element.
        
        Args:
            element_id: The pane or waveform element to check
            min_amplitude: Minimum amplitude threshold to consider waveform present
            message: Optional custom message
        
        Returns:
            True if waveform detected
        """
        result = self.verify_waveform(element_id, min_amplitude=min_amplitude)
        waveform_present = result.get('pass', False)
        amplitude = result.get('amplitude', 0.0)
        
        msg = message or f"Waveform should be visible in '{element_id}' (amplitude={amplitude:.3f})"
        self._record_result(msg, TestResult.PASSED if waveform_present else TestResult.FAILED)
        
        if not waveform_present:
            self._fail_assertion(msg)
        return waveform_present

    def assert_waveform_color(self, element_id: str, expected_color: str, 
                               tolerance: int = 30, message: str = None) -> bool:
        """
        Assert that the waveform in an element has the expected color.
        
        Args:
            element_id: The pane or waveform element
            expected_color: Hex color code (e.g., "#FF0000")
            tolerance: Color matching tolerance (0-255)
            message: Optional custom message
        
        Returns:
            True if color matches
        """
        color_matches = self.verify_color(element_id, expected_color, tolerance)
        msg = message or f"Waveform color should be {expected_color} in '{element_id}'"
        self._record_result(msg, TestResult.PASSED if color_matches else TestResult.FAILED)
        
        if not color_matches:
            self._fail_assertion(msg)
        return color_matches

    def assert_visual_difference(self, baseline_name: str, element_id: str = None,
                                  min_difference: float = 0.05, message: str = None) -> bool:
        """
        Assert that the current visual is different from a baseline.
        Useful for verifying a change actually had a visual effect.
        
        Args:
            baseline_name: Name of baseline to compare against
            element_id: Optional element to compare
            min_difference: Minimum difference required (1.0 - similarity)
            message: Optional custom message
        
        Returns:
            True if visually different enough
        """
        result = self.compare_to_baseline(baseline_name, element_id)
        similarity = result.get('similarity', 1.0)
        difference = 1.0 - similarity
        is_different = difference >= min_difference
        
        msg = message or f"Visual should differ from baseline '{baseline_name}' (diff={difference:.1%})"
        self._record_result(msg, TestResult.PASSED if is_different else TestResult.FAILED)
        
        if not is_different:
            self._fail_assertion(msg)
        return is_different

    # ==================== State Query Extensions ====================

    def get_slider_value(self, element_id: str) -> Optional[float]:
        """
        Get the current value of a slider element.
        
        Args:
            element_id: Test ID of the slider
        
        Returns:
            Current slider value or None if not found
        """
        elem = self.get_element(element_id)
        if elem and elem.extra:
            return elem.extra.get('value')
        return None

    def get_toggle_state(self, element_id: str) -> Optional[bool]:
        """
        Get the current state of a toggle element.
        
        Args:
            element_id: Test ID of the toggle
        
        Returns:
            True if on, False if off, None if not found
        """
        elem = self.get_element(element_id)
        if elem and elem.extra:
            return elem.extra.get('toggled', elem.extra.get('value', None))
        return None

    def get_dropdown_selection(self, element_id: str) -> Optional[str]:
        """
        Get the currently selected item ID in a dropdown.
        
        Args:
            element_id: Test ID of the dropdown
        
        Returns:
            Selected item ID or None if not found
        """
        elem = self.get_element(element_id)
        if elem and elem.extra:
            return elem.extra.get('selectedId', elem.extra.get('selectedItemId'))
        return None

    def get_dropdown_text(self, element_id: str) -> Optional[str]:
        """
        Get the currently displayed text in a dropdown.
        
        Args:
            element_id: Test ID of the dropdown
        
        Returns:
            Selected item text or None if not found
        """
        elem = self.get_element(element_id)
        if elem and elem.extra:
            return elem.extra.get('selectedText', elem.extra.get('text'))
        return None

    def get_text_field_value(self, element_id: str) -> Optional[str]:
        """
        Get the current text in a text field.
        
        Args:
            element_id: Test ID of the text field
        
        Returns:
            Text content or None if not found
        """
        elem = self.get_element(element_id)
        if elem and elem.extra:
            return elem.extra.get('text', elem.extra.get('value'))
        return None

    def get_pane_count(self) -> int:
        """Get the current number of panes"""
        panes = self.get_panes()
        return len(panes)

    def assert_slider_value(self, element_id: str, expected: float, 
                            tolerance: float = 0.01, message: str = None) -> bool:
        """
        Assert a slider has the expected value.
        
        Args:
            element_id: Test ID of the slider
            expected: Expected value
            tolerance: Acceptable difference from expected
            message: Optional custom message
        
        Returns:
            True if value matches within tolerance
        """
        actual = self.get_slider_value(element_id)
        if actual is None:
            msg = message or f"Slider '{element_id}' not found or has no value"
            self._record_result(msg, TestResult.FAILED)
            self._fail_assertion(msg)
            return False
        
        matches = abs(actual - expected) <= tolerance
        msg = message or f"Slider '{element_id}' should be {expected} (got {actual})"
        self._record_result(msg, TestResult.PASSED if matches else TestResult.FAILED)
        
        if not matches:
            self._fail_assertion(msg)
        return matches

    def assert_toggle_state(self, element_id: str, expected: bool, 
                            message: str = None) -> bool:
        """
        Assert a toggle has the expected state.
        
        Args:
            element_id: Test ID of the toggle
            expected: Expected state (True = on, False = off)
            message: Optional custom message
        
        Returns:
            True if state matches
        """
        actual = self.get_toggle_state(element_id)
        if actual is None:
            msg = message or f"Toggle '{element_id}' not found or has no state"
            self._record_result(msg, TestResult.FAILED)
            self._fail_assertion(msg)
            return False
        
        matches = actual == expected
        state_str = "ON" if expected else "OFF"
        actual_str = "ON" if actual else "OFF"
        msg = message or f"Toggle '{element_id}' should be {state_str} (got {actual_str})"
        self._record_result(msg, TestResult.PASSED if matches else TestResult.FAILED)
        
        if not matches:
            self._fail_assertion(msg)
        return matches

    def assert_pane_count(self, expected: int, message: str = None) -> bool:
        """Assert the number of panes equals expected"""
        actual = self.get_pane_count()
        matches = actual == expected
        msg = message or f"Should have {expected} panes (got {actual})"
        self._record_result(msg, TestResult.PASSED if matches else TestResult.FAILED)
        
        if not matches:
            self._fail_assertion(msg)
        return matches

    # ==================== Keyboard Interactions ====================

    def press_key(self, key: str, element_id: str = None) -> bool:
        """
        Press a key, optionally on a specific element.
        
        Args:
            key: Key name (e.g., "escape", "enter", "v", "delete")
            element_id: Optional element to focus first
        
        Returns:
            True if key press succeeded
        """
        try:
            payload = {"key": key}
            if element_id:
                payload["elementId"] = element_id
            
            response = requests.post(
                f"{self.base_url}/ui/keyPress",
                json=payload,
                timeout=self.timeout
            )
            if response.status_code == 200:
                time.sleep(0.1)
                return True
            return False
        except Exception as e:
            print(f"[ERROR] Press key '{key}' failed: {e}")
            return False

    def press_escape(self, element_id: str = None) -> bool:
        """Press the Escape key"""
        return self.press_key("escape", element_id)

    def press_enter(self, element_id: str = None) -> bool:
        """Press the Enter key"""
        return self.press_key("enter", element_id)

    def press_delete(self, element_id: str = None) -> bool:
        """Press the Delete/Backspace key"""
        return self.press_key("delete", element_id)

    def press_tab(self, element_id: str = None) -> bool:
        """Press the Tab key"""
        return self.press_key("tab", element_id)

    # ==================== Debug Logs ====================

    def clear_logs(self) -> bool:
        """Clear the debug log file"""
        try:
            response = requests.post(
                f"{self.base_url}/debug/logs/clear",
                json={},
                timeout=self.timeout
            )
            return response.status_code == 200
        except Exception:
            # Fallback: clear directly
            import os
            log_path = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"
            if os.path.exists(log_path):
                with open(log_path, "w") as f:
                    f.write("")
            return True

    def get_logs(self) -> List[str]:
        """Get debug log contents"""
        try:
            response = requests.get(
                f"{self.base_url}/debug/logs",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return response.text.strip().split("\n")
            return []
        except Exception:
            # Fallback: read directly
            import os
            log_path = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"
            if os.path.exists(log_path):
                with open(log_path, "r") as f:
                    return f.read().strip().split("\n")
            return []

    def wait_for_log(self, pattern: str, timeout_ms: int = 5000) -> Optional[str]:
        """Wait for a log entry matching the pattern"""
        try:
            response = requests.post(
                f"{self.base_url}/debug/logs/wait",
                json={"pattern": pattern, "timeoutMs": timeout_ms},
                timeout=(timeout_ms / 1000) + 2
            )
            if response.status_code == 200:
                data = response.json().get('data', {})
                if data.get('found'):
                    return data.get('matchedLine')
            return None
        except Exception as e:
            print(f"[ERROR] Wait for log failed: {e}")
            return None

    # ==================== Test Assertions ====================

    def _fail_assertion(self, message: str):
        """Handle assertion failure based on strict_mode"""
        if self.strict_mode:
            raise AssertionError(message)

    def assert_element_exists(self, element_id: str, message: str = None) -> bool:
        """Assert that an element exists. Raises AssertionError in strict mode."""
        exists = self.element_exists(element_id)
        msg = message or f"Element '{element_id}' should exist"
        self._record_result(msg, TestResult.PASSED if exists else TestResult.FAILED)
        if not exists:
            self._fail_assertion(msg)
        return exists

    def assert_element_visible(self, element_id: str, message: str = None) -> bool:
        """Assert that an element is visible. Raises AssertionError in strict mode."""
        visible = self.element_visible(element_id)
        msg = message or f"Element '{element_id}' should be visible"
        self._record_result(msg, TestResult.PASSED if visible else TestResult.FAILED)
        if not visible:
            self._fail_assertion(msg)
        return visible

    def assert_element_not_visible(self, element_id: str, message: str = None) -> bool:
        """Assert that an element is not visible. Raises AssertionError in strict mode."""
        elem = self.get_element(element_id)
        not_visible = elem is None or not elem.visible or not elem.showing
        msg = message or f"Element '{element_id}' should not be visible"
        self._record_result(msg, TestResult.PASSED if not_visible else TestResult.FAILED)
        if not not_visible:
            self._fail_assertion(msg)
        return not_visible

    def assert_oscillator_count(self, expected: int, message: str = None) -> bool:
        """Assert the number of oscillators. Raises AssertionError in strict mode."""
        oscillators = self.get_oscillators()
        actual = len(oscillators)
        success = actual == expected
        msg = message or f"Should have {expected} oscillators (got {actual})"
        self._record_result(msg, TestResult.PASSED if success else TestResult.FAILED)
        if not success:
            self._fail_assertion(msg)
        return success

    def assert_true(self, condition: bool, message: str) -> bool:
        """Assert a condition is true. Raises AssertionError in strict mode."""
        self._record_result(message, TestResult.PASSED if condition else TestResult.FAILED)
        if not condition:
            self._fail_assertion(message)
        return condition

    def skip(self, message: str):
        """Skip a test with a message"""
        self._record_result(message, TestResult.SKIPPED)

    def require_element(self, element_id: str, test_name: str = "") -> bool:
        """
        Check if an element exists, skip test if not.
        Returns True if element exists, False if skipped.
        """
        if not self.element_exists(element_id):
            msg = f"[SKIP] {test_name}: Element '{element_id}' not registered in test harness"
            print(msg)
            self._record_result(f"Skipped: {element_id} not found", TestResult.SKIPPED)
            return False
        return True

    def require_any_element(self, element_ids: list, test_name: str = "") -> str:
        """
        Check if any of the elements exist, skip test if none found.
        Returns the first found element_id or None if skipped.
        """
        for eid in element_ids:
            if self.element_exists(eid):
                return eid
        msg = f"[SKIP] {test_name}: None of {element_ids} found in test harness"
        print(msg)
        self._record_result(f"Skipped: none of {element_ids} found", TestResult.SKIPPED)
        return None

    def _record_result(self, message: str, result: TestResult):
        """Record a test result"""
        self._test_results.append((message, result, ""))
        if result == TestResult.PASSED:
            symbol = "[PASS]"
        elif result == TestResult.FAILED:
            symbol = "[FAIL]"
        else:
            symbol = "[SKIP]"
        print(f"  {symbol} {message}")

    def get_results_summary(self) -> Tuple[int, int, int]:
        """Get summary of test results: (passed, failed, skipped)"""
        passed = sum(1 for _, r, _ in self._test_results if r == TestResult.PASSED)
        failed = sum(1 for _, r, _ in self._test_results if r == TestResult.FAILED)
        skipped = sum(1 for _, r, _ in self._test_results if r == TestResult.SKIPPED)
        return passed, failed, skipped

    def print_summary(self):
        """Print test results summary"""
        passed, failed, skipped = self.get_results_summary()
        total = passed + failed + skipped
        print(f"\n{'='*50}")
        print(f"TEST SUMMARY: {passed}/{total} passed, {failed} failed, {skipped} skipped")
        if failed > 0:
            print("FAILED TESTS:")
            for msg, result, _ in self._test_results:
                if result == TestResult.FAILED:
                    print(f"  - {msg}")
        print(f"{'='*50}")
        return failed == 0


class OscilTestRunner:
    """Test runner for organizing and executing E2E tests"""

    def __init__(self, client: OscilTestClient = None):
        self.client = client or OscilTestClient()
        self._tests: List[Tuple[str, callable]] = []
        self._setup: callable = None
        self._teardown: callable = None

    def setup(self, func: callable):
        """Register setup function"""
        self._setup = func
        return func

    def teardown(self, func: callable):
        """Register teardown function"""
        self._teardown = func
        return func

    def test(self, name: str):
        """Decorator to register a test function"""
        def decorator(func: callable):
            self._tests.append((name, func))
            return func
        return decorator

    def run(self) -> bool:
        """Run all registered tests"""
        if not self.client.wait_for_harness():
            return False

        print(f"\nRunning {len(self._tests)} tests...")

        all_passed = True
        for name, func in self._tests:
            print(f"\n--- {name} ---")
            try:
                if self._setup:
                    self._setup(self.client)

                func(self.client)

                if self._teardown:
                    self._teardown(self.client)
            except Exception as e:
                print(f"[ERROR] Test '{name}' threw exception: {e}")
                all_passed = False

        return self.client.print_summary()


# Convenience functions for running standalone tests
def run_test(test_func: callable, setup_func: callable = None, teardown_func: callable = None) -> bool:
    """Run a single test function"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        return False

    try:
        if setup_func:
            setup_func(client)

        test_func(client)

        if teardown_func:
            teardown_func(client)
    except Exception as e:
        print(f"[ERROR] Test threw exception: {e}")
        return False

    return client.print_summary()
