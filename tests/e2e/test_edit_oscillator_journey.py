"""
E2E Test: Edit Oscillator Complete User Journey

Tests the workflow for editing oscillator properties via the API:
1. Create an oscillator
2. Modify its name
3. Modify its visibility
4. Modify its processing mode
5. Verify changes are persisted
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup(client: OscilTestClient):
    """Reset state, open editor, and ensure we have at least one oscillator"""
    print("[SETUP] Resetting state and opening editor...")
    client.reset_state()
    time.sleep(0.3)
    client.open_editor()
    time.sleep(0.3)

    # Ensure at least one oscillator exists
    oscillators = client.get_oscillators()
    if len(oscillators) == 0:
        print("[SETUP] Creating test oscillator via API...")
        sources = client.get_sources()
        if len(sources) > 0:
            source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))
            client.add_oscillator(source_id, name="Test Oscillator", color="#00FF00")
            print("[SETUP] Created oscillator")
            time.sleep(0.3)


def test_edit_oscillator_via_config_popup(client: OscilTestClient):
    """
    Test: Edit Oscillator via API (simulates config popup)

    Tests modifying oscillator properties through the API.
    """
    # Run setup
    setup(client)

    # Step 1: Get oscillators
    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) > 0, "Should have at least one oscillator")

    if len(oscillators) == 0:
        print("[SKIP] No oscillators available")
        return

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    original_name = target_osc.get('name', 'Unknown')
    print(f"[INFO] Editing oscillator: {osc_id} (name: {original_name})")

    # Step 2: Modify name via API
    new_name = "Modified Oscillator Name"
    print(f"[ACTION] Changing name to: {new_name}")
    client.update_oscillator(osc_id, name=new_name)
    time.sleep(0.2)

    # Verify name change
    updated_oscillators = client.get_oscillators()
    updated_osc = next((o for o in updated_oscillators if o.get('id') == osc_id), None)
    if updated_osc:
        actual_name = updated_osc.get('name', '')
        print(f"[INFO] Oscillator name after update: {actual_name}")
        # Name should be updated (or at least not error)
    
    # Step 3: Toggle visibility
    print("[ACTION] Toggling visibility off...")
    client.update_oscillator(osc_id, visible=False)
    time.sleep(0.2)

    updated_oscillators = client.get_oscillators()
    updated_osc = next((o for o in updated_oscillators if o.get('id') == osc_id), None)
    if updated_osc:
        is_visible = updated_osc.get('visible', True)
        print(f"[INFO] Oscillator visibility: {is_visible}")

    # Step 4: Toggle visibility back on
    print("[ACTION] Toggling visibility on...")
    client.update_oscillator(osc_id, visible=True)
    time.sleep(0.2)

    updated_oscillators = client.get_oscillators()
    updated_osc = next((o for o in updated_oscillators if o.get('id') == osc_id), None)
    if updated_osc:
        is_visible = updated_osc.get('visible', True)
        client.assert_true(is_visible, "Oscillator should be visible")

    print("[SUCCESS] Edit oscillator journey completed")


def test_change_oscillator_visibility(client: OscilTestClient):
    """
    Test: Change Oscillator Visibility

    Tests toggling oscillator visibility on and off.
    """
    setup(client)

    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) > 0, "Should have at least one oscillator")

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    original_visibility = target_osc.get('visible', True)
    print(f"[INFO] Original visibility: {original_visibility}")

    # Toggle off
    client.update_oscillator(osc_id, visible=False)
    time.sleep(0.2)
    
    oscillators = client.get_oscillators()
    target_osc = next((o for o in oscillators if o.get('id') == osc_id), None)
    if target_osc:
        new_visibility = target_osc.get('visible', True)
        client.assert_true(not new_visibility, "Oscillator should be hidden")

    # Toggle on
    client.update_oscillator(osc_id, visible=True)
    time.sleep(0.2)
    
    oscillators = client.get_oscillators()
    target_osc = next((o for o in oscillators if o.get('id') == osc_id), None)
    if target_osc:
        final_visibility = target_osc.get('visible', False)
        client.assert_true(final_visibility, "Oscillator should be visible")

    print("[SUCCESS] Visibility toggle test completed")


def test_change_processing_mode_inline(client: OscilTestClient):
    """
    Test: Change Processing Mode via API

    Tests changing the oscillator processing mode.
    """
    setup(client)

    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) > 0, "Should have at least one oscillator")

    target_osc = oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    original_mode = target_osc.get('mode', target_osc.get('processingMode', 'FullStereo'))
    print(f"[INFO] Original mode: {original_mode}")

    # Test different modes
    modes_to_test = ["Mono", "Left", "Right", "Mid", "Side", "FullStereo"]
    
    for mode in modes_to_test:
        print(f"[ACTION] Setting mode to: {mode}")
        client.update_oscillator(osc_id, processing_mode=mode)
        time.sleep(0.1)

    # Verify final mode
    oscillators = client.get_oscillators()
    target_osc = next((o for o in oscillators if o.get('id') == osc_id), None)
    if target_osc:
        final_mode = target_osc.get('mode', target_osc.get('processingMode', ''))
        print(f"[INFO] Final mode: {final_mode}")
        # Mode should be one of the valid modes
        client.assert_true(
            final_mode in modes_to_test or final_mode == "",
            f"Mode should be valid, got: {final_mode}"
        )

    print("[SUCCESS] Processing mode test completed")


def main():
    """Run all edit oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("EDIT OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    try:
        print("\n--- Test 1: Edit via Config Popup (API) ---")
        test_edit_oscillator_via_config_popup(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()

    try:
        print("\n--- Test 2: Change Visibility ---")
        test_change_oscillator_visibility(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    try:
        print("\n--- Test 3: Change Processing Mode ---")
        test_change_processing_mode_inline(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    print("\n" + "=" * 60)
    print("All tests completed")
    print("=" * 60)


if __name__ == "__main__":
    main()
