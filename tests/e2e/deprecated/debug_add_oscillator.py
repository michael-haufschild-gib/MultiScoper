import requests
import time
import sys
import json

HARNESS_URL = "http://localhost:8765"

def get_element(element_id):
    try:
        response = requests.get(f"{HARNESS_URL}/ui/element/{element_id}")
        if response.status_code == 200:
            return response.json().get("data", {})
        return None
    except:
        return None

def main():
    print("--- Debugging Add Oscillator ---")
    requests.post(f"{HARNESS_URL}/track/0/showEditor")
    time.sleep(1.0)

    ids = [
        "sidebar",
        "sidebar_addOscillator",
        "sidebar_oscillators_toolbar",
        # Viewports don't have test IDs by default unless I added them?
        # SidebarComponent.cpp doesn't register viewports.
        # But I can check children of sidebar if I implement child listing?
        # Harness doesn't support child listing easily.
    ]

    for eid in ids:
        print(f"{eid}: {get_element(eid).get('bounds')}")

    print("\nClicking Add Oscillator...")
    requests.post(f"{HARNESS_URL}/ui/click", json={"elementId": "sidebar_addOscillator"})
    time.sleep(1.0)

    popup = get_element("addOscillatorDialog")
    if popup:
        print(f"Popup Visible: {popup.get('visible')}")
        print(f"Popup Bounds: {popup.get('bounds')}")
    else:
        print("Popup Not Found")

if __name__ == "__main__":
    main()
