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

def click(element_id):
    print(f"Clicking {element_id}...")
    requests.post(f"{HARNESS_URL}/ui/click", json={"elementId": element_id})
    time.sleep(0.5)

def main():
    print("--- UI Review & Layout Check ---")
    
    # 1. Open Editor
    print("Opening Editor...")
    requests.post(f"{HARNESS_URL}/track/0/showEditor")
    time.sleep(1.0)

    # 2. Check Sidebar Timing
    print("\n[Timing Section]")
    timing = get_element("sidebar_timing")
    if timing:
        print(f"Timing Section Bounds: {timing.get('bounds')}")
        # Verify content visible (default expanded)
        toggle = get_element("sidebar_timing_modeToggle")
        if toggle and toggle.get("visible"):
            print("Timing content visible: OK")
        else:
            print("Timing content HIDDEN: FAIL")
    else:
        print("Timing Section NOT FOUND: FAIL")

    # 3. Toggle Timing Mode
    print("\nToggling Timing Mode to MELODIC...")
    # Mode toggle is a SegmentedButtonBar. Clicking it might be tricky if it relies on index?
    # But TestUIController::click clicks center. SegmentedButtonBar buttons are children?
    # SegmentedButtonBar.cpp: "auto button = std::make_unique<OscilButton>(label, testId);"
    # It registers children?
    # "OSCIL_REGISTER_CHILD_TEST_ID(*modeToggle_, "sidebar_timing_modeToggle");"
    # This registers the BAR. The buttons inside don't have known IDs unless added with IDs.
    # In TimingSidebarSection.cpp:
    # modeToggle_->addButton("TIME", ...);
    # SegmentedButtonBar::addButton signature: (label, id, testId={})
    # It passes empty testID.
    # So we can't click specific buttons easily via ID.
    # But we can try clicking the bar?
    # The bar center is likely the border between buttons.
    # This is a limitation.
    # However, `sidebar_timing_noteSlider` should be hidden initially.
    note_slider = get_element("sidebar_timing_noteSlider")
    print(f"Note Slider Initial Visibility: {note_slider.get('visible') if note_slider else 'None'}")

    # 4. Add Oscillator (via State API to ensure setup)
    print("\n[Adding Oscillator 1 via API]")
    requests.post(f"{HARNESS_URL}/state/oscillator/add", json={"name": "Oscillator 1"})
    time.sleep(0.5)

    # 5. Add Second Oscillator
    print("\n[Adding Oscillator 2 via API]")
    requests.post(f"{HARNESS_URL}/state/oscillator/add", json={"name": "Oscillator 2"})
    time.sleep(0.5)

    # Verify State
    state_oscs = requests.get(f"{HARNESS_URL}/state/oscillators").json().get("data", [])
    print(f"State Oscillators: {len(state_oscs)}")
    for osc in state_oscs:
        print(f" - {osc.get('id')} : {osc.get('name')} (Order {osc.get('orderIndex')})")

    # 6. Check Oscillator List
    print("\n[Oscillator List]")
    # Refresh element info
    time.sleep(0.5)
    item0 = get_element("sidebar_oscillators_item_0")
    item1 = get_element("sidebar_oscillators_item_1")
    
    if item0 and item1:
        b0 = item0.get("bounds")
        b1 = item1.get("bounds")
        print(f"Item 0 Bounds: {b0}")
        print(f"Item 1 Bounds: {b1}")
        
        if b1['y'] >= b0['y'] + b0['height']:
            print("List Stacking: OK")
        else:
            print("List Stacking: OVERLAP/FAIL")
            
        # 7. Select Item 0
        print("Selecting Item 0...")
        click("sidebar_oscillators_item_0")
        time.sleep(0.5)
        
        item0_expanded = get_element("sidebar_oscillators_item_0")
        b0_exp = item0_expanded.get("bounds")
        print(f"Item 0 Expanded Bounds: {b0_exp}")
        
        if b0_exp['height'] > b0['height']:
            print("Item Expansion: OK")
        else:
            print(f"Item Expansion: FAIL (Height {b0_exp['height']} vs {b0['height']})")
            
        # Check Item 1 position again (should move down)
        item1_after = get_element("sidebar_oscillators_item_1")
        b1_after = item1_after.get("bounds")
        print(f"Item 1 After Bounds: {b1_after}")
        
        if b1_after['y'] >= b0_exp['y'] + b0_exp['height']:
            print("List Layout Update: OK")
        else:
            print("List Layout Update: OVERLAP/FAIL")

    else:
        print("List Items NOT FOUND: FAIL")

if __name__ == "__main__":
    main()
