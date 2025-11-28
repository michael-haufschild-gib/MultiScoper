import requests
import time
import sys
import os
import json

# Configuration
HARNESS_URL = "http://localhost:8765"
MAX_RETRIES = 5
RETRY_DELAY = 1.0

# Test IDs (from C++ code)
SIDEBAR_OPTIONS_DROPDOWN_ID = "sidebar_options_themeDropdown"

def check_harness_health():
    """Checks if the harness is running and reachable."""
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

def debug_list_all_elements():
    """Lists all registered UI elements for debugging."""
    try:
        response = requests.get(f"{HARNESS_URL}/ui/elements")
        if response.status_code == 200:
            data = response.json().get("data", {})
            elements = data.get("elements", [])
            print("\n--- DEBUG: Registered UI Elements ---")
            print(f"Total count: {data.get('count', 0)}")
            found_target = False
            for el in elements:
                test_id = el.get("testId")
                # visible = el.get("visible")
                # enabled = el.get("enabled")
                print(f"- {test_id}")
                if test_id == SIDEBAR_OPTIONS_DROPDOWN_ID:
                    found_target = True
            print("-------------------------------------\n")
            return found_target
        else:
            print(f"Failed to list elements. Status: {response.status_code}")
            return False
    except Exception as e:
        print(f"Error listing elements: {e}")
        return False

def open_editor():
    print("Opening Plugin Editor...")
    try:
        # Assume track 0 exists
        response = requests.post(f"{HARNESS_URL}/track/0/showEditor")
        response.raise_for_status()
        print("Editor opened command sent.")
        time.sleep(1.0) # Wait for UI to build
    except Exception as e:
        print(f"Error opening editor: {e}")
        sys.exit(1)

def main():
    print("--- Running E2E Test: Theme Dropdown Population ---")

    if not check_harness_health():
        print("FAILED: Could not connect to Harness. Is it running?")
        sys.exit(1)

    # IMPORTANT: Open the editor first!
    open_editor()

    # Debug: List all elements to see what's actually registered
    found = debug_list_all_elements()

    if found:
        print(f"SUCCESS: Found {SIDEBAR_OPTIONS_DROPDOWN_ID} in registry!")
    else:
        print(f"FAILURE: {SIDEBAR_OPTIONS_DROPDOWN_ID} NOT found in registry.")
        sys.exit(1)

    print("\n--- E2E Test Finished ---")

if __name__ == "__main__":
    main()
