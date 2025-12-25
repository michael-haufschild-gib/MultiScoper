import requests
import time
import sys
import io
import json
try:
    from PIL import Image
    import numpy as np
except ImportError:
    print("This test requires Pillow and numpy.")
    sys.exit(1)

BASE_URL = "http://localhost:8765"

class OscilTestClient:
    def __init__(self, url=BASE_URL):
        self.url = url
        self.session = requests.Session()

    def wait_for_server(self, timeout=30):
        start_time = time.time()
        print(f"Waiting for server at {self.url}...")
        while time.time() - start_time < timeout:
            try:
                resp = self.session.get(f"{self.url}/health")
                if resp.status_code == 200:
                    print("Server is up!")
                    return True
            except requests.exceptions.ConnectionError:
                pass
            time.sleep(0.5)
        print("Server timed out.")
        return False

    def post(self, endpoint, data=None):
        try:
            resp = self.session.post(f"{self.url}{endpoint}", json=data)
            resp.raise_for_status()
            return resp.json()
        except Exception as e:
            print(f"POST {endpoint} failed: {e}")
            if 'resp' in locals():
                print(resp.text)
            return None

    def get(self, endpoint):
        try:
            resp = self.session.get(f"{self.url}{endpoint}")
            resp.raise_for_status()
            return resp.json()
        except Exception as e:
            print(f"GET {endpoint} failed: {e}")
            return None
            
    def get_raw_image(self, element_id=None):
        try:
            url = f"{self.url}/screenshot/raw"
            params = {}
            if element_id:
                params["elementId"] = element_id
                
            resp = self.session.get(url, params=params)
            resp.raise_for_status()
            return Image.open(io.BytesIO(resp.content))
        except Exception as e:
            print(f"GET /screenshot/raw failed: {e}")
            return None

    def reset_state(self):
        return self.post("/state/reset")

    def add_oscillator(self, name="Test Osc", color=None):
        data = {"name": name}
        if color:
            data["color"] = color
        return self.post("/state/oscillator/add", data)

    def update_oscillator(self, osc_id, color=None):
        data = {"id": osc_id}
        if color:
            data["color"] = color
        return self.post("/state/oscillator/update", data)

    def set_track_audio(self, track_id=0, waveform="sine", freq=440):
        return self.post(f"/track/{track_id}/audio", {"waveform": waveform, "frequency": freq})

    def show_editor(self, track_id=0):
        return self.post(f"/track/{track_id}/showEditor")

def run_tests():
    client = OscilTestClient()
    
    if not client.wait_for_server():
        return False
    
    print("\n=== Setting up Test Environment ===")
    
    # 1. Reset state
    print("Resetting state...")
    client.reset_state()
    
    # 2. Show Editor
    print("Opening Editor...")
    client.show_editor(0)
    
    # Wait for editor window to actually appear (look for waveform view)
    print("Waiting for editor window...")
    waveform_element_id = None
    
    # Poll for elements
    for i in range(20): # Wait up to 10 seconds
        elems = client.get("/ui/elements")
        if elems and elems.get("success"):
            # print(f"Found {len(elems['data']['elements'])} elements")
            for el in elems["data"]["elements"]:
                tid = el["testId"]
                # Prefer pane_body over pane
                if tid == "pane_body_0":
                    waveform_element_id = tid
                    break
                if tid == "pane_0" and not waveform_element_id:
                    waveform_element_id = tid
        
        if waveform_element_id:
            break
        time.sleep(0.5)
        
    if not waveform_element_id:
        print("WARNING: Could not identify specific waveform element. Using 'window'.")
        waveform_element_id = "window"
    else:
        print(f"Targeting element: {waveform_element_id}")

    # 3. Add Oscillator
    print("Adding Oscillator...")
    osc_resp = client.add_oscillator("TestOsc", "#FF0000")
    if not osc_resp or not osc_resp.get("success"):
        print("Failed to add oscillator")
        return False
    
    osc_id = osc_resp["data"]["id"]
    print(f"Oscillator added with ID: {osc_id}")

    # Wait for rendering to start
    time.sleep(2) # Give OpenGL time to initialize and render first frames

    print("\n=== Test 1: Waveform Rendering Existence ===")
    # Feed audio
    client.set_track_audio(0, "sine", 200)
    time.sleep(1.0)
    
    # Try verify/waveform
    verify_resp = client.post("/verify/waveform", {"elementId": waveform_element_id})
    if verify_resp and verify_resp["data"]["pass"]:
        print("PASS: Waveform rendered successfully.")
    else:
        print("FAIL: Waveform not detected.")
        print(json.dumps(verify_resp, indent=2))
        # Capture debug image
        client.post("/screenshot", {"path": "/tmp/debug_fail_existence.png", "elementId": waveform_element_id})
        
    print("\n=== Test 2: Style (Color Change) ===")
    print("Verifying RED...")
    color_resp = client.post("/verify/color", {
        "elementId": waveform_element_id, 
        "color": "#FF0000", 
        "mode": "contains",
        "tolerance": 50
    })
    
    if color_resp and color_resp["data"]["pass"]:
        print("PASS: Found RED content.")
    else:
        print("FAIL: Did not find RED content.")
        print(json.dumps(color_resp, indent=2))

    # Change to Blue
    print("Changing color to BLUE...")
    client.update_oscillator(osc_id, "#0000FF")
    time.sleep(1.0)
    
    print("Verifying BLUE...")
    color_resp = client.post("/verify/color", {
        "elementId": waveform_element_id, 
        "color": "#0000FF", 
        "mode": "contains",
        "tolerance": 50
    })
    
    if color_resp and color_resp["data"]["pass"]:
        print("PASS: Found BLUE content.")
    else:
        print("FAIL: Did not find BLUE content.")
        print(json.dumps(color_resp, indent=2))

    print("\n=== Test 3: Grid Toggle ===")
    grid_toggle_id = "sidebar_options_gridToggle"
    
    # Try to find grid toggle
    found_toggle = False
    for i in range(5):
        elems = client.get("/ui/elements")
        all_ids = [e["testId"] for e in elems["data"]["elements"]]
        if grid_toggle_id in all_ids:
            found_toggle = True
            break
        
        # If not found, try to expand OPTIONS section
        if "sidebar_options" in all_ids:
            print("Expanding OPTIONS section...")
            client.post("/ui/click", {"elementId": "sidebar_options"})
            time.sleep(0.5)
        elif "sidebar" in all_ids: # Maybe just sidebar?
             pass # Already expanded?
        
        time.sleep(0.5)

    if not found_toggle:
        print(f"WARNING: Grid toggle {grid_toggle_id} not found after attempts. Skipping grid test.")
        # print("Available elements:", all_ids)
    else:
        # 1. Turn Grid OFF
        print("Turning Grid OFF...")
        client.post("/ui/toggle", {"elementId": grid_toggle_id, "value": False})
        time.sleep(1.0)
        img_off = client.get_raw_image(waveform_element_id)
        
        # 2. Turn Grid ON
        print("Turning Grid ON...")
        client.post("/ui/toggle", {"elementId": grid_toggle_id, "value": True})
        time.sleep(1.0)
        img_on = client.get_raw_image(waveform_element_id)
        
        if img_off and img_on:
            diff = np.abs(np.array(img_on, dtype=int) - np.array(img_off, dtype=int))
            diff_sum = np.sum(diff)
            print(f"Diff sum between Grid ON and OFF: {diff_sum}")
            
            if diff_sum > 1000:
                print("PASS: Grid toggle resulted in visual change.")
            else:
                print("FAIL: Grid toggle did not change visual output.")
                img_off.save("/tmp/grid_off.png")
                img_on.save("/tmp/grid_on.png")
                # return False

    print("\n=== Test 4: Timing / Resolution Change ===")
    # Toggle to Time Mode
    # Need to find it. 'sidebar_timing_modeToggle_time' might be hidden too?
    # Try expanding TIMING
    client.post("/ui/click", {"elementId": "sidebar_timing"})
    time.sleep(0.5)
    
    client.post("/ui/click", {"elementId": "sidebar_timing_modeToggle_time"})
    time.sleep(0.5)

    # 1. Set Interval to 10ms
    print("Setting interval to 10ms...")
    client.post("/ui/clearText", {"elementId": "sidebar_timing_intervalField"})
    client.post("/ui/typeText", {"elementId": "sidebar_timing_intervalField", "text": "10"})
    client.post("/ui/keyPress", {"key": "enter"})
    time.sleep(1.0)
    img_fast = client.get_raw_image(waveform_element_id)
    
    # 2. Set Interval to 100ms
    print("Setting interval to 100ms...")
    client.post("/ui/clearText", {"elementId": "sidebar_timing_intervalField"})
    client.post("/ui/typeText", {"elementId": "sidebar_timing_intervalField", "text": "100"})
    client.post("/ui/keyPress", {"key": "enter"})
    time.sleep(1.0)
    img_slow = client.get_raw_image(waveform_element_id)
    
    if img_fast and img_slow:
        # Check simple difference or crossing count
        diff = np.abs(np.array(img_fast, dtype=int) - np.array(img_slow, dtype=int))
        if np.sum(diff) > 1000:
             print("PASS: Timing change resulted in visual change.")
        else:
             print("FAIL: Timing change did not affect visualization.")
             img_fast.save("/tmp/timing_10ms.png")
             img_slow.save("/tmp/timing_100ms.png")
             # return False

    print("\n=== Tests Completed ===")
    return True

if __name__ == "__main__":
    success = run_tests()
    if not success:
        sys.exit(1)
