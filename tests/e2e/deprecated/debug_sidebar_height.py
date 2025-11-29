import requests
import time
import sys
import os
import json

HARNESS_URL = "http://localhost:8765"

def get_element_info(element_id):
    try:
        response = requests.get(f"{HARNESS_URL}/ui/element/{element_id}")
        if response.status_code == 200:
            return response.json().get("data", {})
        return None
    except:
        return None

def main():
    print("--- Debugging Sidebar Height ---")
    
    # 1. Open Editor
    requests.post(f"{HARNESS_URL}/track/0/showEditor")
    time.sleep(1.0)

    # 2. Expand Timing Section
    requests.post(f"{HARNESS_URL}/ui/click", json={"elementId": "sidebar_timing"})
    time.sleep(1.0) # Wait for animation

    # 3. Get Bounds of Section and Content
    section_info = get_element_info("sidebar_timing")
    slider_info = get_element_info("sidebar_timing_intervalSlider")

    if section_info:
        bounds = section_info.get("bounds", {})
        print(f"Section Bounds: {bounds}")
        # Header is 40px. If height is ~40, it didn't expand.
        
    if slider_info:
        bounds = slider_info.get("bounds", {})
        print(f"Slider Bounds: {bounds}")
        # Relative to parent (TimingSection).
        
        screen_bounds = slider_info.get("screenBounds", {})
        print(f"Slider Screen Bounds: {screen_bounds}")

if __name__ == "__main__":
    main()