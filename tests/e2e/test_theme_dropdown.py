import requests
import time
import sys
import os

# Configuration
HARNESS_URL = "http://localhost:8765"
MAX_RETRIES = 5
RETRY_DELAY = 1.0

# Test IDs (from C++ code)
SIDEBAR_OPTIONS_DROPDOWN_ID = "sidebar_options_themeDropdown"

def get_ui_element_info(element_id):
    """Fetches information about a UI element by its Test ID."""
    try:
        response = requests.get(f"{HARNESS_URL}/ui/element?id={element_id}")
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"Error fetching element info for {element_id}: {e}")
        return None

def select_ui_dropdown_item(element_id, item_id):
    """Selects an item in a dropdown by its Test ID and item ID."""
    try:
        response = requests.post(f"{HARNESS_URL}/ui/select", json={"id": element_id, "item_id": item_id})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"Error selecting item {item_id} in {element_id}: {e}")
        return None

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

def main():
    print("--- Running E2E Test: Theme Dropdown Population ---")

    if not check_harness_health():
        print("FAILED: Could not connect to Harness. Is it running?")
        sys.exit(1)

    # 1. Verify Dropdown Exists and Has Items
    print(f"Checking for theme dropdown: {SIDEBAR_OPTIONS_DROPDOWN_ID}")
    dropdown_info = get_ui_element_info(SIDEBAR_OPTIONS_DROPDOWN_ID)

    if not dropdown_info or dropdown_info.get("status") != "success":
        print(f"FAILED: Could not find or get info for theme dropdown. Response: {dropdown_info}")
        sys.exit(1)

    element_data = dropdown_info.get("data", {})
    item_count = element_data.get("num_items", 0)
    current_selection_id = element_data.get("selected_id") # Note: Corrected key based on OscilDropdown implementation
    available_items = element_data.get("items", [])

    print(f"Found theme dropdown. Number of items: {item_count}")
    print(f"Currently selected ID: {current_selection_id}")
    
    if available_items:
        print(f"Available items: {[item['id'] for item in available_items]}")

    if item_count == 0:
        print("FAILED: Theme dropdown is empty.")
        sys.exit(1)
    
    print("PASSED: Theme dropdown is populated with items.")

    # 2. Select a different theme and verify
    if item_count > 1:
        # Find a theme that is NOT the current one
        theme_to_select_id = None
        for item in available_items:
            if item["id"] != current_selection_id:
                theme_to_select_id = item["id"]
                break
        
        if not theme_to_select_id:
            print("Could not find a different theme to select (weird).")
            sys.exit(1)

        print(f"\nAttempting to select theme: {theme_to_select_id}")
        
        select_response = select_ui_dropdown_item(SIDEBAR_OPTIONS_DROPDOWN_ID, theme_to_select_id)

        if not select_response or select_response.get("status") != "success":
            print(f"FAILED: Could not select theme {theme_to_select_id}. Response: {select_response}")
            sys.exit(1)
        
        # Short wait for state update
        time.sleep(0.5)

        # Verify new selection
        updated_dropdown_info = get_ui_element_info(SIDEBAR_OPTIONS_DROPDOWN_ID)
        updated_data = updated_dropdown_info.get("data", {})
        updated_selection_id = updated_data.get("selectedId") # Note: Checked key in C++ impl (it's "selectedId")

        # Double check the keys in the response if mismatch persists
        if not updated_selection_id and "selected_id" in updated_data:
             updated_selection_id = updated_data["selected_id"]

        print(f"Updated selection ID: {updated_selection_id}")

        if updated_selection_id == theme_to_select_id:
            print(f"PASSED: Successfully selected theme '{theme_to_select_id}'.")
        else:
            print(f"FAILED: Expected selected theme '{theme_to_select_id}', but found '{updated_selection_id}'.")
            sys.exit(1)
    else:
        print("\nSkipping theme selection test: Only one theme available.")

    print("\n--- E2E Test Finished Successfully ---")

if __name__ == "__main__":
    main()
