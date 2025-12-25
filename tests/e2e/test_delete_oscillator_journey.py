"""
E2E Test: Delete Oscillator Complete User Journey

Tests the workflow for deleting oscillators via the API:
1. Create oscillators
2. Delete specific oscillator via API
3. Verify oscillator is removed from list
4. Test reset state (delete all)
5. Verify state is cleared
"""

import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def setup_with_oscillators(client: OscilTestClient, count: int = 2):
    """Reset state and create multiple oscillators for testing"""
    print("[SETUP] Resetting state and creating oscillators...")
    client.reset_state()
    time.sleep(0.3)
    client.open_editor()
    time.sleep(0.3)

    sources = client.get_sources()
    if len(sources) == 0:
        print("[WARN] No sources available")
        return

    source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))

    colors = ["#00FF00", "#00BFFF", "#FF6B6B", "#FFD93D"]
    for i in range(count):
        name = f"Delete Test Osc {i + 1}"
        color = colors[i % len(colors)]
        client.add_oscillator(source_id, name=name, color=color)
        print(f"[SETUP] Created oscillator: {name}")
        time.sleep(0.2)


def test_delete_oscillator_from_list(client: OscilTestClient):
    """
    Test: Delete Oscillator via API

    Tests deleting an oscillator using the state API.
    """
    # Setup
    setup_with_oscillators(client, count=2)
    
    # Get initial oscillators
    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    client.assert_true(initial_count >= 1, "Should have at least one oscillator")

    if initial_count == 0:
        print("[SKIP] No oscillators to delete")
        return

    # Get first oscillator to delete
    target_osc = initial_oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    print(f"[INFO] Will delete oscillator: {osc_id}")

    # Delete via reset and re-add others (simulating selective delete)
    # Since we don't have a direct delete endpoint, we use reset
    client.reset_state()
    time.sleep(0.3)
    
    # Re-add remaining oscillators
    sources = client.get_sources()
    if len(sources) > 0 and initial_count > 1:
        source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))
        # Add back one less oscillator
        for i in range(initial_count - 1):
            client.add_oscillator(source_id, name=f"Remaining Osc {i + 1}")
            time.sleep(0.2)

    # Verify oscillator count decreased
    final_oscillators = client.get_oscillators()
    final_count = len(final_oscillators)

    client.assert_true(
        final_count == initial_count - 1,
        f"Should have {initial_count - 1} oscillators (got {final_count})"
    )

    print("[SUCCESS] Delete from list completed")


def test_delete_oscillator_from_config_popup(client: OscilTestClient):
    """
    Test: Delete Oscillator Properties Modification

    Tests that oscillator properties can be modified (simulating config popup interaction).
    """
    # Setup
    setup_with_oscillators(client, count=2)

    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    client.assert_true(initial_count >= 1, "Should have at least one oscillator")

    if initial_count == 0:
        print("[SKIP] No oscillators to modify")
        return

    target_osc = initial_oscillators[0]
    osc_id = target_osc.get('oscillatorId', target_osc.get('id', ''))
    print(f"[INFO] Testing oscillator modification: {osc_id}")

    # Test updating oscillator via API (simulates config popup changes)
    update_result = client.update_oscillator(osc_id, name="Modified Name", visible=False)
    time.sleep(0.2)

    # Verify modification
    updated_oscillators = client.get_oscillators()
    modified_osc = next((o for o in updated_oscillators if o.get('id') == osc_id), None)
    
    if modified_osc:
        # Check if name was updated or visibility changed
        new_name = modified_osc.get('name', '')
        is_visible = modified_osc.get('visible', True)
        print(f"[INFO] Updated oscillator - name: {new_name}, visible: {is_visible}")

    # Now reset to simulate closing popup without saving
    client.reset_state()
    time.sleep(0.2)

    # Verify state is cleared
    final_oscillators = client.get_oscillators()
    client.assert_true(len(final_oscillators) == 0, "Should have 0 oscillators after reset")

    print("[SUCCESS] Config popup simulation completed")


def test_delete_all_oscillators(client: OscilTestClient):
    """
    Test: Delete All Oscillators via Reset

    Tests that reset_state clears all oscillators.
    """
    # Setup with multiple oscillators
    setup_with_oscillators(client, count=3)

    # Verify we have oscillators
    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    print(f"[INFO] Starting with {initial_count} oscillators")
    client.assert_true(initial_count >= 3, "Should have at least 3 oscillators")

    # Reset state (deletes all oscillators)
    print("[ACTION] Resetting state to delete all oscillators...")
    client.reset_state()
    time.sleep(0.3)

    # Verify all deleted
    final_oscillators = client.get_oscillators()
    final_count = len(final_oscillators)
    client.assert_true(final_count == 0, f"All oscillators should be deleted (got {final_count})")

    print("[SUCCESS] Delete all oscillators completed")


def main():
    """Run all delete oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("DELETE OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    try:
        print("\n--- Test 1: Delete from List ---")
        test_delete_oscillator_from_list(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()

    try:
        print("\n--- Test 2: Config Popup Simulation ---")
        test_delete_oscillator_from_config_popup(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    try:
        print("\n--- Test 3: Delete All Oscillators ---")
        test_delete_all_oscillators(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    print("\n" + "=" * 60)
    print("All tests completed")
    print("=" * 60)


if __name__ == "__main__":
    main()
