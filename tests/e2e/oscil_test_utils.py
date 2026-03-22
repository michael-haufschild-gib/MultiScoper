"""
Oscil Test Harness Client

HTTP client for driving the Oscil test harness. All UI automation flows through
the C++ TestHttpServer running on localhost:8765. This module provides:

  - OscilTestClient: stateless HTTP client for element queries, interactions,
    state management, transport control, and condition-based waits.
  - No test assertions live here -- assertions belong in pytest test files.
"""

import time
import requests
from typing import Optional, Dict, Any, List
from dataclasses import dataclass, field


@dataclass
class ElementInfo:
    """Snapshot of a UI element's state from the test harness registry."""
    element_id: str
    visible: bool
    enabled: bool
    bounds: Dict[str, int]
    showing: bool = False
    focused: bool = False
    extra: Dict[str, Any] = field(default_factory=dict)

    @property
    def width(self) -> int:
        return self.bounds.get("width", 0)

    @property
    def height(self) -> int:
        return self.bounds.get("height", 0)

    @property
    def is_interactive(self) -> bool:
        return self.visible and self.showing and self.enabled

    @classmethod
    def from_response(cls, element_id: str, data: Dict) -> "ElementInfo":
        return cls(
            element_id=element_id,
            visible=data.get("visible", False),
            enabled=data.get("enabled", True),
            bounds=data.get("bounds", {}),
            showing=data.get("showing", False),
            focused=data.get("focused", False),
            extra=data,
        )


class HarnessConnectionError(Exception):
    """Raised when the test harness is unreachable."""


class ElementNotFoundError(Exception):
    """Raised when a required element is not registered."""


class OscilTestClient:
    """HTTP client for the Oscil test harness."""

    DEFAULT_URL = "http://localhost:8765"
    DEFAULT_TIMEOUT = 10.0

    def __init__(self, base_url: str = DEFAULT_URL):
        self.base_url = base_url
        self.timeout = self.DEFAULT_TIMEOUT

    # ── HTTP primitives ─────────────────────────────────────────────

    def _get(self, path: str, **kwargs) -> requests.Response:
        return requests.get(
            f"{self.base_url}{path}", timeout=self.timeout, **kwargs
        )

    def _post(self, path: str, payload: Dict = None) -> requests.Response:
        return requests.post(
            f"{self.base_url}{path}",
            json=payload or {},
            timeout=self.timeout,
        )

    def _get_json(self, path: str, **kwargs) -> Optional[Dict]:
        try:
            r = self._get(path, **kwargs)
        except requests.exceptions.ConnectionError:
            return None
        if r.status_code == 200:
            return r.json()
        return None

    def _post_json(self, path: str, payload: Dict = None) -> Optional[Dict]:
        try:
            r = self._post(path, payload)
        except requests.exceptions.ConnectionError:
            return None
        if r.status_code == 200:
            return r.json()
        return None

    def _post_ok(self, path: str, payload: Dict = None) -> bool:
        """POST and return True only if response has success=true."""
        resp = self._post_json(path, payload)
        return resp is not None and resp.get("success", False)

    # ── Connection ──────────────────────────────────────────────────

    def wait_for_harness(self, max_retries: int = 15, delay: float = 1.0) -> None:
        """Block until the harness /health endpoint responds. Raises on timeout."""
        for _ in range(max_retries):
            try:
                r = requests.get(f"{self.base_url}/health", timeout=2.0)
                if r.status_code == 200:
                    return
            except requests.exceptions.ConnectionError:
                pass
            time.sleep(delay)
        raise HarnessConnectionError(
            f"Test harness unreachable at {self.base_url} after {max_retries} attempts"
        )

    def health_check(self) -> Dict:
        """Raw health check. Returns full response dict or raises on failure."""
        r = self._get("/health")
        if r.status_code != 200:
            raise RuntimeError(f"Health check failed with status {r.status_code}")
        return r.json()

    # ── Condition-based waits ───────────────────────────────────────

    def wait_until(
        self, predicate, *, timeout_s: float = 5.0, poll_s: float = 0.15, desc: str = ""
    ) -> bool:
        """
        Poll ``predicate()`` until it returns a truthy value or timeout expires.
        Returns the truthy value on success; raises TimeoutError on failure.
        """
        deadline = time.monotonic() + timeout_s
        last_result = None
        while time.monotonic() < deadline:
            last_result = predicate()
            if last_result:
                return last_result
            time.sleep(poll_s)
        msg = desc or "Condition not met"
        raise TimeoutError(f"{msg} (waited {timeout_s}s, last={last_result})")

    def wait_for_element(self, element_id: str, timeout_s: float = 5.0) -> ElementInfo:
        """Wait for an element to be registered. Returns its ElementInfo."""
        def check():
            return self.get_element(element_id)
        return self.wait_until(check, timeout_s=timeout_s, desc=f"element '{element_id}' to exist")

    def wait_for_visible(self, element_id: str, timeout_s: float = 5.0) -> ElementInfo:
        """Wait for an element to be visible+showing."""
        def check():
            el = self.get_element(element_id)
            return el if (el and el.visible and el.showing) else None
        return self.wait_until(check, timeout_s=timeout_s, desc=f"element '{element_id}' to be visible")

    def wait_for_not_visible(self, element_id: str, timeout_s: float = 5.0) -> bool:
        """Wait for an element to disappear or become invisible."""
        def check():
            el = self.get_element(element_id)
            return el is None or not el.visible or not el.showing
        return self.wait_until(check, timeout_s=timeout_s, desc=f"element '{element_id}' to disappear")

    def wait_for_oscillator_count(self, expected: int, timeout_s: float = 5.0) -> List[Dict]:
        """Wait until oscillator list reaches expected count."""
        def check():
            oscs = self.get_oscillators()
            return oscs if len(oscs) == expected else None
        return self.wait_until(
            check, timeout_s=timeout_s,
            desc=f"oscillator count to be {expected}"
        )

    # ── Editor lifecycle ────────────────────────────────────────────

    def open_editor(self, track_id: int = 0, wait_timeout_s: float = 10.0) -> None:
        """
        Open the plugin editor and block until UI elements are registered.
        Raises on failure.
        """
        r = self._post(f"/track/{track_id}/showEditor")
        resp = r.json()
        if r.status_code != 200 or not resp.get("success"):
            raise RuntimeError(f"Failed to open editor: {resp.get('error', 'unknown')}")

        data = resp.get("data", {})
        if data.get("alreadyOpen") and data.get("elementsRegistered", 0) > 0:
            return

        # Poll for elements to appear
        self.wait_until(
            lambda: self._get_json("/ui/elements") and
                    self._get_json("/ui/elements").get("data", {}).get("count", 0) > 0,
            timeout_s=wait_timeout_s,
            desc="editor UI elements to register",
        )

    def close_editor(self, track_id: int = 0) -> None:
        self._post(f"/track/{track_id}/hideEditor")

    def verify_editor_ready(self) -> int:
        """Return the number of registered UI elements, or 0 if not ready."""
        resp = self._get_json("/ui/elements")
        if resp:
            return resp.get("data", {}).get("count", 0)
        return 0

    # ── State management ────────────────────────────────────────────

    def reset_state(self) -> None:
        """Reset plugin to default state (remove all oscillators and panes)."""
        self._post_ok("/state/reset")

    def save_state(self, path: str = "/tmp/state.xml") -> bool:
        return self._post_ok("/state/save", {"path": path})

    def load_state(self, path: str) -> bool:
        return self._post_ok("/state/load", {"path": path})

    # ── Element queries ─────────────────────────────────────────────

    def get_element(self, element_id: str) -> Optional[ElementInfo]:
        """Query element info. Returns None if element not registered."""
        try:
            r = self._get(f"/ui/element/{element_id}")
        except requests.exceptions.ConnectionError:
            return None
        if r.status_code == 404:
            return None
        if r.status_code == 200:
            resp = r.json()
            if not resp.get("success"):
                return None
            return ElementInfo.from_response(element_id, resp.get("data", {}))
        return None

    def element_exists(self, element_id: str) -> bool:
        return self.get_element(element_id) is not None

    def element_visible(self, element_id: str) -> bool:
        el = self.get_element(element_id)
        return el is not None and el.visible and el.showing

    # ── UI interactions ─────────────────────────────────────────────

    def click(self, element_id: str) -> bool:
        return self._post_ok("/ui/click", {"elementId": element_id})

    def double_click(self, element_id: str) -> bool:
        return self._post_ok("/ui/doubleClick", {"elementId": element_id})

    def right_click(self, element_id: str) -> bool:
        return self._post_ok("/ui/rightClick", {"elementId": element_id})

    def hover(self, element_id: str, duration_ms: int = 500) -> bool:
        return self._post_ok("/ui/hover", {"elementId": element_id, "durationMs": duration_ms})

    def type_text(self, element_id: str, text: str) -> bool:
        return self._post_ok("/ui/typeText", {"elementId": element_id, "text": text})

    def clear_text(self, element_id: str) -> bool:
        return self._post_ok("/ui/clearText", {"elementId": element_id})

    def select_dropdown_item(self, element_id: str, item_id: str) -> bool:
        return self._post_ok("/ui/select", {"elementId": element_id, "itemId": item_id})

    def select_dropdown_index(self, element_id: str, index: int) -> bool:
        return self._post_ok("/ui/select", {"elementId": element_id, "itemId": index})

    def set_slider(self, element_id: str, value: float) -> bool:
        return self._post_ok("/ui/slider", {"elementId": element_id, "value": value})

    def increment_slider(self, element_id: str) -> Optional[Dict]:
        return self._post_json("/ui/slider/increment", {"elementId": element_id})

    def decrement_slider(self, element_id: str) -> Optional[Dict]:
        return self._post_json("/ui/slider/decrement", {"elementId": element_id})

    def reset_slider(self, element_id: str) -> Optional[Dict]:
        return self._post_json("/ui/slider/reset", {"elementId": element_id})

    def toggle(self, element_id: str, state: bool) -> bool:
        return self._post_ok("/ui/toggle", {"elementId": element_id, "value": state})

    def drag(self, from_id: str, to_id: str) -> bool:
        return self._post_ok("/ui/drag", {"from": from_id, "to": to_id})

    def drag_offset(self, element_id: str, dx: int, dy: int) -> bool:
        return self._post_ok("/ui/dragOffset", {"elementId": element_id, "deltaX": dx, "deltaY": dy})

    def scroll(self, element_id: str, delta_y: float, delta_x: float = 0.0) -> Optional[Dict]:
        resp = self._post_json("/ui/scroll", {"elementId": element_id, "deltaY": delta_y, "deltaX": delta_x})
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    def key_press(self, key: str) -> bool:
        return self._post_ok("/ui/keyPress", {"key": key})

    # ── Oscillator state ────────────────────────────────────────────

    def get_oscillators(self) -> List[Dict]:
        resp = self._get_json("/state/oscillators")
        if resp and resp.get("success"):
            data = resp.get("data", [])
            return data if isinstance(data, list) else []
        return []

    def get_oscillator_by_id(self, osc_id: str) -> Optional[Dict]:
        for osc in self.get_oscillators():
            if osc.get("id") == osc_id:
                return osc
        return None

    def add_oscillator(
        self,
        source_id: str,
        *,
        name: str = "Test Oscillator",
        colour: str = "",
        pane_id: str = "",
        mode: str = "FullStereo",
    ) -> Optional[str]:
        """Add oscillator via state API. Returns oscillator ID or None."""
        payload: Dict[str, Any] = {"sourceId": source_id, "name": name, "mode": mode}
        if colour:
            payload["colour"] = colour
        if pane_id:
            payload["paneId"] = pane_id
        resp = self._post_json("/state/oscillator/add", payload)
        if resp and resp.get("success"):
            data = resp.get("data", {})
            return data.get("id")
        return None

    def update_oscillator(self, osc_id: str, **fields) -> bool:
        payload = {"id": osc_id, **fields}
        return self._post_ok("/state/oscillator/update", payload)

    def delete_oscillator(self, osc_id: str) -> bool:
        """Delete an oscillator by ID via state API."""
        return self._post_ok("/state/oscillator/delete", {"id": osc_id})

    def reorder_oscillators(self, from_index: int, to_index: int) -> bool:
        return self._post_ok("/state/oscillator/reorder", {"fromIndex": from_index, "toIndex": to_index})

    # ── Pane / Source state ─────────────────────────────────────────

    def get_panes(self) -> List[Dict]:
        resp = self._get_json("/state/panes")
        if resp and resp.get("success"):
            data = resp.get("data", [])
            return data if isinstance(data, list) else []
        return []

    def get_sources(self) -> List[Dict]:
        resp = self._get_json("/state/sources")
        if resp and resp.get("success"):
            data = resp.get("data", [])
            return data if isinstance(data, list) else []
        return []

    def get_first_source_id(self) -> str:
        """Return first source ID or raise if none available."""
        sources = self.get_sources()
        if not sources:
            raise RuntimeError("No audio sources available in test harness")
        return sources[0]["id"]

    # ── Transport control ───────────────────────────────────────────

    def transport_play(self) -> bool:
        return self._post_ok("/transport/play")

    def transport_stop(self) -> bool:
        return self._post_ok("/transport/stop")

    def set_bpm(self, bpm: float) -> bool:
        return self._post_ok("/transport/setBpm", {"bpm": bpm})

    def set_position(self, samples: int) -> bool:
        return self._post_ok("/transport/setPosition", {"samples": samples})

    def get_transport_state(self) -> Optional[Dict]:
        resp = self._get_json("/transport/state")
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    def is_playing(self) -> bool:
        state = self.get_transport_state()
        return bool(state and state.get("playing"))

    def get_bpm(self) -> float:
        state = self.get_transport_state()
        return state.get("bpm", 120.0) if state else 120.0

    # ── Track audio control ─────────────────────────────────────────

    def set_track_audio(
        self, track_id: int, *, waveform: str = "sine",
        frequency: float = 440.0, amplitude: float = 0.8
    ) -> bool:
        return self._post_ok(
            f"/track/{track_id}/audio",
            {"waveform": waveform, "frequency": frequency, "amplitude": amplitude},
        )

    def get_track_info(self, track_id: int) -> Optional[Dict]:
        resp = self._get_json(f"/track/{track_id}/info")
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    # ── Verification (server-side) ──────────────────────────────────

    def verify_waveform_rendered(self, element_id: str, min_amplitude: float = 0.05) -> bool:
        resp = self._post_json("/verify/waveform", {"elementId": element_id, "minAmplitude": min_amplitude})
        if resp and resp.get("success"):
            return resp.get("data", {}).get("pass", False)
        return False

    def verify_element_color(self, element_id: str, color_hex: str, tolerance: int = 10) -> bool:
        resp = self._post_json("/verify/color", {
            "elementId": element_id, "color": color_hex,
            "tolerance": tolerance, "mode": "contains",
        })
        if resp and resp.get("success"):
            return resp.get("data", {}).get("pass", False)
        return False

    def verify_element_bounds(self, element_id: str, width: int, height: int, tolerance: int = 5) -> Dict:
        resp = self._post_json("/verify/bounds", {
            "elementId": element_id, "width": width, "height": height, "tolerance": tolerance,
        })
        if resp and resp.get("success"):
            return resp.get("data", {})
        return {}

    def verify_visible(self, element_id: str) -> bool:
        """Server-side visibility verification (pixel-based, not DOM)."""
        resp = self._post_json("/verify/visible", {"elementId": element_id})
        if resp and resp.get("success"):
            return resp.get("data", {}).get("pass", False)
        return False

    def analyze_waveform(self, element_id: str, bg_color: str = "#000000") -> Optional[Dict]:
        """Server-side waveform analysis: amplitude, activity, zero crossings."""
        resp = self._get_json(
            f"/analyze/waveform?elementId={element_id}&backgroundColor={bg_color}"
        )
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    # ── Screenshots (last resort, for baselines only) ───────────────

    def take_screenshot(self, path: str, element: str = "window") -> bool:
        return self._post_ok("/screenshot", {"path": path, "element": element})

    def compare_screenshot(self, element_id: str, baseline_path: str, tolerance: int = 5) -> Dict:
        resp = self._post_json("/screenshot/compare", {
            "elementId": element_id, "baseline": baseline_path, "tolerance": tolerance,
        })
        if resp and resp.get("success"):
            return resp.get("data", {})
        return {}

    # ── Waveform state ─────────────────────────────────────────────

    def get_waveform_state(self) -> Optional[Dict]:
        """Get full waveform render state including per-pane waveform info."""
        resp = self._get_json("/waveform/state")
        if resp:
            return resp.get("data", resp)
        return None

    def get_waveform_for_pane(self, pane_index: int = 0) -> List[Dict]:
        """Get waveform info for a specific pane. Returns list of waveform dicts."""
        state = self.get_waveform_state()
        if state and "panes" in state and len(state["panes"]) > pane_index:
            return state["panes"][pane_index].get("waveforms", [])
        return []

    def get_display_samples(self, pane_index: int = 0) -> int:
        """Get displaySamples from the first waveform in the given pane."""
        waveforms = self.get_waveform_for_pane(pane_index)
        if waveforms:
            return waveforms[0].get("displaySamples", 0)
        return 0

    def wait_for_waveform_data(
        self, pane_index: int = 0, timeout_s: float = 5.0
    ) -> List[Dict]:
        """Wait until at least one waveform in the pane has data."""
        def check():
            wfs = self.get_waveform_for_pane(pane_index)
            return wfs if wfs and any(w.get("hasWaveformData") for w in wfs) else None
        return self.wait_until(
            check, timeout_s=timeout_s, desc="waveform data to be available"
        )

    # ── Metrics ─────────────────────────────────────────────────────

    def metrics_start(self, interval_ms: int = 50) -> bool:
        return self._post_ok("/metrics/start", {"intervalMs": interval_ms})

    def metrics_stop(self) -> bool:
        return self._post_ok("/metrics/stop")

    def metrics_reset(self) -> bool:
        return self._post_ok("/metrics/reset")

    def metrics_current(self) -> Optional[Dict]:
        resp = self._get_json("/metrics/current")
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    def metrics_stats(self) -> Optional[Dict]:
        resp = self._get_json("/metrics/stats")
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    # ── Focus / keyboard navigation ────────────────────────────────

    def focus(self, element_id: str) -> bool:
        return self._post_ok("/ui/focus", {"elementId": element_id})

    def get_focused(self) -> Optional[Dict]:
        resp = self._get_json("/ui/focused")
        if resp and resp.get("success"):
            return resp.get("data", {})
        return None

    def focus_next(self) -> bool:
        return self._post_ok("/ui/focusNext")

    def focus_previous(self) -> bool:
        return self._post_ok("/ui/focusPrevious")
