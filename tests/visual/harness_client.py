import requests
import time
import json
import os

class HarnessClient:
    def __init__(self, url="http://127.0.0.1:8765"):
        self.url = url
        self.session = requests.Session()

    def post(self, endpoint, data=None):
        try:
            response = self.session.post(f"{self.url}{endpoint}", json=data)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            if hasattr(e.response, 'text'):
                print(f"Error response: {e.response.text}")
            raise

    def get(self, endpoint, params=None):
        try:
            response = self.session.get(f"{self.url}{endpoint}", params=params)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            if hasattr(e.response, 'text'):
                print(f"Error response: {e.response.text}")
            raise

    # --- Transport ---
    def transport_play(self):
        return self.post("/transport/play")

    def transport_stop(self):
        return self.post("/transport/stop")

    # --- Track ---
    def show_editor(self, track_id):
        return self.post(f"/track/{track_id}/showEditor")

    def hide_editor(self, track_id):
        return self.post(f"/track/{track_id}/hideEditor")

    def set_audio_generator(self, track_id, waveform="sine", frequency=440.0, amplitude=0.8):
        return self.post(f"/track/{track_id}/audio", {
            "waveform": waveform,
            "frequency": frequency,
            "amplitude": amplitude
        })

    # --- UI ---
    def get_ui_elements(self):
        return self.get("/ui/elements")

    def wait_for_element(self, element_id, timeout_ms=5000):
        return self.post("/ui/waitForElement", {
            "elementId": elementId,
            "timeoutMs": timeoutMs
        })

    def click(self, element_id):
        return self.post("/ui/click", {"elementId": element_id})

    def set_slider(self, element_id, value):
        return self.post("/ui/slider", {"elementId": elementId, "value": value})

    # --- Screenshot ---
    def capture_screenshot(self, element="window", path="/tmp/screenshot.png"):
        return self.post("/screenshot", {"element": element, "path": path})

    def capture_internal(self, element_id="window", path="/tmp/internal.png"):
        return self.post("/screenshot/internal", {"elementId": element_id, "path": path})

    def verify_waveform(self, element_id, min_amplitude=0.05):
        return self.post("/verify/waveform", {
            "elementId": elementId,
            "minAmplitude": minAmplitude
        })

    def verify_color(self, element_id, color_hex, mode="background", tolerance=10):
        return self.post("/verify/color", {
            "elementId": elementId,
            "color": color_hex,
            "mode": mode,
            "tolerance": tolerance
        })
    
    def analyze_waveform(self, element_id, background_color="#000000"):
        return self.get("/analyze/waveform", params={
            "elementId": elementId,
            "backgroundColor": background_color
        })

    # --- State ---
    def reset_state(self):
        return self.post("/state/reset")

    def add_oscillator(self, name=None, mode="FullStereo", color=None):
        data = {}
        if name: data["name"] = name
        if mode: data["mode"] = mode
        if color: data["colour"] = color
        return self.post("/state/oscillator/add", data)

    def update_oscillator(self, osc_id, **kwargs):
        data = {"id": osc_id}
        data.update(kwargs)
        return self.post("/state/oscillator/update", data)

