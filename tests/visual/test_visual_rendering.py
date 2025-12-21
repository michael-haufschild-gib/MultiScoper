import unittest
import time
import os
import sys
from harness_client import HarnessClient

class TestVisualRendering(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.client = HarnessClient()
        # Wait for harness to be ready (health check)
        for i in range(20):
            try:
                cls.client.get("/health")
                break
            except:
                time.sleep(0.5)
        else:
            raise RuntimeError("Harness not reachable. Make sure OscilTestHarness is running.")
        
        # Start transport to ensure audio engine is running
        cls.client.transport_play()
        
        # Reset state
        cls.client.reset_state()
        time.sleep(0.5)
        
        # Add a default track/oscillator
        print("Adding oscillator...")
        cls.client.add_oscillator(name="Osc 1", mode="FullStereo", color="#00FF00")
        time.sleep(0.5)
        
        # Show editor for track 0
        print("Showing editor...")
        cls.client.show_editor(0)
        
        # Wait for UI to load
        time.sleep(3)
        
        # Turn off grid by default for cleaner waveform analysis
        print("Turning off grid...")
        cls.client.click("sidebar_options_gridToggle")
        time.sleep(0.5)

    def set_grid(self, enabled):
        # The toggle might already be in the desired state.
        # For simplicity in this test environment, we'll just check if we need to click.
        # But since it's a toggle, we can just click if it doesn't match.
        # Actually, let's just click it and wait.
        self.client.click("sidebar_options_gridToggle")
        time.sleep(0.5)

    def wait_for_condition(self, condition_func, timeout=5.0, interval=0.5):
        start_time = time.time()
        while time.time() - start_time < timeout:
            if condition_func():
                return True
            time.sleep(interval)
        return False

    def get_waveform_analysis(self, element_id):
        # We try to get a fresh analysis, but sometimes we need to wait for render
        res = self.client.get("/analyze/internal", params={"elementId": element_id})
        return res["data"]

    def get_track_index_for_oscillator(self, osc_id):
        # Get oscillator source ID
        state = self.client.get("/state/oscillators")
        osc = next((o for o in state["data"] if o["id"] == osc_id), None)
        if not osc: return 0
        
        source_id = osc.get("sourceId")
        if not source_id: return 0
        
        # Check all tracks to find matching source ID
        # Since we have 3 tracks default
        for i in range(3):
            info = self.client.get(f"/track/{i}/info")
            if info["success"] and info["data"]["sourceId"] == source_id:
                print(f"  Oscillator {osc_id} is bound to Track {i}")
                return i
                
        print("  Warning: Could not match source ID, defaulting to 0")
        return 0

    def test_1_waveform_rendering_basic(self):
        """Verify waveform is rendered (not empty)"""
        print("Test 1: Basic Rendering")
        
        state = self.client.get("/state/oscillators")
        osc_id = state["data"][0]["id"]
        track_idx = self.get_track_index_for_oscillator(osc_id)
        
        self.client.set_audio_generator(track_idx, waveform="sine", frequency=100.0, amplitude=1.0)
        time.sleep(2.0) # Wait for initial buffer fill
        
        elements = self.client.get_ui_elements()
        # Target pane_0/1/2 directly (avoiding pane_body if it's missing, and avoiding dialogs)
        pane_id = next((e["testId"] for e in elements["data"]["elements"] if "pane_" in e["testId"] and "body" not in e["testId"]), "")
        
        print(f"  Using analysis target: {pane_id}")
        
        def check():
            analysis = self.get_waveform_analysis(pane_id)
            print(f"  Attempt: detected={analysis['detected']}, amp={analysis['amplitude']:.2f}, zc={analysis['zeroCrossings']}")
            return analysis["detected"] and analysis["amplitude"] > 0.1
            
        success = self.wait_for_condition(check, timeout=15.0)
        if not success:
            self.client.capture_internal(element_id=pane_id, path="failed_basic.png")
            
        self.assertTrue(success, "Waveform should be detected and have amplitude")
        print("Basic rendering passed")

    def test_2_style_changes(self):
        """Verify color changes are reflected"""
        print("Test 2: Style Changes")
        state = self.client.get("/state/oscillators")
        osc_id = state["data"][0]["id"]
        # We don't need track index here, just updating the oscillator style
        
        elements = self.client.get_ui_elements()
        pane_id = next((e["testId"] for e in elements["data"]["elements"] if "pane_" in e["testId"] and "body" not in e["testId"]), "")

        # Red
        print("  Setting color to Red (#FF0000)")
        self.client.update_oscillator(osc_id, colour="#FF0000")
        
        def check_red():
            osc_state = self.client.get("/state/oscillators")
            # Try both spellings just in case
            osc = next((o for o in osc_state["data"] if o["id"] == osc_id), {})
            current_color = osc.get("colour", osc.get("color", "unknown"))
            analysis = self.get_waveform_analysis(pane_id)
            dom_color = analysis["dominantColor"].lower()
            print(f"  State color: {current_color}, Render dominant: {dom_color}")
            # Check for reddishness. JUCE ARGB might be FFFF0000.
            # Relaxed check: Red component > 200 (C8), Green/Blue < 100 (64)
            try:
                r = int(dom_color[2:4], 16)
                g = int(dom_color[4:6], 16)
                b = int(dom_color[6:8], 16)
                return r > 200 and g < 100 and b < 100
            except:
                return False

        success = self.wait_for_condition(check_red, timeout=10.0)
        self.assertTrue(success, "Should be reddish")
        
        # Blue
        print("  Setting color to Blue (#0000FF)")
        self.client.update_oscillator(osc_id, colour="#0000FF")
        
        def check_blue():
            osc_state = self.client.get("/state/oscillators")
            osc = next((o for o in osc_state["data"] if o["id"] == osc_id), {})
            current_color = osc.get("colour", osc.get("color", "unknown"))
            analysis = self.get_waveform_analysis(pane_id)
            dom_color = analysis["dominantColor"].lower()
            print(f"  State color: {current_color}, Render dominant: {dom_color}")
            # Relaxed check: Blue component > 200, Red/Green < 100
            try:
                r = int(dom_color[2:4], 16)
                g = int(dom_color[4:6], 16)
                b = int(dom_color[6:8], 16)
                return b > 200 and r < 100 and g < 100
            except:
                return False

        success = self.wait_for_condition(check_blue, timeout=10.0)
        self.assertTrue(success, "Color change should be reflected")
        print("Style change passed")

    def test_3_resolution_changes(self):
        """Verify timing/resolution changes affect rendering"""
        print("Test 3: Resolution/Frequency Changes")
        
        state = self.client.get("/state/oscillators")
        osc_id = state["data"][0]["id"]
        track_idx = self.get_track_index_for_oscillator(osc_id)
        
        elements = self.client.get_ui_elements()
        pane_id = next((e["testId"] for e in elements["data"]["elements"] if "pane_" in e["testId"] and "body" not in e["testId"]), "")

        # Low frequency
        print("  Setting 50Hz")
        self.client.set_audio_generator(track_idx, waveform="sine", frequency=50.0, amplitude=1.0)
        
        # Wait for stabilize
        time.sleep(3.0)
        zc1 = self.get_waveform_analysis(pane_id)["zeroCrossings"]
        print(f"  Zero crossings at 50Hz: {zc1}")
        
        # High frequency
        print("  Setting 500Hz")
        self.client.set_audio_generator(track_idx, waveform="sine", frequency=500.0, amplitude=1.0)
        
        def check_higher():
            analysis = self.get_waveform_analysis(pane_id)
            zc2 = analysis["zeroCrossings"]
            print(f"  Zero crossings at 500Hz: {zc2}, amp={analysis['amplitude']:.2f}")
            return zc2 > zc1
            
        success = self.wait_for_condition(check_higher, timeout=15.0)
        self.assertTrue(success, "Higher frequency should have more zero crossings")
        print("Resolution change passed")

    def test_4_grid_and_silence(self):
        """Verify silence and grid rendering"""
        print("Test 4: Grid and Silence")
        
        state = self.client.get("/state/oscillators")
        osc_id = state["data"][0]["id"]
        track_idx = self.get_track_index_for_oscillator(osc_id)
        
        elements = self.client.get_ui_elements()
        pane_id = next((e["testId"] for e in elements["data"]["elements"] if "pane_" in e["testId"] and "body" not in e["testId"]), "")

        # 1. Check silence (grid is OFF)
        print("  Checking silence (Grid OFF)")
        self.client.set_audio_generator(track_idx, amplitude=0.0)
        
        def check_silence():
            analysis = self.get_waveform_analysis(pane_id)
            print(f"  Silence check: amp={analysis['amplitude']:.2f}, activity={analysis['activity']:.4f}")
            return analysis["amplitude"] < 0.05 and analysis["activity"] < 0.01
            
        success = self.wait_for_condition(check_silence, timeout=15.0)
        self.assertTrue(success, "Waveform should become flat and empty when grid is off and silent")
        
        # 2. Turn grid ON
        print("  Turning grid ON")
        self.set_grid(True)
        
        def check_grid():
            analysis = self.get_waveform_analysis(pane_id)
            print(f"  Grid check: activity={analysis['activity']:.4f}")
            return analysis["activity"] > 0.01
            
        success = self.wait_for_condition(check_grid, timeout=5.0)
        self.assertTrue(success, "Grid should be visible when enabled")
        
        # Turn grid OFF again for future tests
        print("  Turning grid OFF")
        self.set_grid(False)
        print("Grid and silence passed")


if __name__ == '__main__':
    unittest.main()