import requests
import time
import sys
import os
import json

# Configuration
HARNESS_URL = "http://localhost:8765"
MAX_RETRIES = 5
RETRY_DELAY = 1.0

SIDEBAR_OPTIONS_DROPDOWN_ID = "sidebar_options_themeDropdown"

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

def get_ui_element_info(element_id):
    try:
        response = requests.get(f"{HARNESS_URL}/ui/element/{element_id}")
        if response.status_code == 404:
            return None
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"Error fetching element info for {element_id}: {e}")
        return None

def select_theme(element_id, theme_id):
    try:
        # Note: TestUIController::select supports selectByText if itemId is -1.
        # OscilDropdown items use string IDs.
        # We can try sending itemText.
        print(f"Selecting theme: {theme_id}")
        response = requests.post(f"{HARNESS_URL}/ui/select", json={"elementId": element_id, "itemId": -1, "itemText": theme_id})
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"Error selecting theme {theme_id}: {e}")
        return None

def main():
    print("--- E2E Verification: Theme Dropdown ---")

    if not check_harness_health():
        print("FAILED: Could not connect to Harness.")
        sys.exit(1)

    open_editor()

    print(f"Inspecting dropdown: {SIDEBAR_OPTIONS_DROPDOWN_ID}")
    info_response = get_ui_element_info(SIDEBAR_OPTIONS_DROPDOWN_ID)
    
    if not info_response or not info_response.get("success"):
        print(f"FAILED: Dropdown not found or error in response.")
        sys.exit(1)

    data = info_response.get("data", {})
    print(f"Dropdown Info: Type={data.get('type')}, Visible={data.get('visible')}")
    
    # Verify population
    num_items = data.get("numItems", 0)
    items = data.get("items", [])
    
    print(f"Item Count: {num_items}")
    if items:
        print(f"Items: {[item['text'] for item in items]}")

    if num_items == 0:
        print("FAILED: Dropdown is empty!")
        sys.exit(1)

    # Verify items contain "Dark" (default)
    has_dark = any(item['text'] == "Dark" for item in items)
    if not has_dark:
        print("WARNING: 'Dark' theme not found in items.")

    # Verify Selection
    selected_text = data.get("selectedText")
    print(f"Current Selection: {selected_text}")

    # Try changing selection if multiple items
    if num_items > 1:
        target_theme = items[1]['text'] if items[0]['text'] == selected_text else items[0]['text']
        select_theme(SIDEBAR_OPTIONS_DROPDOWN_ID, target_theme)
        
        time.sleep(0.5) 
        
        # Verify update
        new_info = get_ui_element_info(SIDEBAR_OPTIONS_DROPDOWN_ID)
        new_selection = new_info.get("data", {}).get("selectedText")
        print(f"New Selection: {new_selection}")
        
        if new_selection == target_theme:
            print(f"SUCCESS: Selection changed to {target_theme}")
        else:
            print(f"FAILED: Selection did not update. Expected {target_theme}, got {new_selection}")
            sys.exit(1)

    print("\n--- VERIFICATION PASSED ---")

if __name__ == "__main__":
    main()
