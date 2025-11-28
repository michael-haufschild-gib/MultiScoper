import requests
import time
import sys
import os
import json

# Configuration
HARNESS_URL = "http://localhost:8765"
MAX_RETRIES = 5
RETRY_DELAY = 1.0

def check_harness_health():
    for attempt in range(MAX_RETRIES):
        try:
            response = requests.get(f"{HARNESS_URL}/health")
            if response.status_code == 200:
                print(f"Connected to Harness at {HARNESS_URL}")
                return True
        except requests.exceptions.ConnectionError:
            pass
        print(f"Waiting for Harness... (Attempt {attempt + 1}/{MAX_RETRIES})")
        time.sleep(RETRY_DELAY)
    return False

def open_editor():
    print("Opening Plugin Editor...")
    try:
        response = requests.post(f"{HARNESS_URL}/track/0/showEditor")
        response.raise_for_status()
        time.sleep(1.0) 
    except Exception as e:
        print(f"Error opening editor: {e}")
        sys.exit(1)

def click_element(element_id):
    print(f"Clicking {element_id}...")
    try:
        response = requests.post(f"{HARNESS_URL}/ui/click", json={"elementId": element_id})
        response.raise_for_status()
        return True
    except Exception as e:
        print(f"Error clicking {element_id}: {e}")
        return False

def verify_color(element_id, color_hex, tolerance=10):
    print(f"Verifying color of {element_id} is {color_hex}...")
    try:
        # verify/color uses background mode by default
        response = requests.post(f"{HARNESS_URL}/verify/color", json= {
            "elementId": element_id,
            "color": color_hex,
            "tolerance": tolerance,
            "mode": "contains", # Button might have text/gradient, checking if it contains the bg color is safer or use background if solid
            "minCoverage": 0.5 # At least 50% matches (accounts for text and rounded corners)
        })
        response.raise_for_status()
        result = response.json()
        pass_status = result.get("data", {}).get("pass", False)
        if pass_status:
            print(f"SUCCESS: Color match for {element_id}")
        else:
            print(f"FAILURE: Color mismatch for {element_id}")
        return pass_status
    except Exception as e:
        print(f"Error verifying color: {e}")
        return False

def main():
    print("--- E2E Verification: Button Styles ---")

    if not check_harness_health():
        print("FAILED: Could not connect to Harness.")
        sys.exit(1)

    open_editor()

    # Open Add Oscillator Dialog to see buttons
    # We need to find the "Add Oscillator" button in sidebar.
    # SidebarComponent.cpp registers "sidebar_addOscillator"
    if not click_element("sidebar_addOscillator"):
        print("FAILED: Could not open Add Oscillator dialog.")
        sys.exit(1)
    
    time.sleep(1.0) # Wait for dialog

    # Verify Primary Button (OK)
    # Color: #007ACC
    if not verify_color("addOscillatorDialog_okBtn", "#007ACC"):
        print("FAILED: Primary button color check failed.")
        sys.exit(1)

    # Verify Secondary Button (Cancel)
    # Color: #3A3A3A
    if not verify_color("addOscillatorDialog_cancelBtn", "#3A3A3A"):
        print("FAILED: Secondary button color check failed.")
        sys.exit(1)

    print("\n--- VERIFICATION PASSED ---")

if __name__ == "__main__":
    main()
