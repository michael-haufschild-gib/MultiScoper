import requests
import time
import sys
import json

HARNESS_URL = "http://localhost:8765"

def get_bounds(element_id):
    try:
        response = requests.get(f"{HARNESS_URL}/ui/element/{element_id}")
        if response.status_code == 200:
            return response.json().get("data", {}).get("bounds", {})
        return None
    except:
        return None

def main():
    print("--- Debugging Layout ---")
    
    requests.post(f"{HARNESS_URL}/track/0/showEditor")
    time.sleep(1.0)
    
    # Ensure timing expanded
    requests.post(f"{HARNESS_URL}/ui/click", json={"elementId": "sidebar_timing"})
    time.sleep(1.0)

    ids = [
        "sidebar",
        "sidebar_timing", # Accordion Section
        "sidebar_timing_modeToggle",
        "sidebar_timing_waveformModeDropdown",
        "sidebar_timing_intervalSlider",
        "sidebar_options", # Accordion Section
        "sidebar_options_gainSlider"
    ]

    for eid in ids:
        bounds = get_bounds(eid)
        print(f"{eid}: {bounds}")

if __name__ == "__main__":
    main()