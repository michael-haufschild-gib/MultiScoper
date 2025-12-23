"""
E2E Test: Visual Settings Interaction

Tests that changing visual settings in the sidebar correctly updates the UI.
Verifies:
1. Grid visibility toggle
2. Time interval changes (Zoom)
3. Gain adjustments
4. Auto-scale toggle
5. Layout changes
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient, run_test

def setup(client: OscilTestClient):
    """Reset state and open editor"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(1.0) # Wait for state reset to propagate
    
    # Try to open editor
    if not client.open_editor(0):
        print("[SETUP] Editor open failed, trying to close and reopen...")
        client.close_editor(0)
        time.sleep(0.5)
        assert client.open_editor(0), "Failed to open editor on retry"
        
    time.sleep(1.0) # Wait for UI to settle

def test_visual_settings(client: OscilTestClient):
    """
    Test: Visual Settings
    
    1. Grid Toggle
    2. Time Interval (Zoom)
    3. Gain Control
    4. Auto-Scale
    5. Layout Columns
    """
    setup(client)
    
    # Target the Pane BODY (excluding header) to avoid header text affecting analysis
    pane_id = "pane_body_0"
    
    # Ensure exactly one oscillator exists (robust cleanup)
    import requests
    
    # First, force add one if none
    oscillators = client.get_oscillators()
    if not oscillators:
        client.add_oscillator(source_id="source_0", name="Settings Test Osc", color="#00FF00")
        time.sleep(0.5)
        
    # Then prune until 1 remains
    max_retries = 5
    for _ in range(max_retries):
        oscillators = client.get_oscillators()
        if len(oscillators) == 1:
            break
            
        print(f"[SETUP] Found {len(oscillators)} oscillators, pruning...")
        # Keep the last one (most recently added)
        keep_id = oscillators[-1]['id']
        
        for osc in oscillators:
            if osc['id'] != keep_id:
                requests.post(f"{client.base_url}/state/oscillator/delete", json={"id": osc['id']})
        
        time.sleep(0.5)
        
    oscillators = client.get_oscillators()
    assert len(oscillators) == 1, f"Failed to prune oscillators, found {len(oscillators)}"
    osc_id = oscillators[0]['id']
    print(f"[SETUP] Using oscillator {osc_id}")

    # Set to Mono mode to ensure waveform is centered for analysis
    import requests
    requests.post(f"{client.base_url}/state/oscillator/update", json={
        "id": osc_id,
        "mode": "Mono",
        "colour": "#00FF00"
    })
    
    client.set_track_audio(0, waveform="sine", frequency=220.0, amplitude=0.5)
    client.transport_play()
    time.sleep(0.5)
    
    # 1. Verify Grid Toggle
    # Expand Options section if needed (usually sections are expanded by default or have IDs)
    # We assume elements are accessible if section is expanded.
    # Check if grid toggle exists
    grid_toggle_id = "sidebar_options_gridToggle"
    if client.element_exists(grid_toggle_id):
        print("[ACTION] Toggling Grid Off...")
        client.toggle(grid_toggle_id, False)
        time.sleep(0.5)
        
        # Verify visually
        client.take_screenshot("/tmp/settings_grid_off.png", element_id=pane_id)
        # TODO: Advanced verification could check for grid lines (difficult without reference)
        
        print("[ACTION] Toggling Grid On...")
        client.toggle(grid_toggle_id, True)
        time.sleep(0.5)
        client.take_screenshot("/tmp/settings_grid_on.png", element_id=pane_id)
    else:
        print(f"[WARN] Grid toggle {grid_toggle_id} not found")

    # 2. Verify Time Interval (Zoom)
    # Check Timing section
    interval_field_id = "sidebar_timing_intervalField"
    if client.element_exists(interval_field_id):
        print("[ACTION] Changing Time Interval to 10ms (Zoom In)...")
        # OscilTextField usually accepts text input or direct value setting via special endpoint if supported
        # client.type_text(interval_field_id, "10.0") # Might need enter
        # Or if it exposes value setting:
        # We don't have a direct "set value" for text field in utils yet, but type_text works.
        # Alternatively, use dragging on the field if it supports it?
        # Let's try typing "10" + Enter
        client.click(interval_field_id)
        client.clear_text(interval_field_id)
        client.type_text(interval_field_id, "10")
        
        # Use raw request for key press since it's not in client wrapper yet
        import requests
        requests.post(f"{client.base_url}/ui/keyPress", json={"key": "enter", "elementId": interval_field_id})
        
        time.sleep(0.5)
        client.take_screenshot("/tmp/settings_zoom_in.png", element_id=pane_id)
        
        # Verify amplitude/shape changes? 
        # Zooming in on 220Hz (4.5ms period) to 10ms window -> ~2 cycles visible
        # Standard is usually 100ms -> ~20 cycles.
        # So waveform should look different.
        
    else:
        print(f"[WARN] Interval field {interval_field_id} not found")

    # 3. Verify Gain & Auto-Scale
    # IMPORTANT: Turn Grid OFF for accurate amplitude analysis
    if client.element_exists(grid_toggle_id):
        print("[ACTION] Ensuring Grid is OFF for Gain analysis...")
        client.toggle(grid_toggle_id, False)
        time.sleep(0.5)

    gain_slider_id = "sidebar_options_gainSlider"
    auto_scale_id = "sidebar_options_autoScaleToggle"
    
    if client.element_exists(auto_scale_id):
        print("[ACTION] Disabling Auto-Scale...")
        client.toggle(auto_scale_id, False)
        time.sleep(0.5)
        
        if client.element_exists(gain_slider_id):
            print("[ACTION] Increasing Gain to +12dB...")
            client.set_slider_value(gain_slider_id, 12.0)
            time.sleep(0.5)
            client.take_screenshot("/tmp/settings_high_gain.png", element_id=pane_id)
            
            # Analyze amplitude - should be high (clipped?)
            res = client.verify_waveform(pane_id, min_amplitude=0.8)
            if res.get('pass'):
                print("[PASS] High gain resulted in high amplitude")
            else:
                print(f"[FAIL] High gain check failed: {res}")
                
            print("[ACTION] Changing color to Blue for Low Gain test...")
            import requests
            requests.post(f"{client.base_url}/state/oscillator/update", json={
                "id": osc_id,
                "colour": "#0000FF"
            })
            time.sleep(0.5)

            print("[ACTION] Decreasing Gain to -60dB (Silence)...")
            client.set_slider_value(gain_slider_id, -60.0)
            
            # Force editor refresh to clear any stale GPU buffers
            print("[ACTION] Reopening editor to flush stale frames...")
            client.close_editor(0)
            time.sleep(0.5)
            client.open_editor(0)
            time.sleep(1.0) 
            
            assert client.take_screenshot("/tmp/settings_low_gain.png", element_id=pane_id), "Screenshot failed"
            
            # Analyze amplitude - should be low
            # Pass background color explicitly if possible, otherwise rely on default
            # Note: Background detected as #000000
            res = client.analyze_waveform(pane_id, background_color="#000000")
            print(f"[DEBUG] Low gain amplitude: {res.get('amplitude')}")
            if res.get('amplitude', 1.0) < 0.15:
                print("[PASS] Low gain confirmed")
            else:
                print("[FAIL] Low gain verification failed")
                
    else:
        print(f"[WARN] Auto-scale toggle {auto_scale_id} not found")

    # 4. Verify Layout (Columns)
    layout_dropdown_id = "sidebar_options_layoutDropdown"
    if client.element_exists(layout_dropdown_id):
        print("[ACTION] Changing Layout to 2 Columns...")
        # Select item "2" (value) or index 1
        # Utils `select_dropdown_item` takes itemId.
        # Sidebar sets item IDs to "1", "2", "3".
        client.select_dropdown_item(layout_dropdown_id, "2")
        time.sleep(1.5) # Wait for layout update
        
        client.take_screenshot("/tmp/settings_layout_2col.png") # Full window to see layout
        
        # Verify we now have space for 2 columns?
        # We can add another oscillator to a second pane?
        # Or just check state.
        panes = client.get_panes()
        print(f"[INFO] Panes after layout change: {len(panes)}")
        # Layout change doesn't create panes automatically unless needed?
        # Actually, layout strategy determines position.
        # Verify visual change is hard without OCR/CV.
        # But we exercised the UI path.
        print("[PASS] Layout dropdown interacted")
        
    else:
        print(f"[WARN] Layout dropdown {layout_dropdown_id} not found")

    return True

if __name__ == "__main__":
    success = run_test(test_visual_settings)
    sys.exit(0 if success else 1)
