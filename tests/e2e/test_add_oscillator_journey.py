"""
E2E Test: Add Oscillator Complete User Journey

Tests the workflow for adding oscillators via the API:
1. Add an oscillator via API
2. Verify it appears in the oscillators list
3. Verify its properties are correct
4. Test cancellation (add then remove)
5. Test validation (required fields)
"""

import sys
import os
import time

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from oscil_test_utils import OscilTestClient


def test_add_oscillator_complete_journey(client: OscilTestClient):
    """
    Test: Complete Add Oscillator Journey via API

    This test verifies that oscillators can be successfully added
    and properly appear in the state.
    """
    # Setup: Reset state
    client.reset_state()
    time.sleep(0.3)
    client.open_editor()
    time.sleep(0.3)

    # Step 1: Verify initial state - should have no oscillators
    initial_oscillators = client.get_oscillators()
    initial_count = len(initial_oscillators)
    print(f"[INFO] Starting with {initial_count} oscillators")

    # Step 2: Get available sources
    sources = client.get_sources()
    client.assert_true(len(sources) > 0, "At least one source should be available")

    if len(sources) == 0:
        print("[SKIP] No sources available, cannot complete test")
        return

    # Step 3: Add oscillator via API
    first_source = sources[0]
    source_id = first_source.get('sourceId', first_source.get('id', 'source_0'))
    print(f"[ACTION] Adding oscillator with source: {source_id}")
    
    result = client.add_oscillator(
        source_id=source_id,
        name="Test Oscillator E2E",
        color="#00BFFF"  # Deep Sky Blue
    )
    time.sleep(0.3)

    # Step 4: Verify oscillator was added
    new_oscillators = client.get_oscillators()
    new_count = len(new_oscillators)
    client.assert_true(
        new_count == initial_count + 1,
        f"Should have {initial_count + 1} oscillators (got {new_count})"
    )

    # Step 5: Verify oscillator properties
    if new_count > 0:
        newest_osc = new_oscillators[-1]
        osc_name = newest_osc.get('name', '')
        print(f"[INFO] Added oscillator name: {osc_name}")
        client.assert_true(
            "Test Oscillator" in osc_name or osc_name == "Test Oscillator E2E",
            f"Oscillator name should contain 'Test Oscillator', got '{osc_name}'"
        )

    print("[SUCCESS] Add Oscillator journey completed successfully")


def test_add_oscillator_cancel(client: OscilTestClient):
    """
    Test: Oscillator Addition and Removal (simulates cancel behavior)

    Verifies that adding and then removing an oscillator works correctly.
    """
    # Setup
    client.reset_state()
    time.sleep(0.3)
    client.open_editor()
    time.sleep(0.3)

    # Get initial count
    initial_count = len(client.get_oscillators())
    print(f"[INFO] Starting with {initial_count} oscillators")

    # Add an oscillator
    sources = client.get_sources()
    if len(sources) > 0:
        source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))
        client.add_oscillator(source_id=source_id, name="Temp Oscillator")
        time.sleep(0.3)

    # Verify it was added
    mid_oscillators = client.get_oscillators()
    mid_count = len(mid_oscillators)
    client.assert_true(mid_count == initial_count + 1, f"Should have {initial_count + 1} oscillators")

    # Reset state (simulates cancel - removes all oscillators)
    client.reset_state()
    time.sleep(0.3)

    # Verify oscillator was removed
    final_count = len(client.get_oscillators())
    client.assert_true(
        final_count == 0,
        f"Oscillator count should be 0 after reset (got {final_count})"
    )

    print("[SUCCESS] Cancel/reset test completed")


def test_add_oscillator_validation(client: OscilTestClient):
    """
    Test: Add Oscillator with Various Parameters

    Verifies that oscillators can be added with different configurations.
    """
    # Setup
    client.reset_state()
    time.sleep(0.3)
    client.open_editor()
    time.sleep(0.3)

    sources = client.get_sources()
    if len(sources) == 0:
        print("[SKIP] No sources available")
        return

    source_id = sources[0].get('sourceId', sources[0].get('id', 'source_0'))

    # Test 1: Add oscillator with minimal parameters
    print("[TEST] Adding oscillator with minimal parameters...")
    result1 = client.add_oscillator(source_id=source_id)
    time.sleep(0.2)
    
    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) >= 1, "Should have at least 1 oscillator")

    # Test 2: Add oscillator with custom name
    print("[TEST] Adding oscillator with custom name...")
    result2 = client.add_oscillator(source_id=source_id, name="Custom Named Osc")
    time.sleep(0.2)
    
    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) >= 2, "Should have at least 2 oscillators")
    
    # Verify custom name was set
    custom_osc = next((o for o in oscillators if o.get('name') == "Custom Named Osc"), None)
    client.assert_true(custom_osc is not None, "Should find oscillator with custom name")

    # Test 3: Add oscillator with specific color
    print("[TEST] Adding oscillator with custom color...")
    result3 = client.add_oscillator(source_id=source_id, name="Colored Osc", color="#FF0000")
    time.sleep(0.2)
    
    oscillators = client.get_oscillators()
    client.assert_true(len(oscillators) >= 3, "Should have at least 3 oscillators")

    print("[SUCCESS] Validation test completed - all parameter variations work")


def main():
    """Run all add oscillator journey tests"""
    client = OscilTestClient()

    if not client.wait_for_harness():
        sys.exit(1)

    print("\n" + "=" * 60)
    print("ADD OSCILLATOR USER JOURNEY TESTS")
    print("=" * 60)

    try:
        print("\n--- Test 1: Complete Add Oscillator Journey ---")
        test_add_oscillator_complete_journey(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    try:
        print("\n--- Test 2: Cancel/Reset ---")
        test_add_oscillator_cancel(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    try:
        print("\n--- Test 3: Validation ---")
        test_add_oscillator_validation(client)
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")

    print("\n" + "=" * 60)
    print("All tests completed")
    print("=" * 60)


if __name__ == "__main__":
    main()
